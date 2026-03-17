#pragma once  // include guard — prevents this file being processed twice

#include <cstdint>   // uint64_t, uint32_t
#include <string>

// All our types live in this namespace — good practice from day 1
namespace trading {

// ── Side ─────────────────────────────────────────────────────────────────────
// Whether this order is a buy or a sell
enum class Side {
    BUY,
    SELL
};

// ── OrderType ────────────────────────────────────────────────────────────────
// LIMIT  = "I want to buy/sell at exactly this price or better"
// MARKET = "Fill me immediately at whatever price is available"
// We start with LIMIT only — market orders are an extension
enum class OrderType {
    LIMIT,
    MARKET
};

// ── OrderStatus ───────────────────────────────────────────────────────────────
enum class OrderStatus {
    OPEN,
    PARTIALLY_FILLED,
    FILLED,
    CANCELLED
};

// ── Order ────────────────────────────────────────────────────────────────────
// The atomic unit of the entire system.
// Every order placed by any user becomes one of these.
struct Order {
    uint64_t    order_id;       // unique identifier, assigned by engine
    uint64_t    timestamp;      // nanoseconds since epoch — for time priority
    std::string user_id;        // who placed this order
    std::string instrument;     // e.g. "AAPL", "BTC-USD" — what they're trading
    
    double      price;          // limit price (ignored for MARKET orders)
    uint32_t    quantity;       // total quantity requested
    uint32_t    filled_qty;     // how much has been matched so far
    
    Side        side;
    OrderType   type;
    OrderStatus status;

    // Convenience: how much is still left to fill
    uint32_t remaining_qty() const {
        return quantity - filled_qty;
    }

    bool is_complete() const {
        return status == OrderStatus::FILLED || 
               status == OrderStatus::CANCELLED;
    }
};

// ── Trade ────────────────────────────────────────────────────────────────────
// A Trade is produced when two orders match.
// One buy order + one sell order = one (or more) Trade records.
struct Trade {
    uint64_t    trade_id;
    uint64_t    timestamp;

    uint64_t    buy_order_id;
    uint64_t    sell_order_id;

    std::string buyer_id;
    std::string seller_id;
    std::string instrument;

    double      price;       // the price at which the match happened
    uint32_t    quantity;    // how much traded
};

} // namespace trading