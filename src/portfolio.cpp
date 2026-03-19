#include "portfolio.h"
#include <algorithm>

namespace trading {

// ── apply_trade ───────────────────────────────────────────────────────────────
// Called once for the buyer's portfolio and once for the seller's portfolio
// after every matched trade.
//
// Long/short accounting rules
// ───────────────────────────
// Buying into a long (qty >= 0):
//   new avg_cost = (old_qty * old_avg + trade_qty * trade_price) / new_qty
//
// Buying to close a short (qty < 0):
//   realize PnL = closed_qty * (avg_cost - trade_price)   [sold high, buy back lower = profit]
//   if completely flipped to long: new avg_cost = trade_price
//
// Selling to close a long (qty > 0):
//   realize PnL = closed_qty * (trade_price - avg_cost)   [bought low, sold higher = profit]
//   if completely flipped to short: new avg_cost = trade_price
//
// Selling into a short (qty <= 0):
//   new avg_cost = (old_short_qty * old_avg + trade_qty * trade_price) / new_short_qty
//   (uses absolute short quantities for the weighted average)
void Portfolio::apply_trade(const Trade& trade, const std::string& user_id) {
    auto& pos        = positions_[trade.instrument];
    pos.instrument   = trade.instrument;
    auto trade_qty   = static_cast<int32_t>(trade.quantity);

    if (trade.buyer_id == user_id) {
        if (pos.qty >= 0) {
            // Adding to long (or opening from flat)
            double new_qty   = pos.qty + trade_qty;
            pos.avg_cost     = (pos.qty * pos.avg_cost + trade_qty * trade.price) / new_qty;
            pos.qty          += trade_qty;
        } else {
            // Closing / covering a short
            int32_t closed    = std::min(trade_qty, -pos.qty);
            pos.realized_pnl += closed * (pos.avg_cost - trade.price);
            pos.qty          += trade_qty;
            if (pos.qty > 0) {
                // Flipped from short to long
                pos.avg_cost = trade.price;
            }
            // Still short or exactly flat: avg_cost stays unchanged
        }
    }

    if (trade.seller_id == user_id) {
        if (pos.qty > 0) {
            // Closing / reducing a long
            int32_t closed    = std::min(trade_qty, pos.qty);
            pos.realized_pnl += closed * (trade.price - pos.avg_cost);
            pos.qty          -= trade_qty;
            if (pos.qty < 0) {
                // Flipped from long to short
                pos.avg_cost = trade.price;
            }
            // Still long or exactly flat: avg_cost stays unchanged
        } else {
            // Adding to short (or opening from flat)
            double new_short  = -pos.qty + trade_qty;   // both positive
            pos.avg_cost      = (-pos.qty * pos.avg_cost + trade_qty * trade.price) / new_short;
            pos.qty          -= trade_qty;
        }
    }

    pos.last_price = trade.price;
}

} // namespace trading
