#include "hero/runtime_hero/hero_runtime_tools.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>

#include <unistd.h>

namespace hero_runtime = cuwacunu::hero::runtime;

namespace {

void check(bool condition, const std::string &message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

std::filesystem::path make_tmp_dir(const std::string &label) {
  static int counter = 0;
  auto dir = std::filesystem::temp_directory_path() /
             ("cuwacunu_hero_runtime_wave_" + label + "_" +
              std::to_string(static_cast<long long>(::getpid())) + "_" +
              std::to_string(counter++));
  std::filesystem::remove_all(dir);
  std::filesystem::create_directories(dir);
  return dir;
}

void write_text(const std::filesystem::path &path, const std::string &text) {
  std::ofstream out(path, std::ios::trunc);
  check(out.is_open(), "failed to open text output");
  out << text;
}

std::string run_wave_preview(const std::string &target,
                             const std::string &mode) {
  const auto dir = make_tmp_dir(target);
  const auto wave_path = dir / "kikijyeba.settings.wave.dsl";
  const auto config_path = dir / ".config";

  std::ostringstream wave;
  wave << "WAVE_SETTINGS {\n"
       << "  WAVE_ID = test_wave;\n"
       << "  TARGET = " << target << ";\n"
       << "  MODE = " << mode << ";\n"
       << "  SOURCE_CURSOR_KIND = graph_anchor;\n"
       << "  SOURCE_CURSOR_SCOPE = wave_batch;\n"
       << "  SOURCE_RANGE = anchor_index;\n"
       << "  ANCHOR_INDEX_BEGIN = 1;\n"
       << "  ANCHOR_INDEX_END = 3;\n"
       << "};\n";
  write_text(wave_path, wave.str());

  std::ostringstream config;
  config << "[KIKIJYEBA]\n"
         << "kikijyeba_settings_wave_dsl_path = " << wave_path.string() << "\n";
  write_text(config_path, config.str());

  hero_runtime::runtime_context_t ctx{};
  ctx.global_config_path = config_path;

  std::string result;
  std::string error;
  const bool ok = hero_runtime::execute_tool_json(
      "hero.runtime.wave", "{\"config_path\":\"" + config_path.string() + "\"}",
      &ctx, &result, &error);
  check(ok, "hero.runtime.wave failed: " + error);
  check(!hero_runtime::tool_result_is_error(result),
        "hero.runtime.wave returned error: " + result);
  std::filesystem::remove_all(dir);
  return result;
}

void require_contains(const std::string &text, std::string_view needle,
                      const std::string &message) {
  check(text.find(needle) != std::string::npos,
        message + " missing needle: " + std::string(needle) + "\n" + text);
}

void test_canonical_mdn_previews_channel_job() {
  const auto result =
      run_wave_preview("wikimyei.inference.expected_value.mdn", "run|debug");
  require_contains(result, "\"job_kind\":\"channel_inference_mdn\"",
                   "canonical MDN target previews channel job");
  require_contains(result, "\"train_target\":false",
                   "run mode is non-mutating");
}

void test_canonical_vicreg_previews_channel_job() {
  const auto result = run_wave_preview(
      "wikimyei.representation.encoding.vicreg", "train|debug");
  require_contains(result, "\"job_kind\":\"channel_representation_vicreg\"",
                   "canonical VICReg target previews channel job");
  require_contains(result, "\"train_target\":true",
                   "train mode mutates target");
}

void test_legacy_mdn_previews_node_job() {
  const auto result = run_wave_preview("legacy_inference_mdn", "run|debug");
  require_contains(result, "\"job_kind\":\"inference_mdn\"",
                   "legacy target previews node compatibility job");
  check(result.find("\"job_kind\":\"channel_inference_mdn\"") ==
            std::string::npos,
        "legacy target must not preview channel job");
}

} // namespace

int main() {
  try {
    test_canonical_mdn_previews_channel_job();
    test_canonical_vicreg_previews_channel_job();
    test_legacy_mdn_previews_node_job();
    std::cout << "hero runtime wave preview tests passed\n";
    return 0;
  } catch (const std::exception &ex) {
    std::cerr << "test failed: " << ex.what() << "\n";
    return 1;
  }
}
