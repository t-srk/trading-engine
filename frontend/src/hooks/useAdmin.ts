import { useEffect, useState, useCallback } from 'react';
import type { ServerMsg, ClientMsg, UserPortfolio } from '../types';

interface SubscribableSocket {
  subscribe: (handler: (msg: ServerMsg) => void) => () => void;
}

export interface AdminState {
  halted:            boolean;
  allUsers:          UserPortfolio[];
  halt:              () => void;
  resume:            () => void;
  clearOrders:       () => void;
  clearPositions:    () => void;
  refreshPortfolios: () => void;
  setMarkPrice:      (instrument: string, price: number) => void;
}

export function useAdmin(
  socket: SubscribableSocket,
  send:   (msg: ClientMsg) => void,
): AdminState {
  const [halted,   setHalted]   = useState(false);
  const [allUsers, setAllUsers] = useState<UserPortfolio[]>([]);

  useEffect(() => {
    return socket.subscribe((msg: ServerMsg) => {
      if (msg.event === 'engine_status')  setHalted(msg.halted);
      if (msg.event === 'all_portfolios') setAllUsers(msg.users);
    });
  }, [socket]);

  const halt     = useCallback(() => send({ action: 'admin', token: '', command: 'halt' }),             [send]);
  const resume   = useCallback(() => send({ action: 'admin', token: '', command: 'resume' }),           [send]);
  const clearOrders = useCallback(() => send({ action: 'admin', token: '', command: 'clear_orders' }), [send]);
  const refreshPortfolios = useCallback(() => send({ action: 'admin', token: '', command: 'get_portfolios' }), [send]);

  const clearPositions = useCallback(() => {
    send({ action: 'admin', token: '', command: 'clear_positions' });
    // auto-refresh so the admin table shows the cleared state
    send({ action: 'admin', token: '', command: 'get_portfolios' });
  }, [send]);

  const setMarkPrice = useCallback((instrument: string, price: number) => {
    send({ action: 'admin', token: '', command: 'set_mark_price', instrument, price });
  }, [send]);

  return { halted, allUsers, halt, resume, clearOrders, clearPositions, refreshPortfolios, setMarkPrice };
}
