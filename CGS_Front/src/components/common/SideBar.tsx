import styled from 'styled-components';
import { NavLink } from 'react-router-dom';
import {
  Box,
  Rocket,
  FileText,
  Settings,
} from 'lucide-react';

const NAV_ITEMS = [
  { to: '/', label: 'Bunker', icon: Box, end: true },
  { to: '/missions', label: 'Missions', icon: Rocket },
  { to: '/reports', label: 'Reports', icon: FileText },
];

export default function Sidebar() {
  return (
    <Nav>
      <Brand>
        <Logo>OA</Logo>
        <BrandText>
          <BrandTitle>Bunker Delta Sector</BrandTitle>
          <BrandStatus>
            <StatusDot />
            System Online
          </BrandStatus>
        </BrandText>
      </Brand>

      <ItemList>
        {NAV_ITEMS.map(({ to, label, icon: Icon, end }) => (
          <li key={to}>
            <StyledLink to={to} end={end}>
              <Icon size={18} strokeWidth={1.75} />
              <span>{label}</span>
            </StyledLink>
          </li>
        ))}
      </ItemList>

      <Footer>
        <StyledLink to="/settings">
          <Settings size={18} strokeWidth={1.75} />
          <span>Settings</span>
        </StyledLink>
      </Footer>
    </Nav>
  );
}

const Nav = styled.nav`
  width: 232px;
  min-height: 100vh;
  background: ${({ theme }) => theme.colors.navy900};
  display: flex;
  flex-direction: column;
  padding: 20px 14px;
  box-sizing: border-box;
`;

const Brand = styled.div`
  display: flex;
  align-items: center;
  gap: 10px;
  padding: 4px 8px 24px;
`;

const Logo = styled.div`
  width: 34px;
  height: 34px;
  border-radius: 8px;
  background: ${({ theme }) => theme.colors.white};
  color: ${({ theme }) => theme.colors.navy900};
  font-weight: 700;
  font-size: 13px;
  display: flex;
  align-items: center;
  justify-content: center;
  flex-shrink: 0;
`;

const BrandText = styled.div`
  min-width: 0;
`;

const BrandTitle = styled.div`
  color: ${({ theme }) => theme.colors.white};
  font-size: 13px;
  font-weight: 600;
  white-space: nowrap;
  overflow: hidden;
  text-overflow: ellipsis;
`;

const BrandStatus = styled.div`
  display: flex;
  align-items: center;
  gap: 5px;
  color: ${({ theme }) => theme.colors.navy300};
  font-size: 11px;
  margin-top: 2px;
`;

const StatusDot = styled.span`
  width: 6px;
  height: 6px;
  border-radius: 50%;
  background: #3fbf7f;
  display: inline-block;
`;

const ItemList = styled.ul`
  list-style: none;
  margin: 0;
  padding: 0;
  display: flex;
  flex-direction: column;
  gap: 2px;
  flex: 1;
`;

const StyledLink = styled(NavLink)`
  display: flex;
  align-items: center;
  gap: 12px;
  padding: 10px 12px;
  border-radius: 8px;
  font-size: 13px;
  font-weight: 500;
  text-decoration: none;
  color: ${({ theme }) => theme.colors.navy300};
  transition: background 0.15s ease, color 0.15s ease;

  &:hover {
    background: ${({ theme }) => theme.colors.navy800};
    color: ${({ theme }) => theme.colors.white};
  }

  &.active {
    background: ${({ theme }) => theme.colors.navy700};
    color: ${({ theme }) => theme.colors.white};
  }
`;

const Footer = styled.div`
  border-top: 1px solid ${({ theme }) => theme.colors.navy800};
  padding-top: 10px;
  margin-top: 10px;
`;