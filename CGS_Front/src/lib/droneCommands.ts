// Mirrors DroneCommand enum values in TelemetryManager.h exactly —

export const DRONE_COMMAND_CODES = {
  UPDATE_TARGET: 0,
  RETURN_TO_BUNKER: 1,
  HOLD_POSITION: 2,
  EMERGENCY_LAND: 3,
  STATUS_CHECK: 4,
} as const;