// This file is part of CAF, the C++ Actor Framework. See the file LICENSE in
// the main distribution directory for license terms and copyright or visit
// https://github.com/actor-framework/actor-framework/blob/main/LICENSE.

#pragma once

#include "caf/actor_control_block.hpp"
#include "caf/async/batch.hpp"
#include "caf/cow_string.hpp"
#include "caf/detail/comparable.hpp"
#include "caf/detail/core_export.hpp"
#include "caf/fwd.hpp"

#include <cstddef>
#include <cstdint>
#include <string>

namespace caf {

/// Provides access to a potentially unbound sequence of items emitted by an
/// actor. Each stream is uniquely identified by the address of the hosting
/// actor plus an integer value. Further, streams have human-readable names
/// attached to them in order to make help with observability and logging.
class CAF_CORE_EXPORT stream : private detail::comparable<stream> {
public:
  // -- constructors, destructors, and assignment operators --------------------

  stream() = default;

  stream(stream&&) noexcept = default;

  stream(const stream&) noexcept = default;

  stream& operator=(stream&&) noexcept = default;

  stream& operator=(const stream&) noexcept = default;

  stream(strong_actor_ptr source, type_id_t type, std::string name, uint64_t id)
    : source_(std::move(source)), type_(type), name_(std::move(name)), id_(id) {
    // nop
  }

  stream(strong_actor_ptr source, type_id_t type, cow_string name, uint64_t id)
    : source_(std::move(source)), type_(type), name_(std::move(name)), id_(id) {
    // nop
  }

  // -- properties -------------------------------------------------------------

  /// Checks whether this stream emits elements of type @c T.
  template <class T>
  bool has_element_type() const noexcept {
    return type_id_v<T> == type_;
  }

  /// Queries the source of this stream. Default-constructed streams return a
  /// @c null pointer.
  const strong_actor_ptr& source() const noexcept {
    return source_;
  }

  /// Returns the type ID of the items emitted by the source.
  type_id_t type() const noexcept {
    return type_;
  }

  /// Returns the human-readable name for this stream, as announced by the
  /// source.
  const std::string& name() const noexcept {
    return name_.str();
  }

  /// Returns the source-specific identifier for this stream.
  uint64_t id() const noexcept {
    return id_;
  }

  // -- comparison -------------------------------------------------------------

  ptrdiff_t compare(const stream& other) const noexcept;

  // -- serialization ----------------------------------------------------------

  template <class Inspector>
  friend bool inspect(Inspector& f, stream& obj) {
    return f.object(obj).fields(f.field("source", obj.source_),
                                f.field("type", obj.type_),
                                f.field("name", obj.name_),
                                f.field("id", obj.id_));
  }

private:
  strong_actor_ptr source_;
  type_id_t type_ = invalid_type_id;
  cow_string name_;
  uint64_t id_ = 0;
};

} // namespace caf
