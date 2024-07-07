// Simple chat server with a binary protocol.

#include "caf/net/acceptor_resource.hpp"
#include "caf/net/lp/with.hpp"
#include "caf/net/middleman.hpp"

#include "caf/actor_system.hpp"
#include "caf/actor_system_config.hpp"
#include "caf/caf_main.hpp"
#include "caf/chunk.hpp"
#include "caf/event_based_actor.hpp"
#include "caf/scheduled_actor/flow.hpp"
#include "caf/uuid.hpp"

#include <utility>

namespace lp = caf::net::lp;

// -- constants ----------------------------------------------------------------

// Configures the port for the server to listen on.
static constexpr uint16_t default_port = 7788;

// Configures the maximum number of concurrent connections.
static constexpr size_t default_max_connections = 128;

// Configures the maximum number of buffered messages per connection.
static constexpr size_t max_outstanding_messages = 10;

// -- configuration setup ------------------------------------------------------

struct config : caf::actor_system_config {
  config() {
    opt_group{custom_options_, "global"} //
      .add<uint16_t>("port,p", "port to listen for incoming connections")
      .add<size_t>("max-connections,m", "limit for concurrent clients");
    opt_group{custom_options_, "tls"} //
      .add<std::string>("key-file,k", "path to the private key file")
      .add<std::string>("cert-file,c", "path to the certificate file");
  }

  caf::settings dump_content() const override {
    auto result = actor_system_config::dump_content();
    caf::put_missing(result, "port", default_port);
    caf::put_missing(result, "max-connections", default_max_connections);
    return result;
  }
};

// -- multiplexing logic -------------------------------------------------------

void worker_impl(caf::event_based_actor* self,
                 caf::net::acceptor_resource<lp::frame> events) {
  // Each client gets a UUID for identifying it. While processing messages, we
  // add this ID to the input to tag it.
  using message_t = std::pair<caf::uuid, lp::frame>;
  // Allows us to push new flows into the central merge point.
  caf::flow::multicaster<caf::flow::observable<message_t>> pub{self};
  // Our central merge point combines all inputs into a single, shared flow.
  auto messages = pub.as_observable().merge().share();
  // Have one subscription for debug output. This also makes sure that the
  // shared observable stays subscribed to the merger.
  messages.for_each([self](const message_t& msg) {
    const auto& [conn, frame] = msg;
    self->println("*** got message of size {} from {}", frame.size(), conn);
  });
  // Connect the flows for each incoming connection.
  events
    .observe_on(self) //
    .for_each(
      [self, messages, pub = std::move(pub)](const auto& event) mutable {
        // Each connection gets a unique ID.
        auto conn = caf::uuid::random();
        self->println("*** accepted new connection {}", conn);
        auto& [pull, push] = event.data();
        // Subscribe the `push` end to the central merge point.
        messages
          .filter([conn](const message_t& msg) {
            // Drop all messages received by this conn.
            return msg.first != conn;
          })
          .map([](const message_t& msg) {
            // Remove the server-internal UUID.
            return msg.second;
          })
          // Disconnect if the client is too slow.
          .on_backpressure_buffer(max_outstanding_messages)
          .subscribe(push);
        // Feed messages from the `pull` end into the central merge point.
        auto inputs //
          = pull.observe_on(self)
              .do_on_error([self](const caf::error& err) {
                self->println("*** connection error: {}", err);
              })
              .on_error_complete() // Cary on if a connection breaks.
              .do_on_complete([self, conn] {
                // But still log when a connection is lost.
                self->println("*** lost connection {}", conn);
              })
              .map([conn](const lp::frame& frame) {
                return message_t{conn, frame};
              })
              .as_observable();
        pub.push(inputs);
      });
}

// -- main ---------------------------------------------------------------------

int caf_main(caf::actor_system& sys, const config& cfg) {
  namespace ssl = caf::net::ssl;
  // Read the configuration.
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
    = caf::net::lp::with(sys)
        // Optionally enable TLS.
        .context(ssl::context::enable(key_file && cert_file)
                   .and_then(ssl::emplace_server(ssl::tls::v1_2))
                   .and_then(ssl::use_private_key_file(key_file, pem))
                   .and_then(ssl::use_certificate_file(cert_file, pem)))
        // Bind to the user-defined port.
        .accept(port)
        // Limit how many clients may be connected at any given time.
        .max_connections(max_connections)
        // When started, run our worker actor to handle incoming connections.
        .start([&sys](auto accept_events) {
          sys.spawn(worker_impl, std::move(accept_events));
        });
  sys.println("*** server started");
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
