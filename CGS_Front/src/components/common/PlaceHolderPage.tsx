import styled from 'styled-components';

export default function PlaceHolderPage({ title }: { title: string }) {
  return (
    <Wrapper>
      <h1>{title}</h1>
      <p>This page hasn't been built yet.</p>
    </Wrapper>
  );
}

const Wrapper = styled.div`
  padding: 32px;
  color: ${({ theme }) => theme.colors.textSecondary};

  h1 {
    color: ${({ theme }) => theme.colors.textPrimary};
    margin: 0 0 8px;
  }
`;