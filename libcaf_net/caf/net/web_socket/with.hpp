// This file is part of CAF, the C++ Actor Framework. See the file LICENSE in
// the main distribution directory for license terms and copyright or visit
// https://github.com/actor-framework/actor-framework/blob/main/LICENSE.

#pragma once

#include "caf/net/dsl/generic_config.hpp"
#include "caf/net/dsl/has_accept.hpp"
#include "caf/net/dsl/has_context.hpp"
#include "caf/net/dsl/has_uri_connect.hpp"
#include "caf/net/multiplexer.hpp"
#include "caf/net/ssl/context.hpp"
#include "caf/net/tcp_accept_socket.hpp"
#include "caf/net/web_socket/client_factory.hpp"
#include "caf/net/web_socket/has_on_request.hpp"

#include "caf/extend.hpp"
#include "caf/fwd.hpp"

#include <cstdint>

namespace caf::net::web_socket {

/// Entry point for the `with(...)` DSL.
class with_t : public extend<dsl::base, with_t>:: //
               with<dsl::has_accept, dsl::has_connect, dsl::has_uri_connect,
                    dsl::has_context> {
public:
  using config_type = dsl::generic_config_value;

  template <class... Ts>
  explicit with_t(multiplexer* mpx) : config_(make_counted<config_type>(mpx)) {
    // nop
  }

  with_t(const with_t&) noexcept = default;

  with_t& operator=(const with_t&) noexcept = default;

  /// @private
  config_type& config() {
    return *config_;
  }

  /// @private
  template <class T, class... Ts>
  auto make(dsl::server_config_tag<T> token, Ts&&... xs) {
    return has_on_request{token, *config_, std::forward<Ts>(xs)...};
  }

  /// @private
  template <class T, class... Ts>
  auto make(dsl::client_config_tag<T> token, Ts&&... xs) {
    return client_factory{token, *config_, std::forward<Ts>(xs)...};
  }

private:
  intrusive_ptr<config_type> config_;
};

with_t with(actor_system& sys) {
  return with_t{multiplexer::from(sys)};
}

with_t with(multiplexer* mpx) {
  return with_t{mpx};
}

} // namespace caf::net::web_socket
