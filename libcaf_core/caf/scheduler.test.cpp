// This file is part of CAF, the C++ Actor Framework. See the file LICENSE in
// the main distribution directory for license terms and copyright or visit
// https://github.com/actor-framework/actor-framework/blob/main/LICENSE.

#include "caf/scheduler.hpp"

#include "caf/test/outline.hpp"

#include "caf/actor_system_config.hpp"
#include "caf/detail/latch.hpp"
#include "caf/resumable.hpp"

#include <string>

using namespace caf;
using namespace std::literals;
using detail::latch;

namespace {

struct testee : resumable, ref_counted {
  explicit testee(std::shared_ptr<latch> latch_handle)
    : rendezvous(std::move(latch_handle)) {
  }

  subtype_t subtype() const noexcept override {
    return resumable::function_object;
  }

  resume_result resume(scheduler*, size_t max_throughput) override {
    if (++runs == 10) {
      received_throughput = max_throughput;
      rendezvous->count_down();
      return resumable::done;
    }
    return resumable::resume_later;
  }

  void ref_resumable() const noexcept final {
    ref();
  }

  void deref_resumable() const noexcept final {
    deref();
  }

  std::atomic<size_t> runs = 0;
  std::atomic<size_t> received_throughput = 0;
  std::shared_ptr<latch> rendezvous;
};

OUTLINE("scheduling resumables") {
  GIVEN("an actor system using the work <sched> scheduler") {
    auto sched = block_parameters<std::string>();
    actor_system_config cfg;
    cfg.set("caf.scheduler.max-throughput", 5);
    cfg.set("caf.scheduler.max-threads", 2);
    cfg.set("caf.scheduler.policy", sched);
    WHEN("scheduling a resumable") {
      auto sys = std::make_unique<actor_system>(cfg);
      auto rendezvous = std::make_shared<latch>(2);
      auto worker = make_counted<testee>(rendezvous);
      worker->ref();
      sys->scheduler().schedule(worker.get());
      THEN("expect the resumable to be executed until done") {
        rendezvous->count_down_and_wait();
        check_eq(worker->runs.load(), 10u);
      }
      AND_THEN("expect the correct max throughput") {
        check_eq(worker->received_throughput, 5u);
      }
      AND_THEN("the scheduler releases the ref when done") {
        // Note: destroying the actor system here will cause CAF to shut down.
        //       Ultimately stopping the scheduler and releasing the references.
        sys = nullptr;
        check_eq(worker->get_reference_count(), 1u);
      }
    }
    // TODO: Change to WHEN block after fixing issue #1776.
    AND_WHEN("scheduling multiple resumables") {
      auto sys = std::make_unique<actor_system>(cfg);
      auto workers = std::vector<intrusive_ptr<testee>>{};
      auto rendezvous = std::make_shared<latch>(11);
      for (int i = 0; i < 10; i++) {
        workers.emplace_back(make_counted<testee>(rendezvous));
        workers.back()->ref();
        check_eq(workers.back()->get_reference_count(), 2u);
        sys->scheduler().schedule(workers.back().get());
      }
      THEN("expect the resumables to be executed until done") {
        rendezvous->count_down_and_wait();
        for (const auto& worker : workers) {
          check_eq(worker->runs, 10u);
        }
      }
      AND_THEN("expect the correct max throughput") {
        for (const auto& worker : workers)
          check_eq(worker->received_throughput, 5u);
      }
      AND_THEN("the scheduler releases the ref when done") {
        // Note: destroying the actor system here will cause CAF to shut down.
        //       Ultimately stopping the scheduler and releasing the references.
        sys = nullptr;
        for (const auto& worker : workers)
          check_eq(worker->get_reference_count(), 1u);
      }
    }
  }
  EXAMPLES = R"(
    |    sched    |
    | sharing     |
    | stealing    |
  )";
}

struct awaiting_testee : resumable, ref_counted {
  explicit awaiting_testee(std::shared_ptr<latch> latch_handle)
    : rendezvous(std::move(latch_handle)) {
  }

  subtype_t subtype() const noexcept override {
    return resumable::function_object;
  }

  resume_result resume(scheduler*, size_t) override {
    runs++;
    rendezvous->count_down();
    return resumable::awaiting_message;
  }

  void ref_resumable() const noexcept final {
    ref();
  }

  void deref_resumable() const noexcept final {
    deref();
  }

  std::atomic<size_t> runs = 0;
  std::shared_ptr<latch> rendezvous;
};

OUTLINE("scheduling units that are awaiting") {
  GIVEN("an actor system using the work <sched> scheduler") {
    auto sched = block_parameters<std::string>();
    actor_system_config cfg;
    cfg.set("caf.scheduler.policy", sched);
    cfg.set("caf.scheduler.max-threads", 2);
    cfg.set("caf.scheduler.max-throughput", 5);
    auto sys = std::make_unique<actor_system>(cfg);
    WHEN("having resumables that go to an awaiting state") {
      auto workers = std::vector<intrusive_ptr<awaiting_testee>>{};
      auto rendezvous = std::make_shared<latch>(11);
      for (int i = 0; i < 10; i++) {
        workers.push_back(make_counted<awaiting_testee>(rendezvous));
        workers.back()->ref();
        sys->scheduler().schedule(workers.back().get());
      }
      THEN("expect the resumables to be executed once") {
        rendezvous->count_down_and_wait();
        for (const auto& worker : workers) {
          check_eq(worker->runs, 1u);
        }
      }
      AND_THEN("the scheduler releases the ref when done") {
        // Note: destroying the actor system here will cause CAF to shut down.
        //       Ultimately stopping the scheduler and releasing the references.
        sys = nullptr;
        for (const auto& worker : workers)
          check_eq(worker->get_reference_count(), 1u);
      }
    }
  }
  EXAMPLES = R"(
    |    sched    |
    | sharing     |
    | stealing    |
  )";
}

} // namespace
