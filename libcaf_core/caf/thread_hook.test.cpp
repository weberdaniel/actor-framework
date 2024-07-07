// This file is part of CAF, the C++ Actor Framework. See the file LICENSE in
// the main distribution directory for license terms and copyright or visit
// https://github.com/actor-framework/actor-framework/blob/main/LICENSE.

#include "caf/thread_hook.hpp"

#include "caf/test/test.hpp"

#include "caf/actor_system_config.hpp"
#include "caf/detail/default_thread_count.hpp"
#include "caf/event_based_actor.hpp"
#include "caf/scheduler.hpp"

#include <atomic>

using namespace caf;

namespace {

using atomic_count = std::atomic<size_t>;

size_t assumed_thread_count;
size_t assumed_init_calls;

struct dummy_thread_hook : thread_hook {
  void init(actor_system&) override {
    // nop
  }

  void thread_started(thread_owner) override {
    // nop
  }

  void thread_terminates() override {
    // nop
  }
};

class counting_thread_hook : public thread_hook {
public:
  counting_thread_hook()
    : count_init_{0}, count_thread_started_{0}, count_thread_terminates_{0} {
    // nop
  }

  ~counting_thread_hook() override {
    if (count_init_ != assumed_init_calls)
      CAF_RAISE_ERROR(std::logic_error,
                      "thread_hook init called different times than assumed");
    if (count_thread_started_ != assumed_thread_count)
      CAF_RAISE_ERROR(std::logic_error,
                      "thread_hook started different times than assumed");
    if (count_thread_terminates_ != assumed_thread_count)
      CAF_RAISE_ERROR(std::logic_error,
                      "thread_hook terminated different times than assumed");
  }

  void init(actor_system&) override {
    ++count_init_;
  }

  void thread_started(thread_owner) override {
    ++count_thread_started_;
  }

  void thread_terminates() override {
    ++count_thread_terminates_;
  }

private:
  atomic_count count_init_;
  atomic_count count_thread_started_;
  atomic_count count_thread_terminates_;
};

template <class Hook>
struct config : actor_system_config {
  config() {
    add_thread_hook<Hook>();
  }
};

template <class Hook>
struct fixture {
  config<Hook> cfg;
  actor_system sys;
  fixture() : sys(cfg) {
    // nop
  }
};

TEST("counting_no_system") {
  {
    assumed_init_calls = 0;
    actor_system_config cfg;
    cfg.add_thread_hook<counting_thread_hook>();
  }
}

using dummy_thread_hook_fixture = fixture<dummy_thread_hook>;

WITH_FIXTURE(dummy_thread_hook_fixture) {

TEST("counting_no_args") {
  // nop
}

} // WITH_FIXTURE(dummy_thread_hook_fixture)

using counting_thread_hook_fixture = fixture<counting_thread_hook>;

WITH_FIXTURE(counting_thread_hook_fixture) {

TEST("counting_system_without_actor") {
  {
    assumed_init_calls = 1;
    auto fallback = detail::default_thread_count();
    assumed_thread_count = get_or(cfg, "caf.scheduler.max-threads", fallback)
                           + 3; // clock, private thread pool and printer
  }
}

TEST("counting_system_with_actor") {
  {
    assumed_init_calls = 1;
    auto fallback = detail::default_thread_count();
    assumed_thread_count
      = get_or(cfg, "caf.scheduler.max-threads", fallback)
        + 4; // clock, private thread pool, printer and  detached actor
    sys.spawn<detached>([] {});
    sys.spawn([] {});
  }
}

} // WITH_FIXTURE(counting_thread_hook_fixture)

} // namespace
