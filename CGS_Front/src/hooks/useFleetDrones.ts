import { useEffect, useState } from 'react';
import { fetchBunkerStatus } from '../api/BunkerApi';
import { useTelemetrySocket } from './useTelemetrySocket';
import type { FleetDrone } from '../types/drone';

const BUNKER_POLL_MS = 5000;

export function useFleetDrones() {
  const { drones: activeDrones, connected, initiateRecovery, sendCommand, recoveryStatus } = useTelemetrySocket();
  const [dockedDrones, setDockedDrones] = useState<FleetDrone[]>([]);
  const [lastBunkerFetchAt, setLastBunkerFetchAt] = useState<number | null>(null);

  useEffect(() => {
    let cancelled = false;

    const load = async () => {
      try {
        const data = await fetchBunkerStatus();
        if (cancelled) return;
        const docked: FleetDrone[] = data.slots
          .filter((s) => s.isOccupied && s.drone)
          .map((s) => ({
            droneId: s.drone!.id,
            state: s.drone!.state ?? 'DOCKED',
            batteryLevel: s.drone!.batteryLevel,
            bayId: s.slotId,
            source: 'DOCKED' as const,
          }));
        setDockedDrones(docked);
        setLastBunkerFetchAt(Date.now());
      } catch {
        // non-fatal — keep showing the previous snapshot
      }
    };

    load();
    const interval = window.setInterval(load, BUNKER_POLL_MS);
    return () => {
      cancelled = true;
      window.clearInterval(interval);
    };
  }, []);

  const activeIds = new Set(activeDrones.map((d) => d.droneId));

  const active: FleetDrone[] = activeDrones.map((d) => ({
    droneId: d.droneId,
    state: d.state,
    batteryLevel: d.batteryLevel,
    latitude: d.latitude,
    longitude: d.longitude,
    source: 'ACTIVE' as const,
  }));

  // Live telemetry takes precedence over the docked snapshot — avoids a
  // brief duplicate right at the dock/undock transition.
  const docked = dockedDrones.filter((d) => !activeIds.has(d.droneId));

  return {
    fleet: [...active, ...docked],
    connected,
    lastBunkerFetchAt,
    initiateRecovery,
    sendCommand,
    recoveryStatus,
  };
}