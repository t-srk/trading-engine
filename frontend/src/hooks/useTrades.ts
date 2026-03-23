import { useEffect, useRef, useState } from 'react';
import type { ServerMsg } from '../types';

export interface TradeRecord {
  trade_id:    number;
  received_at: number;
  instrument:  string;
  price:       number;
  quantity:    number;
  buyer_id:    string;
  seller_id:   string;
  mid_at_trade: number;
}

interface SubscribableSocket {
  subscribe: (handler: (msg: ServerMsg) => void) => () => void;
}

export function useTrades(
  socket: SubscribableSocket,
  getMid: () => number,
): TradeRecord[] {
  const [trades, setTrades] = useState<TradeRecord[]>([]);
  const getMidRef = useRef(getMid);
  getMidRef.current = getMid;

  useEffect(() => {
    return socket.subscribe((msg: ServerMsg) => {
      if (msg.event === 'trade') {
        const mid = getMidRef.current();
        setTrades(prev => [
          {
            trade_id:    msg.trade_id,
            received_at: Date.now(),
            instrument:  msg.instrument,
            price:       msg.price,
            quantity:    msg.quantity,
            buyer_id:    msg.buyer_id,
            seller_id:   msg.seller_id,
            mid_at_trade: mid,
          },
          ...prev,
        ]);
      }
    });
  }, [socket]);

  return trades;
}
