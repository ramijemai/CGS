import styled from 'styled-components';
import type { TelemetryDrone } from '../../types/telemetry';

export default function FlightOpsHud({ drone }: { drone: TelemetryDrone | null }) {
  if (!drone) return null;

  return (
    <Hud>
      <CompassRing>
        <Arrow style={{ transform: `rotate(${drone.heading}deg)` }} />
      </CompassRing>
      <Readouts>
        <Row><Label>SPEED</Label><Value>{drone.speed.toFixed(1)} M/S</Value></Row>
        <Row><Label>LAT</Label><Value>{drone.latitude.toFixed(6)}</Value></Row>
        <Row><Label>LON</Label><Value>{drone.longitude.toFixed(6)}</Value></Row>
        <Row><Label>ALT</Label><Value>{drone.altitude.toFixed(1)} M</Value></Row>
      </Readouts>
    </Hud>
  );
}

const Hud = styled.div`
  position: absolute;
  left: 16px;
  bottom: 16px;
  display: flex;
  align-items: center;
  gap: 12px;
  background: rgba(11, 31, 58, 0.9);
  border-radius: 10px;
  padding: 10px 14px;
  z-index: 10;
`;

const CompassRing = styled.div`
  width: 40px;
  height: 40px;
  border-radius: 50%;
  border: 2px solid rgba(255, 255, 255, 0.3);
  position: relative;
  flex-shrink: 0;
`;

const Arrow = styled.div`
  position: absolute;
  top: 4px;
  left: 50%;
  width: 2px;
  height: 16px;
  background: #ffffff;
  transform-origin: bottom center;
  margin-left: -1px;
`;

const Readouts = styled.div`
  display: flex;
  flex-direction: column;
  gap: 2px;
  font-family: ui-monospace, 'SF Mono', Consolas, monospace;
`;

const Row = styled.div`
  display: flex;
  gap: 10px;
  font-size: 10px;
`;

const Label = styled.span`
  color: rgba(255, 255, 255, 0.5);
  min-width: 34px;
`;

const Value = styled.span`
  color: #ffffff;
  font-weight: 600;
`;