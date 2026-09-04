// Shared across every component that loads Google Maps (MissionPathMap,
// OrbitDrawMap, and any future map view). useJsApiLoader requires the same
// `id` + `libraries` across all call sites, or the script gets reloaded
// with conflicting options and Google Maps throws at runtime.
export const GOOGLE_MAPS_LOADER_ID = 'ground-control-google-maps-script';
export const GOOGLE_MAPS_LIBRARIES: ('geometry')[] = ['geometry'];
