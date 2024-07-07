// This file is part of CAF, the C++ Actor Framework. See the file LICENSE in
// the main distribution directory for license terms and copyright or visit
// https://github.com/actor-framework/actor-framework/blob/main/LICENSE.

#include "caf/actor_system_config.hpp"

#include "caf/test/approx.hpp"
#include "caf/test/scenario.hpp"
#include "caf/test/test.hpp"

#include "caf/log/test.hpp"

#include <deque>
#include <list>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace caf;
using namespace std::literals;

// Checks whether both a synced variable and the corresponding entry in
// content(cfg) are equal to `value`.
#define CHECK_SYNCED(var, ...)                                                 \
  do {                                                                         \
    using ref_value_type = std::decay_t<decltype(var)>;                        \
    ref_value_type value{__VA_ARGS__};                                         \
    if constexpr (std::is_arithmetic_v<decltype(var)>)                         \
      check_eq(var, test::approx{value});                                      \
    else                                                                       \
      check_eq(var, value);                                                    \
    if (auto maybe_val = get_as<decltype(var)>(cfg, #var)) {                   \
      if constexpr (std::is_arithmetic_v<decltype(var)>)                       \
        check_eq(*maybe_val, test::approx{value});                             \
      else                                                                     \
        check_eq(*maybe_val, value);                                           \
    } else {                                                                   \
      auto cv = get_if(std::addressof(cfg.content), #var);                     \
      fail("expected type {}, got {}",                                         \
           config_value::mapped_type_name<decltype(var)>(), cv->type_name());  \
    }                                                                          \
  } while (false)

// Checks whether an entry in content(cfg) is equal to `value`.
#define CHECK_TEXT_ONLY(type, var, value)                                      \
  check_eq(get_as<type>(cfg, #var), value)

#define ADD(var) add(var, #var, "...")

#define VAR(type)                                                              \
  auto some_##type = type{};                                                   \
  options("global").add(some_##type, "some_" #type, "...")

#define NAMED_VAR(type, name)                                                  \
  auto name = type{};                                                          \
  options("global").add(name, #name, "...")

namespace {

template <class T>
T unbox(caf::expected<T> x) {
  if (!x)
    test::runnable::current().fail("{}", to_string(x.error()));
  return std::move(*x);
}

timespan operator"" _ms(unsigned long long x) {
  return std::chrono::duration_cast<timespan>(std::chrono::milliseconds(x));
}

uri operator"" _u(const char* str, size_t size) {
  return unbox(make_uri(std::string_view{str, size}));
}

using string_list = std::vector<std::string>;

struct config : actor_system_config {
  config_option_adder options(std::string_view category) {
    return opt_group{custom_options_, category};
  }
};

struct fixture {
  config cfg;

  config_option_adder options(std::string_view category) {
    return cfg.options(category);
  }

  void parse(const char* file_content, string_list args = {}) {
    std::istringstream conf{file_content};
    if (auto err = cfg.parse(std::move(args), conf))
      test::runnable::current().fail("parse() failed: {}", err);
  }
};

WITH_FIXTURE(fixture) {

TEST("parsing - without CLI arguments") {
  auto text = "foo{\nbar=\"hello\"}";
  options("?foo").add<std::string>("bar,b", "some string parameter");
  parse(text);
  check(cfg.remainder().empty());
  check_eq(get_or(cfg, "foo.bar", ""), "hello");
  auto [argc, argv] = cfg.c_args_remainder();
  if (check_eq(argc, 1)) {
    check_eq(argv[0], cfg.program_name());
  }
}

TEST("parsing - without CLI cfg.remainder") {
  auto text = "foo{\nbar=\"hello\"}";
  options("?foo").add<std::string>("bar,b", "some string parameter");
  SECTION("CLI long name") {
    parse(text, {"--foo.bar=test"});
    check(cfg.remainder().empty());
    check_eq(get_or(cfg, "foo.bar", ""), "test");
  }
  SECTION("CLI abbreviated long name") {
    parse(text, {"--bar=test"});
    check(cfg.remainder().empty());
    check_eq(get_or(cfg, "foo.bar", ""), "test");
  }
  SECTION("CLI short name") {
    parse(text, {"-b", "test"});
    check(cfg.remainder().empty());
    check_eq(get_or(cfg, "foo.bar", ""), "test");
  }
  SECTION("CLI short name without whitespace") {
    parse(text, {"-btest"});
    check(cfg.remainder().empty());
    check_eq(get_or(cfg, "foo.bar", ""), "test");
  }
}

TEST("parsing - with CLI cfg.remainder") {
  auto text = "foo{\nbar=\"hello\"}";
  options("?foo").add<std::string>("bar,b", "some string parameter");
  parse(text, {"-b", "test", "hello", "world"});
  check_eq(get_or(cfg, "foo.bar", ""), "test");
  auto remainder = cfg.remainder();
  if (check_eq(remainder.size(), 2u)) {
    check_eq(remainder[0], "hello");
    check_eq(remainder[1], "world");
    auto [argc, argv] = cfg.c_args_remainder();
    if (check_eq(argc, 3)) {
      check_eq(argv[0], cfg.program_name());
      check_eq(argv[1], remainder[0]);
      check_eq(argv[2], remainder[1]);
    }
  }
}

TEST("file input overrides defaults but CLI args always win") {
  const char* file_input = R"__(
    group1 {
      arg1 = 'foobar'
    }
    group2 {
      arg1 = 'hello world'
      arg2 = 2
    }
  )__";
  struct grp {
    std::string arg1 = "default";
    int arg2 = 42;
  };
  grp grp1;
  grp grp2;
  config_option_adder{cfg.custom_options(), "group1"}
    .add(grp1.arg1, "arg1", "")
    .add(grp1.arg2, "arg2", "");
  config_option_adder{cfg.custom_options(), "group2"}
    .add(grp2.arg1, "arg1", "")
    .add(grp2.arg2, "arg2", "");
  string_list args{"--group1.arg2=123", "--group2.arg1=bye"};
  std::istringstream input{file_input};
  auto err = cfg.parse(std::move(args), input);
  check_eq(err, error{});
  check_eq(grp1.arg1, "foobar");
  check_eq(grp1.arg2, 123);
  check_eq(grp2.arg1, "bye");
  check_eq(grp2.arg2, 2);
  settings res;
  put(res, "group1.arg1", "foobar");
  put(res, "group1.arg2", 123);
  put(res, "group2.arg1", "bye");
  put(res, "group2.arg2", 2);
  check_eq(content(cfg), res);
}

TEST("integers and integer containers options") {
  // Use a wild mess of "list-like" and "map-like" containers from the STL.
  using int_list = std::vector<int>;
  using int_list_list = std::list<std::deque<int>>;
  using int_map = std::unordered_map<std::string, int>;
  using int_list_map = std::map<std::string, std::unordered_set<int>>;
  using int_map_list = std::set<std::map<std::string, int>>;
  auto text = R"__(
    some_int = 42
    yet_another_int = 123
    some_int_list = [1, 2, 3]
    some_int_list_list = [[1, 2, 3], [4, 5, 6]]
    some_int_map = {a = 1, b = 2, c = 3}
    some_int_list_map = {a = [1, 2, 3], b = [4, 5, 6]}
    some_int_map_list = [{a = 1, b = 2, c = 3}, {d = 4, e = 5, f = 6}]
  )__";
  NAMED_VAR(int, some_other_int);
  VAR(int);
  VAR(int_list);
  VAR(int_list_list);
  VAR(int_map);
  VAR(int_list_map);
  VAR(int_map_list);
  parse(text, {"--some_other_int=23"});
  CHECK_SYNCED(some_int, 42);
  CHECK_SYNCED(some_other_int, 23);
  CHECK_TEXT_ONLY(int, yet_another_int, 123);
  CHECK_SYNCED(some_int_list, 1, 2, 3);
  CHECK_SYNCED(some_int_list_list, {1, 2, 3}, {4, 5, 6});
  CHECK_SYNCED(some_int_map, {{"a", 1}, {"b", 2}, {"c", 3}});
  CHECK_SYNCED(some_int_list_map, {{"a", {1, 2, 3}}, {"b", {4, 5, 6}}});
  CHECK_SYNCED(some_int_map_list, {{"a", 1}, {"b", 2}, {"c", 3}},
               {{"d", 4}, {"e", 5}, {"f", 6}});
}

TEST("basic and basic containers options") {
  using std::map;
  using std::string;
  using std::vector;
  using int_list = vector<int>;
  using bool_list = vector<bool>;
  using double_list = vector<double>;
  using timespan_list = vector<timespan>;
  using uri_list = vector<uri>;
  using string_list = vector<string>;
  using int_map = map<string, int>;
  using bool_map = map<string, bool>;
  using double_map = map<string, double>;
  using timespan_map = map<string, timespan>;
  using uri_map = map<string, uri>;
  using string_map = map<string, string>;
  auto text = R"__(
    some_int = 42
    some_bool = true
    some_double = 1e23
    some_timespan = 123ms
    some_uri = <foo:bar>
    some_string = "string"
    some_int_list = [1, 2, 3]
    some_bool_list = [false, true]
    some_double_list = [1., 2., 3.]
    some_timespan_list = [123ms, 234ms, 345ms]
    some_uri_list = [<foo:a>, <foo:b>, <foo:c>]
    some_string_list = ["a", "b", "c"]
    some_int_map = {a = 1, b = 2, c = 3}
    some_bool_map = {a = true, b = false}
    some_double_map = {a = 1., b = 2., c = 3.}
    some_timespan_map = {a = 123ms, b = 234ms, c = 345ms}
    some_uri_map = {a = <foo:a>, b = <foo:b>, c = <foo:c>}
    some_string_map = {a = "1", b = "2", c = "3"}
  )__";
  VAR(int);
  VAR(bool);
  VAR(double);
  VAR(timespan);
  VAR(uri);
  VAR(string);
  VAR(int_list);
  VAR(bool_list);
  VAR(double_list);
  VAR(timespan_list);
  VAR(uri_list);
  VAR(string_list);
  VAR(int_map);
  VAR(bool_map);
  VAR(double_map);
  VAR(timespan_map);
  VAR(uri_map);
  VAR(string_map);
  parse(text);
  log::test::debug("check primitive types");
  CHECK_SYNCED(some_int, 42);
  CHECK_SYNCED(some_bool, true);
  CHECK_SYNCED(some_double, 1e23);
  CHECK_SYNCED(some_timespan, 123_ms);
  CHECK_SYNCED(some_uri, "foo:bar"_u);
  CHECK_SYNCED(some_string, "string"s);
  log::test::debug("check list types");
  CHECK_SYNCED(some_int_list, 1, 2, 3);
  CHECK_SYNCED(some_bool_list, false, true);
  CHECK_SYNCED(some_double_list, 1., 2., 3.);
  CHECK_SYNCED(some_timespan_list, 123_ms, 234_ms, 345_ms);
  CHECK_SYNCED(some_uri_list, "foo:a"_u, "foo:b"_u, "foo:c"_u);
  CHECK_SYNCED(some_string_list, "a", "b", "c");
  log::test::debug("check dictionary types");
  CHECK_SYNCED(some_int_map, {"a", 1}, {"b", 2}, {"c", 3});
  CHECK_SYNCED(some_bool_map, {"a", true}, {"b", false});
  CHECK_SYNCED(some_double_map, {"a", 1.}, {"b", 2.}, {"c", 3.});
  CHECK_SYNCED(some_timespan_map, {"a", 123_ms}, {"b", 234_ms}, {"c", 345_ms});
  CHECK_SYNCED(some_uri_map, {"a", "foo:a"_u}, {"b", "foo:b"_u},
               {"c", "foo:c"_u});
  CHECK_SYNCED(some_string_map, {"a", "1"}, {"b", "2"}, {"c", "3"});
}

SCENARIO("config files allow both nested and dot-separated values") {
  GIVEN("the option my.answer.value") {
    config_option_adder{cfg.custom_options(), "my.answer"}
      .add<int32_t>("first", "the first answer")
      .add<int32_t>("second", "the second answer");
    std::vector<std::string> allowed_input_strings{
      "my { answer { first = 1, second = 2 } }",
      "my.answer { first = 1, second = 2 }",
      "my { answer.first = 1, answer.second = 2  }",
      "my.answer.first = 1, my.answer.second = 2",
      "my { answer { first = 1 }, answer.second = 2 }",
      "my { answer.first = 1, answer { second = 2} }",
      "my.answer.first = 1, my { answer { second = 2 } }",
    };
    auto make_result = [] {
      settings answer;
      answer["first"] = 1;
      answer["second"] = 2;
      settings my;
      my["answer"] = std::move(answer);
      settings result;
      result["my"] = std::move(my);
      return result;
    };
    auto result = make_result();
    for (const auto& input_string : allowed_input_strings) {
      WHEN("parsing the file input '" + input_string + "'") {
        std::istringstream input{input_string};
        auto err = cfg.parse(string_list{}, input);
        THEN("the actor system contains values for my.answer.(first|second)") {
          check_eq(err, error{});
          check_eq(get_or(cfg, "my.answer.first", -1), 1);
          check_eq(get_or(cfg, "my.answer.second", -1), 2);
          check_eq(content(cfg), result);
        }
      }
    }
  }
}

} // WITH_FIXTURE(fixture)

} // namespace
