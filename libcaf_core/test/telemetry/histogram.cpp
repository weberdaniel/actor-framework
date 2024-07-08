// This file is part of CAF, the C++ Actor Framework. See the file LICENSE in
// the main distribution directory for license terms and copyright or visit
// https://github.com/actor-framework/actor-framework/blob/master/LICENSE.

#define CAF_SUITE telemetry.histogram

#include "caf/telemetry/histogram.hpp"

#include "core-test.hpp"

#include <cmath>
#include <limits>

#include "caf/telemetry/gauge.hpp"

using namespace caf;
using namespace caf::telemetry;

CAF_TEST(double histograms use infinity for the last bucket) {
  dbl_histogram h1{.1, .2, .4, .8};
  CHECK_EQ(h1.buckets().size(), 5u);
  CHECK_EQ(h1.buckets().front().upper_bound, .1);
  CHECK(std::isinf(h1.buckets().back().upper_bound));
  CHECK_EQ(h1.sum(), 0.0);
}

CAF_TEST(integer histograms use int_max for the last bucket) {
  using limits = std::numeric_limits<int64_t>;
  int_histogram h1{1, 2, 4, 8};
  CHECK_EQ(h1.buckets().size(), 5u);
  CHECK_EQ(h1.buckets().front().upper_bound, 1);
  CHECK_EQ(h1.buckets().back().upper_bound, limits::max());
  CHECK_EQ(h1.sum(), 0);
}

CAF_TEST(histograms aggregate to buckets and keep a sum) {
  int_histogram h1{2, 4, 8};
  for (int64_t value = 1; value < 11; ++value)
    h1.observe(value);
  auto buckets = h1.buckets();
  CAF_REQUIRE_EQUAL(buckets.size(), 4u);
  CHECK_EQ(buckets[0].count.value(), 2); // 1, 2
  CHECK_EQ(buckets[1].count.value(), 2); // 3, 4
  CHECK_EQ(buckets[2].count.value(), 4); // 5, 6, 7, 8
  CHECK_EQ(buckets[3].count.value(), 2); // 9, 10
  CHECK_EQ(h1.sum(), 55);
}
