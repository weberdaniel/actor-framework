// This file is part of CAF, the C++ Actor Framework. See the file LICENSE in
// the main distribution directory for license terms and copyright or visit
// https://github.com/actor-framework/actor-framework/blob/main/LICENSE.

#pragma once

#include "caf/config.hpp"
#include "caf/detail/core_export.hpp"
#include "caf/detail/pp.hpp"

#include <string_view>

#define CAF_ABI_NAMESPACE CAF_PP_PASTE(abi_, CAF_VERSION_MAJOR)

namespace caf {

/// Provides version information for CAF.
class CAF_CORE_EXPORT version {
public:
  /// Returns the major version number of CAF.
  static int get_major() noexcept;

  /// Returns the minor version number of CAF.
  static int get_minor() noexcept;

  /// Returns the patch version number of CAF.
  static int get_patch() noexcept;

  /// Returns the full version number of CAF as human-readable string.
  static std::string_view str() noexcept;

  /// Returns the full version number of CAF as human-readable string.
  static const char* c_str() noexcept;

  /// An opaque token that represents the ABI version of CAF.
  enum class abi_token {};

  /// Checks whether the ABI token is compatible with the current version of
  /// CAF. Otherwise, calls `abort`.
  static void check_abi_compatibility(abi_token token) noexcept;
};

inline namespace CAF_ABI_NAMESPACE {
/// Returns an token that represents the ABI version of CAF.
CAF_CORE_EXPORT version::abi_token make_abi_token() noexcept;
} // namespace CAF_ABI_NAMESPACE

} // namespace caf
