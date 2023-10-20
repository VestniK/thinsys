#pragma once

#include <algorithm>
#include <span>

#include <thinsys/io/input.hpp>
#include <thinsys/io/output.hpp>

template <>
size_t io::input_traits<std::span<const std::byte>>::read(
    std::span<const std::byte>& in, std::span<std::byte> dest,
    std::error_code&) noexcept {
  const auto sz = std::min(in.size(), dest.size());
  std::ranges::copy(in.subspan(0, sz), dest.data());
  in = in.subspan(sz);
  return sz;
}

template <>
size_t io::input_traits<std::span<std::byte>>::read(std::span<std::byte>& in,
    std::span<std::byte> dest, std::error_code&) noexcept {
  const auto sz = std::min(in.size(), dest.size());
  std::ranges::copy(in.subspan(0, sz), dest.data());
  in = in.subspan(sz);
  return sz;
}

template <>
size_t io::output_traits<std::span<std::byte>>::write(std::span<std::byte>& out,
    std::span<const std::byte> buf, std::error_code&) noexcept {
  const auto sz = std::min(out.size(), buf.size());
  std::ranges::copy(buf.subspan(0, sz), out.data());
  out = out.subspan(sz);
  return sz;
}
