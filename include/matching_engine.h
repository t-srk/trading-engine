#pragma once

#include <unordered_map>
#include <string>
#include <vector>
#include <chrono>
#include <stdexcept>
#include "order.h"
#include "order_book.h"

namespace trading {

// ── MatchingEngine ────────────────────────────────────────────────────────────
// The top-level coordinator. Responsibilities:
//   1. Assign unique order IDs and timestamps
//   2. Route orders to the correct OrderBook by instrument
//   3. Track every live order by ID (for cancellation)
//   4. Record all trades that have occurred
//
// This is the only class the outside world (later: the server) talks to.
// OrderBook and PriceLevel are internal implementation details.

class MatchingEngine {
public:
    // ── Order submission ──────────────────────────────────────────────────────

    // Submit a new order. Returns any trades that resulted from matching.
    // Throws std::invalid_argument if the instrument is not registered.
    std::vector<Trade> submit_order(const std::string& user_id,
                                    const std::string& instrument,
                                    Side               side,
                                    double             price,
                                    uint32_t           quantity,
                                    OrderType          type = OrderType::LIMIT);

    // Cancel a resting order by ID.
    // Returns true if found and cancelled, false if not found or already filled.
    bool cancel_order(uint64_t order_id);

    // ── Instrument management ─────────────────────────────────────────────────

    // Register a tradeable instrument (e.g. "BTC-USD", "AAPL")
    // Must be called before any orders are submitted for that instrument
    void add_instrument(const std::string& instrument);

    bool has_instrument(const std::string& instrument) const {
        return books_.count(instrument) > 0;
    }

    // ── Inspection ────────────────────────────────────────────────────────────

    const OrderBook& get_book(const std::string& instrument) const {
        return books_.at(instrument);
    }

    const std::vector<Trade>& trade_history() const {
        return trade_history_;
    }

    // Look up any order by ID (live or completed)
    const Order* find_order(uint64_t order_id) const {
        auto it = orders_.find(order_id);
        if (it == orders_.end()) return nullptr;
        return &it->second;
    }

private:
    // One OrderBook per instrument
    std::unordered_map<std::string, OrderBook> books_;

    // All orders ever submitted, keyed by order_id
    // This is how we support cancellation — we need to find the order fast
    std::unordered_map<uint64_t, Order> orders_;

    // Complete record of every trade that happened
    std::vector<Trade> trade_history_;

    // Monotonically increasing counters
    uint64_t next_order_id_ = 1;

    // Nanosecond timestamp — same resolution real exchanges use
    uint64_t now_ns() const {
        using namespace std::chrono;
        return static_cast<uint64_t>(
            duration_cast<nanoseconds>(
                steady_clock::now().time_since_epoch()
            ).count()
        );
    }
};

} // namespace trading