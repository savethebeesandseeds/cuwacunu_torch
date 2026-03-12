// SPDX-License-Identifier: MIT
#pragma once

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <string>

#include <fcntl.h>
#include <sys/file.h>
#include <unistd.h>

namespace cuwacunu::hero::runtime_lock {

inline constexpr const char* kLockPathEnv =
    "CUWACUNU_HERO_RUNTIME_LOCK_PATH";
inline constexpr const char* kDefaultLockPath =
    "/tmp/cuwacunu.hero.runtime.lock";

[[nodiscard]] inline std::string global_lock_path() {
  const char* env = std::getenv(kLockPathEnv);
  if (env == nullptr || *env == '\0') return std::string(kDefaultLockPath);
  return std::string(env);
}

struct scoped_lock_t {
  int fd{-1};
  std::string path{};

  scoped_lock_t() = default;
  scoped_lock_t(const scoped_lock_t&) = delete;
  scoped_lock_t& operator=(const scoped_lock_t&) = delete;

  scoped_lock_t(scoped_lock_t&& other) noexcept
      : fd(other.fd), path(std::move(other.path)) {
    other.fd = -1;
  }
  scoped_lock_t& operator=(scoped_lock_t&& other) noexcept {
    if (this == &other) return *this;
    release();
    fd = other.fd;
    path = std::move(other.path);
    other.fd = -1;
    return *this;
  }

  ~scoped_lock_t() { release(); }

  void release() noexcept {
    if (fd < 0) return;
    (void)::flock(fd, LOCK_UN);
    (void)::close(fd);
    fd = -1;
  }
};

[[nodiscard]] inline bool acquire(scoped_lock_t* out, std::string* error = nullptr) {
  if (error) error->clear();
  if (!out) {
    if (error) *error = "lock output pointer is null";
    return false;
  }
  out->release();
  out->path = global_lock_path();
  if (out->path.empty()) {
    if (error) *error = "global lock path resolved empty";
    return false;
  }

  std::error_code ec{};
  const std::filesystem::path lock_path_fs(out->path);
  if (lock_path_fs.has_parent_path()) {
    std::filesystem::create_directories(lock_path_fs.parent_path(), ec);
    if (ec) {
      if (error) {
        *error = "failed to create lock directory: " +
                 lock_path_fs.parent_path().string();
      }
      return false;
    }
  }

  const int fd = ::open(out->path.c_str(), O_CREAT | O_RDWR, 0600);
  if (fd < 0) {
    if (error) {
      *error = "failed to open global lock file '" + out->path + "': " +
               std::strerror(errno);
    }
    return false;
  }

  if (::flock(fd, LOCK_EX) != 0) {
    const int saved_errno = errno;
    (void)::close(fd);
    if (error) {
      *error = "failed to acquire global lock '" + out->path + "': " +
               std::strerror(saved_errno);
    }
    return false;
  }

  out->fd = fd;
  return true;
}

}  // namespace cuwacunu::hero::runtime_lock
