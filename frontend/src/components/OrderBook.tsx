import type { OrderBookState } from '../hooks/useOrderBook';

interface Props {
  priceLadder: number[];
  book: OrderBookState;
  onClickBid: (price: number) => void;
  onClickAsk: (price: number) => void;
}

// Build lookup maps from the sorted arrays for O(1) access by price.
// Prices are normalised to 2dp strings to avoid float key issues.
function buildMaps(book: OrderBookState) {
  const bids = new Map<string, number>();
  const asks = new Map<string, number>();
  for (const l of book.bids) bids.set(l.price.toFixed(2), l.quantity);
  for (const l of book.asks) asks.set(l.price.toFixed(2), l.quantity);
  return { bids, asks };
}

export function OrderBook({ priceLadder, book, onClickBid, onClickAsk }: Props) {
  const { bids, asks } = buildMaps(book);

  return (
    <table className="ob-table">
      <thead>
        <tr>
          <th className="th-bid">bid</th>
          <th className="th-price">price</th>
          <th className="th-ask">ask</th>
        </tr>
      </thead>
      <tbody>
        {priceLadder.map(price => {
          const key    = price.toFixed(2);
          const bidQty = bids.get(key);
          const askQty = asks.get(key);
          const isMyBid = book.myBidPrices.has(price);
          const isMyAsk = book.myAskPrices.has(price);

          return (
            <tr key={key}>
              <td
                className={`bid-cell${isMyBid ? ' my-order' : ''}`}
                onClick={() => onClickBid(price)}
              >
                {bidQty !== undefined ? bidQty : ''}
                {isMyBid && <span className="mine-marker" title="your order" />}
              </td>

              <td className="price-cell">{key}</td>

              <td
                className={`ask-cell${isMyAsk ? ' my-order' : ''}`}
                onClick={() => onClickAsk(price)}
              >
                {isMyAsk && <span className="mine-marker" title="your order" />}
                {askQty !== undefined ? askQty : ''}
              </td>
            </tr>
          );
        })}
      </tbody>
    </table>
  );
}
