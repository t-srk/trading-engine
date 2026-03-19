#include "session.h"
#include <iostream>
#include <boost/asio.hpp>

namespace trading {

// ── Constructor ───────────────────────────────────────────────────────────────
Session::Session(tcp::socket socket,
                 MatchingEngine& engine,
                 TradeCallback on_trade)
    : socket_(std::move(socket))
    , engine_(engine)
    , on_trade_(std::move(on_trade))
{}

// ── start ─────────────────────────────────────────────────────────────────────
void Session::start() {
    do_read();
}

// ── do_read ───────────────────────────────────────────────────────────────────
// Ask Asio to read bytes from the socket until it sees a newline.
// When data arrives, handle_message is called with the complete line.
// Then we immediately queue another read — this keeps the loop alive.
void Session::do_read() {
    // shared_from_this() keeps this Session alive for the duration of the async op
    auto self = shared_from_this();

    boost::asio::async_read_until(
        socket_,
        read_buf_,
        '\n',
        [this, self](boost::system::error_code ec, std::size_t /*bytes*/) {
            if (ec) {
                // Client disconnected or error — just stop reading
                std::cout << "[session] client disconnected: "
                          << ec.message() << "\n";
                return;
            }

            // Extract the line from the buffer
            std::istream stream(&read_buf_);
            std::string line;
            std::getline(stream, line);

            if (!line.empty()) {
                handle_message(line);
            }

            // Queue the next read immediately — keep the loop alive
            do_read();
        }
    );
}

// ── handle_message ────────────────────────────────────────────────────────────
// Dispatch incoming JSON to the right handler based on the "action" field
void Session::handle_message(const std::string& line) {
    try {
        // Peek at the action field to determine message type
        std::string action = protocol::extract_string(line, "action");

        if (action == "submit") {
            handle_submit(line);
        } else if (action == "cancel") {
            handle_cancel(line);
        } else {
            ErrorMsg err;
            err.event  = "error";
            err.reason = "Unknown action: " + action;
            deliver(protocol::serialize(err));
        }
    } catch (const std::exception& e) {
        ErrorMsg err;
        err.event  = "error";
        err.reason = std::string("Parse error: ") + e.what();
        deliver(protocol::serialize(err));
    }
}

// ── handle_submit ─────────────────────────────────────────────────────────────
void Session::handle_submit(const std::string& json) {
    auto msg = protocol::parse_submit(json);

    Side side = (msg.side == "BUY") ? Side::BUY : Side::SELL;

    try {
        auto trades = engine_.submit_order(
            msg.user_id,
            msg.instrument,
            side,
            msg.price,
            msg.quantity
        );

        // Send ack back to the submitting client
        AckMsg ack;
        ack.event      = "ack";
        ack.instrument = msg.instrument;
        ack.side       = msg.side;
        ack.price      = msg.price;
        ack.quantity   = msg.quantity;
        ack.status     = "OPEN";

        // Get the order ID that was just assigned
        // It's the most recently added order — next_order_id_ - 1
        const Order* order = engine_.find_order(engine_.last_order_id());
        if (order) ack.order_id = order->order_id;

        deliver(protocol::serialize(ack));

        // Notify server about each trade so it can broadcast
        for (const auto& t : trades) {
            on_trade_(t);
        }

    } catch (const std::exception& e) {
        ErrorMsg err;
        err.event  = "error";
        err.reason = e.what();
        deliver(protocol::serialize(err));
    }
}

// ── handle_cancel ─────────────────────────────────────────────────────────────
void Session::handle_cancel(const std::string& json) {
    auto msg = protocol::parse_cancel(json);

    bool success = engine_.cancel_order(msg.order_id);

    CancelAckMsg ack;
    ack.event    = "cancel_ack";
    ack.order_id = msg.order_id;
    ack.success  = success;
    deliver(protocol::serialize(ack));
}

// ── deliver ───────────────────────────────────────────────────────────────────
// Queue a message for sending. If the queue was empty, start writing immediately.
void Session::deliver(const std::string& message) {
    bool write_in_progress = !write_queue_.empty();
    write_queue_.push_back(message);
    if (!write_in_progress) {
        do_write();
    }
}

// ── do_write ──────────────────────────────────────────────────────────────────
// Write the front of the queue. When done, if more messages are queued,
// write the next one. This chains writes so they don't interleave.
void Session::do_write() {
    auto self = shared_from_this();

    boost::asio::async_write(
        socket_,
        boost::asio::buffer(write_queue_.front()),
        [this, self](boost::system::error_code ec, std::size_t /*bytes*/) {
            if (ec) {
                std::cout << "[session] write error: " << ec.message() << "\n";
                return;
            }
            write_queue_.pop_front();
            if (!write_queue_.empty()) {
                do_write();   // chain the next write
            }
        }
    );
}

} // namespace trading