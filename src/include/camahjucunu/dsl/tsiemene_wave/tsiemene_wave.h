/* tsiemene_wave.h */
#pragma once

#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

namespace cuwacunu {
namespace camahjucunu {

struct tsiemene_wave_wikimyei_decl_t {
  std::string wikimyei_path{};
  bool train{false};
  bool has_train{false};
  std::string profile_id{};
};

struct tsiemene_wave_source_decl_t {
  std::string source_path{};
  std::string symbol{};
  std::string from{};
  std::string to{};
};

struct tsiemene_wave_t {
  std::string name{};
  std::string mode{};
  std::string sampler{};
  std::uint64_t epochs{0};
  std::uint64_t batch_size{0};
  std::uint64_t max_batches_per_epoch{0};
  std::vector<tsiemene_wave_wikimyei_decl_t> wikimyeis{};
  std::vector<tsiemene_wave_source_decl_t> sources{};
};

struct tsiemene_wave_set_t {
  std::vector<tsiemene_wave_t> waves{};
  std::string str() const;
};

namespace dsl {

class tsiemeneWavePipeline {
 public:
  explicit tsiemeneWavePipeline(std::string grammar_text);
  tsiemene_wave_set_t decode(std::string instruction);

 private:
  std::mutex current_mutex_;
  std::string grammar_text_;
};

tsiemene_wave_set_t decode_tsiemene_wave_from_dsl(
    std::string grammar_text,
    std::string instruction_text);

} /* namespace dsl */

} /* namespace camahjucunu */
} /* namespace cuwacunu */
