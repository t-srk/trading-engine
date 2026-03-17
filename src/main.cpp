#include <iostream>
#include "order.h"

int main() {
    // Just a sanity check for now — we'll replace this in the next phase
    trading::Order o;
    o.order_id   = 1;
    o.user_id    = "alice";
    o.instrument = "BTC-USD";
    o.price      = 50000.0;
    o.quantity   = 10;
    o.filled_qty = 0;
    o.side       = trading::Side::BUY;
    o.type       = trading::OrderType::LIMIT;
    o.status     = trading::OrderStatus::OPEN;

    std::cout << "Order created: ID=" << o.order_id
              << " User=" << o.user_id
              << " Remaining=" << o.remaining_qty()
              << std::endl;

    return 0;
}