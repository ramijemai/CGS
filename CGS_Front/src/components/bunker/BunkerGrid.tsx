import styled from 'styled-components';
import BaySlotCard from './BaySlotCard';
import type { BaySlotViewModel } from '../../types/bunker';

interface BunkerGridProps {
  bays: BaySlotViewModel[];
  onViewDrone?: (droneId: string) => void;
  onInitiateRecovery?: (droneId: string) => void;
  onRunDiagnostics?: (droneId: string) => void;
}

export default function BunkerGrid({ bays, onViewDrone, onInitiateRecovery, onRunDiagnostics }: BunkerGridProps) {
  return (
    <Grid>
      {bays.map((bay) => (
        <BaySlotCard
          key={bay.slotId}
          bay={bay}
          onViewDrone={onViewDrone}
          onInitiateRecovery={onInitiateRecovery}
          onRunDiagnostics={onRunDiagnostics}
        />
      ))}
    </Grid>
  );
}

const Grid = styled.div`
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 12px;

  @media (max-width: 760px) {
    grid-template-columns: 1fr;
  }
`;