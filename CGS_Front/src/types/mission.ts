export interface LaunchMissionPayload {
  latitude: number;
  longitude: number;
  altitude: number;
  cruiseAltitude: number;
  droneId?: string;
}

export interface LaunchMissionResponse {
  status: 'SUCCESS' | 'FAILED' | 'ERROR';
  message: string;
}

export type MissionPatternType = 'SINGLE_POINT' | 'WAYPOINT_PATH';

export interface MissionWaypoint {
  latitude: number;
  longitude: number;
  altitude: number;
}

export interface MissionApiEntry {
  missionId: string;
  droneId: string;
  status: string;
  pattern: MissionPatternType;
  target: MissionWaypoint;
  waypoints: MissionWaypoint[];
  currentWaypointIndex: number;
  finalWaypointReached?: boolean;
  cruiseAltitude: number;
  launchTime: string;
  durationSeconds: number; 
}

export interface LaunchWaypointPathPayload {
  waypoints: { latitude: number; longitude: number }[];
  cruiseAltitude: number;
  droneId?: string;
}

export interface MissionHistoryEntry {
  missionId: string;
  droneId: string;
  targetCoords: string;
  launchTime: string;
  outcome: 'COMPLETED' | 'ABORTED' | 'FAILED';
}