/* memory_mapped_error.h */
#pragma once

#include <cerrno>
#include <cstddef>
#include <cstring>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

namespace cuwacunu::ujcamei::source::storage::memory_mapped {

class memory_mapped_error_t : public std::runtime_error {
public:
  explicit memory_mapped_error_t(const std::string &message)
      : std::runtime_error(message) {}
};

struct memory_mapped_error_context_t {
  std::string where{};
  std::string file{};
  std::string csv_file{};
  std::string edge_id{};
  std::string edge_signature{};
  std::optional<std::size_t> channel{};
  std::optional<std::size_t> dataset_index{};
  std::string anchor_key{};
  std::string reason{};
};

inline std::string
format_memory_mapped_error(const memory_mapped_error_context_t &ctx) {
  std::ostringstream oss;
  oss << "[memory_mapped]";
  if (!ctx.where.empty())
    oss << " where=" << ctx.where;
  if (!ctx.edge_id.empty())
    oss << " edge_id=" << ctx.edge_id;
  if (!ctx.edge_signature.empty())
    oss << " edge=" << ctx.edge_signature;
  if (ctx.channel.has_value())
    oss << " channel=" << *ctx.channel;
  if (ctx.dataset_index.has_value())
    oss << " dataset=" << *ctx.dataset_index;
  if (!ctx.anchor_key.empty())
    oss << " anchor_key=" << ctx.anchor_key;
  if (!ctx.file.empty())
    oss << " file=" << ctx.file;
  if (!ctx.csv_file.empty())
    oss << " csv=" << ctx.csv_file;
  if (!ctx.reason.empty())
    oss << " reason=" << ctx.reason;
  return oss.str();
}

[[noreturn]] inline void
throw_memory_mapped_error(memory_mapped_error_context_t ctx) {
  throw memory_mapped_error_t(format_memory_mapped_error(ctx));
}

[[noreturn]] inline void
throw_memory_mapped_errno_error(memory_mapped_error_context_t ctx,
                                int saved_errno) {
  if (!ctx.reason.empty()) {
    ctx.reason += ": ";
  }
  ctx.reason += std::strerror(saved_errno);
  throw_memory_mapped_error(std::move(ctx));
}

} // namespace cuwacunu::ujcamei::source::storage::memory_mapped
