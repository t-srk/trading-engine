import { forwardRef, useImperativeHandle, useRef, useMemo, memo } from 'react';
import type { OrderBookState } from '../hooks/useOrderBook';

export interface OrderBookHandle {
  scrollToMid: () => void;
}

interface Props {
  priceLadder: number[];
  book: OrderBookState;
  midPrice: number;
  onClickBid: (price: number) => void;
  onClickAsk: (price: number) => void;
}

function buildMaps(book: OrderBookState) {
  const bids = new Map<string, number>();
  const asks = new Map<string, number>();
  for (const l of book.bids) bids.set(l.price.toFixed(2), l.quantity);
  for (const l of book.asks) asks.set(l.price.toFixed(2), l.quantity);
  return { bids, asks };
}

export const OrderBook = memo(forwardRef<OrderBookHandle, Props>(
  function OrderBook({ priceLadder, book, midPrice, onClickBid, onClickAsk }, ref) {
    const containerRef = useRef<HTMLDivElement>(null);

    useImperativeHandle(ref, () => ({
      scrollToMid() {
        if (!containerRef.current) return;
        const row = containerRef.current.querySelector(
          `tr[data-price="${midPrice.toFixed(2)}"]`
        ) as HTMLElement | null;
        row?.scrollIntoView({ block: 'center' });
      },
    }), [midPrice]);

    // Rebuild price maps only when the server-side levels actually change
    const { bids, asks } = useMemo(() => buildMaps(book), [book.bids, book.asks]);

    return (
      <div ref={containerRef}>
        <table className="ob-table">
          <colgroup>
            <col className="col-own" />
            <col className="col-bid" />
            <col className="col-price" />
            <col className="col-ask" />
            <col className="col-own" />
          </colgroup>
          <thead>
            <tr>
              <th className="th-own-bid" />
              <th className="th-bid">bid</th>
              <th className="th-price">price</th>
              <th className="th-ask">ask</th>
              <th className="th-own-ask" />
            </tr>
          </thead>
          <tbody>
            {priceLadder.map(price => {
              const key   = price.toFixed(2);
              const myBid = book.myBidQty.get(price);
              const myAsk = book.myAskQty.get(price);
              const isMid = price === midPrice;

              return (
                <tr key={key} data-price={key} className={isMid ? 'ob-mid-row' : undefined}>
                  {/* Own bid: × on the left (outer edge), qty on the right (inner) */}
                  <td className="own-bid-cell">
                    {myBid ? (
                      <span className="own-level own-bid-level">
                        <button
                          className="cancel-btn"
                          onClick={e => { e.stopPropagation(); book.cancelLevel('BUY', price); }}
                          title="Cancel your bids at this level"
                        >×</button>
                        <span className="own-qty">{myBid}</span>
                      </span>
                    ) : null}
                  </td>

                  <td
                    className={`bid-cell${myBid ? ' my-order' : ''}`}
                    onClick={() => onClickBid(price)}
                  >
                    {bids.get(key) ?? ''}
                  </td>

                  <td className="price-cell">{key}</td>

                  <td
                    className={`ask-cell${myAsk ? ' my-order' : ''}`}
                    onClick={() => onClickAsk(price)}
                  >
                    {asks.get(key) ?? ''}
                  </td>

                  {/* Own ask: qty on the left (inner), × on the right (outer edge) */}
                  <td className="own-ask-cell">
                    {myAsk ? (
                      <span className="own-level own-ask-level">
                        <span className="own-qty">{myAsk}</span>
                        <button
                          className="cancel-btn"
                          onClick={e => { e.stopPropagation(); book.cancelLevel('SELL', price); }}
                          title="Cancel your asks at this level"
                        >×</button>
                      </span>
                    ) : null}
                  </td>
                </tr>
              );
            })}
          </tbody>
        </table>
      </div>
    );
  }
));
