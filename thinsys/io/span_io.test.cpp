#include <string_view>

#include <catch2/catch_test_macros.hpp>

#include <thinsys/io/span_io.hpp>

static_assert(io::input<std::span<const std::byte>>);
static_assert(io::input<std::span<std::byte>>);
static_assert(!io::input<std::span<int>>);
static_assert(!io::input<std::span<char>>);
static_assert(!io::input<std::span<const char>>);

static_assert(io::output<std::span<std::byte>>);

using namespace std::literals;

SCENARIO("read from span") {
  GIVEN("some data to read throug span") {
    const auto data = "qwe rty"sv;
    auto in = std::as_bytes(std::span{data});
    std::array<char, 5> buf;

    WHEN("reading less then data size") {
      const auto sz = io::read(in, std::as_writable_bytes(std::span{buf}));
      THEN("requested ammount is read") { REQUIRE(sz == buf.size()); }
      THEN("start of data is read") {
        REQUIRE(std::string_view{buf} == "qwe r");
      }

      AND_WHEN("reading rest of bytes") {
        const auto sz = io::read(in, std::as_writable_bytes(std::span{buf}));
        THEN("remaining ammount is read") {
          REQUIRE(sz == data.size() - buf.size());
        }
        THEN("tail of data is read") {
          REQUIRE(std::string_view{buf}.substr(0, sz) == "ty");
        }
      }
    }
  }
}
