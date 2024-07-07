// This file is part of CAF, the C++ Actor Framework. See the file LICENSE in
// the main distribution directory for license terms and copyright or visit
// https://github.com/actor-framework/actor-framework/blob/main/LICENSE.

#include "caf/log/event.hpp"

#include "caf/logger.hpp"
#include "caf/make_counted.hpp"

#include <new>

namespace caf::log {

namespace {

template <class T>
using allocator_t = detail::monotonic_buffer_resource::allocator<T>;

std::string_view deep_copy_impl(detail::monotonic_buffer_resource* resource,
                                std::string_view str) {
  auto* buf = allocator_t<char>{resource}.allocate(str.size());
  memcpy(buf, str.data(), str.size());
  return std::string_view{buf, str.size()};
}

chunked_string::node_type*
deep_copy_to_node(detail::monotonic_buffer_resource* resource,
                  std::string_view str) {
  using node_type = chunked_string::node_type;
  auto* buf = allocator_t<node_type>{resource}.allocate(1);
  return new (buf) node_type{deep_copy_impl(resource, str), nullptr};
}

chunked_string deep_copy_impl(detail::monotonic_buffer_resource* resource,
                              chunked_string str) {
  using node_type = chunked_string::node_type;
  node_type* head = nullptr;
  node_type* tail = nullptr;
  for (auto chunk : str) {
    auto* node = deep_copy_to_node(resource, chunk);
    if (head == nullptr) {
      head = node;
      tail = node;
    } else {
      tail->next = node;
      tail = node;
    }
  }
  return chunked_string{head};
}

} // namespace

event_ptr event::with_message(std::string_view msg, keep_timestamp_t) const {
  // Note: can't use make_counted here because the constructor is private.
  auto copy = event_ptr{new event, false};
  auto* resource = &copy->resource_;
  copy->level_ = level_;
  copy->component_ = component_;
  copy->line_number_ = line_number_;
  copy->file_name_ = file_name_;
  copy->function_name_ = function_name_;
  copy->aid_ = aid_;
  copy->timestamp_ = timestamp_;
  copy->tid_ = tid_;
  copy->message_ = chunked_string{deep_copy_to_node(resource, msg)};
  auto fields_builder = event_fields_builder{resource};
  for (auto field : fields()) {
    auto add = [&fields_builder, key = field.key](const auto& val) {
      fields_builder.field(key, val);
    };
    std::visit(add, field.value);
  }
  copy->first_field_ = fields_builder.build().head;
  return copy;
}

event_ptr event::with_message(std::string_view msg) const {
  auto copy = with_message(msg, keep_timestamp);
  copy->timestamp_ = make_timestamp();
  return copy;
}

event_ptr event::make(unsigned level, std::string_view component,
                      const detail::source_location& loc, caf::actor_id aid,
                      std::string_view msg) {
  using chunk_node = chunked_string::node_type;
  auto event = make(level, component, loc, aid);
  auto* res = &event->resource_;
  auto* buf = allocator_t<chunk_node>{res}.allocate(1);
  auto* node = new (buf) chunk_node{deep_copy_impl(res, msg), nullptr};
  event->message_ = chunked_string{node};
  return event;
}

event_ptr event::make(unsigned level, std::string_view component,
                      const detail::source_location& loc, caf::actor_id aid) {
  return make_counted<event>(level, component, loc, aid);
}

event_fields_builder::event_fields_builder(resource_type* resource) noexcept {
  if (resource)
    new (&fields_) list_type(resource);
}

void event_fields_builder::field(std::string_view key, chunked_string str) {
  auto& field = fields_.emplace_back(std::string_view{}, std::nullopt);
  field.key = deep_copy(key);
  field.value = deep_copy_impl(resource(), str);
}

void event_fields_builder::field(std::string_view key, event::field_list list) {
  auto& field = fields_.emplace_back(std::string_view{}, std::nullopt);
  field.key = deep_copy(key);
  auto nested = event_fields_builder{resource()};
  for (auto field : list) {
    auto add = [&nested, key = field.key](const auto& val) {
      nested.field(key, val);
    };
    std::visit(add, field.value);
  }
  field.value = nested.build();
}

std::string_view event_fields_builder::deep_copy(std::string_view str) {
  return deep_copy_impl(resource(), str);
}

void event_sender::send() && {
  if (logger_) {
    event_->first_field_ = fields_.build().head;
    logger_->do_log(std::move(event_));
  }
}

} // namespace caf::log
