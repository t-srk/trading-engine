#include <iostream>
#include <cassert>
#include "matching_engine.h"

using namespace trading;

// ── Helpers ───────────────────────────────────────────────────────────────────
MatchingEngine make_engine() {
    MatchingEngine engine;
    engine.add_instrument("BTC-USD");
    return engine;
}

// ── OrderBook tests (now via MatchingEngine) ──────────────────────────────────

void test_no_match_when_spread_exists() {
    auto engine = make_engine();
    auto t1 = engine.submit_order("alice", "BTC-USD", Side::BUY,  99.0, 10);
    auto t2 = engine.submit_order("bob",   "BTC-USD", Side::SELL, 101.0, 10);

    assert(t1.empty());
    assert(t2.empty());
    assert(engine.get_book("BTC-USD").best_bid().value() == 99.0);
    assert(engine.get_book("BTC-USD").best_ask().value() == 101.0);
    std::cout << "[PASS] test_no_match_when_spread_exists\n";
}

void test_full_match() {
    auto engine = make_engine();
    engine.submit_order("alice", "BTC-USD", Side::BUY,  100.0, 10);
    auto trades = engine.submit_order("bob", "BTC-USD", Side::SELL, 100.0, 10);

    assert(trades.size() == 1);
    assert(trades[0].quantity   == 10);
    assert(trades[0].price      == 100.0);
    assert(trades[0].buyer_id   == "alice");
    assert(trades[0].seller_id  == "bob");
    assert(engine.trade_history().size() == 1);
    std::cout << "[PASS] test_full_match\n";
}

void test_partial_match() {
    auto engine = make_engine();
    engine.submit_order("alice", "BTC-USD", Side::BUY,  100.0, 10);
    auto trades = engine.submit_order("bob", "BTC-USD", Side::SELL, 100.0, 6);

    assert(trades.size() == 1);
    assert(trades[0].quantity == 6);
    assert(engine.get_book("BTC-USD").best_bid().value() == 100.0);
    std::cout << "[PASS] test_partial_match\n";
}

void test_time_priority() {
    auto engine = make_engine();
    engine.submit_order("alice", "BTC-USD", Side::BUY, 100.0, 5);
    engine.submit_order("bob",   "BTC-USD", Side::BUY, 100.0, 5);
    auto trades = engine.submit_order("carol", "BTC-USD", Side::SELL, 100.0, 5);

    assert(trades.size() == 1);
    assert(trades[0].buyer_id == "alice");
    std::cout << "[PASS] test_time_priority\n";
}

void test_price_priority() {
    auto engine = make_engine();
    engine.submit_order("alice", "BTC-USD", Side::BUY, 101.0, 5);
    engine.submit_order("bob",   "BTC-USD", Side::BUY,  99.0, 5);
    auto trades = engine.submit_order("carol", "BTC-USD", Side::SELL, 99.0, 5);

    assert(trades.size() == 1);
    assert(trades[0].buyer_id == "alice");
    assert(trades[0].price    == 101.0);
    std::cout << "[PASS] test_price_priority\n";
}

// ── Cancellation tests ────────────────────────────────────────────────────────

void test_cancel_resting_order() {
    auto engine = make_engine();
    engine.submit_order("alice", "BTC-USD", Side::BUY, 100.0, 10);

    // Order 1 is alice's buy — cancel it
    bool cancelled = engine.cancel_order(1);
    assert(cancelled == true);

    // Book should now be empty — bob's sell should not match anything
    auto trades = engine.submit_order("bob", "BTC-USD", Side::SELL, 100.0, 10);
    assert(trades.empty());
    std::cout << "[PASS] test_cancel_resting_order\n";
}

void test_cancel_already_filled_order_fails() {
    auto engine = make_engine();
    engine.submit_order("alice", "BTC-USD", Side::BUY,  100.0, 10);
    engine.submit_order("bob",   "BTC-USD", Side::SELL, 100.0, 10);

    // Order 1 (alice's) is fully filled — should not be cancellable
    bool cancelled = engine.cancel_order(1);
    assert(cancelled == false);
    std::cout << "[PASS] test_cancel_already_filled_order_fails\n";
}

void test_cancel_nonexistent_order_fails() {
    auto engine = make_engine();
    bool cancelled = engine.cancel_order(999);
    assert(cancelled == false);
    std::cout << "[PASS] test_cancel_nonexistent_order_fails\n";
}

void test_unknown_instrument_throws() {
    auto engine = make_engine();
    bool threw = false;
    try {
        engine.submit_order("alice", "ETH-USD", Side::BUY, 100.0, 10);
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    assert(threw == true);
    std::cout << "[PASS] test_unknown_instrument_throws\n";
}

// ── Runner ────────────────────────────────────────────────────────────────────
int main() {
    std::cout << "Running tests...\n\n";

    test_no_match_when_spread_exists();
    test_full_match();
    test_partial_match();
    test_time_priority();
    test_price_priority();

    std::cout << "\n";

    test_cancel_resting_order();
    test_cancel_already_filled_order_fails();
    test_cancel_nonexistent_order_fails();
    test_unknown_instrument_throws();

    std::cout << "\nAll tests passed.\n";
    return 0;
}