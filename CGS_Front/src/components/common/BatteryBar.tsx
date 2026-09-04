import styled from 'styled-components';

type Tone = 'ok' | 'warning' | 'danger';

function toneFor(level: number): Tone {
  if (level <= 15) return 'danger';
  if (level <= 35) return 'warning';
  return 'ok';
}

export default function BatteryBar({ level }: { level: number }) {
  const tone = toneFor(level);
  return (
    <Wrapper>
      <Label>Battery ({level}%)</Label>
      <Track>
        <Fill $pct={level} $tone={tone} />
      </Track>
    </Wrapper>
  );
}

const Wrapper = styled.div`
  display: flex;
  flex-direction: column;
  gap: 4px;
  min-width: 100px;
`;

const Label = styled.span`
  font-size: 10px;
  text-transform: uppercase;
  letter-spacing: 0.04em;
  color: ${({ theme }) => theme.colors.textSecondary};
`;

const Track = styled.div`
  width: 100%;
  height: 6px;
  border-radius: 4px;
  background: ${({ theme }) => theme.colors.navy100};
  overflow: hidden;
`;

const Fill = styled.div<{ $pct: number; $tone: Tone }>`
  height: 100%;
  width: ${({ $pct }) => Math.max(0, Math.min(100, $pct))}%;
  background: ${({ theme, $tone }) =>
    $tone === 'danger' ? theme.colors.danger : $tone === 'warning' ? theme.colors.warning : theme.colors.navy700};
`;