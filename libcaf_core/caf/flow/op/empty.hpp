// This file is part of CAF, the C++ Actor Framework. See the file LICENSE in
// the main distribution directory for license terms and copyright or visit
// https://github.com/actor-framework/actor-framework/blob/main/LICENSE.

#pragma once

#include "caf/flow/observer.hpp"
#include "caf/flow/op/cold.hpp"
#include "caf/flow/subscription.hpp"

#include <cstdint>

namespace caf::flow::op {

/// An observable that represents an empty range. As soon as an observer
/// requests values from this observable, it calls `on_complete`.
template <class T>
class empty : public cold<T> {
public:
  // -- member types -----------------------------------------------------------

  using super = cold<T>;

  // -- constructors, destructors, and assignment operators --------------------

  explicit empty(coordinator* parent) : super(parent) {
    // nop
  }

  // -- implementation of observable<T>::impl ----------------------------------

  disposable subscribe(observer<T> out) override {
    return super::empty_subscription(out);
  }
};

} // namespace caf::flow::op
