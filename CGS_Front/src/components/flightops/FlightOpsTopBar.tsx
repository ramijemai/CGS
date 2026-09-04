import styled from 'styled-components';

interface FlightOpsTopBarProps {
  connected: boolean;
  statusLabel: string;
  batteryLevel?: number;
  draftMode: boolean;
  canLaunch: boolean;
  onStartDraft: () => void;
  onCancelDraft: () => void;
}

export default function FlightOpsTopBar({
  connected,
  statusLabel,
  batteryLevel,
  draftMode,
  canLaunch,
  onStartDraft,
  onCancelDraft,
}: FlightOpsTopBarProps) {
  return (
    <Bar>
      <Left>
        <ConnectionDot $connected={connected} />
        <Label>{statusLabel}</Label>
        {batteryLevel !== undefined && (
          <>
            <Divider />
            <Label>{Math.round(batteryLevel)}%</Label>
          </>
        )}
      </Left>
      {draftMode ? (
        <CancelButton onClick={onCancelDraft}>Cancel Mission</CancelButton>
      ) : (
        <NewMissionButton onClick={onStartDraft} disabled={!canLaunch}>+ New Mission</NewMissionButton>
      )}
    </Bar>
  );
}

const Bar = styled.div`
  position: absolute;
  top: 12px;
  right: 12px;
  left: 12px;
  display: flex;
  align-items: center;
  justify-content: space-between;
  background: rgba(11, 31, 58, 0.9);
  border-radius: 10px;
  padding: 8px 14px;
  z-index: 10;
`;

const Left = styled.div`
  display: flex;
  align-items: center;
  gap: 8px;
  color: #ffffff;
  font-size: 12px;
`;

const ConnectionDot = styled.span<{ $connected: boolean }>`
  width: 7px;
  height: 7px;
  border-radius: 50%;
  background: ${({ $connected }) => ($connected ? '#3fbf7f' : '#e05252')};
`;

const Label = styled.span`
  font-weight: 600;
`;

const Divider = styled.span`
  width: 1px;
  height: 12px;
  background: rgba(255, 255, 255, 0.25);
`;

const NewMissionButton = styled.button`
  background: ${({ theme }) => theme.colors.navy900};
  border: 1px solid rgba(255, 255, 255, 0.2);
  color: #ffffff;
  font-size: 12px;
  font-weight: 600;
  padding: 7px 14px;
  border-radius: 7px;
  cursor: pointer;

  &:disabled {
    opacity: 0.4;
    cursor: default;
  }
`;

const CancelButton = styled(NewMissionButton)`
  background: transparent;
  border-color: ${({ theme }) => theme.colors.danger};
  color: ${({ theme }) => theme.colors.danger};
`;