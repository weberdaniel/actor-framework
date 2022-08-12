// This file is part of CAF, the C++ Actor Framework. See the file LICENSE in
// the main distribution directory for license terms and copyright or visit
// https://github.com/actor-framework/actor-framework/blob/master/LICENSE.

#pragma once

#include "caf/detail/socket_sys_includes.hpp"
#include "caf/fwd.hpp"

namespace caf::detail {

void convert(const ip_endpoint& src, sockaddr_storage& dst);

error convert(const sockaddr_storage& src, ip_endpoint& dst);

} // namespace caf::detail
