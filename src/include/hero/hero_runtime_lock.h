// SPDX-License-Identifier: MIT
#pragma once

#include <cctype>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <string>
#include <string_view>
#include <utility>

#include <fcntl.h>
#include <sys/file.h>
#include <unistd.h>

namespace cuwacunu::hero::runtime_lock {

inline constexpr const char *kLockRootEnv = "CUWACUNU_HERO_RUNTIME_LOCK_ROOT";
inline constexpr const char *kDefaultLockRoot =
    "/tmp/cuwacunu.hero.runtime.locks";

[[nodiscard]] inline std::filesystem::path lock_root() {
  const char *env = std::getenv(kLockRootEnv);
  if (env == nullptr || *env == '\0') {
    return std::filesystem::path(kDefaultLockRoot);
  }
  return std::filesystem::path(env);
}

struct scoped_lock_t {
  int fd{-1};
  std::string path{};

  scoped_lock_t() = default;
  scoped_lock_t(const scoped_lock_t &) = delete;
  scoped_lock_t &operator=(const scoped_lock_t &) = delete;

  scoped_lock_t(scoped_lock_t &&other) noexcept
      : fd(other.fd), path(std::move(other.path)) {
    other.fd = -1;
  }
  scoped_lock_t &operator=(scoped_lock_t &&other) noexcept {
    if (this == &other)
      return *this;
    release();
    fd = other.fd;
    path = std::move(other.path);
    other.fd = -1;
    return *this;
  }

  ~scoped_lock_t() { release(); }

  void release() noexcept {
    if (fd < 0)
      return;
    (void)::flock(fd, LOCK_UN);
    (void)::close(fd);
    fd = -1;
  }
};

[[nodiscard]] inline std::string sanitize_lock_key(std::string_view raw_key) {
  std::string out;
  out.reserve(raw_key.size());
  for (const unsigned char c : raw_key) {
    const bool ok = (std::isalnum(c) != 0) || c == '.' || c == '_' || c == '-';
    out.push_back(ok ? static_cast<char>(c) : '_');
  }
  if (out.empty())
    return "resource";
  return out;
}

[[nodiscard]] inline std::filesystem::path
mutable_resource_lock_path(std::string_view mutable_resource_key) {
  return lock_root() / "mutable" /
         (sanitize_lock_key(mutable_resource_key) + ".lock");
}

[[nodiscard]] inline bool acquire_path(scoped_lock_t *out,
                                       const std::filesystem::path &lock_path,
                                       std::string *error = nullptr) {
  if (error)
    error->clear();
  if (!out) {
    if (error)
      *error = "lock output pointer is null";
    return false;
  }
  out->release();
  out->path = lock_path.lexically_normal().string();
  if (out->path.empty()) {
    if (error)
      *error = "lock path resolved empty";
    return false;
  }

  std::error_code ec{};
  const std::filesystem::path lock_path_fs(out->path);
  if (lock_path_fs.has_parent_path()) {
    std::filesystem::create_directories(lock_path_fs.parent_path(), ec);
    if (ec) {
      if (error) {
        *error = "failed to create runtime lock directory: " +
                 lock_path_fs.parent_path().string();
      }
      return false;
    }
  }

  const int fd = ::open(out->path.c_str(), O_CREAT | O_RDWR, 0600);
  if (fd < 0) {
    if (error) {
      *error = "failed to open runtime lock file '" + out->path +
               "': " + std::strerror(errno);
    }
    return false;
  }

  if (::flock(fd, LOCK_EX) != 0) {
    const int saved_errno = errno;
    (void)::close(fd);
    if (error) {
      *error = "failed to acquire runtime lock '" + out->path +
               "': " + std::strerror(saved_errno);
    }
    return false;
  }

  out->fd = fd;
  return true;
}

[[nodiscard]] inline bool
acquire_mutable_resource(scoped_lock_t *out,
                         std::string_view mutable_resource_key,
                         std::string *error = nullptr) {
  if (mutable_resource_key.empty()) {
    if (error)
      *error = "mutable resource lock key resolved empty";
    return false;
  }
  return acquire_path(out, mutable_resource_lock_path(mutable_resource_key),
                      error);
}

} // namespace cuwacunu::hero::runtime_lock
