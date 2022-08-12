// This file is part of CAF, the C++ Actor Framework. See the file LICENSE in
// the main distribution directory for license terms and copyright or visit
// https://github.com/actor-framework/actor-framework/blob/master/LICENSE.

#include "caf/net/binary/default_trait.hpp"

#include "caf/error.hpp"
#include "caf/logger.hpp"
#include "caf/net/binary/frame.hpp"
#include "caf/sec.hpp"

namespace caf::net::binary {

bool default_trait::convert(const output_type& x, byte_buffer& bytes) {
  auto src = x.bytes();
  bytes.insert(bytes.end(), src.begin(), src.end());
  return true;
}

bool default_trait::convert(const_byte_span bytes, input_type& x) {
  x = frame{bytes};
  return true;
}

error default_trait::last_error() {
  CAF_LOG_ERROR("default_trait::last_error called");
  return {sec::logic_error};
}

} // namespace caf::net::binary
