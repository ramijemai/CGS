import { useEffect, useState, useCallback } from 'react';
import { useNavigate } from 'react-router-dom';
import styled from 'styled-components';
import { fetchBunkerStatus } from '../../api/BunkerApi';
import { useFleetDrones } from '../../hooks/useFleetDrones';
import { useDroneFlightStats } from '../../hooks/useDroneFlightStats';
import { DRONE_COMMAND_CODES } from '../../lib/droneCommands';
import { fetchMissionHistory } from '../../api/missionApi';
import type { BaySlot, BaySlotViewModel } from '../../types/bunker';
import type { MissionApiEntry } from '../../types/mission';
import { BatteryCharging, History, Radio, CheckCircle2, XCircle, AlertTriangle } from 'lucide-react';
import BunkerImageAsset from '../../assets/bunkerimage.png';
import HangarEmptyAsset from '../../assets/hangarEmpty.png';

const POLL_INTERVAL_MS = 5000;
const MISSION_HISTORY_LIMIT = 5;
const READY_BATTERY_THRESHOLD = 20; // mirrors Drone::isReadyForMission() on the backend

function toViewModel(slot: BaySlot): BaySlotViewModel {
  if (!slot.isOccupied) {
    return { ...slot, status: 'VACANT', hatchState: 'CLOSED' };
  }
  return { ...slot, status: 'OCCUPIED', hatchState: 'UNKNOWN' };
}

function formatFlightTime(totalSeconds: number): string {
  const hours = Math.floor(totalSeconds / 3600);
  const minutes = Math.floor((totalSeconds % 3600) / 60);
  if (hours === 0) return `${minutes}m`;
  return `${hours}h ${minutes}m`;
}

export default function BunkerPage() {
  const navigate = useNavigate();
  const { fleet, connected, sendCommand } = useFleetDrones();
  const liveDrone = fleet[0] ?? null;
  const { stats } = useDroneFlightStats(liveDrone?.droneId ?? null);

  const [bays, setBays] = useState<BaySlotViewModel[]>([]);
  const [recentMissions, setRecentMissions] = useState<MissionApiEntry[]>([]);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState<string | null>(null);

  const loadStatus = useCallback(async () => {
    try {
      const data = await fetchBunkerStatus();
      setBays(data.slots.map(toViewModel));
      setError(null);
    } catch (err) {
      setError(err instanceof Error ? err.message : 'Failed to load bunker status.');
    } finally {
      setLoading(false);
    }
  }, []);

  useEffect(() => {
    loadStatus();
    const interval = setInterval(loadStatus, POLL_INTERVAL_MS);
    return () => clearInterval(interval);
  }, [loadStatus]);

  useEffect(() => {
    const loadMissionHistory = async () => {
      try {
        const entries = await fetchMissionHistory();
        setRecentMissions(entries.slice(0, MISSION_HISTORY_LIMIT));
      } catch {
        setRecentMissions([]);
      }
    };
    loadMissionHistory();
    const interval = setInterval(loadMissionHistory, 8000);
    return () => clearInterval(interval);
  }, []);

  const occupied = bays.filter((b) => b.status !== 'VACANT').length;
  const activeDrone = bays.find((b) => b.drone)?.drone;

  const primaryBay = bays[0];
  const bayIsOccupied = primaryBay?.isOccupied ?? false;
  const schematicImage = bayIsOccupied ? BunkerImageAsset : HangarEmptyAsset;
  const schematicAlt = bayIsOccupied ? 'Bunker with docked aircraft' : 'Empty bunker';

  const isDocked = liveDrone?.state === 'DOCKED';
  const isFault = liveDrone?.state === 'FAULT';
  const battery = liveDrone?.batteryLevel ?? 0;
  const batteryOk = battery >= READY_BATTERY_THRESHOLD;
  const canFly = connected && isDocked && batteryOk;

  const checklist = [
    { label: 'Aircraft connected', ok: connected },
    { label: `Battery above ${READY_BATTERY_THRESHOLD}%`, ok: !!liveDrone && batteryOk },
    { label: 'Aircraft docked', ok: isDocked },
  ];

  return (
    <PageLayout>
      <MainColumn>
        <Header>
          <HeaderCopy>
            <Eyebrow>GROUND CONTROL / STORAGE</Eyebrow>
            <Title>Bunker operations</Title>
            <Subtitle>Aircraft readiness, bay allocation and recovery status</Subtitle>
          </HeaderCopy>
          <ConnectionState><Radio size={14} /> {connected ? 'System nominal' : 'Disconnected'}</ConnectionState>
        </Header>

        {isFault && (
          <FaultBanner>
            <AlertTriangle size={16} strokeWidth={2} />
            <FaultText>
              Aircraft reporting a fault. No detailed reason is available from the backend yet —
              run diagnostics to log a status check.
            </FaultText>
            <FaultButton
              onClick={() =>
                liveDrone &&
                sendCommand(liveDrone.droneId, DRONE_COMMAND_CODES.STATUS_CHECK, 'Diagnostics requested from Bunker page')
              }
            >
              Run Diagnostics
            </FaultButton>
          </FaultBanner>
        )}

        <SchematicWrapper>
          <BunkerImageContainer>
            {loading && bays.length === 0 ? (
              <SchematicSkeleton />
            ) : (
              <BunkerImage src={schematicImage} alt={schematicAlt} />
            )}
          </BunkerImageContainer>

          {/* Top-left: real pre-flight checklist, replaces the old fake Weather widget */}
          <ChecklistWidget>
            <WidgetLabel>Before You Fly</WidgetLabel>
            <ChecklistRows>
              {checklist.map((item) => (
                <ChecklistRow key={item.label}>
                  {item.ok ? (
                    <CheckCircle2 size={13} color="#3fbf7f" strokeWidth={2} />
                  ) : (
                    <XCircle size={13} color="#C0392B" strokeWidth={2} />
                  )}
                  <ChecklistLabel $ok={item.ok}>{item.label}</ChecklistLabel>
                </ChecklistRow>
              ))}
            </ChecklistRows>
          </ChecklistWidget>

          

          {/* Top-right */}
          <StatusWidget>
            <WidgetIcon>▣</WidgetIcon>
            <WidgetContent>
              <WidgetLabel>Status</WidgetLabel>
              <WidgetValue>{loading ? 'Syncing' : 'Ready'}</WidgetValue>
              <WidgetDetail>Bay door: {loading ? 'pending' : bayIsOccupied ? 'closed' : 'open'}</WidgetDetail>
            </WidgetContent>
          </StatusWidget>


<StatsWidget>
            <WidgetIcon>◈</WidgetIcon>
            <WidgetContent>
              <WidgetLabel>Flight Stats</WidgetLabel>
              <WidgetValue>{stats?.totalFlights ?? '—'} flights</WidgetValue>
              <WidgetDetail>{stats ? formatFlightTime(stats.totalFlightSeconds) : '—'} total airtime</WidgetDetail>
            </WidgetContent>
          </StatsWidget>
          {/* Mid-right */}
          
          <ActiveDroneWidget>
            <WidgetIcon>✦</WidgetIcon>
            <WidgetContent>
              <WidgetLabel>Active Aircraft</WidgetLabel>
              <WidgetValue>{activeDrone?.id ?? 'No aircraft'}</WidgetValue>
              <WidgetDetail>
                {activeDrone ? `Battery: ${activeDrone.batteryLevel}% · ${activeDrone.state}` : 'Bay vacant'}
              </WidgetDetail>
            </WidgetContent>
            
          </ActiveDroneWidget>



          {/* Bottom bar */}
          <BayOverviewOverlay>
            <OverviewHeader>
              <span>Bay overview</span>
              <strong>{occupied} occupied · {bays.length - occupied} vacant</strong>
            </OverviewHeader>

            <BayPillList>
              {bays.length === 0 ? (
                <EmptyBayState>Waiting for bunker telemetry…</EmptyBayState>
              ) : (
                bays.map((bay) => (
                  <BayChip key={bay.slotId} $occupied={bay.isOccupied}>
                    <BayChipNumber>{bay.slotId}</BayChipNumber>
                    <BayChipInfo>
                      <span>{bay.isOccupied ? bay.drone?.id ?? 'Occupied' : 'Vacant'}</span>
                      <small>{bay.isOccupied ? bay.drone?.state ?? 'Docked' : 'Ready'}</small>
                    </BayChipInfo>
                  </BayChip>
                ))
              )}
            </BayPillList>
          </BayOverviewOverlay>
        </SchematicWrapper>

        <InsightGrid>
          <BatteryWidget>
            <InsightHeader>
              <InsightTitle><BatteryCharging size={16} /> Battery level</InsightTitle>
              <BatteryValue>{liveDrone ? `${battery}%` : '—'}</BatteryValue>
            </InsightHeader>
            <BatteryTrack>
              <BatteryFill $level={battery} />
            </BatteryTrack>
            <InsightDetail>
              {liveDrone ? `${liveDrone.droneId} · ${batteryOk ? 'Ready for mission' : 'Needs charging'}` : 'No aircraft connected'}
            </InsightDetail>
          </BatteryWidget>

          <RecentMissionsWidget>
            <InsightHeader>
              <InsightTitle><History size={16} /> Recent missions</InsightTitle>
              <ViewReportsButton type="button" onClick={() => navigate('/reports')}>View all</ViewReportsButton>
            </InsightHeader>
            {recentMissions.length === 0 ? (
              <EmptyMissionState>No missions recorded yet.</EmptyMissionState>
            ) : (
              <MissionList>
                {recentMissions.map((mission) => (
                  <MissionRow key={mission.missionId} onClick={() => navigate('/reports')}>
                    <MissionRowTop>
                      <MissionId>{mission.missionId}</MissionId>
                      <MissionStatus>{mission.status}</MissionStatus>
                    </MissionRowTop>
                    <MissionRowDetail>{mission.pattern === 'SINGLE_POINT' ? 'Single target' : 'Waypoint path'} · {mission.launchTime}</MissionRowDetail>
                  </MissionRow>
                ))}
              </MissionList>
            )}
          </RecentMissionsWidget>
        </InsightGrid>

        <GoFlyButton disabled={!canFly} onClick={() => canFly && navigate('/missions')}>
          Go Fly
        </GoFlyButton>

        {error && <ErrorBanner>{error}</ErrorBanner>}
      </MainColumn>
    </PageLayout>
  );
}

const PageLayout = styled.div`
  display: block;
  padding: 28px 32px;
  max-width: 1450px;
  margin: 0 auto;

  @media (max-width: 900px) {
    padding: 20px;
  }
`;

const MainColumn = styled.div`
  display: flex;
  flex-direction: column;
  gap: 16px;
`;

const Header = styled.div`
  display: flex;
  align-items: flex-start;
  justify-content: space-between;
  gap: 16px;
`;

const HeaderCopy = styled.div``;

const Eyebrow = styled.div`
  color: ${({ theme }) => theme.colors.navy500};
  font-size: 10px;
  font-weight: 700;
  letter-spacing: 0.12em;
  margin-bottom: 5px;
`;

const Title = styled.h1`
  margin: 0;
  font-size: 26px;
  font-weight: 700;
  color: ${({ theme }) => theme.colors.textPrimary};
`;

const Subtitle = styled.div`
  color: ${({ theme }) => theme.colors.textSecondary};
  font-size: 12px;
  margin-top: 4px;
`;

const ConnectionState = styled.div`
  display: inline-flex;
  align-items: center;
  gap: 7px;
  color: ${({ theme }) => theme.colors.navy700};
  font-size: 11px;
  font-weight: 700;
  padding-top: 5px;
`;

const FaultBanner = styled.div`
  display: flex;
  align-items: center;
  gap: 10px;
  background: ${({ theme }) => theme.colors.dangerBg};
  border: 1px solid ${({ theme }) => theme.colors.danger};
  border-radius: 10px;
  padding: 10px 14px;
  color: ${({ theme }) => theme.colors.danger};
`;

const FaultText = styled.span`
  flex: 1;
  font-size: 12px;
  line-height: 1.4;
`;

const FaultButton = styled.button`
  background: transparent;
  border: 1px solid ${({ theme }) => theme.colors.danger};
  color: ${({ theme }) => theme.colors.danger};
  border-radius: 6px;
  padding: 7px 12px;
  font-size: 12px;
  font-weight: 600;
  cursor: pointer;
  white-space: nowrap;
`;

const SchematicWrapper = styled.div`
  position: relative;
  height: 580px;
  border: 1px solid ${({ theme }) => theme.colors.navy100};
  background: ${({ theme }) => theme.colors.white};
  border-radius: 14px;
  overflow: hidden;
`;

const BunkerImageContainer = styled.div`
  position: absolute;
  top: 28px;
  left: 0;
  right: 0;
  bottom: 140px;
  display: flex;
  align-items: center;
  justify-content: center;
  padding: 12px 40px 0;
`;

const BunkerImage = styled.img`
  max-width: 90%;
  max-height: 150%;
  object-fit: contain;
  opacity: 0.97;
`;

const shimmer = `
  @keyframes bunker-shimmer {
    0% { background-position: -400px 0; }
    100% { background-position: 400px 0; }
  }
`;

const SchematicSkeleton = styled.div`
  ${shimmer}
  width: 60%;
  height: 55%;
  border-radius: 12px;
  background: linear-gradient(90deg, #eef2f9 0%, #f7f9fc 20%, #eef2f9 40%);
  background-size: 800px 100%;
  animation: bunker-shimmer 1.4s ease-in-out infinite;
`;

const InsightGrid = styled.div`
  display: grid;
  grid-template-columns: 1fr;
  gap: 16px;
`;

const InsightWidget = styled.section`
  min-width: 0;
  padding: 16px;
  border: 1px solid ${({ theme }) => theme.colors.navy100};
  border-radius: 12px;
  background: ${({ theme }) => theme.colors.white};
  box-shadow: 0 4px 18px rgba(11, 31, 58, 0.06);
`;

const BatteryWidget = styled(InsightWidget)``;

const RecentMissionsWidget = styled(InsightWidget)``;

const InsightHeader = styled.div`
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 12px;
  margin-bottom: 16px;
`;

const InsightTitle = styled.h2`
  display: flex;
  align-items: center;
  gap: 8px;
  margin: 0;
  color: ${({ theme }) => theme.colors.textPrimary};
  font-size: 12px;
  font-weight: 700;
  text-transform: uppercase;
  letter-spacing: 0.06em;
`;

const BatteryValue = styled.strong`
  color: ${({ theme }) => theme.colors.textPrimary};
  font-size: 22px;
`;

const BatteryTrack = styled.div`
  height: 12px;
  overflow: hidden;
  border-radius: 6px;
  background: ${({ theme }) => theme.colors.navy100};
`;

const BatteryFill = styled.div<{ $level: number }>`
  width: ${({ $level }) => `${Math.min(100, Math.max(0, $level))}%`};
  height: 100%;
  border-radius: inherit;
  background: ${({ $level, theme }) => ($level >= READY_BATTERY_THRESHOLD ? theme.colors.navy700 : theme.colors.danger)};
  transition: width 180ms ease;
`;

const InsightDetail = styled.p`
  margin: 10px 0 0;
  color: ${({ theme }) => theme.colors.textSecondary};
  font-size: 11px;
`;

const ViewReportsButton = styled.button`
  padding: 0;
  border: 0;
  background: transparent;
  color: ${({ theme }) => theme.colors.navy700};
  font-size: 11px;
  font-weight: 700;
  cursor: pointer;
`;

const MissionList = styled.div`
  display: flex;
  flex-direction: column;
  gap: 8px;
`;

const MissionRow = styled.button`
  width: 100%;
  padding: 8px 10px;
  border: 1px solid ${({ theme }) => theme.colors.navy100};
  border-radius: 8px;
  background: ${({ theme }) => theme.colors.navy50};
  text-align: left;
  cursor: pointer;
`;

const MissionRowTop = styled.span`
  display: flex;
  justify-content: space-between;
  gap: 10px;
`;

const MissionId = styled.span`
  overflow: hidden;
  color: ${({ theme }) => theme.colors.textPrimary};
  font-size: 11px;
  font-weight: 700;
  text-overflow: ellipsis;
  white-space: nowrap;
`;

const MissionStatus = styled.span`
  flex-shrink: 0;
  color: ${({ theme }) => theme.colors.navy700};
  font-size: 10px;
  font-weight: 700;
  text-transform: uppercase;
`;

const MissionRowDetail = styled.span`
  display: block;
  margin-top: 3px;
  color: ${({ theme }) => theme.colors.textSecondary};
  font-size: 10px;
`;

const EmptyMissionState = styled.div`
  color: ${({ theme }) => theme.colors.textSecondary};
  font-size: 11px;
`;

const DataWidget = styled.div`
  position: absolute;
  display: flex;
  align-items: flex-start;
  gap: 10px;
  background: rgba(255, 255, 255, 0.92);
  border: 1px solid ${({ theme }) => theme.colors.navy100};
  border-radius: 12px;
  box-shadow: 0 4px 18px rgba(11, 31, 58, 0.08);
  padding: 11px 13px;
  z-index: 5;
  backdrop-filter: blur(8px);
  min-width: 200px;
  max-width: 230px;
`;

const WidgetIcon = styled.div`
  display: flex;
  align-items: center;
  justify-content: center;
  width: 32px;
  height: 32px;
  min-width: 32px;
  border-radius: 8px;
  background: linear-gradient(135deg, #e8f0ff 0%, #f0f5ff 100%);
  color: ${({ theme }) => theme.colors.navy700};
  font-size: 14px;
  font-weight: 700;
`;

const WidgetContent = styled.div`
  display: flex;
  flex-direction: column;
  gap: 3px;
  min-width: 0;
`;

const WidgetLabel = styled.span`
  font-size: 9px;
  text-transform: uppercase;
  letter-spacing: 0.08em;
  color: ${({ theme }) => theme.colors.textSecondary};
  font-weight: 700;
`;

const WidgetValue = styled.span`
  font-size: 13px;
  font-weight: 700;
  color: ${({ theme }) => theme.colors.textPrimary};
`;

const WidgetDetail = styled.span`
  font-size: 10px;
  color: ${({ theme }) => theme.colors.textSecondary};
  line-height: 1.3;
`;

const ChecklistWidget = styled(DataWidget)`
  left: 20px;
  top: 20px;
  height: 250px;
  box-sizing: border-box;
  flex-direction: column;
  gap: 8px;
`;

const ChecklistRows = styled.div`
  display: flex;
  flex-direction: column;
  gap: 5px;
`;

const ChecklistRow = styled.div`
  display: flex;
  align-items: center;
  gap: 6px;
`;

const ChecklistLabel = styled.span<{ $ok: boolean }>`
  font-size: 11px;
  color: ${({ theme, $ok }) => ($ok ? theme.colors.textPrimary : theme.colors.textSecondary)};
`;

const StatsWidget = styled(DataWidget)`
  right: 20px;
  top: 216px;
`;

const StatusWidget = styled(DataWidget)`
  right: 20px;
  top: 20px;
`;

const ActiveDroneWidget = styled(DataWidget)`
  right: 20px;
  top: 118px;
`;

const BayOverviewOverlay = styled.div`
  position: absolute;
  left: 16px;
  right: 16px;
  bottom: 14px;
  background: rgba(255, 255, 255, 0.95);
  border: 1px solid ${({ theme }) => theme.colors.navy100};
  border-radius: 12px;
  box-shadow: 0 4px 16px rgba(11, 31, 58, 0.08);
  backdrop-filter: blur(8px);
  padding: 12px 14px 11px;
  z-index: 4;
`;

const OverviewHeader = styled.div`
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 12px;
  margin-bottom: 10px;
  color: ${({ theme }) => theme.colors.textPrimary};
  font-size: 11px;
  letter-spacing: 0.08em;
  text-transform: uppercase;

  strong {
    font-size: 10px;
    color: ${({ theme }) => theme.colors.navy700};
    letter-spacing: 0.08em;
    font-weight: 700;
  }
`;

const BayPillList = styled.div`
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(160px, 1fr));
  gap: 8px;
`;

const BayChip = styled.div<{ $occupied: boolean }>`
  display: flex;
  align-items: center;
  gap: 8px;
  min-height: 54px;
  padding: 8px 10px;
  border-radius: 10px;
  border: 1px solid ${({ $occupied, theme }) => ($occupied ? theme.colors.navy300 : theme.colors.navy100)};
  background: ${({ $occupied, theme }) => ($occupied ? theme.colors.navy50 : 'rgba(255, 255, 255, 0.5)')};
`;

const BayChipNumber = styled.div`
  width: 26px;
  height: 26px;
  display: grid;
  place-items: center;
  border-radius: 8px;
  background: ${({ theme }) => theme.colors.navy50};
  color: ${({ theme }) => theme.colors.navy700};
  font-size: 11px;
  font-weight: 700;
`;

const BayChipInfo = styled.div`
  display: flex;
  flex-direction: column;
  min-width: 0;

  span {
    color: ${({ theme }) => theme.colors.textPrimary};
    font-size: 11px;
    font-weight: 700;
    white-space: nowrap;
    overflow: hidden;
    text-overflow: ellipsis;
  }

  small {
    color: ${({ theme }) => theme.colors.textSecondary};
    font-size: 10px;
    letter-spacing: 0.03em;
    margin-top: 2px;
    text-transform: uppercase;
  }
`;

const EmptyBayState = styled.div`
  display: flex;
  align-items: center;
  justify-content: center;
  min-height: 54px;
  background: ${({ theme }) => theme.colors.navy50};
  border: 1px dashed ${({ theme }) => theme.colors.navy100};
  color: ${({ theme }) => theme.colors.textSecondary};
  border-radius: 10px;
  font-size: 11px;
  letter-spacing: 0.04em;
  text-transform: uppercase;
`;

const GoFlyButton = styled.button`
  width: 100%;
  padding: 14px;
  border-radius: 10px;
  border: none;
  font-size: 15px;
  font-weight: 700;
  cursor: pointer;
  background: ${({ theme }) => theme.colors.navy900};
  color: ${({ theme }) => theme.colors.white};

  &:disabled {
    background: ${({ theme }) => theme.colors.navy100};
    color: ${({ theme }) => theme.colors.textSecondary};
    cursor: default;
  }
`;

const ErrorBanner = styled.div`
  background: ${({ theme }) => theme.colors.dangerBg};
  color: ${({ theme }) => theme.colors.danger};
  border-radius: 8px;
  padding: 10px 14px;
  font-size: 13px;
`;