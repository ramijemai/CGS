import { BrowserRouter, Routes, Route } from 'react-router-dom';
import Layout from './components/common/Layout';
import BunkerPage from './components/bunker/BunkerPage';
import PlaceholderPage from './components/common/PlaceHolderPage';
import FlightOpsPage from './components/flightops/FlightOpsPage';
import ReportsPage from './components/reports/ReportsPage';

function App() {
  return (
    <BrowserRouter>
      <Routes>
        <Route element={<Layout />}>
          <Route path="/" element={<BunkerPage />} />
          <Route path="/bunker" element={<BunkerPage />} />
          <Route path="/missions" element={<FlightOpsPage />} />
          <Route path="/reports" element={<ReportsPage />} />
          <Route path="/settings" element={<PlaceholderPage title="Settings" />} />
        </Route>
      </Routes>
    </BrowserRouter>
  );
}

export default App;