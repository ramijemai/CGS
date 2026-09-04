import { useCallback, useEffect, useRef, useState } from 'react';
import type { TelemetryDrone, WsInboundEvent } from '../types/telemetry';
import { fetchActiveMissions } from '../api/missionApi';
import type { MissionApiEntry } from '../types/mission';

const API_BASE = import.meta.env.VITE_API_BASE_URL ?? 'http://localhost:18080';
const WS_URL = import.meta.env.VITE_WS_URL ?? `${API_BASE.replace(/^http/, 'ws').replace(/^https/, 'wss')}/ws/telemetry`;
const REQUEST_INTERVAL_MS = 1000;
const MAX_TRAIL_POINTS = 120;

export interface ActiveMissionDrone extends TelemetryDrone {
  elapsedSeconds: number;
}

export interface TrailPoint {
  lat: number;
  lng: number;
}

export function useTelemetrySocket() {
  const [drones, setDrones] = useState<TelemetryDrone[]>([]);
  const [connected, setConnected] = useState(false);
  const [lastError, setLastError] = useState<string | null>(null);
  const [recoveryStatus, setRecoveryStatus] = useState<Record<string, 'PENDING' | 'DOCKED' | 'FAILED'>>({});
  const [missionFinishStatus, setMissionFinishStatus] = useState<Record<string, 'PENDING' | 'COMPLETED' | 'ABORTED'>>({});
  const [lastMissionFinish, setLastMissionFinish] = useState<{ droneId: string; status: 'COMPLETED' | 'ABORTED' } | null>(null);
  const [lastUpdateAt, setLastUpdateAt] = useState<number | null>(null);
  const [trails, setTrails] = useState<Record<string, TrailPoint[]>>({});
  // Full active-mission data keyed by droneId — the source of truth for
  // "what path is this drone actually flying," independent of any local
  // draft/composition UI state, which gets cleared once launched.
  const [activeMissions, setActiveMissions] = useState<Record<string, MissionApiEntry>>({});

  const wsRef = useRef<WebSocket | null>(null);
  const firstSeenRef = useRef<Map<string, number>>(new Map());

  useEffect(() => {
    let cancelled = false;
    let interval: number | undefined;

    const connectTelemetry = async () => {
      try {
        const response = await fetch(`${API_BASE}/api/v1/missions/active`, {
          headers: { Accept: 'application/json' },
        });

        if (!response.ok) {
          throw new Error(`Telemetry probe failed: ${response.status}`);
        }

        if (cancelled) return;

        const ws = new WebSocket(WS_URL);
        wsRef.current = ws;

        ws.onopen = () => {
          setConnected(true);
          ws.send(JSON.stringify({ action: 'REQUEST_TELEMETRY' }));
        };

        ws.onclose = () => {
          setConnected(false);
        };

        ws.onerror = () => {
          setConnected(false);
          setLastError('Telemetry service is unavailable.');
        };

        ws.onmessage = (evt) => {
          try {
            const data = JSON.parse(evt.data) as WsInboundEvent;
            if (data.event === 'TELEMETRY_UPDATE') {
              const seenIds = new Set(data.drones.map((d) => d.droneId));
              const now = Date.now();
              data.drones.forEach((d) => {
                if (!firstSeenRef.current.has(d.droneId)) {
                  firstSeenRef.current.set(d.droneId, now);
                }
              });
              for (const id of firstSeenRef.current.keys()) {
                if (!seenIds.has(id)) firstSeenRef.current.delete(id);
              }
              setDrones(data.drones);
              setLastError(null);
              setLastUpdateAt(now);

              setTrails((prev) => {
                const next: Record<string, TrailPoint[]> = {};
                for (const d of data.drones) {
                  const existing = prev[d.droneId] ?? [];
                  const point: TrailPoint = { lat: d.latitude, lng: d.longitude };
                  next[d.droneId] = [...existing, point].slice(-MAX_TRAIL_POINTS);
                }
                return next;
              });
            } else if (data.event === 'RECOVERY_RESULT') {
              const mapped = data.status === 'RTL_COMMANDED' ? 'PENDING' : data.status;
              setRecoveryStatus((prev) => ({ ...prev, [data.droneId]: mapped }));

            } else if (data.event === 'MISSION_FINISH_RESULT') {
              setMissionFinishStatus((prev) => ({ ...prev, [data.droneId]: data.status }));
              setLastMissionFinish({ droneId: data.droneId, status: data.status });

            } else if (data.event === 'ERROR') {
              setLastError(data.message);
            }
          } catch {
            setLastError('Received malformed telemetry payload.');
          }
        };

        interval = window.setInterval(() => {
          if (ws.readyState === WebSocket.OPEN) {
            ws.send(JSON.stringify({ action: 'REQUEST_TELEMETRY' }));
          }
        }, REQUEST_INTERVAL_MS);
      } catch {
        if (!cancelled) {
          setConnected(false);
          setLastError('Telemetry service is unavailable.');
        }
      }
    };

    void connectTelemetry();

    return () => {
      cancelled = true;
      if (interval) {
        window.clearInterval(interval);
      }
      if (wsRef.current) {
        wsRef.current.close();
        wsRef.current = null;
      }
    };
  }, []);

  const pollActiveMissions = useCallback(async () => {
    try {
      const missions = await fetchActiveMissions();
      const byDrone: Record<string, MissionApiEntry> = {};
      for (const m of missions) byDrone[m.droneId] = m;
      setActiveMissions(byDrone);
    } catch {
      // non-fatal — worst case planned-path overlay/"awaiting recall" badge don't update this tick
    }
  }, []);

  useEffect(() => {
    let cancelled = false;
    const poll = async () => {
      if (!cancelled) await pollActiveMissions();
    };
    poll();
    const interval = window.setInterval(poll, 2000);
    return () => {
      cancelled = true;
      window.clearInterval(interval);
    };
  }, [pollActiveMissions]);

  const initiateRecovery = useCallback((droneId: string) => {
    if (wsRef.current?.readyState === WebSocket.OPEN) {
      setRecoveryStatus((prev) => ({ ...prev, [droneId]: 'PENDING' }));
      wsRef.current.send(JSON.stringify({ action: 'INITIATE_RECOVERY', droneId }));
    }
  }, []);

  const manuallyFinishMission = useCallback((droneId: string) => {
    if (wsRef.current?.readyState === WebSocket.OPEN) {
      setMissionFinishStatus((prev) => ({ ...prev, [droneId]: 'PENDING' }));
      wsRef.current.send(JSON.stringify({ action: 'MANUALLY_FINISH_MISSION', droneId }));
    }
  }, []);

  const sendCommand = useCallback((droneId: string, commandCode: number, text?: string) => {
    if (wsRef.current?.readyState === WebSocket.OPEN) {
      wsRef.current.send(JSON.stringify({ action: 'SEND_COMMAND', droneId, commandCode, text }));
    }
  }, []);

  const requestNow = useCallback(() => {
    if (wsRef.current?.readyState === WebSocket.OPEN) {
      wsRef.current.send(JSON.stringify({ action: 'REQUEST_TELEMETRY' }));
    }
  }, []);

  const activeMissionDrones: ActiveMissionDrone[] = drones.map((d) => {
    const firstSeen = firstSeenRef.current.get(d.droneId) ?? Date.now();
    return { ...d, elapsedSeconds: Math.floor((Date.now() - firstSeen) / 1000) };
  });

  const dronesWithActiveMission = new Set(Object.keys(activeMissions));

  return {
    drones: activeMissionDrones,
    connected,
    lastError,
    initiateRecovery,
    manuallyFinishMission,
    requestNow,
    sendCommand,
    recoveryStatus,
    missionFinishStatus,
    lastMissionFinish,
    dronesWithActiveMission,
    lastUpdateAt,
    trails,
    activeMissions,
    refreshActiveMissions: pollActiveMissions, // NEW — call right after a successful launch
  };
}