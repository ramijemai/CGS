import { useCallback } from 'react';
import styled, { useTheme } from 'styled-components';
import { GoogleMap, Marker, Polyline, useJsApiLoader } from '@react-google-maps/api';
import type { MissionPatternType, MissionWaypoint } from '../../types/mission';
import { GOOGLE_MAPS_LOADER_ID, GOOGLE_MAPS_LIBRARIES } from '../../lib/googleMapsConfig';


interface MissionPathMapProps {
  pattern: MissionPatternType;
  waypoints: MissionWaypoint[];
  currentWaypointIndex: number;
  currentPosition?: { latitude: number; longitude: number } | null;
}

const MAP_CONTAINER_STYLE = { width: '100%', height: '560px' };

// Generate diagonal hatching lines within the bounding box of waypoints
function generateCoverageLines(
  waypoints: MissionWaypoint[],
  lineSpacing: number = 0.0015 // degrees (~165m at equator) - spacing between lines
): Array<{ lat1: number; lng1: number; lat2: number; lng2: number }> {
  if (waypoints.length < 3) return [];

  const lats = waypoints.map((wp) => wp.latitude);
  const lngs = waypoints.map((wp) => wp.longitude);
  const minLat = Math.min(...lats);
  const maxLat = Math.max(...lats);
  const minLng = Math.min(...lngs);
  const maxLng = Math.max(...lngs);

  const lines: Array<{ lat1: number; lng1: number; lat2: number; lng2: number }> = [];

  // Generate horizontal hatching lines across the area
  for (let lat = minLat; lat < maxLat; lat += lineSpacing) {
    lines.push({
      lat1: lat,
      lng1: minLng,
      lat2: lat,
      lng2: maxLng,
    });
  }

  return lines;
}

// Loads the Maps JS script once, keyed by `id`. If more pages later want a
// map (e.g. a fleet overview), hoist this to a shared provider instead of
// duplicating the hook call, so only one script tag is ever injected.
export default function MissionPathMap({
  pattern,
  waypoints,
  currentWaypointIndex,
  currentPosition,
}: MissionPathMapProps) {
  const theme = useTheme();
 const { isLoaded, loadError } = useJsApiLoader({
  id: GOOGLE_MAPS_LOADER_ID,
  googleMapsApiKey: import.meta.env.VITE_GOOGLE_MAPS_API_KEY ?? '',
  libraries: GOOGLE_MAPS_LIBRARIES,
});

  const onMapLoad = useCallback(
    (map: google.maps.Map) => {
      if (waypoints.length === 0) return;
      const bounds = new google.maps.LatLngBounds();
      waypoints.forEach((wp) => bounds.extend({ lat: wp.latitude, lng: wp.longitude }));
      if (currentPosition) {
        bounds.extend({ lat: currentPosition.latitude, lng: currentPosition.longitude });
      }
      map.fitBounds(bounds, 40);
    },
    [waypoints, currentPosition]
  );

  if (loadError) {
    return <StatusBox $error>Failed to load Google Maps. Check VITE_GOOGLE_MAPS_API_KEY.</StatusBox>;
  }
  if (!isLoaded) {
    return <StatusBox>Loading map…</StatusBox>;
  }
  if (waypoints.length === 0) {
    return <StatusBox>No waypoint data available for this mission.</StatusBox>;
  }

  const center = { lat: waypoints[0].latitude, lng: waypoints[0].longitude };

  // Straight-line path between the real GPS waypoints — not Directions API
  // "waypoints" routing, which snaps to roads and would misrepresent an
  // aerial flight path.
  const pathCoords = waypoints.map((wp) => ({ lat: wp.latitude, lng: wp.longitude }));
  
  // Generate coverage lines for hatching pattern
  const coverageLines =
    pattern !== 'SINGLE_POINT' && waypoints.length >= 3
      ? generateCoverageLines(waypoints)
      : [];

  return (
    <MapWrapper>
      <GoogleMap
        mapContainerStyle={MAP_CONTAINER_STYLE}
        center={center}
        zoom={18}
        onLoad={onMapLoad}
        options={{
          disableDefaultUI: true,
          zoomControl: true,
          mapTypeId: 'satellite',
        }}
      >
        {/* Render hatching lines across coverage area */}
        {coverageLines.map((line, i) => (
          <Polyline
            key={i}
            path={[
              { lat: line.lat1, lng: line.lng1 },
              { lat: line.lat2, lng: line.lng2 },
            ]}
            options={{
              strokeColor: theme.colors.navy500,
              strokeOpacity: 0.35,
              strokeWeight: 2,
            }}
          />
        ))}

        {pattern !== 'SINGLE_POINT' && (
          <Polyline
            path={pathCoords}
            options={{
              strokeColor: theme.colors.navy300,
              strokeOpacity: 0.9,
              strokeWeight: 2,
            }}
          />
        )}

        {waypoints.map((wp, i) => (
          <Marker
            key={i}
            position={{ lat: wp.latitude, lng: wp.longitude }}
            label={{ text: String(i + 1), color: '#fff', fontSize: '11px', fontWeight: '600' }}
            icon={{
              path: google.maps.SymbolPath.CIRCLE,
              scale: i === currentWaypointIndex ? 11 : 9,
              fillColor: i === currentWaypointIndex ? theme.colors.warning : theme.colors.navy700,
              fillOpacity: 1,
              strokeColor: '#fff',
              strokeWeight: 2,
            }}
          />
        ))}

        {currentPosition && (
          <Marker
            position={{ lat: currentPosition.latitude, lng: currentPosition.longitude }}
            icon={{
              path: google.maps.SymbolPath.CIRCLE,
              scale: 7,
              fillColor: theme.colors.navy900,
              fillOpacity: 1,
              strokeColor: '#ffffff',
              strokeWeight: 2,
            }}
            zIndex={999}
          />
        )}
      </GoogleMap>

      <Legend>
        <LegendItem color={theme.colors.navy700} label="Waypoint" />
        <LegendItem color={theme.colors.warning} label="Current target" />
        <LegendItem color={theme.colors.navy900} label="Live position" />
        {pattern !== 'SINGLE_POINT' && waypoints.length >= 3 && (
          <LegendItem color={theme.colors.navy500} label="Coverage area" isFill />
        )}
      </Legend>
    </MapWrapper>
  );
}

function LegendItem({ color, label, isFill }: { color: string; label: string; isFill?: boolean }) {
  return (
    <LegendRow>
      {isFill ? (
        <FillDot style={{ background: color }} />
      ) : (
        <Dot style={{ background: color }} />
      )}
      {label}
    </LegendRow>
  );
}

const MapWrapper = styled.div`
  border-radius: ${({ theme }) => theme.radius};
  overflow: hidden;
  border: 1px solid ${({ theme }) => theme.colors.navy100};
`;

const Legend = styled.div`
  display: flex;
  gap: 16px;
  padding: 10px 4px 0;
  font-size: 11px;
  color: ${({ theme }) => theme.colors.textSecondary};
`;

const LegendRow = styled.div`
  display: flex;
  align-items: center;
  gap: 5px;
`;

const Dot = styled.span`
  width: 9px;
  height: 9px;
  border-radius: 50%;
  display: inline-block;
`;

const FillDot = styled.span`
  width: 14px;
  height: 14px;
  border-radius: 2px;
  display: inline-block;
  opacity: 0.3;
  border: 1px solid currentColor;
`;

const StatusBox = styled.div<{ $error?: boolean }>`
  padding: 40px;
  text-align: center;
  font-size: 13px;
  color: ${({ theme, $error }) => ($error ? theme.colors.danger : theme.colors.textSecondary)};
`;