// This file is part of CAF, the C++ Actor Framework. See the file LICENSE in
// the main distribution directory for license terms and copyright or visit
// https://github.com/actor-framework/actor-framework/blob/master/LICENSE.

#define CAF_SUITE load_inspector

#include "caf/load_inspector.hpp"

#include "core-test.hpp"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include "caf/load_inspector_base.hpp"
#include "caf/message.hpp"
#include "caf/span.hpp"
#include "caf/type_id.hpp"

#include "inspector-tests.hpp"

using namespace caf;

namespace {

struct testee : deserializer {
  std::string log;

  bool load_field_failed(std::string_view, sec code) {
    set_error(make_error(code));
    return false;
  }

  size_t indent = 0;

  void reset() {
    log.clear();
    indent = 0;
    set_error(error{});
  }

  void new_line() {
    log += '\n';
    log.insert(log.end(), indent, ' ');
  }

  bool fetch_next_object_type(type_id_t&) override {
    return false;
  }

  bool begin_object(type_id_t, std::string_view object_name) override {
    new_line();
    indent += 2;
    log += "begin object ";
    log.insert(log.end(), object_name.begin(), object_name.end());
    return true;
  }

  bool end_object() override {
    indent -= 2;
    new_line();
    log += "end object";
    return true;
  }

  bool begin_field(std::string_view name) override {
    new_line();
    indent += 2;
    log += "begin field ";
    log.insert(log.end(), name.begin(), name.end());
    return true;
  }

  bool begin_field(std::string_view name, bool& is_present) override {
    new_line();
    indent += 2;
    log += "begin optional field ";
    log.insert(log.end(), name.begin(), name.end());
    is_present = false;
    return true;
  }

  bool begin_field(std::string_view name, span<const type_id_t>,
                   size_t& type_index) override {
    new_line();
    indent += 2;
    log += "begin variant field ";
    log.insert(log.end(), name.begin(), name.end());
    type_index = 0;
    return true;
  }

  bool begin_field(std::string_view name, bool& is_present,
                   span<const type_id_t>, size_t&) override {
    new_line();
    indent += 2;
    log += "begin optional variant field ";
    log.insert(log.end(), name.begin(), name.end());
    is_present = false;
    return true;
  }

  bool end_field() override {
    indent -= 2;
    new_line();
    log += "end field";
    return true;
  }

  bool begin_tuple(size_t size) override {
    new_line();
    indent += 2;
    log += "begin tuple of size ";
    log += std::to_string(size);
    return true;
  }

  bool end_tuple() override {
    indent -= 2;
    new_line();
    log += "end tuple";
    return true;
  }

  bool begin_key_value_pair() override {
    new_line();
    indent += 2;
    log += "begin key-value pair";
    return true;
  }

  bool end_key_value_pair() override {
    indent -= 2;
    new_line();
    log += "end key-value pair";
    return true;
  }

  bool begin_sequence(size_t& size) override {
    size = 0;
    new_line();
    indent += 2;
    log += "begin sequence of size ";
    log += std::to_string(size);
    return true;
  }

  bool end_sequence() override {
    indent -= 2;
    new_line();
    log += "end sequence";
    return true;
  }

  bool begin_associative_array(size_t& size) override {
    size = 0;
    new_line();
    indent += 2;
    log += "begin associative array of size ";
    log += std::to_string(size);
    return true;
  }

  bool end_associative_array() override {
    indent -= 2;
    new_line();
    log += "end associative array";
    return true;
  }

  bool value(bool& x) override {
    new_line();
    log += "bool value";
    x = false;
    return true;
  }

  template <class T>
  bool primitive_value(T& x) {
    new_line();
    auto tn = type_name_v<T>;
    log.insert(log.end(), tn.begin(), tn.end());
    log += " value";
    x = T{};
    return true;
  }

  bool value(std::byte& x) override {
    new_line();
    log += "std::byte value";
    x = std::byte{};
    return true;
  }

  bool value(int8_t& x) override {
    return primitive_value(x);
  }

  bool value(uint8_t& x) override {
    return primitive_value(x);
  }

  bool value(int16_t& x) override {
    return primitive_value(x);
  }

  bool value(uint16_t& x) override {
    return primitive_value(x);
  }

  bool value(int32_t& x) override {
    return primitive_value(x);
  }

  bool value(uint32_t& x) override {
    return primitive_value(x);
  }

  bool value(int64_t& x) override {
    return primitive_value(x);
  }

  bool value(uint64_t& x) override {
    return primitive_value(x);
  }

  bool value(float& x) override {
    return primitive_value(x);
  }

  bool value(double& x) override {
    return primitive_value(x);
  }

  bool value(long double& x) override {
    return primitive_value(x);
  }

  bool value(std::string& x) override {
    return primitive_value(x);
  }

  bool value(std::u16string& x) override {
    return primitive_value(x);
  }

  bool value(std::u32string& x) override {
    return primitive_value(x);
  }

  bool value(span<std::byte> xs) override {
    new_line();
    log += "caf::span<caf::std::byte> value";
    for (auto& x : xs)
      x = std::byte{0};
    return true;
  }
};

struct fixture {
  testee f;
};

} // namespace

BEGIN_FIXTURE_SCOPE(fixture)

CAF_TEST(load inspectors can visit simple POD types) {
  point_3d p{1, 1, 1};
  CHECK_EQ(inspect(f, p), true);
  CHECK_EQ(p.x, 0);
  CHECK_EQ(p.y, 0);
  CHECK_EQ(p.z, 0);
  CHECK_EQ(f.log, R"_(
begin object point_3d
  begin field x
    int32_t value
  end field
  begin field y
    int32_t value
  end field
  begin field z
    int32_t value
  end field
end object)_");
}

CAF_TEST(load inspectors recurse into members) {
  line l{point_3d{1, 1, 1}, point_3d{1, 1, 1}};
  CHECK_EQ(inspect(f, l), true);
  CHECK_EQ(l.p1.x, 0);
  CHECK_EQ(l.p1.y, 0);
  CHECK_EQ(l.p1.z, 0);
  CHECK_EQ(l.p2.x, 0);
  CHECK_EQ(l.p2.y, 0);
  CHECK_EQ(l.p2.z, 0);
  CHECK_EQ(f.log, R"_(
begin object line
  begin field p1
    begin object point_3d
      begin field x
        int32_t value
      end field
      begin field y
        int32_t value
      end field
      begin field z
        int32_t value
      end field
    end object
  end field
  begin field p2
    begin object point_3d
      begin field x
        int32_t value
      end field
      begin field y
        int32_t value
      end field
      begin field z
        int32_t value
      end field
    end object
  end field
end object)_");
}

CAF_TEST(load inspectors support optional) {
  std::optional<int32_t> x;
  CHECK_EQ(f.apply(x), true);
  CHECK_EQ(f.log, R"_(
begin object anonymous
  begin optional field value
  end field
end object)_");
}

CAF_TEST(load inspectors support fields with fallbacks and invariants) {
  duration d{"minutes", 42};
  CHECK_EQ(inspect(f, d), true);
  CHECK_EQ(d.unit, "seconds");
  CHECK_EQ(d.count, 0.0);
  CHECK_EQ(f.log, R"_(
begin object duration
  begin optional field unit
  end field
  begin field count
    double value
  end field
end object)_");
}

CAF_TEST(load inspectors support fields with optional values) {
  person p{"Bruce Almighty", std::string{"776-2323"}};
  CHECK_EQ(inspect(f, p), true);
  CHECK_EQ(p.name, "");
  CHECK_EQ(p.phone, std::nullopt);
  CHECK_EQ(f.log, R"_(
begin object person
  begin field name
    std::string value
  end field
  begin optional field phone
  end field
end object)_");
}

CAF_TEST(load inspectors support fields with getters and setters) {
  foobar fb;
  fb.foo("hello");
  fb.bar("world");
  CHECK_EQ(inspect(f, fb), true);
  CHECK_EQ(fb.foo(), "");
  CHECK_EQ(fb.bar(), "");
  CHECK_EQ(f.log, R"_(
begin object foobar
  begin field foo
    std::string value
  end field
  begin field bar
    std::string value
  end field
end object)_");
}

CAF_TEST(load inspectors support variant fields) {
  dummy_message d;
  d.content = 42.0;
  CHECK(inspect(f, d));
  // Our dummy inspector resets variants to their first type.
  CHECK(holds_alternative<std::string>(d.content));
  CHECK_EQ(f.log, R"_(
begin object dummy_message
  begin variant field content
    std::string value
  end field
end object)_");
}

CAF_TEST(load inspectors support variant fields with fallbacks) {
  fallback_dummy_message d;
  using content_type = decltype(d.content);
  d.content = std::string{"hello world"};
  CHECK(inspect(f, d));
  CHECK_EQ(d.content, content_type{42.0});
  CHECK_EQ(f.log, R"_(
begin object fallback_dummy_message
  begin optional variant field content
  end field
end object)_");
}

CAF_TEST(load inspectors support nasty data structures) {
  nasty x;
  CHECK(inspect(f, x));
  CHECK_EQ(f.log, R"_(
begin object nasty
  begin field field_01
    int32_t value
  end field
  begin optional field field_02
  end field
  begin field field_03
    int32_t value
  end field
  begin optional field field_04
  end field
  begin optional field field_05
  end field
  begin optional field field_07
  end field
  begin variant field field_09
    std::string value
  end field
  begin optional variant field field_10
  end field
  begin variant field field_11
    std::string value
  end field
  begin optional variant field field_12
  end field
  begin field field_13
    begin tuple of size 2
      std::string value
      int32_t value
    end tuple
  end field
  begin optional field field_14
  end field
  begin field field_15
    begin tuple of size 2
      std::string value
      int32_t value
    end tuple
  end field
  begin optional field field_16
  end field
  begin field field_17
    int32_t value
  end field
  begin optional field field_18
  end field
  begin field field_19
    int32_t value
  end field
  begin optional field field_20
  end field
  begin optional field field_21
  end field
  begin optional field field_23
  end field
  begin variant field field_25
    std::string value
  end field
  begin optional variant field field_26
  end field
  begin variant field field_27
    std::string value
  end field
  begin optional variant field field_28
  end field
  begin field field_29
    begin tuple of size 2
      std::string value
      int32_t value
    end tuple
  end field
  begin optional field field_30
  end field
  begin field field_31
    begin tuple of size 2
      std::string value
      int32_t value
    end tuple
  end field
  begin optional field field_32
  end field
  begin optional variant field field_33
  end field
  begin optional field field_34
  end field
  begin optional variant field field_35
  end field
  begin optional field field_36
  end field
end object)_");
}

CAF_TEST(load inspectors support all basic STL types) {
  basics x;
  CHECK(inspect(f, x));
  CHECK_EQ(f.log, R"_(
begin object basics
  begin field v1
    begin object anonymous
    end object
  end field
  begin field v2
    int32_t value
  end field
  begin field v3
    begin tuple of size 4
      int32_t value
      int32_t value
      int32_t value
      int32_t value
    end tuple
  end field
  begin field v4
    begin tuple of size 2
      begin object dummy_message
        begin variant field content
          std::string value
        end field
      end object
      begin object dummy_message
        begin variant field content
          std::string value
        end field
      end object
    end tuple
  end field
  begin field v5
    begin tuple of size 2
      int32_t value
      int32_t value
    end tuple
  end field
  begin field v6
    begin tuple of size 2
      int32_t value
      begin object dummy_message
        begin variant field content
          std::string value
        end field
      end object
    end tuple
  end field
  begin field v7
    begin associative array of size 0
    end associative array
  end field
  begin field v8
    begin sequence of size 0
    end sequence
  end field
end object)_");
}

CAF_TEST(load inspectors support messages) {
  auto msg = make_message(1, "two", 3.0);
}

SCENARIO("load inspectors support apply with a getter and setter") {
  std::string baseline = R"_(
begin object line
  begin field p1
    begin object point_3d
      begin field x
        int32_t value
      end field
      begin field y
        int32_t value
      end field
      begin field z
        int32_t value
      end field
    end object
  end field
  begin field p2
    begin object point_3d
      begin field x
        int32_t value
      end field
      begin field y
        int32_t value
      end field
      begin field z
        int32_t value
      end field
    end object
  end field
end object)_";
  GIVEN("a line object") {
    WHEN("passing a void setter") {
      f.reset();
      auto x = line{{10, 10, 10}, {20, 20, 20}};
      auto get = [&x] { return x; };
      auto set = [&x](line val) { x = val; };
      THEN("the inspector overrides the state using the setter") {
        CHECK(f.apply(get, set));
        CHECK_EQ(f.log, baseline);
        line default_line{{0, 0, 0}, {0, 0, 0}};
        CHECK_EQ(x, default_line);
      }
    }
    WHEN("passing a setter returning true") {
      f.reset();
      auto x = line{{10, 10, 10}, {20, 20, 20}};
      auto get = [&x] { return x; };
      auto set = [&x](line val) {
        x = val;
        return true;
      };
      THEN("the inspector overrides the state using the setter") {
        CHECK(f.apply(get, set));
        CHECK_EQ(f.log, baseline);
        line default_line{{0, 0, 0}, {0, 0, 0}};
        CHECK_EQ(x, default_line);
      }
    }
    WHEN("passing a setter returning false") {
      f.reset();
      auto x = line{{10, 10, 10}, {20, 20, 20}};
      auto get = [&x] { return x; };
      auto set = [](line) { return false; };
      THEN("the inspection fails") {
        CHECK(!f.apply(get, set));
        CHECK_EQ(f.get_error(), sec::save_callback_failed);
      }
    }
    WHEN("passing a setter returning a default-constructed error") {
      f.reset();
      auto x = line{{10, 10, 10}, {20, 20, 20}};
      auto get = [&x] { return x; };
      auto set = [&x](line val) {
        x = val;
        return error{};
      };
      THEN("the inspector overrides the state using the setter") {
        CHECK(f.apply(get, set));
        CHECK_EQ(f.log, baseline);
        line default_line{{0, 0, 0}, {0, 0, 0}};
        CHECK_EQ(x, default_line);
      }
    }
    WHEN("passing a setter returning an error") {
      f.reset();
      auto x = line{{10, 10, 10}, {20, 20, 20}};
      auto get = [&x] { return x; };
      auto set = [](line) { return error{sec::runtime_error}; };
      THEN("the inspection fails") {
        CHECK(!f.apply(get, set));
        CHECK_EQ(f.get_error(), sec::runtime_error);
      }
    }
  }
}

SCENARIO("load inspectors support fields with a getter and setter") {
  std::string baseline = R"_(
begin object person
  begin field name
    std::string value
  end field
  begin optional field phone
  end field
end object)_";
  GIVEN("a person object") {
    WHEN("passing a name setter returning void") {
      f.reset();
      auto x = person{"John Doe", {}};
      auto get_name = [&x] { return x.name; };
      auto set_name = [&x](std::string val) { x.name = std::move(val); };
      THEN("the inspector overrides the state using the setter") {
        CHECK(f.object(x).fields(f.field("name", get_name, set_name),
                                 f.field("phone", x.phone)));
        CHECK_EQ(f.log, baseline);
        CHECK_EQ(x.name, "");
      }
    }
    WHEN("passing a name setter returning true") {
      f.reset();
      auto x = person{"John Doe", {}};
      auto get_name = [&x] { return x.name; };
      auto set_name = [&x](std::string val) {
        x.name = std::move(val);
        return true;
      };
      THEN("the inspector overrides the state using the setter") {
        CHECK(f.object(x).fields(f.field("name", get_name, set_name),
                                 f.field("phone", x.phone)));
        CHECK_EQ(f.log, baseline);
        CHECK_EQ(x.name, "");
      }
    }
    WHEN("passing a name setter returning false") {
      f.reset();
      auto x = person{"John Doe", {}};
      auto get_name = [&x] { return x.name; };
      auto set_name = [](std::string&&) { return false; };
      THEN("the inspection fails") {
        CHECK(!f.object(x).fields(f.field("name", get_name, set_name),
                                  f.field("phone", x.phone)));
        CHECK_EQ(f.get_error(), sec::field_value_synchronization_failed);
      }
    }
    WHEN("passing a name setter returning a default-constructed error") {
      f.reset();
      auto x = person{"John Doe", {}};
      auto get_name = [&x] { return x.name; };
      auto set_name = [&x](std::string val) {
        x.name = std::move(val);
        return error{};
      };
      THEN("the inspector overrides the state using the setter") {
        CHECK(f.object(x).fields(f.field("name", get_name, set_name),
                                 f.field("phone", x.phone)));
        CHECK_EQ(f.log, baseline);
        CHECK_EQ(x.name, "");
      }
    }
    WHEN("passing a name setter returning an error") {
      f.reset();
      auto x = person{"John Doe", {}};
      auto get_name = [&x] { return x.name; };
      auto set_name = [](std::string&&) { return error{sec::runtime_error}; };
      THEN("the inspection fails") {
        CHECK(!f.object(x).fields(f.field("name", get_name, set_name),
                                  f.field("phone", x.phone)));
        CHECK_EQ(f.get_error(), sec::runtime_error);
      }
    }
  }
}

SCENARIO("load inspectors support std::byte") {
  GIVEN("a struct with std::byte") {
    struct byte_test {
      std::byte v1 = std::byte{0};
      std::optional<std::byte> v2;
    };
    auto x = byte_test{};
    WHEN("inspecting the struct") {
      THEN("CAF treats std::byte like an unsigned integer") {
        CHECK(f.object(x).fields(f.field("v1", x.v1), f.field("v2", x.v2)));
        CHECK(!f.get_error());
        std::string baseline = R"_(
begin object anonymous
  begin field v1
    std::byte value
  end field
  begin optional field v2
  end field
end object)_";
        CHECK_EQ(f.log, baseline);
      }
    }
  }
}

END_FIXTURE_SCOPE()
