#pragma once

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <memory>
#include <string>
#include <deque>
#include <functional>
#include "matching_engine.h"
#include "protocol.h"

// Forward-declare Server to avoid a circular include.
// server.h includes session.h, so session.h cannot include server.h.
// The full definition is pulled in by session.cpp which includes server.h directly.
namespace trading { class Server; }

namespace beast     = boost::beast;
namespace websocket = beast::websocket;
namespace net       = boost::asio;
using     tcp       = net::ip::tcp;

namespace trading {

class Session : public std::enable_shared_from_this<Session> {
public:
    using TradeCallback = std::function<void(const Trade&)>;

    Session(tcp::socket socket,
            net::io_context& io_context,
            MatchingEngine& engine,
            Server& server,
            TradeCallback on_trade);

    // Perform the WebSocket handshake then start reading
    void start();

    // Queue a message for sending to this client (thread-safe: posts to strand)
    void deliver(const std::string& message);

    // Close the underlying TCP socket — called by Server when evicting a session
    void force_close();

    bool socket_alive() const {
        return ws_.is_open();
    }

private:
    void do_accept();      // WebSocket handshake
    void do_read();        // async read loop
    void do_write();       // async write loop

    void handle_message(const std::string& msg);
    void handle_login(const std::string& json);
    void handle_submit(const std::string& json);
    void handle_cancel(const std::string& json);

    // strand_ MUST be declared before ws_ — C++ initializes members in declaration
    // order, and strand_ must be constructed from the socket's executor before the
    // socket is moved into ws_.
    net::strand<net::io_context::executor_type> strand_;
    websocket::stream<tcp::socket>              ws_;

    MatchingEngine&                engine_;
    Server&                        server_;
    TradeCallback                  on_trade_;
    beast::flat_buffer             read_buf_;
    std::deque<std::string>        write_queue_;

    bool        authenticated_ = false;
    std::string token_;
    std::string user_id_;
};

} // namespace trading
