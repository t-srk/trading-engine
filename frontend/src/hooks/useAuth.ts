import { useEffect, useRef, useState, useCallback } from 'react';
import type { ClientMsg, ServerMsg } from '../types';
import type { UseSocketResult, ConnectionStatus } from '../ws/useSocket';

export interface UseAuthResult {
  status: ConnectionStatus;
  send: (msg: ClientMsg) => void;
  subscribe: UseSocketResult['subscribe'];
  loggedIn: boolean;
  replaced: boolean;
  // Explicitly start the login handshake with a chosen username.
  // Safe to call before the socket is open — will send as soon as it connects.
  login: (userId: string) => void;
}

export function useAuth(socket: UseSocketResult): UseAuthResult {
  const disableRef = useRef(socket.disable);
  disableRef.current = socket.disable;
  const [loggedIn, setLoggedIn] = useState(false);
  const [replaced, setReplaced] = useState(false);

  const tokenRef  = useRef<string | null>(null);
  // Stores the userId the user explicitly chose. Empty = user hasn't logged in yet.
  // Cleared on session_replaced so a reconnect doesn't auto-steal back the seat.
  const pendingUserRef = useRef<string>('');

  // Always-current refs so effects/callbacks don't need them as deps
  const sendRef   = useRef(socket.send);
  const statusRef = useRef(socket.status);
  sendRef.current   = socket.send;
  statusRef.current = socket.status;

  // When the socket (re)opens, re-send login if the user already chose a name.
  // This handles normal reconnects (network blip) transparently.
  useEffect(() => {
    if (socket.status !== 'open') return;
    if (!pendingUserRef.current) return;       // no name yet — wait for explicit login()
    setLoggedIn(false);
    tokenRef.current = null;
    sendRef.current({ action: 'login', user_id: pendingUserRef.current });
  }, [socket.status]);

  // Handle login_ack and session_replaced
  useEffect(() => {
    return socket.subscribe((msg: ServerMsg) => {
      if (msg.event === 'login_ack') {
        tokenRef.current = msg.token;
        setLoggedIn(true);
        setReplaced(false);
        return;
      }
      if (msg.event === 'error' && msg.reason === 'session_replaced') {
        tokenRef.current       = null;
        pendingUserRef.current = '';
        setLoggedIn(false);
        setReplaced(true);
        // Stop reconnecting — the user must refresh to start a new session
        disableRef.current();
      }
    });
  }, [socket.subscribe]);

  // Called by the UI when the user submits their username.
  const login = useCallback((userId: string) => {
    if (!userId) return;
    pendingUserRef.current = userId;
    if (statusRef.current === 'open') {
      tokenRef.current = null;
      setLoggedIn(false);
      sendRef.current({ action: 'login', user_id: userId });
    }
    // If socket isn't open yet, the status effect above will fire when it connects
  }, []);

  // Wrapped send: inject token; drop silently if not authenticated
  const send = useCallback((msg: ClientMsg) => {
    if (msg.action === 'login') { sendRef.current(msg); return; }
    if (!tokenRef.current) { console.warn('[auth] send dropped — not authenticated'); return; }
    sendRef.current({ ...msg, token: tokenRef.current } as ClientMsg);
  }, []);

  return {
    status:    socket.status,
    send,
    subscribe: socket.subscribe,
    loggedIn,
    replaced,
    login,
  };
}
