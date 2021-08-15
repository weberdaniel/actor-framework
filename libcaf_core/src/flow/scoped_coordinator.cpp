// This file is part of CAF, the C++ Actor Framework. See the file LICENSE in
// the main distribution directory for license terms and copyright or visit
// https://github.com/actor-framework/actor-framework/blob/master/LICENSE.

#include "caf/flow/scoped_coordinator.hpp"

namespace caf::flow {

void scoped_coordinator::run() {
  auto next = [this] {
    std::unique_lock guard{mtx_};
    while (actions_.empty())
      cv_.wait(guard);
    auto res = std::move(actions_[0]);
    actions_.erase(actions_.begin());
    return std::make_pair(res, actions_.empty());
  };
  action_ptr ptr;
  bool at_end = false;
  do {
    std::tie(ptr, at_end) = next();
    (*ptr)();
    drop_disposed_flows();
  } while (!at_end || !watched_disposables_.empty());
}

void scoped_coordinator::ref_coordinator() const noexcept {
  ref();
}

void scoped_coordinator::deref_coordinator() const noexcept {
  deref();
}

void scoped_coordinator::schedule(action_ptr action) {
  std::unique_lock guard{mtx_};
  actions_.emplace_back(std::move(action));
  if (actions_.size() == 1)
    cv_.notify_all();
}

void scoped_coordinator::watch(disposable what) {
  watched_disposables_.emplace_back(std::move(what));
}

intrusive_ptr<scoped_coordinator> scoped_coordinator::make() {
  return {new scoped_coordinator, false};
}

namespace {

template <class T>
auto sptr(T* ptr) {
  return intrusive_ptr<T>{ptr};
};

} // namespace

void scoped_coordinator::dispatch_request(observable_base* source,
                                          observer_base* sink, size_t n) {
  schedule_fn([src{sptr(source)}, snk{sptr(sink)}, n] { //
    src->on_request(snk.get(), n);
  });
}

void scoped_coordinator::dispatch_cancel(observable_base* source,
                                         observer_base* sink) {
  schedule_fn([src{sptr(source)}, snk{sptr(sink)}] { //
    src->on_cancel(snk.get());
  });
}

void scoped_coordinator::drop_disposed_flows() {
  auto disposed = [](auto& hdl) { return hdl.disposed(); };
  auto& xs = watched_disposables_;
  if (auto e = std::remove_if(xs.begin(), xs.end(), disposed); e != xs.end())
    xs.erase(e, xs.end());
}

} // namespace caf::flow
