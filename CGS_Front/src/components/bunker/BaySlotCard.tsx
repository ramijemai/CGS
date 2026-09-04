import styled, { type DefaultTheme } from 'styled-components';
import { Box, Lock, Unlock, AlertTriangle, Loader2 } from 'lucide-react';
import type { BaySlotViewModel } from '../../types/bunker';

interface BaySlotCardProps {
  bay: BaySlotViewModel;
  onViewDrone?: (droneId: string) => void;
  onInitiateRecovery?: (droneId: string) => void;
  onRunDiagnostics?: (droneId: string) => void;
}

type Tone = 'neutral' | 'warning' | 'danger' | 'muted';

const statusMeta: Record<BaySlotViewModel['status'], { label: string; tone: Tone }> = {
  OCCUPIED: { label: 'Occupied', tone: 'neutral' },
  VACANT: { label: 'Vacant', tone: 'muted' },
  FAULT: { label: 'Fault', tone: 'danger' },
  HATCH_OPENING: { label: 'Hatch Opening', tone: 'warning' },
  INCOMING: { label: 'Incoming', tone: 'warning' },
};

function batteryTone(level: number): Tone {
  if (level <= 15) return 'danger';
  if (level <= 35) return 'warning';
  return 'neutral';
}

export default function BaySlotCard({ bay, onViewDrone, onInitiateRecovery, onRunDiagnostics }: BaySlotCardProps) {
  const meta = statusMeta[bay.status];
  const label = `Bay ${String(bay.slotId).padStart(2, '0')}`;

  if (bay.status === 'VACANT') {
    return (
      <Card $tone="muted">
        <CardHeader>
          <BayTitle>{label}</BayTitle>
          <StatusPill $tone="muted">Vacant</StatusPill>
        </CardHeader>
        <EmptyState>
          <Box size={28} strokeWidth={1.5} />
          <span>Ready for landing</span>
        </EmptyState>
      </Card>
    );
  }

  const drone = bay.drone;

  return (
    <Card $tone={meta.tone}>
      <CardHeader>
        <BayTitle $tone={meta.tone}>{label}</BayTitle>
        <StatusPill $tone={meta.tone}>
          {bay.status === 'HATCH_OPENING' && <Loader2 size={12} className="spin" />}
          {bay.status === 'FAULT' && <AlertTriangle size={12} />}
          {meta.label}
        </StatusPill>
      </CardHeader>

      {drone && (
        <DroneRow>
          <DroneIdBlock>
            <DroneId>
              {drone.id}
              {bay.status === 'INCOMING' && <IncomingTag>(Incoming)</IncomingTag>}
            </DroneId>
            <BatteryRow>
              <span>Battery {drone.batteryLevel}%</span>
              <BatteryTrack>
                <BatteryFill $pct={drone.batteryLevel} $tone={batteryTone(drone.batteryLevel)} />
              </BatteryTrack>
            </BatteryRow>
          </DroneIdBlock>

          <HatchBlock>
            {bay.hatchState === 'OPEN' ? <Unlock size={16} /> : <Lock size={16} />}
            <span>{bay.hatchState === 'UNKNOWN' ? '—' : bay.hatchState === 'OPEN' ? 'Open' : 'Closed'}</span>
          </HatchBlock>

          {bay.status === 'FAULT' && (
            <ActionButton $danger onClick={() => onRunDiagnostics?.(drone.id)}>
              Run Diagnostics
            </ActionButton>
          )}
          {bay.status === 'HATCH_OPENING' && (
            <ActionButton onClick={() => onInitiateRecovery?.(drone.id)}>
              Initiate Recovery
            </ActionButton>
          )}
          {bay.status === 'OCCUPIED' && (
            <ActionButton onClick={() => onViewDrone?.(drone.id)}>
              View Drone
            </ActionButton>
          )}
        </DroneRow>
      )}
    </Card>
  );
}

function toneColors(theme: DefaultTheme, tone: Tone) {
  switch (tone) {
    case 'danger':
      return { border: theme.colors.danger, bg: theme.colors.dangerBg, text: theme.colors.danger };
    case 'warning':
      return { border: theme.colors.warning, bg: theme.colors.warningBg, text: theme.colors.warning };
    case 'muted':
      return { border: theme.colors.navy100, bg: theme.colors.navy50, text: theme.colors.textSecondary };
    default:
      return { border: theme.colors.navy100, bg: theme.colors.white, text: theme.colors.navy900 };
  }
}

const Card = styled.div<{ $tone: Tone }>`
  background: ${({ theme, $tone }) => toneColors(theme, $tone).bg};
  border: 1px solid ${({ theme, $tone }) => toneColors(theme, $tone).border};
  border-radius: 8px;
  padding: 14px 16px;
  box-shadow: 0 1px 2px rgba(11, 31, 58, 0.05);
  min-height: 136px;
`;

const CardHeader = styled.div`
  display: flex;
  align-items: center;
  justify-content: space-between;
  margin-bottom: 12px;
`;

const BayTitle = styled.h3<{ $tone?: Tone }>`
  margin: 0;
  font-size: 15px;
  font-weight: 600;
  color: ${({ theme, $tone }) => ($tone === 'danger' ? theme.colors.danger : theme.colors.textPrimary)};
`;

const StatusPill = styled.span<{ $tone: Tone }>`
  display: inline-flex;
  align-items: center;
  gap: 4px;
  font-size: 11px;
  font-weight: 600;
  padding: 3px 10px;
  border-radius: 999px;
  color: ${({ theme, $tone }) => toneColors(theme, $tone).text};
  background: ${({ theme, $tone }) => ($tone === 'neutral' ? theme.colors.navy100 : 'transparent')};

  .spin {
    animation: spin 1.2s linear infinite;
  }
  @keyframes spin {
    to { transform: rotate(360deg); }
  }
`;

const EmptyState = styled.div`
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 8px;
  padding: 28px 0;
  color: ${({ theme }) => theme.colors.textSecondary};
  font-size: 13px;
`;

const DroneRow = styled.div`
  display: grid;
  grid-template-columns: 1fr auto auto;
  align-items: center;
  gap: 12px;

  @media (max-width: 1100px) {
    grid-template-columns: 1fr auto;
  }
`;

const DroneIdBlock = styled.div`
  min-width: 0;
`;

const DroneId = styled.div`
  font-weight: 600;
  font-size: 14px;
  color: ${({ theme }) => theme.colors.textPrimary};
  display: flex;
  align-items: center;
  gap: 6px;
`;

const IncomingTag = styled.span`
  font-size: 11px;
  font-weight: 400;
  color: ${({ theme }) => theme.colors.warning};
`;

const BatteryRow = styled.div`
  display: flex;
  align-items: center;
  gap: 8px;
  margin-top: 6px;
  font-size: 12px;
  color: ${({ theme }) => theme.colors.textSecondary};
`;

const BatteryTrack = styled.div`
  width: 90px;
  height: 6px;
  border-radius: 4px;
  background: ${({ theme }) => theme.colors.navy100};
  overflow: hidden;
`;

const BatteryFill = styled.div<{ $pct: number; $tone: Tone }>`
  height: 100%;
  width: ${({ $pct }) => Math.max(0, Math.min(100, $pct))}%;
  background: ${({ theme, $tone }) =>
    $tone === 'danger' ? theme.colors.danger : $tone === 'warning' ? theme.colors.warning : theme.colors.navy700};
`;

const HatchBlock = styled.div`
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 2px;
  font-size: 11px;
  color: ${({ theme }) => theme.colors.textSecondary};
`;

const ActionButton = styled.button<{ $danger?: boolean }>`
  border-radius: 6px;
  padding: 8px 14px;
  font-size: 12px;
  font-weight: 600;
  cursor: pointer;
  background: transparent;
  border: 1px solid ${({ theme, $danger }) => ($danger ? theme.colors.danger : theme.colors.navy700)};
  color: ${({ theme, $danger }) => ($danger ? theme.colors.danger : theme.colors.navy700)};

  &:hover {
    background: ${({ theme, $danger }) => ($danger ? theme.colors.dangerBg : theme.colors.navy50)};
  }
`;