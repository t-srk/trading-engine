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
  token?: string;
  user_id: string;
  order_id: number;
}

export interface AdminMsg {
  action:      'admin';
  token:       string;
  command:     'halt' | 'resume' | 'clear_orders' | 'clear_positions' | 'get_portfolios' | 'set_mark_price';
  instrument?: string;
  price?:      number;
}

export type ClientMsg = LoginMsg | SubmitOrderMsg | CancelOrderMsg | AdminMsg;

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

export interface EngineStatusMsg {
  event:  'engine_status';
  halted: boolean;
}

export interface AdminAckMsg {
  event:   'admin_ack';
  command: string;
  success: boolean;
}

export interface UserPortfolio {
  user_id:   string;
  positions: PortfolioPosition[];
}

export interface AllPortfoliosMsg {
  event: 'all_portfolios';
  users: UserPortfolio[];
}

export interface OrdersClearedMsg {
  event: 'orders_cleared';
}

export type ServerMsg =
  | LoginAckMsg | AckMsg | TradeMsg | ErrorMsg | CancelAckMsg
  | BookUpdateMsg | SnapshotMsg | PortfolioUpdateMsg
  | EngineStatusMsg | AdminAckMsg | AllPortfoliosMsg | OrdersClearedMsg;

// Shared shape used by both SnapshotMsg and the derived order book
export interface PriceLevel {
  price: number;
  quantity: number;
}
