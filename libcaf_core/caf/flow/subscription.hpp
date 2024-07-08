// This file is part of CAF, the C++ Actor Framework. See the file LICENSE in
// the main distribution directory for license terms and copyright or visit
// https://github.com/actor-framework/actor-framework/blob/master/LICENSE.

#pragma once

#include "caf/detail/core_export.hpp"
#include "caf/detail/plain_ref_counted.hpp"
#include "caf/disposable.hpp"
#include "caf/flow/coordinated.hpp"
#include "caf/flow/fwd.hpp"
#include "caf/intrusive_ptr.hpp"
#include "caf/ref_counted.hpp"

#include <cstddef>
#include <type_traits>

namespace caf::flow {

/// Controls the flow of items from publishers to subscribers.
class CAF_CORE_EXPORT subscription {
public:
  // -- nested types -----------------------------------------------------------

  /// Internal interface of a `subscription`.
  class CAF_CORE_EXPORT impl : public disposable::impl {
  public:
    ~impl() override;

    /// Signals demand for `n` more items.
    virtual void request(size_t n) = 0;
  };

  /// Simple base type for all subscription implementations that implements the
  /// reference counting member functions.
  class CAF_CORE_EXPORT impl_base : public detail::plain_ref_counted,
                                    public impl {
  public:
    void ref_disposable() const noexcept final;

    void deref_disposable() const noexcept final;
  };

  /// Describes a listener to the subscription that will receive an event
  /// whenever the observer calls `request` or `cancel`.
  class CAF_CORE_EXPORT listener : public coordinated {
  public:
    virtual ~listener();

    virtual void on_request(coordinated* sink, size_t n) = 0;

    virtual void on_cancel(coordinated* sink) = 0;
  };

  /// Default implementation for subscriptions that forward `request` and
  /// `cancel` to a @ref listener.
  class CAF_CORE_EXPORT fwd_impl final : public impl_base {
  public:
    fwd_impl(coordinator* ctx, listener* src, coordinated* snk)
      : ctx_(ctx), src_(src), snk_(snk) {
      // nop
    }

    bool disposed() const noexcept override;

    void request(size_t n) override;

    void dispose() override;

    auto* ctx() const noexcept {
      return ctx_;
    }

    /// Creates a new subscription object.
    /// @param ctx The owner of @p src and @p snk.
    /// @param src The @ref observable that emits items.
    /// @param snk the @ref observer that consumes items.
    /// @returns an instance of @ref fwd_impl in a @ref subscription handle.
    template <class Observable, class Observer>
    static subscription make(coordinator* ctx, Observable* src, Observer* snk) {
      static_assert(std::is_base_of_v<listener, Observable>);
      static_assert(std::is_base_of_v<coordinated, Observer>);
      static_assert(std::is_same_v<typename Observable::output_type,
                                   typename Observer::input_type>);
      intrusive_ptr<impl> ptr{new fwd_impl(ctx, src, snk), false};
      return subscription{std::move(ptr)};
    }

    /// Like @ref make but without any type checking.
    static subscription make_unsafe(coordinator* ctx, listener* src,
                                    coordinated* snk) {
      intrusive_ptr<impl> ptr{new fwd_impl(ctx, src, snk), false};
      return subscription{std::move(ptr)};
    }

  private:
    coordinator* ctx_;
    intrusive_ptr<listener> src_;
    intrusive_ptr<coordinated> snk_;
  };

  // -- constructors, destructors, and assignment operators --------------------

  explicit subscription(intrusive_ptr<impl> pimpl) noexcept
    : pimpl_(std::move(pimpl)) {
    // nop
  }

  subscription& operator=(std::nullptr_t) noexcept {
    pimpl_.reset();
    return *this;
  }

  subscription() noexcept = default;
  subscription(subscription&&) noexcept = default;
  subscription(const subscription&) noexcept = default;
  subscription& operator=(subscription&&) noexcept = default;
  subscription& operator=(const subscription&) noexcept = default;

  // -- demand signaling -------------------------------------------------------

  /// Causes the publisher to stop producing items for the subscriber. Any
  /// in-flight items may still get dispatched.
  void dispose() {
    if (pimpl_) {
      pimpl_->dispose();
      pimpl_ = nullptr;
    }
  }

  /// @copydoc impl::request
  /// @pre `valid()`
  void request(size_t n) {
    pimpl_->request(n);
  }

  // -- properties -------------------------------------------------------------

  bool valid() const noexcept {
    return pimpl_ != nullptr;
  }

  explicit operator bool() const noexcept {
    return valid();
  }

  bool operator!() const noexcept {
    return !valid();
  }

  impl* ptr() noexcept {
    return pimpl_.get();
  }

  const impl* ptr() const noexcept {
    return pimpl_.get();
  }

  intrusive_ptr<impl> as_intrusive_ptr() const& noexcept {
    return pimpl_;
  }

  intrusive_ptr<impl>&& as_intrusive_ptr() && noexcept {
    return std::move(pimpl_);
  }

  disposable as_disposable() const& noexcept {
    return disposable{pimpl_};
  }

  disposable as_disposable() && noexcept {
    return disposable{std::move(pimpl_)};
  }

  // -- swapping ---------------------------------------------------------------

  void swap(subscription& other) noexcept {
    pimpl_.swap(other.pimpl_);
  }

private:
  intrusive_ptr<impl> pimpl_;
};

/// @ref subscription
using subscription_impl = subscription::impl;

} // namespace caf::flow
