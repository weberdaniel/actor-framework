// This file is part of CAF, the C++ Actor Framework. See the file LICENSE in
// the main distribution directory for license terms and copyright or visit
// https://github.com/actor-framework/actor-framework/blob/main/LICENSE.

#pragma once

#include "caf/net/dsl/base.hpp"
#include "caf/net/dsl/client_config.hpp"
#include "caf/net/dsl/has_connect.hpp"
#include "caf/net/fwd.hpp"
#include "caf/net/tcp_stream_socket.hpp"

#include "caf/make_counted.hpp"
#include "caf/uri.hpp"

#include <cstdint>
#include <string>

namespace caf::net::dsl {

/// DSL entry point for creating a client from an URI.
template <class Base, class Subtype>
class has_uri_connect : public Base {
public:
  /// Creates a `connect_factory` object for the given TCP `endpoint`.
  ///
  /// @param endpoint The endpoint of the TCP server to connect to.
  /// @returns a `connect_factory` object initialized with the given parameters.
  auto connect(const uri& endpoint) {
    auto& dref = static_cast<Subtype&>(*this);
    return dref.make(client_config::lazy_v, endpoint);
  }

  /// Creates a `connect_factory` object for the given TCP `endpoint`.
  ///
  /// @param endpoint The endpoint of the TCP server to connect to.
  /// @returns a `connect_factory` object initialized with the given parameters.
  auto connect(expected<uri> endpoint) {
    if (endpoint)
      return connect(*endpoint);
    auto& dref = static_cast<Subtype&>(*this);
    return dref.make(client_config::fail_v, std::move(endpoint.error()));
  }
};

} // namespace caf::net::dsl
