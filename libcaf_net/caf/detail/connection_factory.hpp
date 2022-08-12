// This file is part of CAF, the C++ Actor Framework. See the file LICENSE in
// the main distribution directory for license terms and copyright or visit
// https://github.com/actor-framework/actor-framework/blob/master/LICENSE.

#pragma once

#include "caf/fwd.hpp"
#include "caf/net/fwd.hpp"

#include <memory>

namespace caf::detail {

/// Creates new socket managers for an @ref accept_handler.
template <class ConnectionHandle>
class connection_factory {
public:
  using connection_handle_type = ConnectionHandle;

  virtual ~connection_factory() {
    // nop
  }

  virtual error start(net::socket_manager*, const settings&) {
    return none;
  }

  virtual void abort(const error&) {
    // nop
  }

  virtual net::socket_manager_ptr make(net::multiplexer*, ConnectionHandle) = 0;
};

template <class ConnectionHandle>
using connection_factory_ptr
  = std::unique_ptr<connection_factory<ConnectionHandle>>;

} // namespace caf::detail
