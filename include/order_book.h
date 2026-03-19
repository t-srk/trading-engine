#pragma once

#include <map>
#include <vector>
#include <functional>
#include <optional>
#include "order.h"
#include "price_level.h"

namespace trading {

// The OrderBook holds all resting (unmatched) orders for one instrument.
//
// Structure:
//   bids_ — buy orders,  sorted highest price first (we want best bid = highest)
//   asks_ — sell orders, sorted lowest price first  (we want best ask = lowest)
//
// When a new order arrives:
//   1. Try to match it against the opposite side
//   2. If fully filled → done
//   3. If partially filled or no match → rest the remainder in the book
//
// std::greater<double> on bids_ makes the map iterate highest → lowest
// Default std::less<double> on asks_ makes it iterate lowest → highest

class OrderBook {
public:
    // ── Order entry ───────────────────────────────────────────────────────────

    // Submit a new order — attempts to match, then rests any remainder
    std::vector<Trade> add_order(Order order);

    // ── Book inspection ───────────────────────────────────────────────────────

    // Best bid = highest buy price in the book
    std::optional<double> best_bid() const {
        if (bids_.empty()) return std::nullopt;
        return bids_.begin()->first;
    }

    // Best ask = lowest sell price in the book
    std::optional<double> best_ask() const {
        if (asks_.empty()) return std::nullopt;
        return asks_.begin()->first;
    }

    bool bids_empty() const { return bids_.empty(); }
    bool asks_empty() const { return asks_.empty(); }

    // Read-only access to the full book — used by Server to serialize book_update
    const std::map<double, PriceLevel, std::greater<double>>& get_bids() const { return bids_; }
    const std::map<double, PriceLevel>&                       get_asks() const { return asks_; }

    // Cancel a resting order by ID
    // Returns true if found and removed
    bool cancel_order(uint64_t order_id, Side side, double price);

private:
    std::map<double, PriceLevel, std::greater<double>> bids_;
    std::map<double, PriceLevel>                       asks_;

    template<typename MapType>
    std::vector<Trade> match_against(Order& incoming, MapType& opposite_side);

    std::vector<Trade> match_order(Order& incoming);
    void               rest_order(Order& order);

    uint64_t next_trade_id_ = 1;
};

} // namespace trading