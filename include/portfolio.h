#pragma once

#include <string>
#include <unordered_map>
#include "order.h"   // Trade

namespace trading {

// ── Position ──────────────────────────────────────────────────────────────────
// Tracks one user's net exposure for a single instrument.
// qty > 0 = long, qty < 0 = short, qty == 0 = flat.
struct Position {
    std::string instrument;
    int32_t     qty         = 0;
    double      avg_cost    = 0.0;   // volume-weighted average entry price
    double      realized_pnl = 0.0;
    double      last_price  = 0.0;   // last traded price for mark-to-market

    double unrealized_pnl() const {
        return qty * (last_price - avg_cost);
    }
};

// ── Portfolio ─────────────────────────────────────────────────────────────────
// Maintains all positions for one user across every instrument.
class Portfolio {
public:
    // Update positions using a completed trade.
    // user_id tells us which side of the trade this portfolio belongs to.
    void apply_trade(const Trade& trade, const std::string& user_id);

    const std::unordered_map<std::string, Position>& positions() const {
        return positions_;
    }

private:
    std::unordered_map<std::string, Position> positions_;   // instrument → Position
};

} // namespace trading
