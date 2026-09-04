// src/types/bunker.ts
import type { DroneOpState } from './drone';

export interface DroneSummary {
  id: string;
  batteryLevel: number;
  state?: DroneOpState;
}

export interface BaySlot {
  slotId: number;
  isOccupied: boolean;
  drone?: DroneSummary;
}

export interface BunkerStatusResponse {
  slots: BaySlot[];
}

export interface WeatherSnapshot {
  temperature: number;
  humidity: number;
  windSpeed: number;
  source: string;
}

// UI-only concepts not yet returned by the backend.
// TODO(backend): extend handleGetBunkerStatus() to include hatchState.
export type HatchState = 'OPEN' | 'CLOSED' | 'UNKNOWN';
export type BayStatus = 'OCCUPIED' | 'VACANT' | 'FAULT' | 'HATCH_OPENING' | 'INCOMING';

export interface BaySlotViewModel extends BaySlot {
  hatchState: HatchState;
  status: BayStatus;
}