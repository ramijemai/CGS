import styled from 'styled-components';

interface DroneIllustrationProps {
  size?: number;
  tone?: 'navy' | 'white';
}

export default function DroneIllustration({ size = 120, tone = 'navy' }: DroneIllustrationProps) {
  return (
    <Svg width={size} height={size} viewBox="0 0 120 120" $tone={tone} role="img" aria-label="Drone illustration">
      <line x1="60" y1="60" x2="24" y2="24" strokeWidth="4" strokeLinecap="round" className="arm" />
      <line x1="60" y1="60" x2="96" y2="24" strokeWidth="4" strokeLinecap="round" className="arm" />
      <line x1="60" y1="60" x2="24" y2="96" strokeWidth="4" strokeLinecap="round" className="arm" />
      <line x1="60" y1="60" x2="96" y2="96" strokeWidth="4" strokeLinecap="round" className="arm" />

      <circle cx="24" cy="24" r="14" className="prop" />
      <circle cx="96" cy="24" r="14" className="prop" />
      <circle cx="24" cy="96" r="14" className="prop" />
      <circle cx="96" cy="96" r="14" className="prop" />

      <circle cx="24" cy="24" r="4" className="hub" />
      <circle cx="96" cy="24" r="4" className="hub" />
      <circle cx="24" cy="96" r="4" className="hub" />
      <circle cx="96" cy="96" r="4" className="hub" />

      <rect x="46" y="46" width="28" height="28" rx="8" className="body" />
      <circle cx="60" cy="78" r="6" className="gimbal" />
    </Svg>
  );
}

const Svg = styled.svg<{ $tone: 'navy' | 'white' }>`
  .arm {
    stroke: ${({ theme, $tone }) => ($tone === 'white' ? 'rgba(255,255,255,0.5)' : theme.colors.navy300)};
  }
  .prop {
    fill: none;
    stroke: ${({ theme, $tone }) => ($tone === 'white' ? 'rgba(255,255,255,0.35)' : theme.colors.navy100)};
    stroke-width: 3;
  }
  .hub {
    fill: ${({ theme, $tone }) => ($tone === 'white' ? '#ffffff' : theme.colors.navy700)};
  }
  .body {
    fill: ${({ theme, $tone }) => ($tone === 'white' ? '#ffffff' : theme.colors.navy900)};
  }
  .gimbal {
    fill: ${({ theme, $tone }) => ($tone === 'white' ? 'rgba(255,255,255,0.7)' : theme.colors.navy500)};
  }
`;