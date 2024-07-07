// This file is part of CAF, the C++ Actor Framework. See the file LICENSE in
// the main distribution directory for license terms and copyright or visit
// https://github.com/actor-framework/actor-framework/blob/main/LICENSE.

#pragma once

#include "caf/actor_cast.hpp"
#include "caf/actor_clock.hpp"
#include "caf/actor_control_block.hpp"
#include "caf/disposable.hpp"
#include "caf/fwd.hpp"
#include "caf/mailbox_element.hpp"
#include "caf/message_id.hpp"

#include <vector>

namespace caf::detail {

template <class Self, class SelfHandle, class Handle, class... Ts>
void profiled_send(Self* self, SelfHandle&& src, const Handle& dst,
                   message_id msg_id, scheduler* sched, Ts&&... xs) {
  if (dst) {
    auto element = make_mailbox_element(std::forward<SelfHandle>(src), msg_id,
                                        std::forward<Ts>(xs)...);
    dst->enqueue(std::move(element), sched);
  } else {
    self->home_system().base_metrics().rejected_messages->inc();
  }
}

template <class Self, class SelfHandle, class Handle, class... Ts>
disposable profiled_send(Self* self, SelfHandle&& src, const Handle& dst,
                         actor_clock& clock, actor_clock::time_point timeout,
                         [[maybe_unused]] message_id msg_id, Ts&&... xs) {
  if (dst) {
    auto element = make_mailbox_element(std::forward<SelfHandle>(src), msg_id,
                                        std::forward<Ts>(xs)...);
    return clock.schedule_message(timeout, actor_cast<strong_actor_ptr>(dst),
                                  std::move(element));
  } else {
    self->home_system().base_metrics().rejected_messages->inc();
    return {};
  }
}

} // namespace caf::detail
