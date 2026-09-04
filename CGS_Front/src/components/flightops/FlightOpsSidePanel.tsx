import styled from 'styled-components';
import { droneStateMeta } from '../../lib/droneState';
import BatteryBar from '../common/BatteryBar';
import DroneIllustration from '../common/DroneIllustration';
import { computeRouteLengthMeters, DEFAULT_SIM_SPEED_MPS } from '../../utils/routeMath';
import type { ActiveMissionDrone } from '../../hooks/useTelemetrySocket';

interface DraftPoint {
  lat: number;
  lng: number;
}

interface DraftProps {
  mode: 'draft';
  droneId: string;
  draftWaypoints: DraftPoint[];
  draftAltitude: string;
  setDraftAltitude: (v: string) => void;
  onUndo: () => void;
  onClear: () => void;
  onLaunch: () => void;
  onCancel: () => void;
  submitting: boolean;
  error: string | null;
}

interface LiveProps {
  mode: 'live';
  drone: ActiveMissionDrone;
  hasActiveMission: boolean;
  recoveryStatus?: 'PENDING' | 'DOCKED' | 'FAILED';
  onRecall: (droneId: string) => void;
  onFinish: (droneId: string) => void;
}

interface DockedProps {
  mode: 'docked';
  droneId: string;
  batteryLevel: number;
  onLaunchMission: () => void;
}

interface FaultProps {
  mode: 'fault';
  droneId: string;
  batteryLevel: number;
  onRunDiagnostics: () => void;
}

interface EmptyProps {
  mode: 'empty';
}

type FlightOpsSidePanelProps = DraftProps | LiveProps | DockedProps | FaultProps | EmptyProps;

export default function FlightOpsSidePanel(props: FlightOpsSidePanelProps) {
  if (props.mode === 'empty') {
    return (
      <Panel>
        <DroneIllustration size={80} tone="navy" />
        <EmptyState>No aircraft connected. Check the MAVLink/SITL connection.</EmptyState>
      </Panel>
    );
  }

  if (props.mode === 'fault') {
    const { droneId, batteryLevel, onRunDiagnostics } = props;
    return (
      <Panel>
        <Header>
          <DroneId>{droneId}</DroneId>
          <StatusPill $tone="danger">Fault</StatusPill>
        </Header>

        <BatteryBar level={batteryLevel} />

        <DangerButton onClick={onRunDiagnostics}>Run Diagnostics</DangerButton>

        <Note>
          No detailed fault report is available yet — the backend doesn't currently track a
          fault reason. Running diagnostics sends a status-check command and logs it, but
          resolving the fault state itself isn't automated.
        </Note>
      </Panel>
    );
  }

  if (props.mode === 'docked') {
    const { droneId, batteryLevel, onLaunchMission } = props;
    return (
      <Panel>
        <Header>
          <DroneId>{droneId}</DroneId>
          <StatusPill $tone="neutral">Docked</StatusPill>
        </Header>

        <BatteryBar level={batteryLevel} />

        <Note>Aircraft is docked and ready. Start a new mission to begin.</Note>

        <LaunchButton onClick={onLaunchMission}>+ New Mission</LaunchButton>
      </Panel>
    );
  }

  if (props.mode === 'live') {
    const { drone, hasActiveMission, recoveryStatus, onRecall, onFinish } = props;
    const meta = droneStateMeta[drone.state];
    const awaitingRecall = drone.state === 'IN_FLIGHT' && !hasActiveMission;
    const canRecall = drone.state !== 'RETURNING' && drone.state !== 'LANDING';

    return (
      <Panel>
        <Header>
          <DroneId>{drone.droneId}</DroneId>
          <StatusPill $tone={awaitingRecall ? 'warning' : meta.tone}>
            {awaitingRecall ? 'Awaiting Recall' : meta.label}
          </StatusPill>
        </Header>

        <BatteryBar level={drone.batteryLevel} />

        <InfoGrid>
          <InfoItem><InfoLabel>Speed</InfoLabel><InfoValue>{drone.speed.toFixed(1)} m/s</InfoValue></InfoItem>
          <InfoItem><InfoLabel>Altitude</InfoLabel><InfoValue>{drone.altitude.toFixed(1)}m</InfoValue></InfoItem>
          <InfoItem><InfoLabel>Heading</InfoLabel><InfoValue>{drone.heading.toFixed(0)}°</InfoValue></InfoItem>
          <InfoItem><InfoLabel>Elapsed</InfoLabel><InfoValue>{Math.floor(drone.elapsedSeconds / 60)}m</InfoValue></InfoItem>
        </InfoGrid>

        {canRecall && (
          <MissionActions>
            <RecallButton onClick={() => onRecall(drone.droneId)} disabled={recoveryStatus === 'PENDING'}>
              {recoveryStatus === 'PENDING' ? 'Recalling…' : 'Recall'}
            </RecallButton>
            {hasActiveMission && <FinishButton onClick={() => onFinish(drone.droneId)}>Finish</FinishButton>}
          </MissionActions>
        )}
      </Panel>
    );
  }

  // draft mode
  const {
    droneId,
    draftWaypoints,
    draftAltitude,
    setDraftAltitude,
    onUndo,
    onClear,
    onLaunch,
    onCancel,
    submitting,
    error,
  } = props;

  const routeLength = computeRouteLengthMeters(draftWaypoints);
  const estSeconds = routeLength > 0 ? routeLength / DEFAULT_SIM_SPEED_MPS : 0;
  const estMinutes = Math.floor(estSeconds / 60);
  const estRemSeconds = Math.round(estSeconds % 60);
  const missionType = draftWaypoints.length === 1 ? 'Single Target' : 'Waypoint Path';

  return (
    <Panel>
      <Header>
        <DroneName>New Mission</DroneName>
      </Header>

      <Note>Aircraft: <strong>{droneId}</strong></Note>

      <InfoGrid>
        <InfoItem><InfoLabel>Mission Type</InfoLabel><InfoValue>{missionType}</InfoValue></InfoItem>
        <InfoItem>
          <InfoLabel>Est. Flight Time</InfoLabel>
          <InfoValue>{draftWaypoints.length >= 2 ? `${estMinutes}m ${estRemSeconds}s` : '—'}</InfoValue>
        </InfoItem>
        <InfoItem><InfoLabel>Route Length</InfoLabel><InfoValue>{Math.round(routeLength)}m</InfoValue></InfoItem>
        <InfoItem><InfoLabel>Waypoints</InfoLabel><InfoValue>{draftWaypoints.length} pts</InfoValue></InfoItem>
      </InfoGrid>

      <Note>
        Flight speed in this plan is a display estimate — the real aircraft is commanded over
        MAVLink and flies at whatever speed ArduPilot actually executes.
      </Note>

      <Field>
        <Label>Altitude (m)</Label>
        <Input value={draftAltitude} onChange={(e) => setDraftAltitude(e.target.value)} placeholder="30" />
      </Field>

      <WaypointActions>
        <TextButton onClick={onUndo} disabled={draftWaypoints.length === 0}>Undo last</TextButton>
        <TextButton onClick={onClear} disabled={draftWaypoints.length === 0}>Clear all</TextButton>
      </WaypointActions>

      {error && <ErrorText>{error}</ErrorText>}

      <ButtonRow>
        <CancelButton onClick={onCancel}>Cancel</CancelButton>
        <LaunchButton onClick={onLaunch} disabled={submitting || draftWaypoints.length === 0}>
          {submitting ? 'Launching…' : 'Launch'}
        </LaunchButton>
      </ButtonRow>
    </Panel>
  );
}

const Panel = styled.div`
  width: 320px;
  height: 100%;
  background: ${({ theme }) => theme.colors.white};
  border-left: 1px solid ${({ theme }) => theme.colors.navy100};
  padding: 20px;
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 14px;
  overflow-y: auto;
  box-sizing: border-box;
`;

const EmptyState = styled.div`
  color: ${({ theme }) => theme.colors.textSecondary};
  font-size: 13px;
  text-align: center;
  padding: 8px 0 20px;
`;

const Header = styled.div`
  width: 100%;
  display: flex;
  align-items: center;
  gap: 10px;
`;

const DroneId = styled.h2`
  margin: 0;
  font-size: 20px;
  color: ${({ theme }) => theme.colors.textPrimary};
`;

const DroneName = styled.h2`
  margin: 0;
  font-size: 16px;
  color: ${({ theme }) => theme.colors.textPrimary};
`;

const StatusPill = styled.span<{ $tone: 'neutral' | 'warning' | 'danger' }>`
  font-size: 11px;
  font-weight: 600;
  padding: 3px 9px;
  border-radius: 999px;
  background: ${({ theme, $tone }) =>
    $tone === 'danger' ? theme.colors.dangerBg : $tone === 'warning' ? theme.colors.warningBg : theme.colors.navy100};
  color: ${({ theme, $tone }) =>
    $tone === 'danger' ? theme.colors.danger : $tone === 'warning' ? theme.colors.warning : theme.colors.navy700};
`;

const InfoGrid = styled.div`
  width: 100%;
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 12px;
`;

const InfoItem = styled.div``;

const InfoLabel = styled.div`
  font-size: 10px;
  text-transform: uppercase;
  letter-spacing: 0.04em;
  color: ${({ theme }) => theme.colors.textSecondary};
  margin-bottom: 3px;
`;

const InfoValue = styled.div`
  font-size: 13px;
  font-weight: 600;
  color: ${({ theme }) => theme.colors.textPrimary};
`;

const Note = styled.p`
  width: 100%;
  margin: 0;
  font-size: 11px;
  line-height: 1.5;
  color: ${({ theme }) => theme.colors.textSecondary};
  background: ${({ theme }) => theme.colors.navy50};
  padding: 8px 10px;
  border-radius: 6px;
  box-sizing: border-box;

  strong {
    color: ${({ theme }) => theme.colors.textPrimary};
  }
`;

const Field = styled.div`
  width: 100%;
  display: flex;
  flex-direction: column;
  gap: 5px;
`;

const Label = styled.label`
  font-size: 11px;
  font-weight: 600;
  color: ${({ theme }) => theme.colors.textSecondary};
`;

const Input = styled.input`
  padding: 8px 10px;
  border-radius: 6px;
  font-size: 13px;
  border: 1px solid ${({ theme }) => theme.colors.navy100};
  width: 100%;
  box-sizing: border-box;
`;

const WaypointActions = styled.div`
  width: 100%;
  display: flex;
  gap: 14px;
`;

const TextButton = styled.button`
  border: none;
  background: transparent;
  color: ${({ theme }) => theme.colors.navy700};
  font-size: 11px;
  font-weight: 600;
  cursor: pointer;

  &:disabled {
    opacity: 0.4;
    cursor: default;
  }
`;

const ErrorText = styled.div`
  width: 100%;
  font-size: 12px;
  color: ${({ theme }) => theme.colors.danger};
`;

const ButtonRow = styled.div`
  width: 100%;
  display: flex;
  gap: 10px;
  margin-top: auto;

  & > * {
    flex: 1;
  }
`;

const buttonBase = `
  width: 100%;
  padding: 10px 14px;
  border-radius: 8px;
  font-size: 13px;
  font-weight: 600;
  cursor: pointer;
  box-sizing: border-box;
`;

const CancelButton = styled.button`
  ${buttonBase}
  background: transparent;
  border: 1px solid ${({ theme }) => theme.colors.navy100};
  color: ${({ theme }) => theme.colors.textSecondary};
`;

const LaunchButton = styled.button`
  ${buttonBase}
  background: ${({ theme }) => theme.colors.navy900};
  border: none;
  color: ${({ theme }) => theme.colors.white};

  &:disabled {
    opacity: 0.6;
    cursor: default;
  }
`;

const RecallButton = styled.button`
  ${buttonBase}
  background: transparent;
  border: 1px solid ${({ theme }) => theme.colors.warning};
  color: ${({ theme }) => theme.colors.warning};
  &:disabled {
    opacity: 0.6;
    cursor: default;
  }
`;

const DangerButton = styled.button`
  ${buttonBase}
  background: transparent;
  border: 1px solid ${({ theme }) => theme.colors.danger};
  color: ${({ theme }) => theme.colors.danger};
`;

const MissionActions = styled.div`
  display: flex;
  gap: 8px;
  margin-top: auto;

  & > * {
    flex: 1;
  }
`;

const FinishButton = styled(RecallButton)`
  border-color: ${({ theme }) => theme.colors.navy900};
  background: ${({ theme }) => theme.colors.navy900};
  color: ${({ theme }) => theme.colors.white};
`;