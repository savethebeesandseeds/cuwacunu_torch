#pragma once

#include <mutex>
#include <string>
#include <vector>

#include "camahjucunu/dsl/wave_contract_binding/wave_contract_binding.h"

namespace cuwacunu {
namespace camahjucunu {

struct jkimyei_campaign_contract_decl_t {
  std::string id{};
  std::string file{};
};

struct jkimyei_campaign_wave_decl_t {
  std::string id{};
  std::string file{};
};

using jkimyei_campaign_bind_decl_t = wave_contract_binding_decl_t;

struct jkimyei_campaign_decl_t {
  std::string id{};
  std::vector<std::string> bind_refs{};
};

struct jkimyei_campaign_instruction_t {
  std::string active_campaign_id{};
  std::vector<jkimyei_campaign_contract_decl_t> contracts{};
  std::vector<jkimyei_campaign_wave_decl_t> waves{};
  std::vector<jkimyei_campaign_bind_decl_t> binds{};
  std::vector<jkimyei_campaign_decl_t> campaigns{};
  std::string str() const;
};

namespace dsl {

class jkimyeiCampaignPipeline {
 public:
  explicit jkimyeiCampaignPipeline(std::string grammar_text);
  jkimyei_campaign_instruction_t decode(std::string instruction);

 private:
  std::mutex current_mutex_{};
  std::string grammar_text_{};
};

jkimyei_campaign_instruction_t decode_jkimyei_campaign_from_dsl(
    std::string grammar_text,
    std::string instruction_text);

}  // namespace dsl

}  // namespace camahjucunu
}  // namespace cuwacunu
