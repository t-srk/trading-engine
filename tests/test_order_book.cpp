#include <iostream>
#include <cassert>
#include "order_book.h"

using namespace trading;

// ── Helpers ───────────────────────────────────────────────────────────────────
// Makes creating test orders less verbose
Order make_order(uint64_t id, const std::string& user,
                 Side side, double price, uint32_t qty)
{
    Order o;
    o.order_id   = id;
    o.timestamp  = id;       // use id as timestamp so lower id = earlier
    o.user_id    = user;
    o.instrument = "BTC-USD";
    o.price      = price;
    o.quantity   = qty;
    o.filled_qty = 0;
    o.side       = side;
    o.type       = OrderType::LIMIT;
    o.status     = OrderStatus::OPEN;
    return o;
}

// ── Tests ─────────────────────────────────────────────────────────────────────

void test_no_match_when_spread_exists() {
    // Bid at 99, ask at 101 — no match should occur
    OrderBook book;
    auto trades1 = book.add_order(make_order(1, "alice", Side::BUY,  99.0, 10));
    auto trades2 = book.add_order(make_order(2, "bob",   Side::SELL, 101.0, 10));

    assert(trades1.empty());
    assert(trades2.empty());
    assert(book.best_bid().value() == 99.0);
    assert(book.best_ask().value() == 101.0);
    std::cout << "[PASS] test_no_match_when_spread_exists\n";
}

void test_full_match_at_same_price() {
    // Buy and sell at same price, same quantity — should fully match
    OrderBook book;
    book.add_order(make_order(1, "alice", Side::BUY,  100.0, 10));
    auto trades = book.add_order(make_order(2, "bob", Side::SELL, 100.0, 10));

    assert(trades.size() == 1);
    assert(trades[0].quantity == 10);
    assert(trades[0].price    == 100.0);
    assert(trades[0].buyer_id  == "alice");
    assert(trades[0].seller_id == "bob");
    assert(book.bids_empty());
    assert(book.asks_empty());
    std::cout << "[PASS] test_full_match_at_same_price\n";
}

void test_partial_match() {
    // Alice wants 10, Bob only sells 6 — partial fill
    OrderBook book;
    book.add_order(make_order(1, "alice", Side::BUY,  100.0, 10));
    auto trades = book.add_order(make_order(2, "bob", Side::SELL, 100.0, 6));

    assert(trades.size() == 1);
    assert(trades[0].quantity == 6);
    // Alice's order should still be in the book with 4 remaining
    assert(book.best_bid().value() == 100.0);
    assert(book.asks_empty());
    std::cout << "[PASS] test_partial_match\n";
}

void test_time_priority() {
    // Two buyers at same price — first in should get filled first
    OrderBook book;
    book.add_order(make_order(1, "alice", Side::BUY, 100.0, 5));  // first
    book.add_order(make_order(2, "bob",   Side::BUY, 100.0, 5));  // second

    // Seller can only fill 5 — should go to alice (first in)
    auto trades = book.add_order(make_order(3, "carol", Side::SELL, 100.0, 5));

    assert(trades.size() == 1);
    assert(trades[0].buyer_id == "alice");   // alice gets priority
    assert(trades[0].quantity == 5);
    // Bob's order should still be resting
    assert(book.best_bid().value() == 100.0);
    std::cout << "[PASS] test_time_priority\n";
}

void test_price_priority() {
    // Two buyers at different prices — higher price gets filled first
    OrderBook book;
    book.add_order(make_order(1, "alice", Side::BUY, 101.0, 5));  // better price
    book.add_order(make_order(2, "bob",   Side::BUY,  99.0, 5));  // worse price

    auto trades = book.add_order(make_order(3, "carol", Side::SELL, 99.0, 5));

    assert(trades.size() == 1);
    assert(trades[0].buyer_id == "alice");   // higher bidder gets priority
    assert(trades[0].price    == 101.0);
    std::cout << "[PASS] test_price_priority\n";
}

// ── Runner ────────────────────────────────────────────────────────────────────
int main() {
    std::cout << "── Running OrderBook tests ──\n";
    test_no_match_when_spread_exists();
    test_full_match_at_same_price();
    test_partial_match();
    test_time_priority();
    test_price_priority();
    std::cout << "── All tests passed ──\n";
    return 0;
}