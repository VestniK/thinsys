#pragma once

#include <fcntl.h>
#include <sys/file.h>
#include <sys/mman.h>
#include <unistd.h>

#include <filesystem>
#include <span>
#include <system_error>
#include <utility>

#include <thinsys/io/input.hpp>
#include <thinsys/io/output.hpp>

namespace fs = std::filesystem;

namespace thinsys::io {

class file_descriptor {
  static constexpr int invalid = -1;

public:
  using native_handle_t = int;

  constexpr file_descriptor() noexcept = default;
  constexpr explicit file_descriptor(native_handle_t fd) noexcept : fd_(fd) {}

  file_descriptor(const file_descriptor&) = delete;
  file_descriptor& operator=(const file_descriptor&) = delete;

  constexpr file_descriptor(file_descriptor&& rhs) noexcept : fd_(rhs.fd_) {
    rhs.fd_ = invalid;
  }
  constexpr file_descriptor& operator=(file_descriptor&& rhs) noexcept {
    if (fd_ >= 0)
      ::close(fd_);
    fd_ = rhs.fd_;
    rhs.fd_ = invalid;
    return *this;
  }

  ~file_descriptor() noexcept {
    if (fd_ >= 0)
      ::close(fd_);
  }

  void close() noexcept {
    if (fd_ >= 0)
      ::close(std::exchange(fd_, invalid));
  }
  constexpr explicit operator bool() const noexcept { return fd_ != invalid; }
  constexpr native_handle_t native_handle() const noexcept { return fd_; }
  constexpr native_handle_t release() noexcept {
    return std::exchange(fd_, invalid);
  }

  void lock() { flock(LOCK_EX); }
  bool try_lock() { return flock_nb(LOCK_EX); }
  void lock_shared() { flock(LOCK_SH); }
  bool try_lock_shared() { return flock_nb(LOCK_SH); }
  void unlock() { flock(LOCK_UN); }
  void unlock_shared() { flock(LOCK_UN); }

private:
  void flock(int op) {
    int res;
    do
      res = ::flock(fd_, op);
    while (res != 0 && errno == EINTR);
    if (res != 0)
      throw std::system_error{errno, std::system_category(), "flock"};
  }

  bool flock_nb(int op) {
    int res;
    do
      res = ::flock(fd_, op | LOCK_NB);
    while (res != 0 && errno == EINTR);
    return res == 0               ? true
           : errno == EWOULDBLOCK ? false
                                  : throw std::system_error{
                                        errno, std::system_category(), "flock"};
  }

private:
  native_handle_t fd_ = invalid;
};

enum class mode : int {
  create = O_CREAT,
  write_only = O_WRONLY,
  read_only = O_RDONLY,
  read_write = O_RDWR,
  truncate = O_TRUNC,
  tmpfile = O_TMPFILE,
  ndelay = O_NDELAY,
  nonblock = O_NONBLOCK,
};
constexpr mode operator|(mode lhs, mode rhs) noexcept {
  return static_cast<mode>(static_cast<int>(lhs) | static_cast<int>(rhs));
}

constexpr fs::perms default_perms =
    fs::perms::owner_read | fs::perms::owner_write | fs::perms::group_read |
    fs::perms::others_read;

inline file_descriptor open(
    const fs::path& path, mode flags, fs::perms perms = default_perms) {
  int fd = -1;
  do
    fd = ::open(path.c_str(), static_cast<int>(flags), perms);
  while (fd < 0 && errno == EINTR);
  if (fd < 0)
    throw std::system_error{
        errno, std::system_category(), "open " + path.string()};
  return file_descriptor{fd};
}
inline file_descriptor open_anonymous(
    const fs::path& dir, mode flags, fs::perms perms = default_perms) {
  return open(dir, flags | mode::tmpfile, perms);
}

template <>
inline size_t input_traits<file_descriptor>::read(file_descriptor& fd,
    std::span<std::byte> buf, std::error_code& ec) noexcept {
  ssize_t res;
  do
    res = ::read(fd.native_handle(), buf.data(), buf.size());
  while (res < 0 && errno == EINTR);
  if (res < 0) {
    ec = {errno, std::system_category()};
    return 0;
  }
  return static_cast<size_t>(res);
}

template <>
inline size_t output_traits<file_descriptor>::write(file_descriptor& fd,
    std::span<const std::byte> data, std::error_code& ec) noexcept {
  ssize_t res;
  do
    res = ::write(fd.native_handle(), data.data(), data.size());
  while (res < 0 && errno == EINTR);
  if (res < 0) {
    ec = {errno, std::system_category()};
    return 0;
  }
  return static_cast<size_t>(res);
}

inline void sync(const file_descriptor& fd, std::error_code& ec) noexcept {
  int res;
  do
    res = ::fsync(fd.native_handle());
  while (res < 0 && errno == EINTR);
  if (res < 0)
    ec = {errno, std::system_category()};
}
inline void sync(const file_descriptor& fd) {
  std::error_code ec;
  sync(fd, ec);
  if (ec)
    throw std::system_error{ec, "fsync"};
}

inline void truncate(const file_descriptor& fd, std::streamoff off,
    std::error_code& ec) noexcept {
  int res;
  do
    res = ::ftruncate(fd.native_handle(), static_cast<off_t>(off));
  while (res < 0 && errno == EINTR);
  if (res < 0)
    ec = {errno, std::system_category()};
}
inline void truncate(const file_descriptor& fd, std::streamoff off) {
  std::error_code ec;
  truncate(fd, off, ec);
  if (ec)
    throw std::system_error{ec, "ftruncate"};
}

enum class seek_whence { set = SEEK_SET, cur = SEEK_CUR, end = SEEK_END };
inline std::streamoff seek(const file_descriptor& fd, std::streamoff off,
    seek_whence whence, std::error_code ec) noexcept {
  const std::streamoff res =
      ::lseek(fd.native_handle(), off, std::to_underlying(whence));
  if (res < 0)
    ec = std::error_code{errno, std::system_category()};
  return res;
}

inline std::streamoff seek(
    const file_descriptor& fd, std::streamoff off, seek_whence whence) {
  std::error_code ec;
  const auto res = seek(fd, off, whence, ec);
  if (ec)
    throw std::system_error{ec, "lseek"};
  return res;
}

class transactional_file {
public:
  transactional_file() noexcept = default;
  transactional_file(
      const fs::path& path, mode mode, fs::perms perms = io::default_perms)
      : fd_{io::open_anonymous(path.parent_path(), mode, perms)},
        dest_path_{path} {}

  operator const file_descriptor&() const noexcept { return fd_; }

  void commit() {
    sync(fd_);
    std::array<char, 64> buf;
    ::snprintf(buf.data(), buf.size(), "/proc/self/fd/%d",
        static_cast<int>(fd_.native_handle()));
    if (::linkat(AT_FDCWD, buf.data(), AT_FDCWD, dest_path_.c_str(),
            AT_SYMLINK_FOLLOW) < 0)
      throw std::system_error{errno, std::system_category(), "linkat"};
  }

private:
  file_descriptor fd_;
  fs::path dest_path_;
};

} // namespace thinsys::io
