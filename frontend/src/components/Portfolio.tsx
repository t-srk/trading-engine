import { memo } from 'react';
import type { PortfolioPosition } from '../types';

interface Props {
  positions: PortfolioPosition[];
}

function fmt(n: number, decimals = 2): string {
  return n.toFixed(decimals);
}

function pnlClass(n: number): string {
  return n > 0 ? ' pf-pos' : n < 0 ? ' pf-neg' : '';
}

function signedFmt(n: number): string {
  return (n >= 0 ? '+' : '') + fmt(n);
}

export const Portfolio = memo(function Portfolio({ positions }: Props) {
  return (
    <table className="pf-table">
      <colgroup>
        <col className="pf-col-instr" />
        <col className="pf-col-qty" />
        <col className="pf-col-num" />
        <col className="pf-col-num" />
        <col className="pf-col-num" />
        <col className="pf-col-num" />
        <col className="pf-col-num" />
      </colgroup>
      <thead>
        <tr>
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
        {positions.length === 0 ? (
          <tr>
            <td className="pf-empty" colSpan={7}>no positions</td>
          </tr>
        ) : (
          positions.map(pos => {
            const total = pos.unrealized_pnl + pos.realized_pnl;
            return (
              <tr key={pos.instrument} className="pf-row">
                <td className="pf-td pf-instr">{pos.instrument}</td>
                <td className={`pf-td pf-td-r pf-qty${pos.qty > 0 ? ' pf-long' : pos.qty < 0 ? ' pf-short' : ''}`}>
                  {pos.qty > 0 ? `+${pos.qty}` : pos.qty}
                </td>
                <td className="pf-td pf-td-r">{fmt(pos.avg_cost)}</td>
                <td className="pf-td pf-td-r">{fmt(pos.last_price)}</td>
                <td className={`pf-td pf-td-r pf-pnl${pnlClass(pos.unrealized_pnl)}`}>
                  {signedFmt(pos.unrealized_pnl)}
                </td>
                <td className={`pf-td pf-td-r pf-pnl${pnlClass(pos.realized_pnl)}`}>
                  {signedFmt(pos.realized_pnl)}
                </td>
                <td className={`pf-td pf-td-r pf-pnl pf-total${pnlClass(total)}`}>
                  {signedFmt(total)}
                </td>
              </tr>
            );
          })
        )}
      </tbody>
    </table>
  );
});
