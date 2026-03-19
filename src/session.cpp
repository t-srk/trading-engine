#include "session.h"
#include <iostream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace trading {

// ── Constructor ───────────────────────────────────────────────────────────────
Session::Session(tcp::socket socket,
                 MatchingEngine& engine,
                 TradeCallback on_trade)
    : ws_(std::move(socket))
    , engine_(engine)
    , on_trade_(std::move(on_trade))
{}

// ── start ─────────────────────────────────────────────────────────────────────
void Session::start() {
    do_accept();
}

// ── do_accept ─────────────────────────────────────────────────────────────────
// Perform the WebSocket upgrade handshake asynchronously.
// A browser first sends a plain HTTP request asking to "upgrade" to WebSocket.
// Beast handles this negotiation automatically.
void Session::do_accept() {
    auto self = shared_from_this();
    ws_.async_accept([this, self](beast::error_code ec) {
        if (ec) {
            std::cout << "[session] handshake error: " << ec.message() << "\n";
            return;
        }
        do_read();
    });
}

// ── do_read ───────────────────────────────────────────────────────────────────
void Session::do_read() {
    auto self = shared_from_this();
    ws_.async_read(
        read_buf_,
        [this, self](beast::error_code ec, std::size_t /*bytes*/) {
            if (ec) {
                std::cout << "[session] disconnected: " << ec.message() << "\n";
                return;
            }

            std::string msg = beast::buffers_to_string(read_buf_.data());
            read_buf_.consume(read_buf_.size());

            handle_message(msg);
            do_read();
        }
    );
}

// ── handle_message ────────────────────────────────────────────────────────────
void Session::handle_message(const std::string& msg) {
    try {
        auto j      = json::parse(msg);
        auto action = j.at("action").get<std::string>();

        if      (action == "submit") handle_submit(msg);
        else if (action == "cancel") handle_cancel(msg);
        else {
            deliver(json{{"event","error"},{"reason","unknown action"}}.dump() + "\n");
        }
    } catch (const std::exception& e) {
        deliver(json{{"event","error"},{"reason",e.what()}}.dump() + "\n");
    }
}

// ── handle_submit ─────────────────────────────────────────────────────────────
void Session::handle_submit(const std::string& msg) {
    auto j = json::parse(msg);

    std::string user_id    = j.at("user_id").get<std::string>();
    std::string instrument = j.at("instrument").get<std::string>();
    std::string side_str   = j.at("side").get<std::string>();
    double      price      = j.at("price").get<double>();
    uint32_t    quantity   = j.at("quantity").get<uint32_t>();

    Side side = (side_str == "BUY") ? Side::BUY : Side::SELL;

    try {
        auto trades = engine_.submit_order(
            user_id, instrument, side, price, quantity);

        uint64_t order_id = engine_.last_order_id();

        // Ack back to submitting client
        json ack = {
            {"event",      "ack"},
            {"order_id",   order_id},
            {"status",     "OPEN"},
            {"instrument", instrument},
            {"side",       side_str},
            {"price",      price},
            {"quantity",   quantity}
        };
        deliver(ack.dump() + "\n");

        // Notify server of trades for broadcast
        for (const auto& t : trades) {
            on_trade_(t);
        }

    } catch (const std::exception& e) {
        deliver(json{{"event","error"},{"reason",e.what()}}.dump() + "\n");
    }
}

// ── handle_cancel ─────────────────────────────────────────────────────────────
void Session::handle_cancel(const std::string& msg) {
    auto     j        = json::parse(msg);
    uint64_t order_id = j.at("order_id").get<uint64_t>();

    bool success = engine_.cancel_order(order_id);

    json ack = {
        {"event",    "cancel_ack"},
        {"order_id", order_id},
        {"success",  success}
    };
    deliver(ack.dump() + "\n");
}

// ── deliver ───────────────────────────────────────────────────────────────────
void Session::deliver(const std::string& message) {
    bool write_in_progress = !write_queue_.empty();
    write_queue_.push_back(message);
    if (!write_in_progress) {
        do_write();
    }
}

// ── do_write ──────────────────────────────────────────────────────────────────
void Session::do_write() {
    auto self = shared_from_this();
    ws_.async_write(
        net::buffer(write_queue_.front()),
        [this, self](beast::error_code ec, std::size_t /*bytes*/) {
            if (ec) {
                std::cout << "[session] write error: " << ec.message() << "\n";
                return;
            }
            write_queue_.pop_front();
            if (!write_queue_.empty()) {
                do_write();
            }
        }
    );
}

} // namespace trading