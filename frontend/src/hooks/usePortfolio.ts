import { useEffect, useState } from 'react';
import type { PortfolioPosition, ServerMsg } from '../types';

interface SubscribableSocket {
  subscribe: (handler: (msg: ServerMsg) => void) => () => void;
}

export function usePortfolio(socket: SubscribableSocket): PortfolioPosition[] {
  const [positions, setPositions] = useState<PortfolioPosition[]>([]);

  useEffect(() => {
    return socket.subscribe((msg: ServerMsg) => {
      if (msg.event === 'portfolio_update') {
        setPositions(msg.positions);
      }
    });
  }, [socket]);

  return positions;
}
