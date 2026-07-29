#pragma once
#include <string>

// The full HTML dashboard, embedded in a header so the server serves it from memory.
// Edit this file to change the dashboard UI. No build step needed — just recompile.
inline std::string dashboardHtml() {
    return R"HTML(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>CGS — Drone Dashboard</title>
<style>
*, *::before, *::after { box-sizing: border-box; margin: 0; padding: 0; }

:root {
  --bg: #1a1a2e;
  --surface: #16213e;
  --card: #0f3460;
  --accent: #e94560;
  --accent2: #533483;
  --success: #2ecc71;
  --warning: #f39c12;
  --danger: #e74c3c;
  --text: #eee;
  --text-dim: #8899aa;
  --border: #2a2a4a;
  --shadow: 4px 4px 0 #0a0a1a;
}

body {
  font-family: 'Segoe UI', system-ui, -apple-system, sans-serif;
  background: var(--bg);
  color: var(--text);
  min-height: 100vh;
  padding: 20px;
}

/* === HEADER === */
.header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 16px 24px;
  background: var(--surface);
  border: 2px solid var(--border);
  box-shadow: var(--shadow);
  margin-bottom: 24px;
}
.header h1 {
  font-size: 24px;
  font-weight: 800;
  text-transform: uppercase;
  letter-spacing: 2px;
}
.header h1 span { color: var(--accent); }
.header-status {
  display: flex;
  align-items: center;
  gap: 12px;
  font-size: 14px;
  color: var(--text-dim);
}
.dot {
  width: 10px;
  height: 10px;
  border-radius: 50%;
  display: inline-block;
}
.dot.online { background: var(--success); box-shadow: 0 0 8px var(--success); }
.dot.offline { background: var(--danger); }

/* === GRID === */
.grid {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 20px;
  margin-bottom: 20px;
}

.card {
  background: var(--surface);
  border: 2px solid var(--border);
  box-shadow: var(--shadow);
  padding: 20px;
}
.card h2 {
  font-size: 14px;
  text-transform: uppercase;
  letter-spacing: 1.5px;
  color: var(--text-dim);
  margin-bottom: 16px;
  padding-bottom: 8px;
  border-bottom: 1px solid var(--border);
}

/* === BUNKER SLOTS === */
.slots-grid {
  display: grid;
  grid-template-columns: repeat(auto-fill, minmax(180px, 1fr));
  gap: 12px;
}
.slot-card {
  background: var(--card);
  border: 2px solid var(--border);
  padding: 16px;
  position: relative;
  transition: transform 0.15s;
}
.slot-card:hover { transform: translate(-2px, -2px); box-shadow: var(--shadow); }
.slot-card .slot-id {
  font-size: 12px;
  color: var(--text-dim);
  text-transform: uppercase;
  letter-spacing: 1px;
  margin-bottom: 4px;
}
.slot-card .state-badge {
  display: inline-block;
  padding: 2px 8px;
  font-size: 10px;
  font-weight: 700;
  text-transform: uppercase;
  margin-bottom: 8px;
}
.state-badge.vacant { background: var(--success); color: #000; }
.state-badge.occupied { background: var(--accent); color: #fff; }
.state-badge.maintenance { background: var(--warning); color: #000; }

.slot-card .drone-name {
  font-size: 18px;
  font-weight: 700;
  margin-bottom: 4px;
}
.slot-card .drone-battery {
  font-size: 13px;
  color: var(--text-dim);
}
.slot-card .hatch {
  position: absolute;
  top: 12px;
  right: 12px;
  font-size: 11px;
  color: var(--text-dim);
  text-transform: uppercase;
}

.battery-bar {
  width: 100%;
  height: 6px;
  background: #333;
  margin-top: 8px;
  border: 1px solid #444;
}
.battery-fill {
  height: 100%;
  transition: width 0.5s, background 0.3s;
}
.battery-fill.high { background: var(--success); }
.battery-fill.med { background: var(--warning); }
.battery-fill.low { background: var(--danger); }

/* === ACTIVE DRONES === */
.drone-row {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 12px 0;
  border-bottom: 1px solid var(--border);
}
.drone-row:last-child { border-bottom: none; }
.drone-row .drone-label { font-weight: 600; min-width: 120px; }
.drone-row .drone-telem {
  display: flex;
  gap: 20px;
  font-size: 13px;
  color: var(--text-dim);
}
.drone-row .drone-telem span { white-space: nowrap; }
.drone-row .drone-telem strong { color: var(--text); }

.empty-state {
  color: var(--text-dim);
  text-align: center;
  padding: 24px;
  font-style: italic;
}

/* === COMMAND LOG === */
.log-list {
  max-height: 240px;
  overflow-y: auto;
  font-family: 'Courier New', monospace;
  font-size: 12px;
}
.log-entry {
  padding: 4px 0;
  border-bottom: 1px solid rgba(255,255,255,0.05);
}
.log-entry .time { color: var(--text-dim); margin-right: 8px; }

/* === COMMAND PANEL === */
.command-panel {
  grid-column: 1 / -1;
}
.command-row {
  display: flex;
  gap: 12px;
  flex-wrap: wrap;
  align-items: center;
}
.command-row select {
  background: var(--card);
  color: var(--text);
  border: 2px solid var(--border);
  padding: 10px 16px;
  font-size: 14px;
  font-weight: 600;
  min-width: 140px;
}
.cmd-btn {
  padding: 10px 20px;
  font-size: 13px;
  font-weight: 700;
  text-transform: uppercase;
  letter-spacing: 1px;
  border: 2px solid var(--border);
  cursor: pointer;
  transition: transform 0.1s, box-shadow 0.1s;
  box-shadow: var(--shadow);
}
.cmd-btn:hover { transform: translate(-2px, -2px); }
.cmd-btn:active { transform: translate(0, 0); box-shadow: none; }
.cmd-btn.launch { background: var(--success); color: #000; }
.cmd-btn.return { background: var(--warning); color: #000; }
.cmd-btn.hold { background: var(--accent); color: #fff; }
.cmd-btn.emergency { background: var(--danger); color: #fff; }
.cmd-btn.status { background: var(--accent2); color: #fff; }

.cmd-label {
  font-size: 12px;
  color: var(--text-dim);
  text-transform: uppercase;
  letter-spacing: 1px;
  margin-right: 4px;
}

/* === ACTIVE COUNT === */
.count-badge {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  width: 28px;
  height: 28px;
  border-radius: 50%;
  background: var(--accent);
  color: #fff;
  font-weight: 700;
  font-size: 14px;
  margin-left: 8px;
}

/* === SCROLLBAR === */
::-webkit-scrollbar { width: 6px; }
::-webkit-scrollbar-track { background: var(--bg); }
::-webkit-scrollbar-thumb { background: var(--border); }

/* === RESPONSIVE === */
@media (max-width: 860px) {
  .grid { grid-template-columns: 1fr; }
  .header { flex-direction: column; gap: 12px; text-align: center; }
  .drone-row { flex-direction: column; align-items: flex-start; gap: 8px; }
  .drone-row .drone-telem { flex-wrap: wrap; }
}
</style>
</head>
<body>

<div class="header">
  <h1><span>◈</span> CGS <span>◈</span> Drone Dashboard</h1>
  <div class="header-status">
    <span id="timeDisplay">--:--:--</span>
    <span class="dot online" id="statusDot"></span>
    <span id="statusText">CONNECTING...</span>
  </div>
</div>

<div class="grid">
  <!-- BUNKER OVERVIEW -->
  <div class="card">
    <h2>Bunker Bays <span id="slotCount" class="count-badge">0</span></h2>
    <div class="slots-grid" id="slotsContainer">
      <div class="empty-state">Waiting for data...</div>
    </div>
  </div>

  <!-- ACTIVE DRONES -->
  <div class="card">
    <h2>Active Drones <span id="activeCount" class="count-badge">0</span></h2>
    <div id="dronesContainer">
      <div class="empty-state">No drones in flight.</div>
    </div>
  </div>

  <!-- COMMAND LOG -->
  <div class="card">
    <h2>Event Log</h2>
    <div class="log-list" id="logContainer">
      <div class="empty-state">No events yet.</div>
    </div>
  </div>

  <!-- COMMAND PANEL -->
  <div class="card command-panel">
    <h2>Send Command</h2>
    <div class="command-row">
      <span class="cmd-label">Drone:</span>
      <select id="droneSelect">
        <option value="">-- Select --</option>
      </select>
      <button class="cmd-btn launch" data-cmd="LAUNCH_MISSION">Launch Mission</button>
      <button class="cmd-btn return" data-cmd="RETURN_TO_BUNKER">Return to Bunker</button>
      <button class="cmd-btn hold" data-cmd="HOLD_POSITION">Hold Position</button>
      <button class="cmd-btn emergency" data-cmd="EMERGENCY_LAND">Emergency Land</button>
      <button class="cmd-btn status" data-cmd="STATUS_CHECK">Status Check</button>
    </div>
  </div>
</div>

<script>
// ============================================================
// WebSocket connection
// ============================================================
const wsProto = window.location.protocol === 'https:' ? 'wss:' : 'ws:';
const wsUrl = wsProto + '//' + window.location.host;
let ws = null;
let reconnectTimer = null;

function connect() {
  if (ws && ws.readyState === WebSocket.OPEN) return;

  setStatus('offline', 'CONNECTING...');
  ws = new WebSocket(wsUrl);

  ws.onopen = () => {
    setStatus('online', 'CONNECTED');
    console.log('[WS] Connected');
  };

  ws.onmessage = (event) => {
    try {
      const state = JSON.parse(event.data);
      renderBunker(state);
      renderDrones(state);
      renderLog(state);
      updateCounts(state);
      updateDroneSelect(state);
    } catch (e) {
      console.warn('[WS] Bad message:', e);
    }
  };

  ws.onclose = () => {
    setStatus('offline', 'DISCONNECTED — reconnecting...');
    scheduleReconnect();
  };

  ws.onerror = () => {
    ws.close();
  };
}

function scheduleReconnect() {
  if (reconnectTimer) clearTimeout(reconnectTimer);
  reconnectTimer = setTimeout(connect, 2000);
}

function setStatus(dotClass, text) {
  document.getElementById('statusDot').className = 'dot ' + dotClass;
  document.getElementById('statusText').textContent = text;
}

// ============================================================
// Clock
// ============================================================
function updateClock() {
  const now = new Date();
  document.getElementById('timeDisplay').textContent =
    now.toLocaleTimeString('en-GB', { hour12: false });
}
setInterval(updateClock, 1000);
updateClock();

// ============================================================
// Render functions
// ============================================================
function renderBunker(state) {
  const container = document.getElementById('slotsContainer');
  if (!state.slots || state.slots.length === 0) {
    container.innerHTML = '<div class="empty-state">No bay data.</div>';
    return;
  }

  container.innerHTML = state.slots.map(slot => {
    const stateClass = slot.bayState.toLowerCase();
    const emptyLabel = slot.bayState === 'Vacant' ? '— EMPTY —' : slot.droneId;
    const battery = slot.batteryLevel;
    const batClass = battery > 40 ? 'high' : (battery > 20 ? 'med' : 'low');

    return `
      <div class="slot-card">
        <div class="slot-id">Bay ${slot.slotId}</div>
        <span class="state-badge ${stateClass}">${slot.bayState}</span>
        <span class="hatch">${slot.hatchState}</span>
        <div class="drone-name">${emptyLabel}</div>
        ${slot.droneId ? `<div class="drone-battery">Battery: ${battery.toFixed(1)}%</div>` : ''}
        ${slot.droneId ? `
          <div class="battery-bar">
            <div class="battery-fill ${batClass}" style="width:${battery}%"></div>
          </div>
        ` : ''}
      </div>
    `;
  }).join('');
}

function renderDrones(state) {
  const container = document.getElementById('dronesContainer');
  if (!state.activeDrones || state.activeDrones.length === 0) {
    container.innerHTML = '<div class="empty-state">No drones in flight.</div>';
    return;
  }

  container.innerHTML = state.activeDrones.map(d => `
    <div class="drone-row">
      <div class="drone-label">${d.droneId}</div>
      <div class="drone-telem">
        <span>📍 <strong>${d.latitude.toFixed(4)}, ${d.longitude.toFixed(4)}</strong></span>
        <span>⬆ <strong>${d.altitude.toFixed(1)}</strong> m</span>
        <span>🔋 <strong>${d.batteryLevel.toFixed(1)}</strong>%</span>
        <span>💨 <strong>${d.speed.toFixed(1)}</strong> m/s</span>
        <span>🧭 <strong>${d.heading.toFixed(0)}</strong>°</span>
      </div>
    </div>
  `).join('');
}

function renderLog(state) {
  const container = document.getElementById('logContainer');
  if (!state.commandLog || state.commandLog.length === 0) {
    container.innerHTML = '<div class="empty-state">No events yet.</div>';
    return;
  }

  container.innerHTML = state.commandLog.map(msg => {
    // Split timestamp from message if colon-separated
    const idx = msg.indexOf(']');
    if (idx > 0 && idx < 20) {
      return `<div class="log-entry"><span class="time">${msg.substring(0, idx + 1)}</span>${msg.substring(idx + 1)}</div>`;
    }
    return `<div class="log-entry">${msg}</div>`;
  }).join('');

  // Auto-scroll to bottom
  container.scrollTop = container.scrollHeight;
}

function updateCounts(state) {
  document.getElementById('slotCount').textContent = state.slots ? state.slots.length : 0;
  document.getElementById('activeCount').textContent = state.activeDroneCount || 0;
}

function updateDroneSelect(state) {
  const select = document.getElementById('droneSelect');
  const currentVal = select.value;

  // Build options: all drones (in bays + active)
  const allDrones = [];
  if (state.slots) {
    state.slots.forEach(s => {
      if (s.droneId && s.droneId.length > 0) allDrones.push(s.droneId);
    });
  }
  if (state.activeDrones) {
    state.activeDrones.forEach(d => {
      if (!allDrones.includes(d.droneId)) allDrones.push(d.droneId);
    });
  }

  select.innerHTML = '<option value="">-- Select Drone --</option>' +
    allDrones.map(id => `<option value="${id}" ${id === currentVal ? 'selected' : ''}>${id}</option>`).join('');
}

// ============================================================
// Send commands via WebSocket
// ============================================================
document.querySelectorAll('.cmd-btn').forEach(btn => {
  btn.addEventListener('click', () => {
    const droneId = document.getElementById('droneSelect').value;
    if (!droneId) {
      alert('Select a drone first.');
      return;
    }

    const cmd = btn.dataset.cmd;
    const msg = JSON.stringify({
      type: cmd,
      droneId: droneId
    });

    if (ws && ws.readyState === WebSocket.OPEN) {
      ws.send(msg);
    } else {
      alert('Not connected to server.');
    }
  });
});

// ============================================================
// Init
// ============================================================
connect();
</script>
</body>
</html>
)HTML";
}
