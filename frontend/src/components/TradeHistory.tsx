import type { TradeRecord } from '../hooks/useTrades';

interface Props {
  trades:  TradeRecord[];
  userId:  string;
  isAdmin: boolean;
}

function fmtTime(ts: number): string {
  const d = new Date(ts);
  return d.toTimeString().slice(0, 8);
}

function fmtEdge(n: number): string {
  return (n >= 0 ? '+' : '') + n.toFixed(2);
}

function edgeClass(n: number): string {
  return n > 0 ? ' tr-pos' : n < 0 ? ' tr-neg' : '';
}

export function TradeHistory({ trades, userId, isAdmin }: Props) {
  const visible = isAdmin
    ? trades
    : trades.filter(t => t.buyer_id === userId || t.seller_id === userId);

  return (
    <table className="tr-table">
      <colgroup>
        <col className="tr-col-time" />
        <col className="tr-col-instr" />
        {isAdmin ? (
          <>
            <col className="tr-col-party" />
            <col className="tr-col-party" />
          </>
        ) : (
          <>
            <col className="tr-col-side" />
            <col className="tr-col-party" />
          </>
        )}
        <col className="tr-col-num" />
        <col className="tr-col-num" />
        <col className="tr-col-num" />
      </colgroup>
      <thead>
        <tr>
          <th className="tr-th">time</th>
          <th className="tr-th">instrument</th>
          {isAdmin ? (
            <>
              <th className="tr-th">buyer</th>
              <th className="tr-th">seller</th>
            </>
          ) : (
            <>
              <th className="tr-th">side</th>
              <th className="tr-th">counterparty</th>
            </>
          )}
          <th className="tr-th tr-th-r">price</th>
          <th className="tr-th tr-th-r">qty</th>
          <th className="tr-th tr-th-r">edge</th>
        </tr>
      </thead>
      <tbody>
        {visible.length === 0 ? (
          <tr>
            <td className="tr-empty" colSpan={isAdmin ? 8 : 8}>no trades yet</td>
          </tr>
        ) : (
          visible.map(t => {
            const isBuyer    = t.buyer_id === userId;
            const side       = isBuyer ? 'BUY' : 'SELL';
            const counterparty = isBuyer ? t.seller_id : t.buyer_id;
            // edge: how much better than mid the user traded
            // buyer:  positive when bought below mid  → mid - price
            // seller: positive when sold above mid    → price - mid
            // admin:  show buyer's edge
            const edgePerUnit = isAdmin
              ? t.mid_at_trade - t.price
              : (isBuyer ? t.mid_at_trade - t.price : t.price - t.mid_at_trade);
            const edge = edgePerUnit * t.quantity;

            return (
              <tr key={t.trade_id} className="tr-row">
                <td className="tr-td tr-time">{fmtTime(t.received_at)}</td>
                <td className="tr-td tr-instr">{t.instrument}</td>
                {isAdmin ? (
                  <>
                    <td className="tr-td tr-party">{t.buyer_id}</td>
                    <td className="tr-td tr-party">{t.seller_id}</td>
                  </>
                ) : (
                  <>
                    <td className="tr-td">
                      <span className={`tr-side-badge tr-side-${side.toLowerCase()}`}>{side}</span>
                    </td>
                    <td className="tr-td tr-party">{counterparty}</td>
                  </>
                )}
                <td className="tr-td tr-td-r">{t.price.toFixed(2)}</td>
                <td className="tr-td tr-td-r">{t.quantity}</td>
                <td className={`tr-td tr-td-r tr-edge${edgeClass(edge)}`}>{fmtEdge(edge)}</td>
              </tr>
            );
          })
        )}
      </tbody>
    </table>
  );
}
