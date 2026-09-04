import type {
  LaunchMissionPayload,
  LaunchMissionResponse,
  LaunchWaypointPathPayload,
  MissionApiEntry,
} from '../types/mission';

const API_BASE = import.meta.env.VITE_API_BASE_URL ?? 'http://localhost:18080';

export async function launchMission(payload: LaunchMissionPayload): Promise<LaunchMissionResponse> {
  const res = await fetch(`${API_BASE}/api/v1/missions/launch`, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(payload),
  });
  const body = (await res.json()) as LaunchMissionResponse;
  if (!res.ok && !body.status) throw new Error(`Launch request failed: ${res.status}`);
  return body;
}

// Endpoint name is a holdover from an earlier "area scan" design — the
// route itself (/missions/launch-scan) didn't need to change when the
// underlying pattern moved from generated shapes to manual waypoints.
export async function launchWaypointPath(payload: LaunchWaypointPathPayload): Promise<LaunchMissionResponse> {
  const res = await fetch(`${API_BASE}/api/v1/missions/launch-scan`, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(payload),
  });
  const body = (await res.json()) as LaunchMissionResponse;
  if (!res.ok && !body.status) throw new Error(`Waypoint path launch request failed: ${res.status}`);
  return body;
}

export async function fetchMissionHistory(): Promise<MissionApiEntry[]> {
  const res = await fetch(`${API_BASE}/api/v1/missions/history`);
  if (!res.ok) throw new Error(`Failed to fetch mission history: ${res.status}`);
  const body = (await res.json()) as { missions: MissionApiEntry[] };
  return body.missions ?? [];
}

export async function fetchActiveMissions(): Promise<MissionApiEntry[]> {
  const res = await fetch(`${API_BASE}/api/v1/missions/active`);
  if (!res.ok) throw new Error(`Failed to fetch active missions: ${res.status}`);
  const body = (await res.json()) as { missions: MissionApiEntry[] };
  return body.missions ?? [];
}