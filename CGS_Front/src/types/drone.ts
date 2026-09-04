export type DroneOpState = 'IN_FLIGHT' | 'RETURNING' | 'LANDING' | 'FAULT' | 'DOCKED'|'CHARGING'|'IDLE'|'UNKNOWN';

export interface FleetDrone {
  droneId: string;
  state: DroneOpState;
  batteryLevel: number;
  bayId?: number;                 // present only for docked drones
  latitude?: number;
  longitude?: number;
  source: 'DOCKED' | 'ACTIVE';    // drives "last updated" semantics
}

export interface DroneFlightStats {
  totalFlights: number;
  totalFlightSeconds: number;
}