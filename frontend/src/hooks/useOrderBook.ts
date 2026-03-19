import { useEffect, useState } from 'react';
import type { PriceLevel, ServerMsg } from '../types';
import type { UseSocketResult } from '../ws/useSocket';

export interface OrderBookState {
  bids: PriceLevel[];                             // sorted descending — authoritative from server
  asks: PriceLevel[];                             // sorted ascending  — authoritative from server
  myBidQty: Map<number, number>;                  // price → own resting qty on bid side
  myAskQty: Map<number, number>;                  // price → own resting qty on ask side
  cancelLevel: (side: 'BUY' | 'SELL', price: number) => void;
  cancelAll: () => void;
}

// Tracks own live orders for highlighting — separate from the server book state
// so a book_update (which carries no ownership info) can't wipe the highlights.
interface MyOrder {
  side: 'BUY' | 'SELL';
  price: number;
  qty: number;  // remaining; decremented on partial fills, deleted when zero
}

export function useOrderBook(
  socket: UseSocketResult,
  instrument: string,
  userId: string,
): OrderBookState {
  // Server-authoritative price levels — replaced wholesale on book_update
  const [bids, setBids] = useState<PriceLevel[]>([]);
  const [asks, setAsks] = useState<PriceLevel[]>([]);

  // Own orders tracked locally for the "my order" highlight
  const [myOrders, setMyOrders] = useState<Map<number, MyOrder>>(new Map());

  useEffect(() => {
    return socket.subscribe((msg: ServerMsg) => {

      // ── Book_update / snapshot: server is authoritative ──────────────────
      if (
        (msg.event === 'book_update' || msg.event === 'snapshot') &&
        msg.instrument === instrument
      ) {
        setBids(msg.bids);
        setAsks(msg.asks);
        return;
      }

      // ── Ack: record own order for highlighting ────────────────────────────
      if (msg.event === 'ack' && msg.instrument === instrument) {
        setMyOrders(prev => {
          const next = new Map(prev);
          next.set(msg.order_id, { side: msg.side, price: msg.price, qty: msg.quantity });
          return next;
        });
        return;
      }

      // ── Trade: reduce own order qty; remove if fully filled ───────────────
      if (msg.event === 'trade' && msg.instrument === instrument) {
        setMyOrders(prev => {
          const next = new Map(prev);
          for (const oid of [msg.buy_order_id, msg.sell_order_id]) {
            const o = next.get(oid);
            if (!o) continue;
            const remaining = o.qty - msg.quantity;
            if (remaining <= 0) next.delete(oid);
            else next.set(oid, { ...o, qty: remaining });
          }
          return next;
        });
        return;
      }

      // ── Cancel ack: remove own order ──────────────────────────────────────
      if (msg.event === 'cancel_ack' && msg.success) {
        setMyOrders(prev => {
          const next = new Map(prev);
          next.delete(msg.order_id);
          return next;
        });
      }
    });
  }, [socket, instrument]);

  // Aggregate own order quantities per price level for the "own qty" display
  const myBidQty = new Map<number, number>();
  const myAskQty = new Map<number, number>();
  for (const o of myOrders.values()) {
    const map = o.side === 'BUY' ? myBidQty : myAskQty;
    map.set(o.price, (map.get(o.price) ?? 0) + o.qty);
  }

  // Cancel all own resting orders at a given price level on one side
  function cancelLevel(side: 'BUY' | 'SELL', price: number) {
    for (const [orderId, o] of myOrders.entries()) {
      if (o.side === side && o.price === price) {
        socket.send({ action: 'cancel', user_id: userId, order_id: orderId });
      }
    }
  }

  // Cancel every own resting order across all levels and sides
  function cancelAll() {
    for (const orderId of myOrders.keys()) {
      socket.send({ action: 'cancel', user_id: userId, order_id: orderId });
    }
  }

  return { bids, asks, myBidQty, myAskQty, cancelLevel, cancelAll };
}
