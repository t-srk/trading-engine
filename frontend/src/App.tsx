import { useState, useCallback, useEffect } from 'react';
import { useSocket } from './ws/useSocket';
import { useOrderBook } from './hooks/useOrderBook';
import { OrderBook } from './components/OrderBook';

const WS_URL     = 'ws://localhost:9000';
const INSTRUMENT = 'product_1.0';
const ORDER_QTY  = 1;

// Fixed price ladder: 41 levels centred on 100, tick = 0.50 (high → low)
const TICK   = 0.50;
const MID    = 100;
const LEVELS = 20;
const PRICE_LADDER: number[] = Array.from(
  { length: LEVELS * 2 + 1 },
  (_, i) => parseFloat((MID + (LEVELS - i) * TICK).toFixed(2)),
);

const STATUS_COLOR: Record<string, string> = {
  open:       '#16a34a',
  connecting: '#ca8a04',
  closed:     '#dc2626',
  error:      '#dc2626',
};

export function App() {
  const [userId,    setUserId]    = useState('trader1');
  const [serverMsg, setServerMsg] = useState<{ text: string; ok: boolean } | null>(null);

  const socket = useSocket({ url: WS_URL });
  const book   = useOrderBook(socket, INSTRUMENT, userId);

  // Surface acks and errors so the user can see what's happening
  useEffect(() => {
    let timer: ReturnType<typeof setTimeout>;
    return socket.subscribe(msg => {
      clearTimeout(timer);
      if (msg.event === 'ack') {
        setServerMsg({ text: `ack #${msg.order_id} — ${msg.side} ${msg.quantity} @ ${msg.price}`, ok: true });
      } else if (msg.event === 'error') {
        setServerMsg({ text: `error: ${msg.reason}`, ok: false });
      } else if (msg.event === 'cancel_ack') {
        setServerMsg({ text: `cancel ${msg.success ? 'ok' : 'failed'} #${msg.order_id}`, ok: msg.success });
      }
      timer = setTimeout(() => setServerMsg(null), 4000);
    });
  }, [socket]);

  const handleClickBid = useCallback((price: number) => {
    socket.send({ action: 'submit', user_id: userId, instrument: INSTRUMENT, side: 'BUY',  price, quantity: ORDER_QTY });
  }, [socket, userId]);

  const handleClickAsk = useCallback((price: number) => {
    socket.send({ action: 'submit', user_id: userId, instrument: INSTRUMENT, side: 'SELL', price, quantity: ORDER_QTY });
  }, [socket, userId]);

  return (
    <div className="app">
      <header className="topbar">
        <span className="product-name">{INSTRUMENT}</span>

        <label className="user-label">
          user
          <input
            className="user-input"
            value={userId}
            onChange={e => setUserId(e.target.value.trim())}
            spellCheck={false}
          />
        </label>

        <span
          className="conn-dot"
          style={{ color: STATUS_COLOR[socket.status] ?? '#9ca3af' }}
          title={socket.status}
        >
          ● {socket.status}
        </span>
      </header>

      <main className="workspace">
        <div className="panel">
          <div className="panel-header">Order Book</div>
          <div className="panel-body">
            <OrderBook
              priceLadder={PRICE_LADDER}
              book={book}
              onClickBid={handleClickBid}
              onClickAsk={handleClickAsk}
            />
          </div>
        </div>
      </main>

      {serverMsg && (
        <div className={`toast ${serverMsg.ok ? 'toast-ok' : 'toast-err'}`}>
          {serverMsg.text}
        </div>
      )}
    </div>
  );
}
