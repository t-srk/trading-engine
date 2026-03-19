#pragma once

#include <string>
#include <sstream>
#include <stdexcept>
#include "message.h"

namespace trading::protocol {

// ── Serialize: struct → JSON string ──────────────────────────────────────────
// Each message becomes one line ending in \n

inline std::string serialize(const AckMsg& msg) {
    std::ostringstream ss;
    ss << "{"
       << "\"event\":\"ack\","
       << "\"order_id\":"   << msg.order_id    << ","
       << "\"status\":\""   << msg.status      << "\","
       << "\"instrument\":\"" << msg.instrument << "\","
       << "\"side\":\""     << msg.side        << "\","
       << "\"price\":"      << msg.price       << ","
       << "\"quantity\":"   << msg.quantity
       << "}\n";
    return ss.str();
}

inline std::string serialize(const TradeMsg& msg) {
    std::ostringstream ss;
    ss << "{"
       << "\"event\":\"trade\","
       << "\"trade_id\":"      << msg.trade_id      << ","
       << "\"instrument\":\"" << msg.instrument     << "\","
       << "\"price\":"        << msg.price          << ","
       << "\"quantity\":"     << msg.quantity       << ","
       << "\"buyer_id\":\""   << msg.buyer_id       << "\","
       << "\"seller_id\":\""  << msg.seller_id      << "\","
       << "\"buy_order_id\":" << msg.buy_order_id   << ","
       << "\"sell_order_id\":" << msg.sell_order_id
       << "}\n";
    return ss.str();
}

inline std::string serialize(const ErrorMsg& msg) {
    std::ostringstream ss;
    ss << "{"
       << "\"event\":\"error\","
       << "\"reason\":\"" << msg.reason << "\""
       << "}\n";
    return ss.str();
}

inline std::string serialize(const CancelAckMsg& msg) {
    std::ostringstream ss;
    ss << "{"
       << "\"event\":\"cancel_ack\","
       << "\"order_id\":" << msg.order_id << ","
       << "\"success\":"  << (msg.success ? "true" : "false")
       << "}\n";
    return ss.str();
}

// ── Parse helpers ─────────────────────────────────────────────────────────────
// Extract a string value from a flat JSON line by key.
// e.g. extract_string("{"side":"BUY"}", "side") → "BUY"
// This is intentionally simple — not a general JSON parser.

inline std::string extract_string(const std::string& json,
                                   const std::string& key) {
    // Look for "key":"value"
    std::string search = "\"" + key + "\":\"";
    auto start = json.find(search);
    if (start == std::string::npos)
        throw std::runtime_error("Key not found: " + key);
    start += search.size();
    auto end = json.find("\"", start);
    if (end == std::string::npos)
        throw std::runtime_error("Malformed value for: " + key);
    return json.substr(start, end - start);
}

inline double extract_double(const std::string& json,
                              const std::string& key) {
    std::string search = "\"" + key + "\":";
    auto start = json.find(search);
    if (start == std::string::npos)
        throw std::runtime_error("Key not found: " + key);
    start += search.size();
    return std::stod(json.substr(start));
}

inline uint64_t extract_uint64(const std::string& json,
                                const std::string& key) {
    std::string search = "\"" + key + "\":";
    auto start = json.find(search);
    if (start == std::string::npos)
        throw std::runtime_error("Key not found: " + key);
    start += search.size();
    return std::stoull(json.substr(start));
}

inline uint32_t extract_uint32(const std::string& json,
                                const std::string& key) {
    return static_cast<uint32_t>(extract_uint64(json, key));
}

// ── Parse: JSON string → struct ───────────────────────────────────────────────

inline SubmitOrderMsg parse_submit(const std::string& json) {
    SubmitOrderMsg msg;
    msg.action     = extract_string(json, "action");
    msg.user_id    = extract_string(json, "user_id");
    msg.instrument = extract_string(json, "instrument");
    msg.side       = extract_string(json, "side");
    msg.price      = extract_double(json, "price");
    msg.quantity   = extract_uint32(json, "quantity");
    return msg;
}

inline CancelOrderMsg parse_cancel(const std::string& json) {
    CancelOrderMsg msg;
    msg.action   = extract_string(json, "action");
    msg.user_id  = extract_string(json, "user_id");
    msg.order_id = extract_uint64(json, "order_id");
    return msg;
}

} // namespace trading::protocol