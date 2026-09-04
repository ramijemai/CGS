import { useEffect, useState } from 'react';
import { fetchMissionHistory } from '../api/missionApi';
import type { DroneFlightStats } from '../types/drone';

export function useDroneFlightStats(droneId: string | null) {
  const [stats, setStats] = useState<DroneFlightStats | null>(null);
  const [loading, setLoading] = useState(false);

  useEffect(() => {
    if (!droneId) {
      setStats(null);
      return;
    }
    let cancelled = false;
    setLoading(true);

    fetchMissionHistory()
      .then((entries) => {
        if (cancelled) return;
        const mine = entries.filter((e) => e.droneId === droneId);
        const totalFlightSeconds = mine.reduce((sum, e) => sum + (e.durationSeconds ?? 0), 0);
        setStats({ totalFlights: mine.length, totalFlightSeconds });
      })
      .catch(() => {
        if (!cancelled) setStats(null);
      })
      .finally(() => {
        if (!cancelled) setLoading(false);
      });

    return () => {
      cancelled = true;
    };
  }, [droneId]);

  return { stats, loading };
}