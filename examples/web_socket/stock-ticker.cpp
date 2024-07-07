// Pseudo "Stock Ticker" that publishes random updates once per second via
// WebSocket feed.

#include "caf/net/acceptor_resource.hpp"
#include "caf/net/middleman.hpp"
#include "caf/net/web_socket/frame.hpp"
#include "caf/net/web_socket/with.hpp"

#include "caf/actor_from_state.hpp"
#include "caf/actor_system.hpp"
#include "caf/actor_system_config.hpp"
#include "caf/caf_main.hpp"
#include "caf/cow_string.hpp"
#include "caf/cow_tuple.hpp"
#include "caf/event_based_actor.hpp"
#include "caf/json_writer.hpp"
#include "caf/scheduled_actor/flow.hpp"
#include "caf/span.hpp"

#include <cassert>
#include <chrono>
#include <cstdint>
#include <random>
#include <utility>

using namespace std::literals;

// -- constants ----------------------------------------------------------------

// Configures the port for the server to listen on.
static constexpr uint16_t default_port = 8080;

// Configures the maximum number of concurrent connections.
static constexpr size_t default_max_connections = 128;

// Configures the maximum number of buffered messages per connection.
static constexpr size_t max_outstanding_messages = 10;

// Configures the update interval for the stock ticker.
static constexpr caf::timespan default_interval = 1s;

// -- custom types -------------------------------------------------------------

namespace stock {

struct info {
  std::string symbol;
  std::string currency;
  double current;
  double open;
  double high;
  double low;
};

template <class Inspector>
bool inspect(Inspector& f, info& x) {
  return f.object(x).fields(f.field("symbol", x.symbol),
                            f.field("currency", x.currency),
                            f.field("open", x.open), f.field("high", x.high),
                            f.field("low", x.low));
}

} // namespace stock

// -- actor for generating a random feed ---------------------------------------

struct random_feed_state {
  using frame = caf::net::web_socket::frame;

  random_feed_state(caf::event_based_actor* selfptr,
                    caf::net::acceptor_resource<frame> events,
                    caf::timespan update_interval)
    : self(selfptr), val_dist(0, 100000), index_dist(0, 19) {
    // Init random number generator.
    std::random_device rd;
    rng.seed(rd());
    // Fill vector with some stuff.
    for (size_t i = 0; i < 20; ++i) {
      std::uniform_int_distribution<int> char_dist{'A', 'Z'};
      std::string symbol;
      for (size_t j = 0; j < 5; ++j)
        symbol += static_cast<char>(char_dist(rng));
      auto val = next_value();
      infos.emplace_back(
        stock::info{std::move(symbol), "USD", val, val, val, val});
    }
    // Create the feed to push updates once per second.
    writer.skip_object_type_annotation(true);
    feed = self->make_observable()
             .interval(update_interval)
             .map([this](int64_t) {
               writer.reset();
               auto& x = update();
               if (!writer.apply(x)) {
                 self->println("*** failed to generate JSON: {}",
                               writer.get_error());
                 return frame{};
               }
               return frame{writer.str()};
             })
             .filter([](const frame& x) {
               // Just in case: drop frames that failed to generate JSON.
               return x.is_text();
             })
             .share();
    // Subscribe once to start the feed immediately and to keep it running.
    feed.for_each([this, n = 1](const frame&) mutable {
      self->println("*** tick {}", n++);
    });
    // Add each incoming WebSocket listener to the feed.
    auto n = std::make_shared<int>(0);
    events
      .observe_on(self) //
      .for_each([this, n](const auto& ev) {
        self->println("*** added listener (n = {})", ++*n);
        auto [pull, push] = ev.data();
        pull.observe_on(self)
          .do_finally([this, n] { //
            self->println("*** removed listener (n = {})", --*n);
          })
          .subscribe(std::ignore);
        // Forward the quotes to the client and disconnect if the client is too
        // slow to keep up with the feed.
        feed.on_backpressure_buffer(max_outstanding_messages).subscribe(push);
      });
  }

  // Picks a random stock, assigns a new value to it, and returns it.
  stock::info& update() {
    auto& x = infos[index_dist(rng)];
    auto val = next_value();
    x.current = val;
    x.high = std::max(x.high, val);
    x.low = std::min(x.high, val);
    return x;
  }

  double next_value() {
    return val_dist(rng) / 100.0;
  }

  caf::behavior make_behavior() {
    // Returning a default-constructed behavior will terminate the actor once
    // the flows are done.
    return {};
  }

  caf::event_based_actor* self;
  caf::flow::observable<frame> feed;
  caf::json_writer writer;
  std::vector<stock::info> infos;
  std::minstd_rand rng;
  std::uniform_int_distribution<int> val_dist;
  std::uniform_int_distribution<size_t> index_dist;
};

// -- configuration setup ------------------------------------------------------

struct config : caf::actor_system_config {
  config() {
    opt_group{custom_options_, "global"} //
      .add<uint16_t>("port,p", "port to listen for incoming connections")
      .add<size_t>("max-connections,m", "limit for concurrent clients")
      .add<caf::timespan>("interval,i", "update interval");
    ;
    opt_group{custom_options_, "tls"} //
      .add<std::string>("key-file,k", "path to the private key file")
      .add<std::string>("cert-file,c", "path to the certificate file");
  }

  caf::settings dump_content() const override {
    auto result = actor_system_config::dump_content();
    caf::put_missing(result, "port", default_port);
    caf::put_missing(result, "max-connections", default_max_connections);
    caf::put_missing(result, "interval", default_interval);
    return result;
  }
};

// -- main ---------------------------------------------------------------------

int caf_main(caf::actor_system& sys, const config& cfg) {
  namespace http = caf::net::http;
  namespace ssl = caf::net::ssl;
  namespace ws = caf::net::web_socket;
  // Read the configuration.
  auto interval = caf::get_or(cfg, "interval", default_interval);
  auto port = caf::get_or(cfg, "port", default_port);
  auto pem = ssl::format::pem;
  auto key_file = caf::get_as<std::string>(cfg, "tls.key-file");
  auto cert_file = caf::get_as<std::string>(cfg, "tls.cert-file");
  auto max_connections = caf::get_or(cfg, "max-connections",
                                     default_max_connections);
  if (!key_file != !cert_file) {
    sys.println("*** inconsistent TLS config: declare neither file or both");
    return EXIT_FAILURE;
  }
  // Open up a TCP port for incoming connections and start the server.
  auto server
    = ws::with(sys)
        // Optionally enable TLS.
        .context(ssl::context::enable(key_file && cert_file)
                   .and_then(ssl::emplace_server(ssl::tls::v1_2))
                   .and_then(ssl::use_private_key_file(key_file, pem))
                   .and_then(ssl::use_certificate_file(cert_file, pem)))
        // Bind to the user-defined port.
        .accept(port)
        // Limit how many clients may be connected at any given time.
        .max_connections(max_connections)
        // Add handler for incoming connections.
        .on_request([](ws::acceptor<>& acc) {
          // Ignore all header fields and accept the connection.
          acc.accept();
        })
        // When started, run our worker actor to handle incoming connections.
        .start([&sys, interval](auto events) {
          sys.spawn(caf::actor_from_state<random_feed_state>, std::move(events),
                    interval);
        });
  // Report any error to the user.
  if (!server) {
    sys.println("*** unable to run at port {}: {}", port, server.error());
    return EXIT_FAILURE;
  }
  // Note: the actor system will keep the application running for as long as the
  // workers are still alive.
  return EXIT_SUCCESS;
}

CAF_MAIN(caf::net::middleman)
