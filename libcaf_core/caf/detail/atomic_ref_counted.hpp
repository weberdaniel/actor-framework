// This file is part of CAF, the C++ Actor Framework. See the file LICENSE in
// the main distribution directory for license terms and copyright or visit
// https://github.com/actor-framework/actor-framework/blob/master/LICENSE.

#pragma once

#include <atomic>
#include <cstddef>

#include "caf/detail/core_export.hpp"

namespace caf::detail {

/// Base class for reference counted objects with an atomic reference count.
class CAF_CORE_EXPORT atomic_ref_counted {
public:
  virtual ~atomic_ref_counted();

  atomic_ref_counted();
  atomic_ref_counted(const atomic_ref_counted&);
  atomic_ref_counted& operator=(const atomic_ref_counted&);

  /// Increases reference count by one.
  void ref() const noexcept {
    rc_.fetch_add(1, std::memory_order_relaxed);
  }

  /// Decreases reference count by one and calls `request_deletion`
  /// when it drops to zero.
  void deref() const noexcept;

  /// Queries whether there is exactly one reference.
  bool unique() const noexcept {
    return rc_ == 1;
  }

  /// Queries the current reference count for this object.
  size_t get_reference_count() const noexcept {
    return rc_.load();
  }

protected:
  mutable std::atomic<size_t> rc_;
};

} // namespace caf::detail
