// Requires the 'geometry' library to be loaded via GOOGLE_MAPS_LIBRARIES.
export function computeRouteLengthMeters(points: { lat: number; lng: number }[]): number {
  if (points.length < 2 || !window.google?.maps?.geometry) return 0;
  let total = 0;
  for (let i = 0; i < points.length - 1; i++) {
    const a = new google.maps.LatLng(points[i].lat, points[i].lng);
    const b = new google.maps.LatLng(points[i + 1].lat, points[i + 1].lng);
    total += google.maps.geometry.spherical.computeDistanceBetween(a, b);
  }
  return total;
}

// Matches DroneSimulator's fixed speed in main.cpp. NOT yet a real
// per-mission parameter — see FlightOpsSidePanel for the honest label.
export const DEFAULT_SIM_SPEED_MPS = 15;