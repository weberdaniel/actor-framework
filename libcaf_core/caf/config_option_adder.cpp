// This file is part of CAF, the C++ Actor Framework. See the file LICENSE in
// the main distribution directory for license terms and copyright or visit
// https://github.com/actor-framework/actor-framework/blob/main/LICENSE.

#include "caf/config_option_adder.hpp"

#include "caf/config.hpp"
#include "caf/config_option_set.hpp"

namespace caf {

config_option_adder::config_option_adder(config_option_set& target,
                                         std::string_view category)
  : xs_(target), category_(category) {
  // nop
}

config_option_adder& config_option_adder::add_impl(config_option&& opt) {
  xs_.add(std::move(opt));
  return *this;
}

} // namespace caf
