// This file is part of CAF, the C++ Actor Framework. See the file LICENSE in
// the main distribution directory for license terms and copyright or visit
// https://github.com/actor-framework/actor-framework/blob/master/LICENSE.

#pragma once

#include "caf/flow/observer.hpp"
#include "caf/flow/op/cold.hpp"
#include "caf/flow/op/empty.hpp"
#include "caf/flow/subscription.hpp"

#include <deque>
#include <memory>
#include <numeric>
#include <utility>
#include <variant>
#include <vector>

namespace caf::flow::op {

/// Combines items from any number of observables.
template <class T>
class concat_sub : public subscription::impl_base {
public:
  // -- member types -----------------------------------------------------------

  using input_key = size_t;

  using input_type = std::variant<observable<T>, observable<observable<T>>>;

  // -- constructors, destructors, and assignment operators --------------------

  concat_sub(coordinator* ctx, observer<T> out, std::vector<input_type> inputs)
    : ctx_(ctx), out_(out), inputs_(std::move(inputs)) {
    CAF_ASSERT(!inputs_.empty());
    subscribe_next();
  }

  // -- input management -------------------------------------------------------

  void subscribe_to(observable<T> what) {
    CAF_ASSERT(!active_sub_);
    active_key_ = next_key_++;
    using fwd_t = forwarder<T, concat_sub, size_t>;
    auto fwd = make_counted<fwd_t>(this, active_key_);
    what.subscribe(fwd->as_observer());
  }

  void subscribe_to(observable<observable<T>> what) {
    CAF_ASSERT(!active_sub_);
    CAF_ASSERT(!factory_sub_);
    factory_key_ = next_key_++;
    using fwd_t = forwarder<observable<T>, concat_sub, size_t>;
    auto fwd = make_counted<fwd_t>(this, factory_key_);
    what.subscribe(fwd->as_observer());
  }

  void subscribe_next() {
    if (factory_key_ != 0) {
      // Ask the factory for the next observable.
      CAF_ASSERT(!active_sub_);
      factory_sub_.request(1);
    } else if (!inputs_.empty()) {
      // Subscribe to the next observable from the list.
      std::visit([this](auto& x) { this->subscribe_to(x); }, inputs_.front());
      inputs_.erase(inputs_.begin());
    } else {
      // Done!
      fin();
    }
  }

  // -- callbacks for the forwarders -------------------------------------------

  void fwd_on_subscribe(input_key key, subscription sub) {
    if (active_key_ == key && !active_sub_) {
      active_sub_ = std::move(sub);
      if (in_flight_ > 0)
        active_sub_.request(in_flight_);
    } else if (factory_key_ == key && !factory_sub_) {
      CAF_ASSERT(!active_sub_);
      factory_sub_ = std::move(sub);
      factory_sub_.request(1);
    } else {
      sub.dispose();
    }
  }

  void fwd_on_complete(input_key key) {
    if (active_key_ == key && active_sub_) {
      active_sub_ = nullptr;
      subscribe_next();
    } else if (factory_key_ == key && factory_sub_) {
      factory_sub_ = nullptr;
      factory_key_ = 0;
      if (!active_sub_)
        subscribe_next();
    }
  }

  void fwd_on_error(input_key key, const error& what) {
    if (key == active_key_ || key == factory_key_) {
      CAF_ASSERT(out_);
      if (delay_error_) {
        if (!err_)
          err_ = what;
        subscribe_next();
      } else {
        err_ = what;
        fin();
      }
    }
  }

  void fwd_on_next(input_key key, const T& item) {
    if (key == active_key_) {
      CAF_ASSERT(out_);
      --in_flight_;
      out_.on_next(item);
    }
  }

  void fwd_on_next(input_key key, const observable<T>& item) {
    if (key == factory_key_) {
      CAF_ASSERT(!active_sub_);
      subscribe_to(item);
    }
  }

  // -- implementation of subscription -----------------------------------------

  bool disposed() const noexcept override {
    return !out_;
  }

  void dispose() override {
    if (out_) {
      ctx_->delay_fn([strong_this = intrusive_ptr<concat_sub>{this}] {
        if (strong_this->out_) {
          strong_this->err_.reset();
          strong_this->fin();
        }
      });
    }
  }

  void request(size_t n) override {
    CAF_ASSERT(out_.valid());
    if (active_sub_)
      active_sub_.request(n);
    in_flight_ += n;
  }

private:
  void fin() {
    CAF_ASSERT(out_);
    if (factory_sub_) {
      factory_sub_.dispose();
      factory_sub_ = nullptr;
    }
    if (active_sub_) {
      active_sub_.dispose();
      active_sub_ = nullptr;
    }
    factory_key_ = 0;
    active_key_ = 0;
    if (err_)
      out_.on_error(err_);
    else
      out_.on_complete();
    out_ = nullptr;
  }

  /// Stores the context (coordinator) that runs this flow.
  coordinator* ctx_;

  /// Stores a handle to the subscribed observer.
  observer<T> out_;

  /// Configures whether we carry on after an error.
  bool delay_error_ = false;

  /// Caches an on_error reason if delay_error_ is true.
  error err_;

  /// Stores our input sources. The first input is active (subscribed to) while
  /// the others are pending (not subscribed to).
  std::vector<input_type> inputs_;

  /// If set, identifies the subscription to an observable factory.
  subscription factory_sub_;

  /// Our currently active subscription.
  subscription active_sub_;

  /// Identifies the active forwarder.
  input_key factory_key_ = 0;

  /// Identifies the active forwarder.
  input_key active_key_ = 0;

  /// Stores the next available key.
  input_key next_key_ = 1;

  /// Stores how much demand we have left. When switching to a new input, we
  /// pass any demand unused by the previous input to the new one.
  size_t in_flight_ = 0;
};

template <class T>
class concat : public cold<T> {
public:
  // -- member types -----------------------------------------------------------

  using super = cold<T>;

  using input_type = std::variant<observable<T>, observable<observable<T>>>;

  // -- constructors, destructors, and assignment operators --------------------

  template <class... Ts, class... Inputs>
  explicit concat(coordinator* ctx, Inputs&&... inputs) : super(ctx) {
    (add(std::forward<Inputs>(inputs)), ...);
  }

  // -- properties -------------------------------------------------------------

  size_t inputs() const noexcept {
    return inputs_.size();
  }

  // -- implementation of observable<T> -----------------------------------

  disposable subscribe(observer<T> out) override {
    if (inputs() == 0) {
      return make_counted<empty<T>>(super::ctx_)->subscribe(std::move(out));
    } else {
      auto ptr = make_counted<concat_sub<T>>(super::ctx_, out, inputs_);
      out.on_subscribe(subscription{ptr});
      return ptr->as_disposable();
    }
  }

private:
  template <class Input>
  void add(Input&& x) {
    using input_t = std::decay_t<Input>;
    if constexpr (detail::is_iterable_v<input_t>) {
      for (auto& in : x)
        add(in);
    } else {
      static_assert(is_observable_v<input_t>);
      inputs_.emplace_back(std::move(x).as_observable());
    }
  }

  std::vector<input_type> inputs_;
};

} // namespace caf::flow::op
