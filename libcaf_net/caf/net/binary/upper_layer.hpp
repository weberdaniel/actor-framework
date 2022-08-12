// This file is part of CAF, the C++ Actor Framework. See the file LICENSE in
// the main distribution directory for license terms and copyright or visit
// https://github.com/actor-framework/actor-framework/blob/master/LICENSE.

#pragma once

#include "caf/detail/net_export.hpp"
#include "caf/fwd.hpp"
#include "caf/net/binary/fwd.hpp"
#include "caf/net/fwd.hpp"
#include "caf/net/generic_upper_layer.hpp"

namespace caf::net::binary {

/// Consumes binary messages from the lower layer.
class CAF_NET_EXPORT upper_layer : public generic_upper_layer {
public:
  virtual ~upper_layer();

  /// Initializes the upper layer.
  /// @param down A pointer to the lower layer that remains valid for the
  ///             lifetime of the upper layer.
  /// @param config Protocol-dependent configuration parameters.
  virtual error start(lower_layer* down, const settings& config) = 0;

  /// Consumes bytes from the lower layer.
  /// @param payload Payload of the received message.
  /// @returns The number of consumed bytes or a negative value to signal an
  ///          error.
  /// @note Discarded data is lost permanently.
  [[nodiscard]] virtual ptrdiff_t consume(byte_span payload) = 0;
};

} // namespace caf::net::binary
