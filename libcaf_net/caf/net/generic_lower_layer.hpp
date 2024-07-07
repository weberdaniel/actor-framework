// This file is part of CAF, the C++ Actor Framework. See the file LICENSE in
// the main distribution directory for license terms and copyright or visit
// https://github.com/actor-framework/actor-framework/blob/main/LICENSE.

#pragma once

#include "caf/net/fwd.hpp"

#include "caf/detail/net_export.hpp"
#include "caf/fwd.hpp"

namespace caf::net {

/// Bundles protocol-independent, generic member functions for (almost all)
/// lower layers.
class CAF_NET_EXPORT generic_lower_layer {
public:
  virtual ~generic_lower_layer();

  /// Returns the @ref multiplexer instance that executes this protocol stack.
  [[nodiscard]] multiplexer& mpx() noexcept;

  /// Returns the manager that owns this layer.
  [[nodiscard]] virtual socket_manager* manager() noexcept = 0;

  /// Queries whether the output device can accept more data straight away.
  [[nodiscard]] virtual bool can_send_more() const noexcept = 0;

  /// Queries whether the lower layer is currently reading from its input
  /// device.
  [[nodiscard]] virtual bool is_reading() const noexcept = 0;

  /// Triggers a write callback after the write device signals downstream
  /// capacity. Does nothing if this layer is already writing.
  virtual void write_later() = 0;

  /// Shuts down any connection or session gracefully. Any pending data gets
  /// flushed before closing the socket.
  virtual void shutdown() = 0;

  /// Shuts down any connection or session due to an error. Any pending data
  /// gets flushed before closing the socket. Protocols with a dedicated closing
  /// handshake such as WebSocket may send the close reason to the peer.
  virtual void shutdown(const error& reason);
};

} // namespace caf::net
