// This file is part of CAF, the C++ Actor Framework. See the file LICENSE in
// the main distribution directory for license terms and copyright or visit
// https://github.com/actor-framework/actor-framework/blob/main/LICENSE.

#pragma once

#include "caf/config.hpp"
#include "caf/detail/parser/chars.hpp"
#include "caf/detail/parser/read_bool.hpp"
#include "caf/detail/parser/read_number_or_timespan.hpp"
#include "caf/detail/parser/read_string.hpp"
#include "caf/detail/parser/read_uri.hpp"
#include "caf/detail/scope_guard.hpp"
#include "caf/pec.hpp"
#include "caf/uri_builder.hpp"

#include <ctype.h>

#include <stack>

CAF_PUSH_UNUSED_LABEL_WARNING

#include "caf/detail/parser/fsm.hpp"

namespace caf::detail::parser {

// Example input:
//
// section1 {
//   value1 = 123
//   value2 = "string"
//   subsection1 = {
//     value3 = 1.23
//     value4 = 4e20
//   }
// }
// section2 {
//   value5 = 'atom'
//   value6 = [1, 'two', "three", {
//     a = "b",
//     b = "c",
//   }]
// }
//

template <class State, class Consumer>
void read_config_comment(State& ps, Consumer&&) {
  // clang-format off
  start();
  term_state(init) {
    transition(done, '\n')
    transition(had_carriage_return, '\r')
    transition(init)
  }
  state(had_carriage_return) {
    transition(done, '\n')
  }
  term_state(done) {
    // nop
  }
  fin();
  // clang-format on
}

template <class State, class Consumer, class InsideList = std::false_type>
void read_config_value(State& ps, Consumer&& consumer,
                       InsideList inside_list = {});

template <class State, class Consumer>
void read_config_list(State& ps, Consumer&& consumer) {
  // clang-format off
  start();
  state(init) {
    epsilon(before_value)
  }
  state(before_value) {
    transition(before_value, whitespace_chars)
    transition(done, ']', consumer.end_list())
    fsm_epsilon(read_config_comment(ps, consumer), before_value, '#')
    fsm_epsilon(read_config_value(ps, consumer, std::true_type{}), after_value)
  }
  state(after_value) {
    transition(after_value, whitespace_chars)
    transition(before_value, ',')
    transition(done, ']', consumer.end_list())
    fsm_epsilon(read_config_comment(ps, consumer), after_value, '#')
  }
  term_state(done) {
    // nop
  }
  fin();
  // clang-format on
}

// Like read_config_list, but without surrounding '[]'.
template <class State, class Consumer>
void lift_config_list(State& ps, Consumer&& consumer) {
  // clang-format off
  start();
  state(init) {
    epsilon(before_value)
  }
  term_state(before_value) {
    transition(before_value, whitespace_chars)
    fsm_epsilon(read_config_comment(ps, consumer), before_value, '#')
    fsm_epsilon(read_config_value(ps, consumer, std::true_type{}), after_value)
  }
  term_state(after_value) {
    transition(after_value, whitespace_chars)
    transition(before_value, ',')
    fsm_epsilon(read_config_comment(ps, consumer), after_value, '#')
  }
  fin();
  // clang-format on
}

/// Reads a dictionary of key-value pairs.
/// @param ps The parser state.
/// @param consumer The consumer to invoke for each key-value pair.
/// @param after_dot Whether the parser is currently after a dot in a nested
///                  key name.
template <bool Nested = true, class State, class Consumer>
void read_config_map(State& ps, Consumer&& consumer, bool after_dot = false) {
  std::string key;
  auto alnum_or_dash = [](char x) {
    return isalnum(x) || x == '-' || x == '_';
  };
  auto set_key = [&consumer, &key] {
    std::string tmp;
    tmp.swap(key);
    consumer.key(std::move(tmp));
  };
  auto recurse = [&set_key, &consumer]() -> decltype(auto) {
    set_key();
    return consumer.begin_map();
  };
  // clang-format off
  start();
  // Input may not be empty if we read a nested key.
  unstable_state(init) {
    epsilon_if(after_dot, await_nested_key_name)
    epsilon(after_init)
  }
  term_state(after_init) {
    epsilon(await_key_name)
  }
  state(await_key_name) {
    transition(await_key_name, whitespace_chars)
    fsm_epsilon(read_config_comment(ps, consumer), await_key_name, '#')
    fsm_epsilon(read_string(ps, key), await_assignment, quote_marks)
    transition(read_key_name, alnum_or_dash, key = ch)
    transition_if(Nested, done, '}', consumer.end_map())
  }
  // Reads a key of a "key=value" line.
  state(read_key_name) {
    transition(read_key_name, alnum_or_dash, key += ch)
    fsm_transition(read_config_map(ps, recurse(), true), after_value, '.')
    epsilon(await_assignment)
  }
  state(await_nested_key_name) {
    transition(read_key_name, alnum_or_dash, key = ch)
    fsm_epsilon(read_string(ps, key), await_assignment, quote_marks)
  }
  // Reads the assignment operator in a "key=value" line.
  state(await_assignment) {
    fsm_transition(read_config_map(ps, recurse(), true), after_value, '.')
    transition(await_assignment, " \t")
    transition(await_value, "=:", set_key())
    epsilon(await_value, '{', set_key())
  }
  // Reads the value in a "key=value" line.
  state(await_value) {
    transition(await_value, " \t")
    fsm_epsilon(read_config_value(ps, consumer), after_value)
  }
  // Waits for end-of-line after reading a value. When called to read a single
  // value, stop at this point to return to the caller.
  unstable_state(after_value) {
    epsilon_if(after_dot, done, any_char, consumer.end_map())
    transition(after_value, " \t")
    transition(had_carriage_return, "\r")
    transition(had_newline, "\n")
    transition_if(!Nested, after_comma, ',')
    transition(await_key_name, ',')
    transition_if(Nested, done, '}', consumer.end_map())
    fsm_epsilon(read_config_comment(ps, consumer), had_newline, '#')
    epsilon_if(!Nested, done)
    epsilon(unexpected_end_of_input)
  }
  // Handle Windows-style line endings. A carriage return must be followed by
  // a newline (line feed) character.
  state(had_carriage_return) {
    transition(had_newline, "\n")
  }
  // Allows users to skip the ',' for separating key/value pairs
  unstable_state(had_newline) {
    transition(had_newline, " \t\n")
    transition(had_carriage_return, "\r")
    transition(await_key_name, ',')
    transition_if(Nested, done, '}', consumer.end_map())
    fsm_epsilon(read_config_comment(ps, consumer), had_newline, '#')
    fsm_epsilon(read_string(ps, key), await_assignment, quote_marks)
    epsilon(read_key_name, alnum_or_dash)
    epsilon_if(!Nested, done)
    epsilon(unexpected_end_of_input)
  }
  term_state(after_comma) {
    epsilon(await_key_name)
  }
  state(unexpected_end_of_input) {
    // no transitions, only needed for the unstable states
  }
  term_state(done) {
    //nop
  }
  fin();
  // clang-format on
}

template <class State, class Consumer>
void read_config_uri(State& ps, Consumer&& consumer) {
  uri_builder builder;
  // clang-format off
  start();
  state(init) {
    transition(init, whitespace_chars)
    transition(before_uri, '<')
  }
  state(before_uri) {
    transition(before_uri, whitespace_chars)
    fsm_epsilon(read_uri(ps, builder), after_uri)
  }
  state(after_uri) {
    transition(after_uri, whitespace_chars)
    transition(done, '>')
  }
  term_state(done) {
    // nop
  }
  fin();
  // clang-format on
  if (ps.code <= pec::trailing_character)
    consumer.value(builder.make());
}

template <class State, class Consumer, class InsideList>
void read_config_value(State& ps, Consumer&& consumer, InsideList inside_list) {
  // clang-format off
  start();
  state(init) {
    fsm_epsilon(read_string(ps, consumer), done, quote_marks)
    fsm_epsilon(read_number(ps, consumer), done, '.')
    fsm_epsilon(read_bool(ps, consumer), done, "ft")
    fsm_epsilon(read_number_or_timespan(ps, consumer, inside_list),
                done, "0123456789+-")
    fsm_epsilon(read_config_uri(ps, consumer), done, '<')
    fsm_transition(read_config_list(ps, consumer.begin_list()), done, '[')
    fsm_transition(read_config_map(ps, consumer.begin_map()), done, '{')
  }
  term_state(done) {
    // nop
  }
  fin();
  // clang-format on
}

template <class State, class Consumer>
void read_config(State& ps, Consumer&& consumer) {
  auto key_char = [](char x) {
    return isalnum(x) || x == '-' || x == '_' || x == '"';
  };
  // clang-format off
  start();
  // Checks whether there's a top-level '{'.
  term_state(init) {
    transition(init, whitespace_chars)
    fsm_epsilon(read_config_comment(ps, consumer), init, '#')
    fsm_transition(read_config_map<false>(ps, consumer),
                   await_closing_brace, '{')
    fsm_epsilon(read_config_map<false>(ps, consumer), init, key_char)
  }
  state(await_closing_brace) {
    transition(await_closing_brace, whitespace_chars)
    fsm_epsilon(read_config_comment(ps, consumer), await_closing_brace, '#')
    transition(done, '}')
  }
  term_state(done) {
    transition(done, whitespace_chars)
    fsm_epsilon(read_config_comment(ps, consumer), done, '#')
  }
  fin();
  // clang-format on
}

} // namespace caf::detail::parser

#include "caf/detail/parser/fsm_undef.hpp"

CAF_POP_WARNINGS
