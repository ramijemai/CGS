import styled from 'styled-components';

export interface ActivityLogEntry {
  id: string;
  time: string;
  message: string;
  emphasis?: boolean;
}

export default function ActivityLog({ entries }: { entries: ActivityLogEntry[] }) {
  return (
    <Panel>
      <Title>Bunker Activity Log</Title>
      <List>
        {entries.map((entry) => (
          <Item key={entry.id}>
            <Time>{entry.time}</Time>
            <Message $emphasis={entry.emphasis}>{entry.message}</Message>
          </Item>
        ))}
        {entries.length === 0 && <Empty>No recent activity.</Empty>}
      </List>
    </Panel>
  );
}

const Panel = styled.aside`
  background: ${({ theme }) => theme.colors.white};
  border: 1px solid ${({ theme }) => theme.colors.navy100};
  border-radius: ${({ theme }) => theme.radius};
  padding: 16px 18px;
  height: fit-content;
`;

const Title = styled.h3`
  margin: 0 0 12px;
  font-size: 14px;
  font-weight: 600;
  color: ${({ theme }) => theme.colors.textPrimary};
`;

const List = styled.ul`
  list-style: none;
  margin: 0;
  padding: 0;
  display: flex;
  flex-direction: column;
  gap: 14px;
`;

const Item = styled.li`
  display: flex;
  gap: 10px;
  font-size: 12px;
`;

const Time = styled.span`
  color: ${({ theme }) => theme.colors.textSecondary};
  min-width: 34px;
`;

const Message = styled.span<{ $emphasis?: boolean }>`
  color: ${({ theme }) => theme.colors.textPrimary};
  font-weight: ${({ $emphasis }) => ($emphasis ? 600 : 400)};
`;

const Empty = styled.li`
  color: ${({ theme }) => theme.colors.textSecondary};
  font-size: 12px;
`;