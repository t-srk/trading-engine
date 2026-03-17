#include <iostream>
#include <cassert>
#include "order.h"

// Minimal hand-rolled test runner — no framework needed yet
// We'll add Google Test in Phase 2 when tests get complex

void test_order_remaining_quantity() {
    trading::Order o;
    o.quantity   = 100;
    o.filled_qty = 40;
    
    assert(o.remaining_qty() == 60);
    std::cout << "[PASS] test_order_remaining_quantity\n";
}

void test_order_is_complete_when_filled() {
    trading::Order o;
    o.quantity   = 100;
    o.filled_qty = 100;
    o.status     = trading::OrderStatus::FILLED;

    assert(o.is_complete() == true);
    std::cout << "[PASS] test_order_is_complete_when_filled\n";
}

void test_order_is_not_complete_when_open() {
    trading::Order o;
    o.quantity   = 100;
    o.filled_qty = 0;
    o.status     = trading::OrderStatus::OPEN;

    assert(o.is_complete() == false);
    std::cout << "[PASS] test_order_is_not_complete_when_open\n";
}

int main() {
    std::cout << "── Running tests ──\n";
    test_order_remaining_quantity();
    test_order_is_complete_when_filled();
    test_order_is_not_complete_when_open();
    std::cout << "── All tests passed ──\n";
    return 0;
}