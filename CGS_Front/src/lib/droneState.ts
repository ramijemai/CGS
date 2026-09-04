import { 
  Plane, 
  ArrowLeftCircle, 
  PlaneLanding, 
  AlertTriangle, 
  Home, 
  CheckCircle, 
  BatteryCharging, 
  HelpCircle, 
  type LucideIcon 
} from 'lucide-react';
import type { DroneOpState } from '../types/drone';

export const droneStateMeta: Record<
  DroneOpState, 
  { label: string; tone: 'neutral' | 'warning' | 'danger'; icon: LucideIcon }
> = {
  IDLE: { label: 'Idle', tone: 'neutral', icon: CheckCircle },
  IN_FLIGHT: { label: 'In Flight', tone: 'neutral', icon: Plane },
  RETURNING: { label: 'Returning', tone: 'warning', icon: ArrowLeftCircle },
  LANDING: { label: 'Landing', tone: 'neutral', icon: PlaneLanding },
  FAULT: { label: 'Fault', tone: 'danger', icon: AlertTriangle },
  DOCKED: { label: 'Docked', tone: 'neutral', icon: Home },
  CHARGING: { label: 'Charging', tone: 'warning', icon: BatteryCharging },
  UNKNOWN: { label: 'Unknown', tone: 'neutral', icon: HelpCircle },
};