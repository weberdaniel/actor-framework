// This file is part of CAF, the C++ Actor Framework. See the file LICENSE in
// the main distribution directory for license terms and copyright or visit
// https://github.com/actor-framework/actor-framework/blob/main/LICENSE.

#pragma once

#include <caf/detail/monotonic_buffer_resource.hpp>

#include <cstddef>
#include <iterator>
#include <new>
#include <utility>

namespace caf::detail {

template <class T>
struct mbr_list_node {
  T value;
  mbr_list_node* next;
};

template <class T>
class mbr_list_iterator {
public:
  using difference_type = ptrdiff_t;

  using value_type = T;

  using pointer = value_type*;

  using reference = value_type&;

  using iterator_category = std::forward_iterator_tag;

  using node_pointer
    = std::conditional_t<std::is_const_v<T>,
                         const mbr_list_node<std::remove_const_t<T>>*,
                         mbr_list_node<value_type>*>;

  constexpr mbr_list_iterator() noexcept = default;

  constexpr explicit mbr_list_iterator(node_pointer ptr) noexcept : ptr_(ptr) {
    // nop
  }

  constexpr mbr_list_iterator(const mbr_list_iterator&) noexcept = default;

  constexpr mbr_list_iterator& operator=(const mbr_list_iterator&) noexcept
    = default;

  constexpr node_pointer get() const noexcept {
    return ptr_;
  }

  mbr_list_iterator& operator++() noexcept {
    ptr_ = ptr_->next;
    return *this;
  }

  mbr_list_iterator operator++(int) noexcept {
    auto temp = ptr_;
    ptr_ = ptr_->next;
    return mbr_list_iterator{temp};
  }

  T& operator*() const noexcept {
    return ptr_->value;
  }

  T* operator->() const noexcept {
    return std::addressof(ptr_->value);
  }

private:
  node_pointer ptr_ = nullptr;
};

template <class T>
constexpr bool operator==(mbr_list_iterator<T> lhs, mbr_list_iterator<T> rhs) {
  return lhs.get() == rhs.get();
}

template <class T>
constexpr bool operator!=(mbr_list_iterator<T> lhs, mbr_list_iterator<T> rhs) {
  return !(lhs == rhs);
}

// A minimal version of a linked list that has constexpr constructor and an
// iterator type where the default-constructed iterator is the past-the-end
// iterator. Properties that std::list unfortunately lacks.
//
// The default-constructed list object is an empty list that does not allow
// push_back.
template <class T>
class mbr_list {
public:
  using value_type = T;

  using node_type = mbr_list_node<value_type>;

  using allocator_type = monotonic_buffer_resource::allocator<node_type>;

  using reference = value_type&;

  using const_reference = const value_type&;

  using pointer = value_type*;

  using const_pointer = const value_type*;

  using node_pointer = node_type*;

  using const_node_pointer = const node_type*;

  using iterator = mbr_list_iterator<value_type>;

  using const_iterator = mbr_list_iterator<const value_type>;

  mbr_list() noexcept {
    // nop
  }

  ~mbr_list() {
    auto* ptr = head_;
    while (ptr != nullptr) {
      auto* next = ptr->next;
      ptr->~node_type();
      allocator_.deallocate(ptr, 1);
      ptr = next;
    }
  }

  explicit mbr_list(allocator_type allocator) noexcept : allocator_(allocator) {
    // nop
  }

  explicit mbr_list(monotonic_buffer_resource* resource) noexcept
    : allocator_(resource) {
    // nop
  }

  mbr_list(const mbr_list&) = delete;

  mbr_list(mbr_list&& other)
    : size_(other.size_),
      head_(other.head_),
      tail_(other.tail_),
      allocator_(other.allocator_) {
    other.size_ = 0;
    other.head_ = nullptr;
    other.tail_ = nullptr;
  }

  mbr_list& operator=(const mbr_list&) = delete;

  mbr_list& operator=(mbr_list&& other) {
    using std::swap;
    swap(size_, other.size_);
    swap(head_, other.head_);
    swap(tail_, other.tail_);
    swap(allocator_, other.allocator_);
    return *this;
  }

  [[nodiscard]] bool empty() const noexcept {
    return size_ == 0;
  }

  [[nodiscard]] size_t size() const noexcept {
    return size_;
  }

  [[nodiscard]] iterator begin() noexcept {
    return iterator{head_};
  }

  [[nodiscard]] const_iterator begin() const noexcept {
    return const_iterator{head_};
  }

  [[nodiscard]] const_iterator cbegin() const noexcept {
    return begin();
  }

  [[nodiscard]] iterator end() noexcept {
    return {};
  }

  [[nodiscard]] const_iterator end() const noexcept {
    return {};
  }

  [[nodiscard]] const_iterator cend() const noexcept {
    return {};
  }

  [[nodiscard]] reference front() noexcept {
    return head_->value;
  }

  [[nodiscard]] const_reference front() const noexcept {
    return head_->value;
  }

  [[nodiscard]] reference back() noexcept {
    return tail_->value;
  }

  [[nodiscard]] const_reference back() const noexcept {
    return tail_->value;
  }

  [[nodiscard]] allocator_type get_allocator() const noexcept {
    return allocator_;
  }

  void push_back(T value) {
    ++size_;
    auto new_node = allocator_.allocate(1);
    new (new_node) node_type{std::move(value), nullptr};
    if (head_ == nullptr) {
      head_ = tail_ = new_node;
    } else {
      tail_->next = new_node;
      tail_ = new_node;
    }
  }

  template <class... Ts>
  reference emplace_back(Ts&&... args) {
    ++size_;
    auto new_node = allocator_.allocate(1);
    new (new_node) node_type{T{std::forward<Ts>(args)...}, nullptr};
    if (head_ == nullptr) {
      head_ = tail_ = new_node;
    } else {
      tail_->next = new_node;
      tail_ = new_node;
    }
    return new_node->value;
  }

  node_pointer head() noexcept {
    return head_;
  }

  const_node_pointer head() const noexcept {
    return head_;
  }

private:
  size_t size_ = 0;
  node_pointer head_ = nullptr;
  node_pointer tail_ = nullptr;
  allocator_type allocator_;
};

template <class T>
bool operator==(const mbr_list<T>& lhs, const mbr_list<T>& rhs) {
  return std::equal(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

template <class T>
bool operator!=(const mbr_list<T>& lhs, const mbr_list<T>& rhs) {
  return !(lhs == rhs);
}

} // namespace caf::detail
