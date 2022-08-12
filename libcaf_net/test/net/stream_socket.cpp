// This file is part of CAF, the C++ Actor Framework. See the file LICENSE in
// the main distribution directory for license terms and copyright or visit
// https://github.com/actor-framework/actor-framework/blob/master/LICENSE.

#define CAF_SUITE net.stream_socket

#include "caf/net/stream_socket.hpp"

#include "net-test.hpp"

#include "caf/byte.hpp"
#include "caf/byte_buffer.hpp"
#include "caf/span.hpp"

using namespace caf;
using namespace caf::net;

namespace {

std::byte operator"" _b(unsigned long long x) {
  return static_cast<std::byte>(x);
}

} // namespace

namespace {

struct fixture {
  fixture() : rd_buf(124) {
    std::tie(first, second) = unbox(make_stream_socket_pair());
    CAF_REQUIRE_EQUAL(nonblocking(first, true), caf::none);
    CAF_REQUIRE_EQUAL(nonblocking(second, true), caf::none);
    CAF_REQUIRE_NOT_EQUAL(unbox(send_buffer_size(first)), 0u);
    CAF_REQUIRE_NOT_EQUAL(unbox(send_buffer_size(second)), 0u);
  }

  ~fixture() {
    close(first);
    close(second);
  }

  stream_socket first;
  stream_socket second;
  byte_buffer rd_buf;
};

} // namespace

CAF_TEST_FIXTURE_SCOPE(network_socket_tests, fixture)

CAF_TEST(invalid socket) {
  stream_socket x;
  CHECK_EQ(keepalive(x, true), sec::network_syscall_failed);
  CHECK_EQ(nodelay(x, true), sec::network_syscall_failed);
  CHECK_EQ(allow_sigpipe(x, true), sec::network_syscall_failed);
}

CAF_TEST(read on empty sockets) {
  CHECK_LE(read(first, rd_buf), 0);
  CHECK(last_socket_error_is_temporary());
  CHECK_LE(read(second, rd_buf), 0);
  CHECK(last_socket_error_is_temporary());
}

CAF_TEST(transfer data from first to second socket) {
  byte_buffer wr_buf{1_b, 2_b, 4_b, 8_b, 16_b, 32_b, 64_b};
  CAF_MESSAGE("transfer data from first to second socket");
  CHECK_EQ(static_cast<size_t>(write(first, wr_buf)), wr_buf.size());
  CHECK_EQ(static_cast<size_t>(read(second, rd_buf)), wr_buf.size());
  CHECK(std::equal(wr_buf.begin(), wr_buf.end(), rd_buf.begin()));
  rd_buf.assign(rd_buf.size(), std::byte{0});
}

CAF_TEST(transfer data from second to first socket) {
  byte_buffer wr_buf{1_b, 2_b, 4_b, 8_b, 16_b, 32_b, 64_b};
  CHECK_EQ(static_cast<size_t>(write(second, wr_buf)), wr_buf.size());
  CHECK_EQ(static_cast<size_t>(read(first, rd_buf)), wr_buf.size());
  CHECK(std::equal(wr_buf.begin(), wr_buf.end(), rd_buf.begin()));
}

CAF_TEST(shut down first socket and observe shutdown on the second one) {
  close(first);
  CHECK_EQ(read(second, rd_buf), 0);
  first.id = invalid_socket_id;
}

CAF_TEST(transfer data using multiple buffers) {
  byte_buffer wr_buf_1{1_b, 2_b, 4_b};
  byte_buffer wr_buf_2{8_b, 16_b, 32_b, 64_b};
  byte_buffer full_buf;
  full_buf.insert(full_buf.end(), wr_buf_1.begin(), wr_buf_1.end());
  full_buf.insert(full_buf.end(), wr_buf_2.begin(), wr_buf_2.end());
  CHECK_EQ(static_cast<size_t>(
             write(second, {make_span(wr_buf_1), make_span(wr_buf_2)})),
           full_buf.size());
  CHECK_EQ(static_cast<size_t>(read(first, rd_buf)), full_buf.size());
  CHECK(std::equal(full_buf.begin(), full_buf.end(), rd_buf.begin()));
}

CAF_TEST_FIXTURE_SCOPE_END()
