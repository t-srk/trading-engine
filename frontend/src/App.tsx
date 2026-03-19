import { useState, useCallback, useEffect } from 'react';
import { useSocket } from './ws/useSocket';
import { useAuth } from './hooks/useAuth';
import { useOrderBook } from './hooks/useOrderBook';
import { OrderBook } from './components/OrderBook';

const WS_URL     = 'ws://localhost:9000';
const INSTRUMENT = 'product_1.0';
const ORDER_QTY  = 1;

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
  const [draftUser, setDraftUser] = useState('');   // what's typed in the box
  const [lockedUser, setLockedUser] = useState(''); // confirmed identity after login_ack

  const [serverMsg, setServerMsg] = useState<{ text: string; ok: boolean } | null>(null);

  const rawSocket = useSocket({ url: WS_URL });
  const auth      = useAuth(rawSocket);
  const book      = useOrderBook(auth, INSTRUMENT, lockedUser);

  // Lock the username once login is acknowledged
  useEffect(() => {
    if (auth.loggedIn) setLockedUser(draftUser);
  }, [auth.loggedIn]); // eslint-disable-line react-hooks/exhaustive-deps

  // Unlock and clear on session_replaced so the user can choose again
  useEffect(() => {
    if (auth.replaced) {
      setLockedUser('');
      // keep draftUser so the box still shows their name — easy to re-submit
    }
  }, [auth.replaced]);

  // Toast notifications
  useEffect(() => {
    let timer: ReturnType<typeof setTimeout>;
    return auth.subscribe(msg => {
      clearTimeout(timer);
      if (msg.event === 'ack') {
        setServerMsg({ text: `ack #${msg.order_id} — ${msg.side} ${msg.quantity} @ ${msg.price}`, ok: true });
      } else if (msg.event === 'error' && msg.reason !== 'session_replaced') {
        setServerMsg({ text: `error: ${msg.reason}`, ok: false });
      } else if (msg.event === 'cancel_ack') {
        setServerMsg({ text: `cancel ${msg.success ? 'ok' : 'failed'} #${msg.order_id}`, ok: msg.success });
      }
      timer = setTimeout(() => setServerMsg(null), 4000);
    });
  }, [auth.subscribe]);

  function handleLoginSubmit(e: React.FormEvent) {
    e.preventDefault();
    if (draftUser.trim()) auth.login(draftUser.trim());
  }

  const handleClickBid = useCallback((price: number) => {
    auth.send({ action: 'submit', token: '', user_id: lockedUser, instrument: INSTRUMENT, side: 'BUY',  price, quantity: ORDER_QTY });
  }, [auth, lockedUser]);

  const handleClickAsk = useCallback((price: number) => {
    auth.send({ action: 'submit', token: '', user_id: lockedUser, instrument: INSTRUMENT, side: 'SELL', price, quantity: ORDER_QTY });
  }, [auth, lockedUser]);

  return (
    <div className="app">
      <header className="topbar">
        <span className="product-name">{INSTRUMENT}</span>

        <form className="user-form" onSubmit={handleLoginSubmit}>
          <label className="user-label">
            user
            <input
              className={`user-input${auth.loggedIn ? ' locked' : ''}`}
              value={draftUser}
              onChange={e => { if (!auth.loggedIn) setDraftUser(e.target.value); }}
              readOnly={auth.loggedIn}
              placeholder="enter username…"
              spellCheck={false}
              autoComplete="off"
            />
          </label>
          {!auth.loggedIn && (
            <button type="submit" className="login-btn" disabled={!draftUser.trim()}>
              ↵
            </button>
          )}
          {auth.loggedIn && <span className="login-badge">●</span>}
        </form>

        <span
          className="conn-dot"
          style={{ color: STATUS_COLOR[auth.status] ?? '#9ca3af' }}
          title={auth.status}
        >
          ● {auth.status}
        </span>
      </header>

      {auth.replaced && (
        <div className="replaced-banner">
          Your session was taken over by another connection. Refresh the page to reconnect.
        </div>
      )}

      <main className="workspace">
        <div className="panel">
          <div className="panel-header">
            Order Book
            <button className="cancel-all-btn" onClick={() => book.cancelAll()}>
              cancel all
            </button>
          </div>
          <div className={`panel-body${!auth.loggedIn ? ' panel-disabled' : ''}`}>
            {!auth.loggedIn && !auth.replaced && (
              <div className="login-prompt">Enter a username above and press ↵ to start trading.</div>
            )}
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
