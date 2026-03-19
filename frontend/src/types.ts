// ── Client → Server ──────────────────────────────────────────────────────────

export interface LoginMsg {
  action: 'login';
  user_id: string;
}

export interface SubmitOrderMsg {
  action: 'submit';
  token: string;
  user_id: string;
  instrument: string;
  side: 'BUY' | 'SELL';
  price: number;
  quantity: number;
}

export interface CancelOrderMsg {
  action: 'cancel';
  token: string;
  user_id: string;
  order_id: number;
}

export type ClientMsg = LoginMsg | SubmitOrderMsg | CancelOrderMsg;

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

export interface LoginAckMsg {
  event: 'login_ack';
  user_id: string;
  token: string;
}

// Sent by the server after every submit/cancel and on new client connect.
// Replaces local book state with the authoritative server view.
export interface BookUpdateMsg {
  event: 'book_update';
  instrument: string;
  bids: PriceLevel[];
  asks: PriceLevel[];
}

// Legacy snapshot shape (same format, kept for forward compatibility)
export interface SnapshotMsg {
  event: 'snapshot';
  instrument: string;
  bids: PriceLevel[];
  asks: PriceLevel[];
}

export interface PortfolioPosition {
  instrument:    string;
  qty:           number;
  avg_cost:      number;
  realized_pnl:  number;
  unrealized_pnl: number;
  last_price:    number;
}

export interface PortfolioUpdateMsg {
  event:     'portfolio_update';
  positions: PortfolioPosition[];
}

export type ServerMsg = LoginAckMsg | AckMsg | TradeMsg | ErrorMsg | CancelAckMsg | BookUpdateMsg | SnapshotMsg | PortfolioUpdateMsg;

// Shared shape used by both SnapshotMsg and the derived order book
export interface PriceLevel {
  price: number;
  quantity: number;
}
