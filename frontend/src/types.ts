// ── Client → Server ──────────────────────────────────────────────────────────
// Mirrors SubmitOrderMsg / CancelOrderMsg in include/message.h

export interface SubmitOrderMsg {
  action: 'submit';
  user_id: string;
  instrument: string;
  side: 'BUY' | 'SELL';
  price: number;
  quantity: number;
}

export interface CancelOrderMsg {
  action: 'cancel';
  user_id: string;
  order_id: number;
}

export type ClientMsg = SubmitOrderMsg | CancelOrderMsg;

// ── Server → Client ──────────────────────────────────────────────────────────
// Mirrors AckMsg / TradeMsg / ErrorMsg / CancelAckMsg in include/message.h

export interface AckMsg {
  event: 'ack';
  order_id: number;
  status: string;
  instrument: string;
  side: 'BUY' | 'SELL';
  price: number;
  quantity: number;
}

export interface TradeMsg {
  event: 'trade';
  trade_id: number;
  instrument: string;
  price: number;
  quantity: number;
  buyer_id: string;
  seller_id: string;
  buy_order_id: number;
  sell_order_id: number;
}

export interface ErrorMsg {
  event: 'error';
  reason: string;
}

export interface CancelAckMsg {
  event: 'cancel_ack';
  order_id: number;
  success: boolean;
}

// Future: server will push this after the client sends { action: "subscribe", instrument }
// See TODO in session.cpp for implementation guide.
export interface SnapshotMsg {
  event: 'snapshot';
  instrument: string;
  bids: PriceLevel[];
  asks: PriceLevel[];
}

export type ServerMsg = AckMsg | TradeMsg | ErrorMsg | CancelAckMsg | SnapshotMsg;

// Shared shape used by both SnapshotMsg and the derived order book
export interface PriceLevel {
  price: number;
  quantity: number;
}
