import styled from 'styled-components';
import MissionPathMap from './MissionPathMap';
import type { MissionApiEntry } from '../../types/mission';

interface MissionMapModalProps {
  mission: MissionApiEntry;
  onClose: () => void;
}

const patternLabels: Record<string, string> = {
  SINGLE_POINT: 'Single Target',
  WAYPOINT_PATH: 'Waypoint Path',
};

export default function MissionMapModal({ mission, onClose }: MissionMapModalProps) {
  return (
    <Overlay onClick={onClose}>
      <Panel onClick={(e) => e.stopPropagation()}>
        <Header>
          <div>
            <Title>Drone {mission.droneId}</Title>
            <Subtitle>
              {mission.missionId} — {patternLabels[mission.pattern] ?? mission.pattern}
            </Subtitle>
          </div>
          <CloseButton onClick={onClose}>Close</CloseButton>
        </Header>

        <MissionPathMap
          pattern={mission.pattern}
          waypoints={mission.waypoints}
          currentWaypointIndex={mission.currentWaypointIndex}
          currentPosition={null}
        />
      </Panel>
    </Overlay>
  );
}

const Overlay = styled.div`
  position: fixed;
  inset: 0;
  background: rgba(11, 31, 58, 0.35);
  display: flex;
  align-items: center;
  justify-content: center;
  z-index: 50;
`;

const Panel = styled.div`
  background: ${({ theme }) => theme.colors.white};
  border-radius: ${({ theme }) => theme.radius};
  padding: 20px 24px 24px;
  width: 820px;
  max-width: 92vw;
  max-height: 90vh;
  overflow-y: auto;
  box-shadow: 0 12px 32px rgba(11, 31, 58, 0.2);
`;

const Header = styled.div`
  display: flex;
  align-items: flex-start;
  justify-content: space-between;
  margin-bottom: 12px;
`;

const Title = styled.h2`
  margin: 0;
  font-size: 16px;
  color: ${({ theme }) => theme.colors.textPrimary};
`;

const Subtitle = styled.div`
  font-size: 12px;
  color: ${({ theme }) => theme.colors.textSecondary};
  margin-top: 2px;
`;

const CloseButton = styled.button`
  border: 1px solid ${({ theme }) => theme.colors.navy100};
  background: transparent;
  color: ${({ theme }) => theme.colors.textSecondary};
  border-radius: 6px;
  padding: 6px 12px;
  font-size: 12px;
  cursor: pointer;
`;