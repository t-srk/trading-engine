#pragma once
#include <string>
#include <cstdint>

namespace trading {

// ── Client → Server ───────────────────────────────────────────────────────────

struct SubmitOrderMsg {
    std::string action;       // "submit"
    std::string user_id;
    std::string instrument;
    std::string side;         // "BUY" or "SELL"
    double      price;
    uint32_t    quantity;
};

struct CancelOrderMsg {
    std::string action;       // "cancel"
    std::string user_id;
    uint64_t    order_id;
};

// ── Server → Client ───────────────────────────────────────────────────────────

struct AckMsg {
    std::string event;        // "ack"
    uint64_t    order_id;
    std::string status;       // "OPEN"
    std::string instrument;
    std::string side;
    double      price;
    uint32_t    quantity;
};

struct TradeMsg {
    std::string event;        // "trade"
    uint64_t    trade_id;
    std::string instrument;
    double      price;
    uint32_t    quantity;
    std::string buyer_id;
    std::string seller_id;
    uint64_t    buy_order_id;
    uint64_t    sell_order_id;
};

struct ErrorMsg {
    std::string event;        // "error"
    std::string reason;
};

struct CancelAckMsg {
    std::string event;        // "cancel_ack"
    uint64_t    order_id;
    bool        success;
};

} // namespace trading