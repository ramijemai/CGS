import { useEffect, useState } from 'react';
import styled from 'styled-components';
import MissionHistoryTable from '../mission/MissionHistoryTable';
import MissionMapModal from '../mission/MissionMapModal';
import { fetchMissionHistory } from '../../api/missionApi';
import type { MissionApiEntry, MissionHistoryEntry } from '../../types/mission';
import { useTelemetrySocket } from '../../hooks/useTelemetrySocket';

export default function ReportsPage() {
  const [missions, setMissions] = useState<MissionApiEntry[]>([]);
  const [selected, setSelected] = useState<MissionApiEntry | null>(null);
  const { lastMissionFinish } = useTelemetrySocket();

  const refresh = async () => {
    try {
      const entries = await fetchMissionHistory();
      setMissions(entries);
    } catch {
      setMissions([]);
    }
  };

  useEffect(() => {
    refresh();
    const interval = window.setInterval(refresh, 8000);
    return () => window.clearInterval(interval);
  }, []);

  useEffect(() => {
    if (lastMissionFinish) void refresh();
  }, [lastMissionFinish]);

  const historyEntries: MissionHistoryEntry[] = missions.map((m) => ({
    missionId: m.missionId,
    droneId: m.droneId,
    targetCoords: `${m.target.latitude.toFixed(2)}, ${m.target.longitude.toFixed(2)}`,
    launchTime: m.launchTime,
    outcome: (m.status as MissionHistoryEntry['outcome']) ?? 'COMPLETED',
  }));

  return (
    <PageLayout>
      <Title>Reports</Title>
      <Subtitle>Completed mission history</Subtitle>

      <TableWrapper
        onClick={(e) => {
          const target = e.target as HTMLElement;
          const row = target.closest('tr');
          if (!row) return;
          const missionId = row.querySelector('td')?.textContent;
          const match = missions.find((m) => m.missionId === missionId);
          if (match) setSelected(match);
        }}
      >
        <MissionHistoryTable
          entries={historyEntries}
        />
      </TableWrapper>

      {selected && <MissionMapModal mission={selected} onClose={() => setSelected(null)} />}
    </PageLayout>
  );
}

const PageLayout = styled.div`
  padding: 24px 32px;
  display: flex;
  flex-direction: column;
  gap: 6px;
`;

const Title = styled.h1`
  margin: 0;
  font-size: 24px;
  font-weight: 700;
  color: ${({ theme }) => theme.colors.textPrimary};
`;

const Subtitle = styled.div`
  font-size: 13px;
  color: ${({ theme }) => theme.colors.textSecondary};
  margin-bottom: 12px;
`;

const TableWrapper = styled.div`
  cursor: pointer;
`;