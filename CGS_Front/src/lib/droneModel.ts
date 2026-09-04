// Static placeholder spec sheet — the fleet currently operates one
// aircraft type; per-drone model differentiation isn't tracked by the
// backend yet (Drone has no `model` field). Purely presentational, mirrors
// the "aircraft info" section a real GCS shows for a connected drone.
export const FLEET_DRONE_MODEL = {
  name: 'Falcon X1',
  tagline: 'Quadcopter — Inspection Class',
  maxSpeedMs: 18,
  maxFlightTimeMin: 32,
  weightKg: 1.4,
};