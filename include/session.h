#pragma once

#include <boost/asio.hpp>
#include <memory>
#include <string>
#include <deque>
#include <functional>
#include "matching_engine.h"
#include "protocol.h"

namespace trading {

using boost::asio::ip::tcp;

// Forward declare Server so Session can call back into it
class Server;

// ── Session ───────────────────────────────────────────────────────────────────
// One Session per connected client.
// Responsibilities:
//   - Read newline-delimited messages from the client
//   - Parse and dispatch to the MatchingEngine
//   - Write responses back to this client
//   - Tell the Server about trades (so it can broadcast to all clients)
//
// Sessions are reference-counted via shared_ptr — the Session stays alive
// as long as async operations are pending, even if the Server drops its reference.

class Session : public std::enable_shared_from_this<Session> {
public:
   using TradeCallback = std::function<void(const Trade&)>;
   
   bool socket_alive() const {
      return socket_.is_open();
   }

   Session(tcp::socket socket,
         MatchingEngine& engine,
         TradeCallback on_trade);

   // Start reading from this client
   void start();

   // Write a message to this client (queued, non-blocking)
   void deliver(const std::string& message);

private:
    // ── Async read loop ───────────────────────────────────────────────────────
    void do_read();
    void handle_message(const std::string& line);

    // ── Async write loop ──────────────────────────────────────────────────────
    void do_write();

    // ── Message handlers ──────────────────────────────────────────────────────
    void handle_submit(const std::string& json);
    void handle_cancel(const std::string& json);

    // ── Members ───────────────────────────────────────────────────────────────
    tcp::socket      socket_;
    MatchingEngine&  engine_;
    TradeCallback    on_trade_;

    // Boost.Asio reads into this buffer until it sees a newline
    boost::asio::streambuf read_buf_;

    // Outbound message queue — messages waiting to be written
    std::deque<std::string> write_queue_;
};

} // namespace trading