#include <string_view>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_range_equals.hpp>

#include <thinsys/io/span_io.hpp>

namespace thinsys::io {

using Catch::Matchers::RangeEquals;

static_assert(input<std::span<const std::byte>>);
static_assert(input<std::span<std::byte>>);
static_assert(!input<std::span<int>>);
static_assert(!input<std::span<char>>);
static_assert(!input<std::span<const char>>);

static_assert(output<std::span<std::byte>>);

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

SCENARIO("write to span") {
  GIVEN("output buffer and span pointing to it") {
    std::vector<std::byte> buf;
    buf.resize(64);
    std::span<std::byte> out{buf};

    WHEN("byte data is written") {
      std::array<std::byte, 4> data = {
          std::byte{0x1f}, std::byte{0x8b}, std::byte{0x08}, std::byte{0x00}};
      io::write(out, data);

      THEN("span is shrinked by written number of bytes") {
        REQUIRE(out.size() == buf.size() - data.size());
      }

      THEN("span points to data after last write position") {
        REQUIRE(out.data() == buf.data() + data.size());
      }

      THEN("data is written to buf") {
        REQUIRE_THAT(std::span{buf}.subspan(0, data.size()), RangeEquals(data));
      }
    }

    WHEN("string view is written") {
      io::write(out, "hello world"sv);

      THEN("data is actually written to buf") {
        std::string_view written{
            reinterpret_cast<const char*>(buf.data()), buf.size() - out.size()};
        REQUIRE(written == "hello world");
      }
    }
  }
}

} // namespace thinsys::io
