import { useState, useCallback, useEffect, useRef, useMemo } from 'react';
import { useSocket } from './ws/useSocket';
import { useAuth } from './hooks/useAuth';
import { useOrderBook } from './hooks/useOrderBook';
import { usePortfolio } from './hooks/usePortfolio';
import { useAdmin } from './hooks/useAdmin';
import { OrderBook } from './components/OrderBook';
import type { OrderBookHandle } from './components/OrderBook';
import { Portfolio } from './components/Portfolio';
import { AdminPanel } from './components/AdminPanel';

const WS_URL     = 'ws://localhost:9000';
const INSTRUMENT = 'product_1.0';

// Full integer ladder 200 → 0 (high to low, as displayed in the book)
const PRICE_LADDER: number[] = Array.from({ length: 201 }, (_, i) => 200 - i);

const STATUS_COLOR: Record<string, string> = {
  open:       '#16a34a',
  connecting: '#ca8a04',
  closed:     '#dc2626',
  error:      '#dc2626',
};

export function App() {
  const [draftUser, setDraftUser] = useState('');
  const [lockedUser, setLockedUser] = useState('');
  const [orderQty, setOrderQty] = useState(1);
  const [engineHalted, setEngineHalted] = useState(false);

  const [serverMsg, setServerMsg] = useState<{ text: string; ok: boolean } | null>(null);

  const rawSocket    = useSocket({ url: WS_URL });
  const auth         = useAuth(rawSocket);
  const book         = useOrderBook(auth, INSTRUMENT, lockedUser);
  const positions    = usePortfolio(auth);
  const admin        = useAdmin(auth, auth.send);
  const orderBookRef = useRef<OrderBookHandle>(null);

  const midPrice = useMemo(() => {
    if (book.bids.length > 0 && book.asks.length > 0)
      return Math.round((book.bids[0].price + book.asks[0].price) / 2);
    if (book.bids.length > 0) return Math.round(book.bids[0].price);
    if (book.asks.length > 0) return Math.round(book.asks[0].price);
    return 100;
  }, [book.bids, book.asks]);

  const isAdmin = lockedUser === 'purplepoet';

  // Lock the username once login is acknowledged
  useEffect(() => {
    if (auth.loggedIn) setLockedUser(draftUser);
  }, [auth.loggedIn]); // eslint-disable-line react-hooks/exhaustive-deps

  // Auto-load portfolios when admin logs in
  useEffect(() => {
    if (auth.loggedIn && isAdmin) admin.refreshPortfolios();
  }, [auth.loggedIn]); // eslint-disable-line react-hooks/exhaustive-deps

  // Unlock and clear on session_replaced
  useEffect(() => {
    if (auth.replaced) setLockedUser('');
  }, [auth.replaced]);

  // Track engine halted state (all users receive engine_status)
  useEffect(() => {
    return auth.subscribe(msg => {
      if (msg.event === 'engine_status') setEngineHalted(msg.halted);
    });
  }, [auth.subscribe]);

  // Toast notifications
  useEffect(() => {
    let timer: ReturnType<typeof setTimeout>;
    return auth.subscribe(msg => {
      clearTimeout(timer);
      if (msg.event === 'error' && msg.reason !== 'session_replaced') {
        setServerMsg({ text: `error: ${msg.reason}`, ok: false });
      } else if (msg.event === 'cancel_ack') {
        setServerMsg({ text: `cancel ${msg.success ? 'ok' : 'failed'} #${msg.order_id}`, ok: msg.success });
      } else if (msg.event === 'admin_ack') {
        setServerMsg({ text: `admin: ${msg.command} ${msg.success ? 'ok' : 'failed'}`, ok: msg.success });
      }
      timer = setTimeout(() => setServerMsg(null), 3000);
    });
  }, [auth.subscribe]);

  function handleLoginSubmit(e: React.FormEvent) {
    e.preventDefault();
    if (draftUser.trim()) auth.login(draftUser.trim());
  }

  const handleClickBid = useCallback((price: number) => {
    auth.send({ action: 'submit', token: '', user_id: lockedUser, instrument: INSTRUMENT, side: 'BUY',  price, quantity: orderQty });
  }, [auth, lockedUser, orderQty]);

  const handleClickAsk = useCallback((price: number) => {
    auth.send({ action: 'submit', token: '', user_id: lockedUser, instrument: INSTRUMENT, side: 'SELL', price, quantity: orderQty });
  }, [auth, lockedUser, orderQty]);

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
            <button type="submit" className="login-btn" disabled={!draftUser.trim()}>↵</button>
          )}
          {auth.loggedIn && <span className="login-badge">●</span>}
        </form>

        {engineHalted && <span className="halted-badge">■ HALTED</span>}

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
        {/* ── Order Book ── */}
        <div className="panel">
          <div className="panel-header">
            Order Book
            <div className="panel-header-right">
              {isAdmin && (
                <label className="admin-qty-label">
                  qty
                  <input
                    type="number"
                    min={1}
                    value={orderQty}
                    onChange={e => setOrderQty(Math.max(1, parseInt(e.target.value) || 1))}
                    className="admin-qty-input"
                  />
                </label>
              )}
              <button className="centre-btn" onClick={() => orderBookRef.current?.scrollToMid()}>
                centre
              </button>
              <button className="cancel-all-btn" onClick={() => book.cancelAll()}>
                cancel all
              </button>
            </div>
          </div>
          <div className={`panel-body${!auth.loggedIn ? ' panel-disabled' : ''}`}>
            {!auth.loggedIn && !auth.replaced && (
              <div className="login-prompt">Enter a username above and press ↵ to start trading.</div>
            )}
            <OrderBook
              ref={orderBookRef}
              priceLadder={PRICE_LADDER}
              book={book}
              midPrice={midPrice}
              onClickBid={handleClickBid}
              onClickAsk={handleClickAsk}
            />
          </div>
        </div>

        {/* ── Portfolio ── */}
        <div className="panel">
          <div className="panel-header">Portfolio</div>
          <div className={`panel-body${!auth.loggedIn ? ' panel-disabled' : ''}`}>
            <Portfolio positions={positions} />
          </div>
        </div>

        {/* ── Admin Panel (admin only) ── */}
        {isAdmin && (
          <div className="panel admin-panel">
            <div className="panel-header">Admin</div>
            <AdminPanel admin={admin} />
          </div>
        )}
      </main>

      {serverMsg && (
        <div className={`toast ${serverMsg.ok ? 'toast-ok' : 'toast-err'}`}>
          {serverMsg.text}
        </div>
      )}
    </div>
  );
}
