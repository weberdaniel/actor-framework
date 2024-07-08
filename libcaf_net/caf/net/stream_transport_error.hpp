// This file is part of CAF, the C++ Actor Framework. See the file LICENSE in
// the main distribution directory for license terms and copyright or visit
// https://github.com/actor-framework/actor-framework/blob/master/LICENSE.

#pragma once

#include "caf/default_enum_inspect.hpp"
#include "caf/detail/net_export.hpp"
#include "caf/fwd.hpp"
#include "caf/is_error_code_enum.hpp"

#include <cstdint>
#include <string>
#include <type_traits>

namespace caf::net {

enum class stream_transport_error {
  /// Indicates that the transport should try again later.
  temporary,
  /// Indicates that the transport must read data before trying again.
  want_read,
  /// Indicates that the transport must write data before trying again.
  want_write,
  /// Indicates that the transport cannot resume this operation.
  permanent,
};

/// @relates stream_transport_error
CAF_NET_EXPORT std::string to_string(stream_transport_error);

/// @relates stream_transport_error
CAF_NET_EXPORT bool from_string(std::string_view, stream_transport_error&);

/// @relates stream_transport_error
CAF_NET_EXPORT bool from_integer(std::underlying_type_t<stream_transport_error>,
                                 stream_transport_error&);

/// @relates stream_transport_error
template <class Inspector>
bool inspect(Inspector& f, stream_transport_error& x) {
  return default_enum_inspect(f, x);
}

} // namespace caf::net
