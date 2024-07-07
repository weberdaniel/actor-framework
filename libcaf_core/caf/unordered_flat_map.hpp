// This file is part of CAF, the C++ Actor Framework. See the file LICENSE in
// the main distribution directory for license terms and copyright or visit
// https://github.com/actor-framework/actor-framework/blob/main/LICENSE.

#pragma once

#include "caf/detail/comparable.hpp"
#include "caf/detail/type_traits.hpp"
#include "caf/fwd.hpp"
#include "caf/raise_error.hpp"

#include <algorithm>
#include <functional>
#include <vector>

namespace caf {

/// A map abstraction with an unsorted `std::vector` providing `O(n)` lookup.
template <class Key, class T, class Allocator>
class unordered_flat_map {
public:
  // -- member types ----------------------------------------------------------

  using key_type = Key;

  using mapped_type = T;

  using value_type = std::pair<Key, T>;

  using vector_type = std::vector<value_type, Allocator>;

  using allocator_type = typename vector_type::allocator_type;

  using size_type = typename vector_type::size_type;

  using difference_type = typename vector_type::difference_type;

  using reference = typename vector_type::reference;

  using const_reference = typename vector_type::const_reference;

  using pointer = typename vector_type::pointer;

  using const_pointer = typename vector_type::const_pointer;

  using iterator = typename vector_type::iterator;

  using const_iterator = typename vector_type::const_iterator;

  using reverse_iterator = typename vector_type::reverse_iterator;

  using const_reverse_iterator = typename vector_type::const_reverse_iterator;

  // -- constructors, destructors, and assignment operators -------------------

  unordered_flat_map() = default;

  unordered_flat_map(std::initializer_list<value_type> l) : xs_(l) {
    // nop
  }

  template <class InputIterator>
  unordered_flat_map(InputIterator first, InputIterator last)
    : xs_(first, last) {
    // nop
  }

  // -- iterator access --------------------------------------------------------

  iterator begin() noexcept {
    return xs_.begin();
  }

  const_iterator begin() const noexcept {
    return xs_.begin();
  }

  const_iterator cbegin() const noexcept {
    return xs_.cbegin();
  }

  reverse_iterator rbegin() noexcept {
    return xs_.rbegin();
  }

  const_reverse_iterator rbegin() const noexcept {
    return xs_.rbegin();
  }

  iterator end() noexcept {
    return xs_.end();
  }

  const_iterator end() const noexcept {
    return xs_.end();
  }

  const_iterator cend() const noexcept {
    return xs_.end();
  }

  reverse_iterator rend() noexcept {
    return xs_.rend();
  }

  const_reverse_iterator rend() const noexcept {
    return xs_.rend();
  }

  // -- size and capacity ------------------------------------------------------

  bool empty() const noexcept {
    return xs_.empty();
  }

  size_type size() const noexcept {
    return xs_.size();
  }

  void reserve(size_type count) {
    xs_.reserve(count);
  }

  void shrink_to_fit() {
    xs_.shrink_to_fit();
  }

  // -- access to members ------------------------------------------------------

  /// Gives raw access to the underlying container.
  vector_type& container() noexcept {
    return xs_;
  }

  /// Gives raw access to the underlying container.
  const vector_type& container() const noexcept {
    return xs_;
  }

  // -- modifiers -------------------------------------------------------------

  void clear() noexcept {
    return xs_.clear();
  }

  void swap(unordered_flat_map& other) {
    xs_.swap(other.xs_);
  }

  // -- insertion -------------------------------------------------------------

  std::pair<iterator, bool> insert(value_type x) {
    auto i = find(x.first);
    if (i == end()) {
      xs_.emplace_back(std::move(x));
      return {xs_.end() - 1, true};
    }
    return {i, false};
  }

  iterator insert(const_iterator, value_type x) {
    return insert(std::move(x)).first;
  }

  template <class InputIterator>
  void insert(InputIterator first, InputIterator last) {
    while (first != last)
      insert(*first++);
  }

  template <class... Ts>
  std::pair<iterator, bool> emplace(Ts&&... xs) {
    return insert(value_type(std::forward<Ts>(xs)...));
  }

  template <class... Ts>
  iterator emplace_hint(const_iterator hint, Ts&&... xs) {
    return insert(hint, value_type(std::forward<Ts>(xs)...));
  }

  std::pair<iterator, bool> insert_or_assign(const key_type& key,
                                             mapped_type val) {
    if (auto i = find(key); i != end()) {
      i->second = std::move(val);
      return {i, false};
    }
    xs_.emplace_back(value_type{key, std::move(val)});
    return {xs_.end() - 1, true};
  }

  iterator insert_or_assign(const_iterator, const key_type& key,
                            mapped_type val) {
    return insert_or_assign(key, std::move(val)).first;
  }

  // -- removal ----------------------------------------------------------------

  iterator erase(iterator i) {
    if (auto tail = end() - 1; i != tail)
      std::iter_swap(i, tail);
    xs_.pop_back();
    return i; // Now points to the element that was tail or to end().
  }

  iterator erase(const_iterator i) {
    return erase(begin() + (i - begin()));
  }

  iterator erase(const_iterator first, const_iterator last) {
    return xs_.erase(first, last);
  }

  size_type erase(const key_type& x) {
    if (auto i = find(x); i != end()) {
      erase(i);
      return 1;
    }
    return 0;
  }

  // -- lookup -----------------------------------------------------------------

  template <class K>
  mapped_type& at(const K& key) {
    if (auto i = find(key); i != end())
      return i->second;
    CAF_RAISE_ERROR(std::out_of_range,
                    "caf::unordered_flat_map::at out of range");
  }

  template <class K>
  const mapped_type& at(const K& key) const {
    if (auto i = find(key); i != end())
      return i->second;
    CAF_RAISE_ERROR(std::out_of_range,
                    "caf::unordered_flat_map::at out of range");
  }

  mapped_type& operator[](const key_type& key) {
    if (auto i = find(key); i != end())
      return i->second;
    return xs_.emplace_back(key, mapped_type{}).second;
  }

  template <class K>
  iterator find(const K& key) {
    auto pred = [&](const value_type& y) { return key == y.first; };
    return std::find_if(xs_.begin(), xs_.end(), pred);
  }

  template <class K>
  const_iterator find(const K& key) const {
    auto pred = [&](const value_type& y) { return key == y.first; };
    return std::find_if(xs_.begin(), xs_.end(), pred);
  }

  template <class K>
  size_type count(const K& key) const {
    return find(key) == end() ? 0 : 1;
  }

  bool contains(const key_type& key) const {
    return find(key) != end();
  }

private:
  vector_type xs_;
};

/// @relates unordered_flat_map
template <class K, class T, class A>
bool operator==(const unordered_flat_map<K, T, A>& lhs,
                const unordered_flat_map<K, T, A>& rhs) {
  if (lhs.size() != rhs.size())
    return false;
  for (const auto& [key, val] : lhs) {
    if (auto i = rhs.find(key); i == rhs.end() || i->second != val)
      return false;
  }
  return true;
}

/// @relates unordered_flat_map
template <class K, class T, class A>
bool operator!=(const unordered_flat_map<K, T, A>& lhs,
                const unordered_flat_map<K, T, A>& rhs) {
  return !(lhs == rhs);
}

} // namespace caf
