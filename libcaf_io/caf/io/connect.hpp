// This file is part of CAF, the C++ Actor Framework. See the file LICENSE in
// the main distribution directory for license terms and copyright or visit
// https://github.com/actor-framework/actor-framework/blob/main/LICENSE.

#pragma once

#include "caf/io/middleman.hpp"

#include "caf/actor_system.hpp"
#include "caf/node_id.hpp"

#include <cstdint>
#include <string>

namespace caf::io {

/// Tries to connect to given node.
/// @experimental
inline expected<node_id> connect(actor_system& sys, std::string host,
                                 uint16_t port) {
  return sys.middleman().connect(std::move(host), port);
}

} // namespace caf::io
