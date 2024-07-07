// This file is part of CAF, the C++ Actor Framework. See the file LICENSE in
// the main distribution directory for license terms and copyright or visit
// https://github.com/actor-framework/actor-framework/blob/main/LICENSE.

#pragma once

#include "caf/net/fwd.hpp"
#include "caf/net/octet_stream/upper_layer.hpp"
#include "caf/net/web_socket/lower_layer.hpp"
#include "caf/net/web_socket/upper_layer.hpp"

#include <memory>

namespace caf::net::web_socket {

/// Implements the WebSocket framing protocol as defined in RFC-6455.
class CAF_NET_EXPORT framing : public octet_stream::upper_layer,
                               public web_socket::lower_layer {
public:
  // -- member types -----------------------------------------------------------

  using upper_layer_ptr = std::unique_ptr<web_socket::upper_layer>;

  // -- constructors, destructors, and assignment operators --------------------

  ~framing() override;

  // -- factories --------------------------------------------------------------

  /// Creates a new framing protocol for client mode.
  static std::unique_ptr<framing> make_client(upper_layer_ptr up);

  /// Creates a new framing protocol for server mode.
  static std::unique_ptr<framing> make_server(upper_layer_ptr up);
};

} // namespace caf::net::web_socket
