#include "matching_engine.h"
#include <unordered_set>

namespace trading {

// ── add_instrument ────────────────────────────────────────────────────────────
void MatchingEngine::add_instrument(const std::string& instrument) {
    // emplace does nothing if key already exists — safe to call twice
    books_.emplace(instrument, OrderBook());
}

// ── submit_order ──────────────────────────────────────────────────────────────
std::vector<Trade> MatchingEngine::submit_order(
    const std::string& user_id,
    const std::string& instrument,
    Side               side,
    double             price,
    uint32_t           quantity,
    OrderType          type)
{
    if (books_.find(instrument) == books_.end())
        throw std::invalid_argument("Unknown instrument: " + instrument);
    if (quantity == 0)
        throw std::invalid_argument("Order quantity must be > 0");
    if (price <= 0.0 && type == OrderType::LIMIT)
        throw std::invalid_argument("Limit order price must be > 0");

    Order order;
    order.order_id   = next_order_id_++;
    order.timestamp  = now_ns();
    order.user_id    = user_id;
    order.instrument = instrument;
    order.side       = side;
    order.type       = type;
    order.price      = price;
    order.quantity   = quantity;
    order.filled_qty = 0;
    order.status     = OrderStatus::OPEN;

    uint64_t id  = order.order_id;
    orders_[id]  = order;

    auto& book   = books_.at(instrument);
    auto  trades = book.add_order(order);

    // Sync incoming order state back
    orders_[id].filled_qty = order.filled_qty;
    orders_[id].status     = order.status;

    // Sync resting orders that got filled, record trades, update portfolios
    for (const auto& t : trades) {
        trade_history_.push_back(t);

        uint64_t resting_id = (order.side == Side::BUY)
            ? t.sell_order_id
            : t.buy_order_id;

        auto it = orders_.find(resting_id);
        if (it != orders_.end()) {
            it->second.filled_qty += t.quantity;
            it->second.status =
                (it->second.remaining_qty() == 0)
                    ? OrderStatus::FILLED
                    : OrderStatus::PARTIALLY_FILLED;
        }

        // Update both sides' portfolios
        portfolios_[t.buyer_id].apply_trade(t, t.buyer_id);
        portfolios_[t.seller_id].apply_trade(t, t.seller_id);
    }

    return trades;
}

// ── cancel_all_orders ─────────────────────────────────────────────────────────
std::vector<std::string> MatchingEngine::cancel_all_orders() {
    std::unordered_set<std::string> affected;
    for (auto& [order_id, order] : orders_) {
        if (order.is_complete()) continue;
        order.status = OrderStatus::CANCELLED;
        books_.at(order.instrument).cancel_order(order_id, order.side, order.price);
        affected.insert(order.instrument);
    }
    return std::vector<std::string>(affected.begin(), affected.end());
}

// ── cancel_order ──────────────────────────────────────────────────────────────
bool MatchingEngine::cancel_order(uint64_t order_id) {
    auto it = orders_.find(order_id);

    // Order doesn't exist
    if (it == orders_.end()) return false;

    Order& order = it->second;

    // Already done — can't cancel a filled or already-cancelled order
    if (order.is_complete()) return false;

    // Mark it cancelled in our records
    order.status = OrderStatus::CANCELLED;

    // Tell the book to remove it
    // (We'll implement OrderBook::cancel_order next)
    books_.at(order.instrument).cancel_order(order_id, order.side, order.price);

    return true;
}

} // namespace trading