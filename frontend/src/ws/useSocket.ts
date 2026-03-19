import { useEffect, useRef, useCallback, useState } from 'react';
import type { ClientMsg, ServerMsg } from '../types';

export type ConnectionStatus = 'connecting' | 'open' | 'closed' | 'error';

type MessageHandler = (msg: ServerMsg) => void;

export interface UseSocketResult {
  status: ConnectionStatus;
  send: (msg: ClientMsg) => void;
  /** Register a handler; returns an unsubscribe function. */
  subscribe: (handler: MessageHandler) => () => void;
  /** Permanently stop reconnecting and close the socket. */
  disable: () => void;
}

interface UseSocketOptions {
  url: string;
  reconnectDelayMs?: number;
}

export function useSocket({
  url,
  reconnectDelayMs = 2000,
}: UseSocketOptions): UseSocketResult {
  const [status, setStatus] = useState<ConnectionStatus>('connecting');

  const wsRef           = useRef<WebSocket | null>(null);
  const handlersRef     = useRef<Set<MessageHandler>>(new Set());
  const reconnectTimer  = useRef<ReturnType<typeof setTimeout> | null>(null);
  const unmountedRef    = useRef(false);
  const disabledRef     = useRef(false);

  const connect = useCallback(() => {
    if (unmountedRef.current) return;

    setStatus('connecting');
    const ws = new WebSocket(url);
    wsRef.current = ws;

    ws.onopen = () => {
      if (!unmountedRef.current) setStatus('open');
    };

    ws.onclose = () => {
      if (unmountedRef.current) return;
      setStatus('closed');
      if (!disabledRef.current) {
        reconnectTimer.current = setTimeout(connect, reconnectDelayMs);
      }
    };

    ws.onerror = () => {
      if (!unmountedRef.current) setStatus('error');
      ws.close(); // triggers onclose → reconnect
    };

    ws.onmessage = (event: MessageEvent<string>) => {
      let msg: ServerMsg;
      try {
        msg = JSON.parse(event.data) as ServerMsg;
      } catch {
        console.warn('[ws] unparseable message:', event.data);
        return;
      }
      handlersRef.current.forEach(h => h(msg));
    };
  }, [url, reconnectDelayMs]);

  useEffect(() => {
    unmountedRef.current = false;
    connect();
    return () => {
      unmountedRef.current = true;
      if (reconnectTimer.current) clearTimeout(reconnectTimer.current);
      wsRef.current?.close();
    };
  }, [connect]);

  const send = useCallback((msg: ClientMsg) => {
    if (wsRef.current?.readyState === WebSocket.OPEN) {
      wsRef.current.send(JSON.stringify(msg));
    } else {
      console.warn('[ws] send dropped — not connected');
    }
  }, []);

  const subscribe = useCallback((handler: MessageHandler) => {
    handlersRef.current.add(handler);
    return () => {
      handlersRef.current.delete(handler);
    };
  }, []);

  const disable = useCallback(() => {
    disabledRef.current = true;
    if (reconnectTimer.current) clearTimeout(reconnectTimer.current);
    wsRef.current?.close();
  }, []);

  return { status, send, subscribe, disable };
}
