#pragma once

#include <queue>
#include <cstdint>
#include "order.h"

namespace trading {

// A PriceLevel represents all resting orders at one specific price.
// Orders within a level are matched in FIFO order — first in, first out.
// This is the "time priority" part of price-time priority matching.
//
// Example: if three buyers all want to buy at $100.00, the one who
// submitted their order first gets filled first.

class PriceLevel {
public:
    explicit PriceLevel(double price) : price_(price), total_quantity_(0) {}

    // ── Getters ───────────────────────────────────────────────────────────────
    double   price()          const { return price_; }
    uint32_t total_quantity() const { return total_quantity_; }
    bool     empty()          const { return orders_.empty(); }

    // ── Mutators ──────────────────────────────────────────────────────────────

    // Add an order to the back of the queue (time priority)
    void add_order(const Order& order) {
        total_quantity_ += order.remaining_qty();
        orders_.push(order);
    }

    // Peek at the front order (next to be matched) without removing it
    const Order& front() const {
        return orders_.front();
    }

    Order& front() {
        return orders_.front();
    }

    // Remove the front order (called after it's fully filled)
    void pop_front() {
        total_quantity_ -= orders_.front().remaining_qty();
        orders_.pop();
    }

    // Reduce the total quantity tracked at this level (called on partial fill)
    void reduce_quantity(uint32_t amount) {
        total_quantity_ -= amount;
    }

private:
    double              price_;
    uint32_t            total_quantity_;   // sum of all remaining quantities
    std::queue<Order>   orders_;           // FIFO queue
};

} // namespace trading