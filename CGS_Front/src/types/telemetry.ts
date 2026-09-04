import type { DroneOpState } from './drone';

export interface TelemetryDrone {
  droneId: string;
  latitude: number;
  longitude: number;
  altitude: number;
  batteryLevel: number;
  speed: number;
  heading: number;
  state: DroneOpState;
}

export interface TelemetryUpdateEvent {
  event: 'TELEMETRY_UPDATE';
  drones: TelemetryDrone[];
}

export interface RecoveryResultEvent {
  event: 'RECOVERY_RESULT';
  droneId: string;
  status: 'DOCKED' | 'FAILED' | 'RTL_COMMANDED';
}

export interface MissionFinishResultEvent {
  event: 'MISSION_FINISH_RESULT';
  droneId: string;
  status: 'COMPLETED' | 'ABORTED';
}

export interface WsErrorEvent {
  event: 'ERROR';
  message: string;
}

export type WsInboundEvent =
  | TelemetryUpdateEvent
  | RecoveryResultEvent
  | MissionFinishResultEvent
  | WsErrorEvent;