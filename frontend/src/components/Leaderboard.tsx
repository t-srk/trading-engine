import { useState, useEffect, memo } from 'react';
import type { LeaderboardEntry } from '../types';

interface Props {
  entries:       LeaderboardEntry[];
  currentUser:   string;
  onThrowTomato: (targetId: string) => void;
}

const COOLDOWN_MS = 10_000;

export const Leaderboard = memo(function Leaderboard({ entries, currentUser, onThrowTomato }: Props) {
  const [cooldownUntil, setCooldownUntil] = useState<number | null>(null);
  const [cooldownLeft,  setCooldownLeft]  = useState(0);

  // Tick down the displayed cooldown counter
  useEffect(() => {
    if (!cooldownUntil) return;
    const id = setInterval(() => {
      const left = Math.ceil((cooldownUntil - Date.now()) / 1000);
      if (left <= 0) {
        setCooldownUntil(null);
        setCooldownLeft(0);
      } else {
        setCooldownLeft(left);
      }
    }, 200);
    return () => clearInterval(id);
  }, [cooldownUntil]);

  const sorted  = [...entries].sort((a, b) => b.total_pnl - a.total_pnl);
  const topUser = sorted[0]?.user_id;

  function handleThrow() {
    if (!topUser || cooldownUntil) return;
    onThrowTomato(topUser);
    setCooldownUntil(Date.now() + COOLDOWN_MS);
    setCooldownLeft(10);
  }

  if (sorted.length === 0) {
    return <div className="lb-empty">no traders yet</div>;
  }

  return (

    <table className="lb-table">
      <thead>
        <tr>
          <th className="lb-th lb-th-rank">#</th>
          <th className="lb-th">Trader</th>
          <th className="lb-th lb-th-r">PnL</th>
          <th className="lb-th lb-th-action"></th>
        </tr>
      </thead>
      <tbody>
        {sorted.map((e, i) => {
          const isTop = e.user_id === topUser;
          const isMe  = e.user_id === currentUser;
          const canThrow = isTop && !isMe && currentUser !== '';
          return (
            <tr key={e.user_id} className={isTop ? 'lb-row-top' : ''}>
              <td className="lb-td lb-rank">{i + 1}</td>
              <td className={`lb-td lb-name${isMe ? ' lb-me' : ''}`}>{e.user_id}</td>
              <td className={`lb-td lb-td-r lb-pnl ${e.total_pnl >= 0 ? 'pf-pos' : 'pf-neg'}`}>
                {e.total_pnl >= 0 ? '+' : ''}{e.total_pnl.toFixed(2)}
              </td>
              <td className="lb-td lb-action">
                {canThrow && (
                  <button
                    className={`tomato-btn${cooldownUntil ? ' tomato-btn-cd' : ''}`}
                    onClick={handleThrow}
                    disabled={!!cooldownUntil}
                    title={cooldownUntil
                      ? `cooldown: ${cooldownLeft}s`
                      : `throw tomato at ${e.user_id}`}
                  >
                    {cooldownUntil ? `🍅 ${cooldownLeft}s` : '🍅'}
                  </button>
                )}
              </td>
            </tr>
          );
        })}
      </tbody>
    </table>
  );
});
