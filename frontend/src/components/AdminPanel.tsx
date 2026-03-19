import type { AdminState } from '../hooks/useAdmin';
import type { PortfolioPosition } from '../types';

interface Props {
  admin: AdminState;
}

function fmt(n: number): string { return n.toFixed(2); }

function signedFmt(n: number): string {
  return (n >= 0 ? '+' : '') + fmt(n);
}

function pnlClass(n: number): string {
  return n > 0 ? ' pf-pos' : n < 0 ? ' pf-neg' : '';
}

function PositionRows({ positions, userId }: { positions: PortfolioPosition[]; userId: string }) {
  if (positions.length === 0) {
    return (
      <tr>
        <td className="pf-td admin-user-cell">{userId}</td>
        <td className="pf-td pf-td-r admin-dim" colSpan={7}>flat</td>
      </tr>
    );
  }
  return (
    <>
      {positions.map((pos, i) => {
        const total = pos.unrealized_pnl + pos.realized_pnl;
        return (
          <tr key={pos.instrument}>
            <td className="pf-td admin-user-cell">{i === 0 ? userId : ''}</td>
            <td className="pf-td">{pos.instrument}</td>
            <td className={`pf-td pf-td-r pf-qty${pos.qty > 0 ? ' pf-long' : pos.qty < 0 ? ' pf-short' : ''}`}>
              {pos.qty > 0 ? `+${pos.qty}` : pos.qty}
            </td>
            <td className="pf-td pf-td-r">{fmt(pos.avg_cost)}</td>
            <td className="pf-td pf-td-r">{fmt(pos.last_price)}</td>
            <td className={`pf-td pf-td-r pf-pnl${pnlClass(pos.unrealized_pnl)}`}>{signedFmt(pos.unrealized_pnl)}</td>
            <td className={`pf-td pf-td-r pf-pnl${pnlClass(pos.realized_pnl)}`}>{signedFmt(pos.realized_pnl)}</td>
            <td className={`pf-td pf-td-r pf-pnl pf-total${pnlClass(total)}`}>{signedFmt(total)}</td>
          </tr>
        );
      })}
    </>
  );
}

export function AdminPanel({ admin }: Props) {
  return (
    <div className="admin-panel-body">
      {/* ── Engine status + actions ── */}
      <div className="admin-controls">
        <div className="admin-engine-status">
          <span className={`admin-engine-dot ${admin.halted ? 'admin-dot-halted' : 'admin-dot-running'}`}>●</span>
          <span className="admin-engine-label">{admin.halted ? 'halted' : 'running'}</span>
        </div>
        <div className="admin-btns">
          {admin.halted
            ? <button className="admin-btn admin-btn-resume" onClick={admin.resume}>Resume Engine</button>
            : <button className="admin-btn admin-btn-halt"   onClick={admin.halt}>Halt Engine</button>
          }
          <button className="admin-btn admin-btn-clear" onClick={admin.clearOrders}>Clear All Orders</button>
          <button className="admin-btn admin-btn-clear" onClick={admin.clearPositions}>Clear All Positions</button>
          <button className="admin-btn" onClick={admin.refreshPortfolios}>Refresh</button>
        </div>
      </div>

      {/* ── All-users portfolio table ── */}
      <table className="pf-table admin-table">
        <thead>
          <tr>
            <th className="pf-th admin-th-user">user</th>
            <th className="pf-th">instrument</th>
            <th className="pf-th pf-th-r">qty</th>
            <th className="pf-th pf-th-r">avg cost</th>
            <th className="pf-th pf-th-r">price</th>
            <th className="pf-th pf-th-r">unreal p&amp;l</th>
            <th className="pf-th pf-th-r">real p&amp;l</th>
            <th className="pf-th pf-th-r">total p&amp;l</th>
          </tr>
        </thead>
        <tbody>
          {admin.allUsers.length === 0 ? (
            <tr>
              <td className="pf-empty" colSpan={8}>press Refresh to load portfolios</td>
            </tr>
          ) : (
            admin.allUsers.map(u => (
              <PositionRows key={u.user_id} userId={u.user_id} positions={u.positions} />
            ))
          )}
        </tbody>
      </table>
    </div>
  );
}
