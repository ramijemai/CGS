import { useCallback, useRef } from 'react';
import styled, { useTheme } from 'styled-components';
import { GoogleMap, Marker, Polyline, useJsApiLoader } from '@react-google-maps/api';
import { GOOGLE_MAPS_LOADER_ID, GOOGLE_MAPS_LIBRARIES } from '../../lib/googleMapsConfig';
import { droneStateMeta } from '../../lib/droneState';
import { BUNKER_LOCATION } from '../../lib/bunkerLocation';
import type { TelemetryDrone } from '../../types/telemetry';
import type { TrailPoint } from '../../hooks/useTelemetrySocket';

interface DraftPoint {
  lat: number;
  lng: number;
}

interface FlightOpsMapProps {
  drones: TelemetryDrone[]; // always 0 or 1 entries — the single connected aircraft
  trails: Record<string, TrailPoint[]>;
  plannedPaths: Record<string, DraftPoint[]>;
  draftMode: boolean;
  draftWaypoints: DraftPoint[];
  onDraftMapClick: (point: DraftPoint) => void;
}

const MAP_CONTAINER_STYLE = { width: '100%', height: '100%' };
const MISSION_LINE_COLOR = '#2E8B57';
const BUNKER_LEG_FALLBACK = '#C0392B';

function dashedLineIcon(color: string) {
  return [
    {
      icon: { path: 'M 0,-1 0,1', strokeOpacity: 1, strokeColor: color, scale: 3 },
      offset: '0',
      repeat: '14px',
    },
  ];
}

export default function FlightOpsMap({
  drones,
  trails,
  plannedPaths,
  draftMode,
  draftWaypoints,
  onDraftMapClick,
}: FlightOpsMapProps) {
  const theme = useTheme();
  const mapRef = useRef<google.maps.Map | null>(null);
  const framedOnceRef = useRef(false);

  const { isLoaded, loadError } = useJsApiLoader({
    id: GOOGLE_MAPS_LOADER_ID,
    googleMapsApiKey: import.meta.env.VITE_GOOGLE_MAPS_API_KEY ?? '',
    libraries: GOOGLE_MAPS_LIBRARIES,
  });

  const onMapLoad = useCallback((map: google.maps.Map) => {
    mapRef.current = map;
  }, []);

  const handleMapClick = useCallback(
    (e: google.maps.MapMouseEvent) => {
      if (!draftMode || !e.latLng) return;
      onDraftMapClick({ lat: e.latLng.lat(), lng: e.latLng.lng() });
    },
    [draftMode, onDraftMapClick]
  );

  // Frame once when the aircraft appears in telemetry, not on every tick —
  // with a single aircraft there's no "switch focus between drones" case
  // that needs continuous re-framing.
  if (!draftMode && mapRef.current && drones.length > 0 && !framedOnceRef.current) {
    framedOnceRef.current = true;
    const bounds = new google.maps.LatLngBounds();
    bounds.extend(BUNKER_LOCATION);
    drones.forEach((d) => bounds.extend({ lat: d.latitude, lng: d.longitude }));
    mapRef.current.fitBounds(bounds, 60);
  }
  if (drones.length === 0) {
    framedOnceRef.current = false; // re-frame next time the aircraft reconnects
  }

  if (loadError) {
    return <StatusBox $error>Failed to load Google Maps. Check VITE_GOOGLE_MAPS_API_KEY.</StatusBox>;
  }
  if (!isLoaded) {
    return <StatusBox>Loading map…</StatusBox>;
  }

  const bunkerLegColor = theme.colors.danger ?? BUNKER_LEG_FALLBACK;
  const plannedPathColor = theme.colors.navy500 ?? '#2F5A96';

  return (
    <MapWrapper>
      <GoogleMap
        mapContainerStyle={MAP_CONTAINER_STYLE}
        center={BUNKER_LOCATION}
        zoom={15}
        onLoad={onMapLoad}
        onClick={handleMapClick}
        options={{ disableDefaultUI: true, zoomControl: true, mapTypeId: 'satellite' }}
      >
        <Marker
          position={BUNKER_LOCATION}
          label={{ text: 'BUNKER', color: '#ffffff', fontSize: '9px', fontWeight: '700' }}
          icon={{
            path: google.maps.SymbolPath.CIRCLE,
            scale: 9,
            fillColor: theme.colors.navy900,
            fillOpacity: 1,
            strokeColor: '#ffffff',
            strokeWeight: 2,
          }}
          zIndex={500}
        />

        {!draftMode &&
          drones.map((drone) => {
            const planned = plannedPaths[drone.droneId];
            const flown = trails[drone.droneId];

            return (
              <span key={`lines-${drone.droneId}`}>
                {planned && planned.length >= 2 && (
                  <Polyline
                    path={planned}
                    options={{ strokeOpacity: 0, icons: dashedLineIcon(plannedPathColor), zIndex: 3 }}
                  />
                )}
                {planned &&
                  planned.map((wp, i) => (
                    <Marker
                      key={`planned-pt-${drone.droneId}-${i}`}
                      position={wp}
                      icon={{
                        path: google.maps.SymbolPath.CIRCLE,
                        scale: 5,
                        fillColor: '#ffffff',
                        fillOpacity: 1,
                        strokeColor: plannedPathColor,
                        strokeWeight: 2,
                      }}
                      zIndex={3}
                    />
                  ))}

                {flown && flown.length > 0 && (
                  <Polyline
                    path={[BUNKER_LOCATION, flown[0]]}
                    options={{ strokeColor: bunkerLegColor, strokeOpacity: 0.9, strokeWeight: 3, zIndex: 4 }}
                  />
                )}
                {flown && flown.length >= 2 && (
                  <Polyline
                    path={flown}
                    options={{ strokeColor: MISSION_LINE_COLOR, strokeOpacity: 0.95, strokeWeight: 3, zIndex: 5 }}
                  />
                )}
              </span>
            );
          })}

        {draftMode && draftWaypoints.length >= 2 && (
          <Polyline path={draftWaypoints} options={{ strokeColor: MISSION_LINE_COLOR, strokeOpacity: 0.9, strokeWeight: 3 }} />
        )}
        {draftMode &&
          draftWaypoints.map((wp, i) => (
            <Marker
              key={`draft-${i}`}
              position={wp}
              label={{ text: String(i + 1), color: '#ffffff', fontSize: '11px', fontWeight: '600' }}
              icon={{
                path: google.maps.SymbolPath.CIRCLE,
                scale: 10,
                fillColor: theme.colors.navy700,
                fillOpacity: 1,
                strokeColor: '#ffffff',
                strokeWeight: 2,
              }}
            />
          ))}

        {/* Active drone — directional arrow instead of a plain circle, so
            the marker communicates real heading, not just position.
            FORWARD_CLOSED_ARROW points north (0°) by default; `rotation`
            is clockwise degrees, matching MavlinkFlightController's
            heading convention exactly, no conversion needed. */}
        {!draftMode &&
          drones.map((drone) => {
            const meta = droneStateMeta[drone.state];
            const color =
              meta.tone === 'danger' ? theme.colors.danger : meta.tone === 'warning' ? theme.colors.warning : theme.colors.navy700;
            return (
              <Marker
                key={drone.droneId}
                position={{ lat: drone.latitude, lng: drone.longitude }}
                label={{ text: drone.droneId, color: '#ffffff', fontSize: '10px', fontWeight: '600' }}
                icon={{
                  path: google.maps.SymbolPath.FORWARD_CLOSED_ARROW,
                  scale: 6,
                  rotation: drone.heading,
                  fillColor: color,
                  fillOpacity: 1,
                  strokeColor: '#ffffff',
                  strokeWeight: 2,
                  anchor: new google.maps.Point(0, 2.6),
                }}
                zIndex={999}
              />
            );
          })}
      </GoogleMap>

      {draftMode && <DraftHint>Click the map to place waypoints — Launch requires at least 1 point.</DraftHint>}
    </MapWrapper>
  );
}

const MapWrapper = styled.div`
  position: relative;
  width: 100%;
  height: 100%;
`;

const DraftHint = styled.div`
  position: absolute;
  top: 12px;
  left: 50%;
  transform: translateX(-50%);
  background: rgba(11, 31, 58, 0.85);
  color: #ffffff;
  font-size: 12px;
  padding: 8px 16px;
  border-radius: 999px;
`;

const StatusBox = styled.div<{ $error?: boolean }>`
  padding: 60px;
  text-align: center;
  font-size: 13px;
  color: ${({ theme, $error }) => ($error ? theme.colors.danger : theme.colors.textSecondary)};
`;