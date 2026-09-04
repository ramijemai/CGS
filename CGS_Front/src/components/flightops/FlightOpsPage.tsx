import { useEffect, useState } from 'react';
import type { ReactNode } from 'react';
import styled from 'styled-components';
import FlightOpsMap from './FlightOpsMap';
import FlightOpsSidePanel from './FlightOpsSidePanel';
import FlightOpsHud from './FlightOpsHud';
import FlightOpsTopBar from './FlightOpsTopBar';
import { useTelemetrySocket } from '../../hooks/useTelemetrySocket';
import { DRONE_COMMAND_CODES } from '../../lib/droneCommands';
import { fetchBunkerStatus } from '../../api/BunkerApi';
import { launchMission, launchWaypointPath } from '../../api/missionApi';
import type { BaySlot } from '../../types/bunker';

export default function FlightOpsPage() {
  const {
    drones,
    connected,
    initiateRecovery,
    manuallyFinishMission,
    sendCommand,
    recoveryStatus,
    dronesWithActiveMission,
    requestNow,
    trails,
    activeMissions,
    refreshActiveMissions,
  } = useTelemetrySocket();

  const [allSlots, setAllSlots] = useState<BaySlot[]>([]);

  const [draftMode, setDraftMode] = useState(false);
  const [draftWaypoints, setDraftWaypoints] = useState<{ lat: number; lng: number }[]>([]);
  const [draftAltitude, setDraftAltitude] = useState('');
  const [submitting, setSubmitting] = useState(false);
  const [error, setError] = useState<string | null>(null);

  const refreshBunker = async () => {
    try {
      const data = await fetchBunkerStatus();
      setAllSlots(data.slots);
    } catch {
      // non-fatal
    }
  };

  useEffect(() => {
    refreshBunker();
  }, []);

  const liveDrone = drones[0] ?? null;
  const occupiedSlot = allSlots.find((s) => s.isOccupied);
  const dockedDrone = occupiedSlot?.drone && occupiedSlot.drone.state !== 'FAULT' ? occupiedSlot.drone : null;
  const faultedDrone = occupiedSlot?.drone && occupiedSlot.drone.state === 'FAULT' ? occupiedSlot.drone : null;

  const startDraft = () => {
    setError(null);
    setDraftWaypoints([]);
    setDraftAltitude('');
    refreshBunker();
    setDraftMode(true);
  };

  const cancelDraft = () => {
    setDraftMode(false);
    setDraftWaypoints([]);
    setError(null);
  };

  const handleLaunch = async () => {
    setError(null);
    if (!dockedDrone) {
      setError('No aircraft is docked and ready.');
      return;
    }
    if (!draftAltitude || Number.isNaN(Number(draftAltitude))) {
      setError('Enter a valid altitude.');
      return;
    }
    if (draftWaypoints.length === 0) {
      setError('Place at least 1 waypoint.');
      return;
    }

    setSubmitting(true);
    try {
      const altitude = Number(draftAltitude);
      const result =
        draftWaypoints.length === 1
          ? await launchMission({
              latitude: draftWaypoints[0].lat,
              longitude: draftWaypoints[0].lng,
              altitude,
              cruiseAltitude: altitude,
              droneId: dockedDrone.id,
            })
          : await launchWaypointPath({
              waypoints: draftWaypoints.map((p) => ({ latitude: p.lat, longitude: p.lng })),
              cruiseAltitude: altitude,
              droneId: dockedDrone.id,
            });

      if (result.status !== 'SUCCESS') {
        setError(result.message);
        return;
      }

      setDraftMode(false);
      setDraftWaypoints([]);
      requestNow();
      refreshActiveMissions();
      refreshBunker();
    } catch (err) {
      setError(err instanceof Error ? err.message : 'Launch failed.');
    } finally {
      setSubmitting(false);
    }
  };

  const plannedPaths: Record<string, { lat: number; lng: number }[]> = Object.fromEntries(
    Object.entries(activeMissions).map(([droneId, mission]) => [
      droneId,
      mission.waypoints.map((wp) => ({ lat: wp.latitude, lng: wp.longitude })),
    ])
  );

  let panel: ReactNode;
  if (draftMode) {
    panel = (
      <FlightOpsSidePanel
        mode="draft"
        droneId={dockedDrone?.id ?? '—'}
        draftWaypoints={draftWaypoints}
        draftAltitude={draftAltitude}
        setDraftAltitude={setDraftAltitude}
        onUndo={() => setDraftWaypoints((prev) => prev.slice(0, -1))}
        onClear={() => setDraftWaypoints([])}
        onLaunch={handleLaunch}
        onCancel={cancelDraft}
        submitting={submitting}
        error={error}
      />
    );
  } else if (liveDrone) {
    panel = (
      <FlightOpsSidePanel
        mode="live"
        drone={liveDrone}
        hasActiveMission={dronesWithActiveMission.has(liveDrone.droneId)}
        recoveryStatus={recoveryStatus[liveDrone.droneId]}
        onRecall={initiateRecovery}
        onFinish={manuallyFinishMission}
      />
    );
  } else if (dockedDrone) {
    panel = (
      <FlightOpsSidePanel
        mode="docked"
        droneId={dockedDrone.id}
        batteryLevel={dockedDrone.batteryLevel}
        onLaunchMission={startDraft}
      />
    );
  } else if (faultedDrone) {
    panel = (
      <FlightOpsSidePanel
        mode="fault"
        droneId={faultedDrone.id}
        batteryLevel={faultedDrone.batteryLevel}
        onRunDiagnostics={() =>
          sendCommand(faultedDrone.id, DRONE_COMMAND_CODES.STATUS_CHECK, 'Diagnostics requested from Flight Ops')
        }
      />
    );
  } else {
    panel = <FlightOpsSidePanel mode="empty" />;
  }

  const statusLabel = liveDrone
    ? `${liveDrone.droneId} · In Flight`
    : dockedDrone
      ? `${dockedDrone.id} · Docked`
      : faultedDrone
        ? `${faultedDrone.id} · Fault`
        : 'No Aircraft';

  const batteryLevel = liveDrone?.batteryLevel ?? dockedDrone?.batteryLevel ?? faultedDrone?.batteryLevel;

  return (
    <PageLayout>
      <MapArea>
        <FlightOpsMap
          drones={drones}
          trails={trails}
          plannedPaths={plannedPaths}
          draftMode={draftMode}
          draftWaypoints={draftWaypoints}
          onDraftMapClick={(pt) => setDraftWaypoints((prev) => [...prev, pt])}
        />
        <FlightOpsTopBar
          connected={connected}
          statusLabel={statusLabel}
          batteryLevel={batteryLevel}
          draftMode={draftMode}
          canLaunch={!draftMode && !liveDrone && !!dockedDrone}
          onStartDraft={startDraft}
          onCancelDraft={cancelDraft}
        />
        {!draftMode && <FlightOpsHud drone={liveDrone} />}
      </MapArea>
      {panel}
    </PageLayout>
  );
}

const PageLayout = styled.div`
  display: flex;
  height: 100vh;
`;

const MapArea = styled.div`
  position: relative;
  flex: 1;
`;