#pragma once

#include <mutex>
#include <cstdint>
#include <string>
#include <vector>

#include "camahjucunu/dsl/wave_contract_binding/wave_contract_binding.h"

namespace cuwacunu {
namespace camahjucunu {

struct iitepi_campaign_contract_decl_t {
  std::string id{};
  std::string file{};
};

struct iitepi_campaign_wave_decl_t {
  std::string id{};
  std::string file{};
};

enum class iitepi_campaign_mount_selector_kind_e : std::uint8_t {
  Exact = 0,
  Rank = 1,
};

struct iitepi_campaign_mount_decl_t {
  std::string wave_binding_id{};
  iitepi_campaign_mount_selector_kind_e selector_kind{
      iitepi_campaign_mount_selector_kind_e::Exact};
  std::string exact_hashimyei{};
  std::uint64_t rank{0};
};

struct iitepi_campaign_bind_decl_t : wave_contract_binding_decl_t {
  std::vector<iitepi_campaign_mount_decl_t> mounts{};
};

struct iitepi_campaign_run_decl_t {
  std::string bind_ref{};
};

struct iitepi_campaign_instruction_t {
  std::vector<iitepi_campaign_contract_decl_t> contracts{};
  std::vector<iitepi_campaign_wave_decl_t> waves{};
  std::string marshal_objective_file{};
  std::vector<iitepi_campaign_bind_decl_t> binds{};
  std::vector<iitepi_campaign_run_decl_t> runs{};
  std::string str() const;
};

namespace dsl {

class iitepiCampaignPipeline {
 public:
  explicit iitepiCampaignPipeline(std::string grammar_text);
  iitepi_campaign_instruction_t decode(std::string instruction);

 private:
  std::mutex current_mutex_{};
  std::string grammar_text_{};
};

iitepi_campaign_instruction_t decode_iitepi_campaign_from_dsl(
    std::string grammar_text,
    std::string instruction_text);

}  // namespace dsl

}  // namespace camahjucunu
}  // namespace cuwacunu
