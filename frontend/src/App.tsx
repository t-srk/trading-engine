import { useState, useCallback, useEffect, useRef, useMemo } from 'react';
import { useSocket } from './ws/useSocket';
import { useAuth } from './hooks/useAuth';
import { useOrderBook } from './hooks/useOrderBook';
import { usePortfolio } from './hooks/usePortfolio';
import { useAdmin } from './hooks/useAdmin';
import { useTrades } from './hooks/useTrades';
import { OrderBook } from './components/OrderBook';
import type { OrderBookHandle } from './components/OrderBook';
import { Portfolio } from './components/Portfolio';
import { TradeHistory } from './components/TradeHistory';
import { AdminPanel } from './components/AdminPanel';
import { Leaderboard } from './components/Leaderboard';
import type { LeaderboardEntry } from './types';

const WS_URL = import.meta.env.VITE_WS_URL ?? 'ws://localhost:9000';
const INSTRUMENT = 'product_1.0';

// Generate a random tomato-splat polygon path (centered at 0,0, fits in ~±210px).
function makeSplatPath(): string {
  const arms      = 7 + Math.floor(Math.random() * 3);   // 7 – 9 arms
  const total     = arms * 2;
  const rotOffset = Math.random() * Math.PI * 2;
  const pts: string[] = [];
  for (let i = 0; i < total; i++) {
    const angle = (i / total) * Math.PI * 2 + rotOffset + (Math.random() - 0.5) * 0.3;
    const r = i % 2 === 0
      ? 115 + Math.random() * 90   // arm tip  : 115 – 205
      :  40 + Math.random() * 50;  // valley   :  40 –  90
    pts.push(`${(Math.cos(angle) * r).toFixed(1)},${(Math.sin(angle) * r).toFixed(1)}`);
  }
  return `M ${pts.join(' L ')} Z`;
}

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
  const [leaderboard, setLeaderboard] = useState<LeaderboardEntry[]>([]);
  const [tomatoSplash, setTomatoSplash] = useState(false);
  const [splatPath,    setSplatPath]    = useState('');

  const rawSocket    = useSocket({ url: WS_URL });
  const auth         = useAuth(rawSocket);
  const book         = useOrderBook(auth, INSTRUMENT, lockedUser);
  const positions    = usePortfolio(auth);
  const admin        = useAdmin(auth, auth.send);
  const midRef       = useRef(100);
  const orderBookRef = useRef<OrderBookHandle>(null);

  const midPrice = useMemo(() => {
    if (book.bids.length > 0 && book.asks.length > 0)
      return Math.round((book.bids[0].price + book.asks[0].price) / 2);
    if (book.bids.length > 0) return Math.round(book.bids[0].price);
    if (book.asks.length > 0) return Math.round(book.asks[0].price);
    return 100;
  }, [book.bids, book.asks]);
  midRef.current = midPrice;

  const trades = useTrades(auth, () => midRef.current);

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

  // Leaderboard + tomato splash
  useEffect(() => {
    return auth.subscribe(msg => {
      if (msg.event === 'leaderboard_update') setLeaderboard(msg.users);
      if (msg.event === 'tomato_hit') {
        setSplatPath(makeSplatPath());
        setTomatoSplash(true);
        setTimeout(() => setTomatoSplash(false), 1500);
      }
    });
  }, [auth.subscribe]);

  const handleThrowTomato = useCallback((targetId: string) => {
    auth.send({ action: 'throw_tomato', token: '', user_id: lockedUser, target_id: targetId });
  }, [auth, lockedUser]);

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
              {auth.loggedIn && (
                <label className="admin-qty-label">
                  qty
                  <input
                    type="number"
                    min={1}
                    max={10}
                    value={orderQty}
                    onChange={e => setOrderQty(Math.min(10, Math.max(1, parseInt(e.target.value) || 1)))}
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

        {/* ── Portfolio + Trades column ── */}
        <div className="right-col">
          <div className="panel">
            <div className="panel-header">Portfolio</div>
            <div className={`panel-body${!auth.loggedIn ? ' panel-disabled' : ''}`}>
              <Portfolio positions={positions} />
            </div>
          </div>
          <div className="panel">
            <div className="panel-header">{isAdmin ? 'All Trades' : 'My Trades'}</div>
            <div className={`panel-body${!auth.loggedIn ? ' panel-disabled' : ''}`}>
              <TradeHistory trades={trades} userId={lockedUser} isAdmin={isAdmin} />
            </div>
          </div>
        </div>

        {/* ── Leaderboard (all logged-in users) ── */}
        {auth.loggedIn && (
          <div className="panel leaderboard-panel">
            <div className="panel-header">Leaderboard</div>
            <div className="panel-body lb-body">
              <Leaderboard
                entries={leaderboard}
                currentUser={lockedUser}
                onThrowTomato={handleThrowTomato}
              />
            </div>
          </div>
        )}

        {/* ── Admin Panel (admin only) ── */}
        {isAdmin && (
          <div className="panel admin-panel">
            <div className="panel-header">Admin</div>
            <AdminPanel admin={admin} />
          </div>
        )}
      </main>

      {/* ── Tomato splash overlay ── */}
      {tomatoSplash && (
        <div className="tomato-splash">
          <svg className="tomato-splat" viewBox="-220 -220 440 440" xmlns="http://www.w3.org/2000/svg">
            <path d={splatPath} />
          </svg>
        </div>
      )}

      {serverMsg && (
        <div className={`toast ${serverMsg.ok ? 'toast-ok' : 'toast-err'}`}>
          {serverMsg.text}
        </div>
      )}
    </div>
  );
}
