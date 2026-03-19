#include "order_book.h"

namespace trading {

// ── Public: add_order ─────────────────────────────────────────────────────────
std::vector<Trade> OrderBook::add_order(Order order) {
   std::vector<Trade> trades = match_order(order);

   if (order.remaining_qty() > 0 && !order.is_complete()) {
      rest_order(order);
   }
   return trades;
}

// ── Private: match_against ────────────────────────────────────────────────────
// Template so it works for both map types (std::less and std::greater)
// This is the core insight: the matching algorithm is identical regardless
// of which side of the book we're walking — only the type differs.
template<typename MapType>
std::vector<Trade> OrderBook::match_against(Order& incoming, MapType& opposite_side) {
   // empty list to store any trades that may happen
   std::vector<Trade> trades;

   // keep trying to match as long as the incoming orders is not zero and there is something on the opposite side
   while (incoming.remaining_qty() > 0 && !opposite_side.empty()) {
      // get the best level on the opposite side to get a potential match with
      auto it = opposite_side.begin();
      // get the value from the map first level and assign it to pricelevel level
      PriceLevel& level = it->second;
      
      // Price check: does this order cross the spread?
      bool price_matches = (incoming.side == Side::BUY)
         ? (incoming.price >= level.price()) // if aggressor is buying then incoming price has to be geq the first ask level
         : (incoming.price <= level.price()); // if aggressor is selling then incoming price has to be leq the first bid level

      // if it does not cross spread we dont need to match
      if (!price_matches) break;

      // Match against orders at this level one at a time (time priority)
      while (incoming.remaining_qty() > 0 && !level.empty()) {
         Order& resting = level.front();

         // fill based on which ever is minimum
         uint32_t fill_qty = std::min(
            incoming.remaining_qty(),
            resting.remaining_qty()
         );

         // ── Record the trade ──────────────────────────────────────────────
         Trade trade;
         trade.trade_id  = next_trade_id_++;
         trade.timestamp = incoming.timestamp;
         trade.instrument = incoming.instrument;
         trade.price     = resting.price;   // resting order sets the price
         trade.quantity  = fill_qty;

         if (incoming.side == Side::BUY) {
            trade.buy_order_id  = incoming.order_id;
            trade.sell_order_id = resting.order_id;
            trade.buyer_id      = incoming.user_id;
            trade.seller_id     = resting.user_id;
         } else {
            trade.buy_order_id  = resting.order_id;
            trade.sell_order_id = incoming.order_id;
            trade.buyer_id      = resting.user_id;
            trade.seller_id     = incoming.user_id;
         }

         // add the trade to the list 
         trades.push_back(trade);

         // ── Update quantities ─────────────────────────────────────────────
         incoming.filled_qty += fill_qty;
         resting.filled_qty  += fill_qty;
         level.reduce_quantity(fill_qty);

         incoming.status = (incoming.remaining_qty() == 0)
               ? OrderStatus::FILLED : OrderStatus::PARTIALLY_FILLED;
         resting.status = (resting.remaining_qty() == 0)
               ? OrderStatus::FILLED : OrderStatus::PARTIALLY_FILLED;

         if (resting.is_complete()) {
               level.pop_front();
         }
      }

      // Remove the price level if it's been fully drained
      if (level.empty()) {
         opposite_side.erase(it);
      }
   }

   return trades;
}

// ── Private: match_order ──────────────────────────────────────────────────────
// Dispatches to the correct side based on incoming order direction
std::vector<Trade> OrderBook::match_order(Order& incoming) {
   // match with opposite side
   if (incoming.side == Side::BUY) {
      return match_against(incoming, asks_);
   } else {
      return match_against(incoming, bids_);
   }
}

// ── Private: rest_order ───────────────────────────────────────────────────────
void OrderBook::rest_order(Order& order) {
    order.status = (order.filled_qty > 0)
        ? OrderStatus::PARTIALLY_FILLED
        : OrderStatus::OPEN;

    if (order.side == Side::BUY) {
        if (bids_.find(order.price) == bids_.end()) {
            bids_.emplace(order.price, PriceLevel(order.price));
        }
        bids_.at(order.price).add_order(order);
    } else {
        if (asks_.find(order.price) == asks_.end()) {
            asks_.emplace(order.price, PriceLevel(order.price));
        }
        asks_.at(order.price).add_order(order);
    }
}
// ── Public: cancel_order ──────────────────────────────────────────────────────
bool OrderBook::cancel_order(uint64_t order_id, Side side, double price) {
    // Select the right side of the book
    auto cancel_from = [&](auto& book_side) -> bool {
        auto it = book_side.find(price);
        if (it == book_side.end()) return false;

        PriceLevel& level = it->second;

        // std::queue has no erase — we rebuild it without the target order
        // This is O(n) at the price level. Acceptable for now.
        // Production systems use a doubly-linked list for O(1) removal.
        std::queue<Order> rebuilt;
        bool found = false;

        // Drain the existing queue, skip the cancelled order
        while (!level.empty()) {
            Order& front = level.front();
            if (front.order_id == order_id) {
                found = true;
                level.pop_front();      // discard it
            } else {
                rebuilt.push(front);
                level.pop_front();
            }
        }

        // Rebuild the level with remaining orders
        // We need to re-add them properly — easier to reconstruct the level
        if (found) {
            // Remove old level and rebuild
            double lvl_price = level.price();
            book_side.erase(it);

            if (!rebuilt.empty()) {
                book_side.emplace(lvl_price, PriceLevel(lvl_price));
                PriceLevel& new_level = book_side.at(lvl_price);
                while (!rebuilt.empty()) {
                    new_level.add_order(rebuilt.front());
                    rebuilt.pop();
                }
            }
        }

        return found;
    };

    if (side == Side::BUY) {
        return cancel_from(bids_);
    } else {
        return cancel_from(asks_);
    }
}

} // namespace trading