// src/api/bunkerapi.ts
import type { BunkerStatusResponse, WeatherSnapshot } from '../types/bunker';
import { BUNKER_LOCATION } from '../lib/bunkerLocation';

const API_BASE = import.meta.env.VITE_API_BASE_URL ?? 'http://localhost:18080';
const WEATHER_TIMEOUT_MS = 6000;

export async function fetchBunkerStatus(): Promise<BunkerStatusResponse> {
  const res = await fetch(`${API_BASE}/api/v1/bunker/status`);
  if (!res.ok) {
    throw new Error(`Bunker status request failed: ${res.status} ${res.statusText}`);
  }
  return res.json() as Promise<BunkerStatusResponse>;
}

interface OpenMeteoResponse {
  current?: {
    temperature_2m?: number;
    relative_humidity_2m?: number;
    wind_speed_10m?: number;
  };
}

export async function fetchWeatherSnapshot(): Promise<WeatherSnapshot> {
  const params = new URLSearchParams({
    latitude: String(BUNKER_LOCATION.lat),
    longitude: String(BUNKER_LOCATION.lng),
    current: 'temperature_2m,relative_humidity_2m,wind_speed_10m',
    wind_speed_unit: 'ms', // matches the `m/s` label used in WeatherWidget
    timezone: 'auto',
  });

  const controller = new AbortController();
  const timeout = setTimeout(() => controller.abort(), WEATHER_TIMEOUT_MS);

  let res: Response;
  try {
    res = await fetch(`https://api.open-meteo.com/v1/forecast?${params.toString()}`, {
      signal: controller.signal,
    });
  } catch (err) {
    if (err instanceof DOMException && err.name === 'AbortError') {
      throw new Error('Weather service request timed out');
    }
    throw err;
  } finally {
    clearTimeout(timeout);
  }

  if (!res.ok) {
    throw new Error(`Weather service request failed: ${res.status} ${res.statusText}`);
  }

  const data = (await res.json()) as OpenMeteoResponse;

  if (
    !data.current ||
    data.current.temperature_2m === undefined ||
    data.current.relative_humidity_2m === undefined ||
    data.current.wind_speed_10m === undefined
  ) {
    throw new Error('Weather response missing expected fields');
  }

  return {
    temperature: data.current.temperature_2m,
    humidity: Math.round(data.current.relative_humidity_2m),
    windSpeed: Number(data.current.wind_speed_10m.toFixed(1)),
    source: 'Open-Meteo',
  };
}