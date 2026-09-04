import styled from 'styled-components';
import { Outlet } from 'react-router-dom';
import Sidebar from './SideBar';

export default function Layout() {
  return (
    <Shell>
      <Sidebar />
      <Content>
        <Outlet />
      </Content>
    </Shell>
  );
}

const Shell = styled.div`
  display: flex;
  min-height: 100vh;
  background: ${({ theme }) => theme.colors.navy50};
`;

const Content = styled.main`
  flex: 1;
  min-width: 0;
`;