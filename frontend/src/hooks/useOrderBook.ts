import { useEffect, useState } from 'react';
import type { PriceLevel, ServerMsg } from '../types';
import type { UseSocketResult } from '../ws/useSocket';

export interface OrderBookState {
  bids: PriceLevel[];       // sorted descending — best bid first
  asks: PriceLevel[];       // sorted ascending  — best ask first
  myBidPrices: Set<number>; // prices where this user has a resting bid
  myAskPrices: Set<number>; // prices where this user has a resting ask
}

interface TrackedOrder {
  side: 'BUY' | 'SELL';
  price: number;
  remaining: number;
  isOwn: boolean; // true for orders placed by this session via ack
}

export function useOrderBook(
  socket: UseSocketResult,
  instrument: string,
): OrderBookState {
  const [orders, setOrders] = useState<Map<number, TrackedOrder>>(new Map());

  useEffect(() => {
    return socket.subscribe((msg: ServerMsg) => {
      if (msg.event === 'ack' && msg.instrument === instrument) {
        setOrders(prev => {
          const next = new Map(prev);
          next.set(msg.order_id, {
            side: msg.side,
            price: msg.price,
            remaining: msg.quantity,
            isOwn: true,
          });
          return next;
        });
        return;
      }

      if (msg.event === 'trade' && msg.instrument === instrument) {
        setOrders(prev => {
          const next = new Map(prev);
          for (const oid of [msg.buy_order_id, msg.sell_order_id]) {
            const o = next.get(oid);
            if (!o) continue;
            const rem = o.remaining - msg.quantity;
            if (rem <= 0) next.delete(oid);
            else next.set(oid, { ...o, remaining: rem });
          }
          return next;
        });
        return;
      }

      if (msg.event === 'cancel_ack' && msg.success) {
        setOrders(prev => {
          const next = new Map(prev);
          next.delete(msg.order_id);
          return next;
        });
        return;
      }

      // Future server-pushed snapshot — synthetic orders are not "own"
      if (msg.event === 'snapshot' && msg.instrument === instrument) {
        const synth = new Map<number, TrackedOrder>();
        let fakeId = -(1 << 20);
        for (const l of msg.bids) synth.set(fakeId--, { side: 'BUY',  price: l.price, remaining: l.quantity, isOwn: false });
        for (const l of msg.asks) synth.set(fakeId--, { side: 'SELL', price: l.price, remaining: l.quantity, isOwn: false });
        setOrders(synth);
      }
    });
  }, [socket, instrument]);

  const bidMap = new Map<number, number>();
  const askMap = new Map<number, number>();
  const myBidPrices = new Set<number>();
  const myAskPrices = new Set<number>();

  for (const o of orders.values()) {
    const map = o.side === 'BUY' ? bidMap : askMap;
    map.set(o.price, (map.get(o.price) ?? 0) + o.remaining);
    if (o.isOwn) {
      if (o.side === 'BUY') myBidPrices.add(o.price);
      else                  myAskPrices.add(o.price);
    }
  }

  const bids: PriceLevel[] = Array.from(bidMap.entries())
    .map(([price, quantity]) => ({ price, quantity }))
    .sort((a, b) => b.price - a.price);

  const asks: PriceLevel[] = Array.from(askMap.entries())
    .map(([price, quantity]) => ({ price, quantity }))
    .sort((a, b) => a.price - b.price);

  return { bids, asks, myBidPrices, myAskPrices };
}
