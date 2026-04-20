/* jk_setup.cpp */
#include "jkimyei/training_setup/jk_setup.h"

#include <cctype>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

#include "iitepi/contract_space_t.h"
#include "piaabo/dfiles.h"

namespace {

[[nodiscard]] bool has_non_ws_ascii(std::string_view text) {
  for (const unsigned char c : text) {
    if (!std::isspace(c))
      return true;
  }
  return false;
}

[[nodiscard]] std::string trim_ascii_copy(std::string s) {
  std::size_t b = 0;
  while (b < s.size() && std::isspace(static_cast<unsigned char>(s[b])))
    ++b;
  std::size_t e = s.size();
  while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1])))
    --e;
  return s.substr(b, e - b);
}

[[nodiscard]] std::string unquote_if_wrapped(std::string s) {
  s = trim_ascii_copy(std::move(s));
  if (s.size() >= 2) {
    const char a = s.front();
    const char b = s.back();
    if ((a == '"' && b == '"') || (a == '\'' && b == '\'')) {
      return s.substr(1, s.size() - 2);
    }
  }
  return s;
}

[[nodiscard]] std::string resolve_path_from_folder(const std::string &folder,
                                                   std::string path) {
  path = trim_ascii_copy(std::move(path));
  if (path.empty())
    return {};
  const std::filesystem::path p(path);
  if (p.is_absolute())
    return p.string();
  if (folder.empty())
    return p.string();
  return (std::filesystem::path(folder) / p).string();
}

[[nodiscard]] std::string load_jkimyei_grammar_or_throw() {
  const std::string grammar_path = resolve_path_from_folder(
      std::filesystem::path(cuwacunu::iitepi::config_space_t::config_file_path)
          .parent_path()
          .string(),
      unquote_if_wrapped(cuwacunu::iitepi::config_space_t::get<std::string>(
          "BNF", "jkimyei_specs_grammar_filename")));
  if (!has_non_ws_ascii(grammar_path) ||
      !std::filesystem::exists(grammar_path) ||
      !std::filesystem::is_regular_file(grammar_path)) {
    throw std::runtime_error(
        "invalid [BNF].jkimyei_specs_grammar_filename path");
  }
  const std::string grammar =
      cuwacunu::piaabo::dfiles::readFileToString(grammar_path);
  if (!has_non_ws_ascii(grammar)) {
    throw std::runtime_error("empty jkimyei grammar payload");
  }
  return grammar;
}

[[nodiscard]] cuwacunu::camahjucunu::jkimyei_specs_t
decode_contract_selected_jkimyei_or_throw(
    const cuwacunu::iitepi::contract_hash_t &contract_hash) {
  if (!has_non_ws_ascii(contract_hash)) {
    throw std::runtime_error("missing contract hash");
  }

  const auto contract_itself =
      cuwacunu::iitepi::contract_space_t::contract_itself(contract_hash);
  if (!contract_itself) {
    throw std::runtime_error("failed to resolve contract snapshot");
  }
  const auto sec_it = contract_itself->module_sections.find("VICReg");
  if (sec_it == contract_itself->module_sections.end()) {
    throw std::runtime_error("missing contract VICReg module section");
  }
  const auto key_it = sec_it->second.find("jkimyei_dsl_file");
  if (key_it == sec_it->second.end()) {
    throw std::runtime_error("missing component-local jkimyei payload in "
                             "VICReg module (expected key jkimyei_dsl_file)");
  }
  std::string module_path;
  const auto module_path_it =
      contract_itself->module_section_paths.find("VICReg");
  if (module_path_it != contract_itself->module_section_paths.end()) {
    module_path = module_path_it->second;
  }
  std::string module_folder{};
  if (has_non_ws_ascii(module_path)) {
    const std::filesystem::path p(module_path);
    if (p.has_parent_path())
      module_folder = p.parent_path().string();
  }

  const std::string jkimyei_path = resolve_path_from_folder(
      module_folder, unquote_if_wrapped(key_it->second));
  if (!has_non_ws_ascii(jkimyei_path) ||
      !std::filesystem::exists(jkimyei_path) ||
      !std::filesystem::is_regular_file(jkimyei_path)) {
    throw std::runtime_error("invalid VICReg.jkimyei_dsl_file path: " +
                             jkimyei_path);
  }
  const std::string dsl_text =
      cuwacunu::piaabo::dfiles::readFileToString(jkimyei_path);
  if (!has_non_ws_ascii(dsl_text)) {
    throw std::runtime_error("empty jkimyei DSL payload: " + jkimyei_path);
  }
  const std::string grammar = load_jkimyei_grammar_or_throw();
  return cuwacunu::camahjucunu::dsl::decode_jkimyei_specs_from_dsl(
      grammar, dsl_text, jkimyei_path);
}

} // namespace

namespace cuwacunu {
namespace jkimyei {

/* --------- static definitions (ODR-safe) --------- */
jk_setup_t jk_setup_t::registry{};
jk_setup_t::_init jk_setup_t::_initializer{};

/* ----------------- lifecycle (optional) ----------------- */
void jk_setup_t::init() { log_info("[jk_setup] initialising\n"); }
void jk_setup_t::finit() { log_info("[jk_setup] finalising\n"); }

std::string jk_setup_t::make_component_key(
    const cuwacunu::iitepi::contract_hash_t &contract_hash,
    const std::string &runtime_component_name) {
  return contract_hash + ":" + runtime_component_name;
}

/* ------------------- lazy accessor ------------------- */
jk_component_t &
jk_setup_t::operator()(const std::string &component_name,
                       const cuwacunu::iitepi::contract_hash_t &contract_hash) {
  if (contract_hash.empty()) {
    log_fatal("[jk_setup] missing contract hash for component '%s'\n",
              component_name.c_str());
  }
  const std::string component_key =
      make_component_key(contract_hash, component_name);
  {
    std::lock_guard<std::mutex> lk(mtx);
    auto it = components.find(component_key);
    if (it != components.end())
      return it->second;
  }

  std::string component_lookup_name = component_name;
  std::string instr_text;
  {
    std::lock_guard<std::mutex> lk(mtx);
    auto override_it = component_instruction_overrides.find(component_key);
    if (override_it != component_instruction_overrides.end()) {
      component_lookup_name = override_it->second.component_lookup_name.empty()
                                  ? component_name
                                  : override_it->second.component_lookup_name;
      instr_text = override_it->second.instruction_text;
    }
  }

  // Cache miss: decode instruction from override text or contract payload.
  cuwacunu::camahjucunu::jkimyei_specs_t inst{};
  if (instr_text.empty()) {
    inst = decode_contract_selected_jkimyei_or_throw(contract_hash);
  } else {
    const std::string grammar = load_jkimyei_grammar_or_throw();
    inst = cuwacunu::camahjucunu::dsl::decode_jkimyei_specs_from_dsl(
        grammar, std::move(instr_text));
  }

  {
    std::lock_guard<std::mutex> lk(mtx);
    // Build (or rebuild) into the map entry.
    auto &comp = components[component_key];
    comp.build_from(inst, component_lookup_name, component_name);
    return comp;
  }
}

void jk_setup_t::set_component_instruction_override(
    const cuwacunu::iitepi::contract_hash_t &contract_hash,
    std::string runtime_component_name, std::string component_lookup_name,
    std::string instruction_text) {
  if (contract_hash.empty()) {
    log_fatal("[jk_setup] missing contract hash for override '%s'\n",
              runtime_component_name.c_str());
  }
  if (runtime_component_name.empty())
    return;
  if (component_lookup_name.empty())
    component_lookup_name = runtime_component_name;

  std::lock_guard<std::mutex> lk(mtx);
  const std::string runtime_key =
      make_component_key(contract_hash, runtime_component_name);
  component_instruction_overrides[runtime_key] =
      component_instruction_override_t{
          .component_lookup_name = std::move(component_lookup_name),
          .instruction_text = std::move(instruction_text),
      };
  components.erase(runtime_key);
}

void jk_setup_t::clear_component_instruction_override(
    const cuwacunu::iitepi::contract_hash_t &contract_hash,
    const std::string &runtime_component_name) {
  if (contract_hash.empty()) {
    log_fatal("[jk_setup] missing contract hash while clearing override '%s'\n",
              runtime_component_name.c_str());
  }
  std::lock_guard<std::mutex> lk(mtx);
  const std::string runtime_key =
      make_component_key(contract_hash, runtime_component_name);
  component_instruction_overrides.erase(runtime_key);
  components.erase(runtime_key);
}

void jk_setup_t::clear_component_instruction_overrides(
    const cuwacunu::iitepi::contract_hash_t &contract_hash) {
  if (contract_hash.empty()) {
    log_fatal("[jk_setup] missing contract hash while clearing all overrides "
              "for contract\n");
  }
  std::lock_guard<std::mutex> lk(mtx);
  for (auto comp_it = components.begin(); comp_it != components.end();) {
    if (comp_it->first.rfind(contract_hash + ":", 0) == 0) {
      comp_it = components.erase(comp_it);
    } else {
      ++comp_it;
    }
  }
  for (auto ov_it = component_instruction_overrides.begin();
       ov_it != component_instruction_overrides.end();) {
    if (ov_it->first.rfind(contract_hash + ":", 0) == 0) {
      ov_it = component_instruction_overrides.erase(ov_it);
    } else {
      ++ov_it;
    }
  }
}

void jk_setup_t::clear_all_component_instruction_overrides() {
  std::lock_guard<std::mutex> lk(mtx);
  component_instruction_overrides.clear();
  components.clear();
}

} // namespace jkimyei
} // namespace cuwacunu
