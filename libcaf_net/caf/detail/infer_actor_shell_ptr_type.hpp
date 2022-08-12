// This file is part of CAF, the C++ Actor Framework. See the file LICENSE in
// the main distribution directory for license terms and copyright or visit
// https://github.com/actor-framework/actor-framework/blob/master/LICENSE.

#pragma once

#include "caf/fwd.hpp"
#include "caf/net/fwd.hpp"

namespace caf::detail {

template <class T>
struct actor_shell_ptr_type_oracle;

template <>
struct actor_shell_ptr_type_oracle<actor> {
  using type = net::actor_shell_ptr;
};

template <class... Sigs>
struct actor_shell_ptr_type_oracle<typed_actor<Sigs...>> {
  using type = net::typed_actor_shell_ptr<Sigs...>;
};

template <class T>
using infer_actor_shell_ptr_type =
  typename actor_shell_ptr_type_oracle<T>::type;

} // namespace caf::detail
