import styled from 'styled-components';
import { History } from 'lucide-react';
import type { MissionHistoryEntry } from '../../types/mission';

const outcomeTone: Record<MissionHistoryEntry['outcome'], 'ok' | 'danger' | 'neutral'> = {
  COMPLETED: 'ok',
  FAILED: 'danger',
  ABORTED: 'neutral',
};

interface MissionHistoryTableProps {
  entries: MissionHistoryEntry[];
}

export default function MissionHistoryTable({
  entries,
}: MissionHistoryTableProps) {
  return (
    <Section>
      <SectionHeader>
        <History size={16} />
        <h2>Mission History</h2>
      </SectionHeader>

      <TableWrapper>
        <Table>
          <thead>
            <tr>
              <th>ID</th>
              <th>Drone</th>
              <th>Target Coords</th>
              <th>Launch Time</th>
              <th>Outcome</th>
              <th></th>
            </tr>
          </thead>
          <tbody>
            {entries.map((m) => (
              <tr key={m.missionId}>
                <td>{m.missionId}</td>
                <td>{m.droneId}</td>
                <td>{m.targetCoords}</td>
                <td>{m.launchTime}</td>
                <td>
                  <OutcomeTag $tone={outcomeTone[m.outcome]}>{m.outcome}</OutcomeTag>
                </td>
                <td>
                   
                    <DetailsLink>Details</DetailsLink>
                  
                </td>
              </tr>
            ))}
          </tbody>
        </Table>
        {entries.length === 0 && (
          <EmptyNote>
            Mission history isn't tracked by the backend yet — this table will populate once
            mission persistence is added.
          </EmptyNote>
        )}
      </TableWrapper>
    </Section>
  );
}

const Section = styled.section`
  display: flex;
  flex-direction: column;
  gap: 12px;
`;

const SectionHeader = styled.div`
  display: flex;
  align-items: center;
  gap: 8px;

  h2 {
    margin: 0;
    font-size: 16px;
    color: ${({ theme }) => theme.colors.textPrimary};
  }
`;

const TableWrapper = styled.div`
  background: ${({ theme }) => theme.colors.white};
  border: 1px solid ${({ theme }) => theme.colors.navy100};
  border-radius: ${({ theme }) => theme.radius};
  overflow: hidden;
`;

const Table = styled.table`
  width: 100%;
  border-collapse: collapse;
  font-size: 13px;

  th {
    text-align: left;
    font-size: 11px;
    text-transform: uppercase;
    letter-spacing: 0.04em;
    color: ${({ theme }) => theme.colors.textSecondary};
    padding: 12px 20px;
    border-bottom: 1px solid ${({ theme }) => theme.colors.navy100};
  }

  td {
    padding: 14px 20px;
    border-bottom: 1px solid ${({ theme }) => theme.colors.navy50};
    color: ${({ theme }) => theme.colors.textPrimary};
  }

  tr:last-child td {
    border-bottom: none;
  }
`;

const OutcomeTag = styled.span<{ $tone: 'ok' | 'danger' | 'neutral' }>`
  font-size: 11px;
  font-weight: 600;
  padding: 2px 8px;
  border-radius: 999px;
  background: ${({ theme, $tone }) =>
    $tone === 'ok' ? '#E6F6EC' : $tone === 'danger' ? theme.colors.dangerBg : theme.colors.navy100};
  color: ${({ theme, $tone }) =>
    $tone === 'ok' ? '#2E8B57' : $tone === 'danger' ? theme.colors.danger : theme.colors.navy700};
`;

const DetailsLink = styled.span`
  color: ${({ theme }) => theme.colors.navy700};
  font-weight: 600;
  cursor: pointer;
  font-size: 12px;
`;

const EmptyNote = styled.div`
  padding: 20px;
  font-size: 12px;
  color: ${({ theme }) => theme.colors.textSecondary};
  text-align: center;
`;

