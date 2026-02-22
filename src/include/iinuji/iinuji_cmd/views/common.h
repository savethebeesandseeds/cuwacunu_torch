#pragma once

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "camahjucunu/BNF/implementations/tsiemene_board/tsiemene_board.h"
#include "camahjucunu/BNF/implementations/tsiemene_board/tsiemene_board_runtime.h"
#include "iinuji/iinuji_types.h"
#include "piaabo/dconfig.h"
#include "piaabo/dlogs.h"
#include "tsiemene/utils/directives.h"

#include "iinuji/iinuji_cmd/state.h"

namespace cuwacunu {
namespace iinuji {
namespace iinuji_cmd {
using cuwacunu::camahjucunu::tsiemene_circuit_decl_t;

inline std::string to_lower_copy(std::string s) {
  for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  return s;
}

inline bool parse_positive_index(std::string_view s, std::size_t* out) {
  if (!out || s.empty()) return false;
  std::size_t v = 0;
  for (const char c : s) {
    if (c < '0' || c > '9') return false;
    v = v * 10 + (std::size_t)(c - '0');
  }
  if (v == 0) return false;
  *out = v;
  return true;
}

inline std::string short_type(std::string_view full) {
  const std::size_t p = full.rfind('.');
  if (p == std::string_view::npos) return std::string(full);
  return std::string(full.substr(p + 1));
}

inline std::string trim_to_width(const std::string& s, int width) {
  if (width <= 0) return {};
  if ((int)s.size() <= width) return s;
  if (width <= 3) return s.substr(0, (std::size_t)width);
  return s.substr(0, (std::size_t)(width - 3)) + "...";
}

inline std::string trim_copy(const std::string& s) {
  std::size_t b = 0;
  while (b < s.size() && std::isspace(static_cast<unsigned char>(s[b]))) ++b;
  std::size_t e = s.size();
  while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1]))) --e;
  return s.substr(b, e - b);
}

inline bool read_text_file_safe(const std::string& path, std::string* out, std::string* error) {
  if (out) out->clear();
  if (error) error->clear();
  if (path.empty()) {
    if (error) *error = "path is empty";
    return false;
  }
  std::ifstream in(path, std::ios::binary);
  if (!in) {
    if (error) *error = "cannot open file";
    return false;
  }
  std::ostringstream ss;
  ss << in.rdbuf();
  if (out) *out = ss.str();
  return true;
}

inline std::string format_file_status(const std::string& path) {
  namespace fs = std::filesystem;
  std::ostringstream oss;
  oss << path;
  if (path.empty()) {
    oss << " (unset)";
    return oss.str();
  }
  std::error_code ec;
  if (!fs::exists(path, ec)) {
    oss << " (missing)";
    return oss.str();
  }
  if (!fs::is_regular_file(path, ec)) {
    oss << " (not-regular)";
    return oss.str();
  }
  const auto bytes = fs::file_size(path, ec);
  if (!ec) oss << " (" << bytes << " bytes)";
  return oss.str();
}

inline std::string masked_preview(const std::string& s) {
  if (s.empty()) return "<empty>";
  std::string clean = trim_copy(s);
  if (clean.empty()) return "<empty>";
  if (clean.size() <= 4) return std::string(clean.size(), '*');
  std::string out = clean;
  for (std::size_t i = 2; i + 2 < out.size(); ++i) out[i] = '*';
  return out;
}

inline bool lookup_config_value(const std::string& section,
                                const std::string& key,
                                std::string* out) {
  if (!out) return false;
  const auto sec_it = cuwacunu::piaabo::dconfig::config_space_t::config.find(section);
  if (sec_it == cuwacunu::piaabo::dconfig::config_space_t::config.end()) return false;
  const auto key_it = sec_it->second.find(key);
  if (key_it == sec_it->second.end()) return false;
  *out = key_it->second;
  return true;
}

inline BoardState load_board_from_config() {
  BoardState out{};
  out.raw_instruction = cuwacunu::piaabo::dconfig::config_space_t::tsiemene_board_instruction();

  auto parser = cuwacunu::camahjucunu::BNF::tsiemeneBoard();
  out.board = parser.decode(out.raw_instruction);

  std::string error;
  if (!cuwacunu::camahjucunu::validate_board_instruction(out.board, &error)) {
    out.ok = false;
    out.error = error;
    return out;
  }

  out.resolved_hops.clear();
  out.resolved_hops.reserve(out.board.circuits.size());
  for (std::size_t i = 0; i < out.board.circuits.size(); ++i) {
    std::vector<tsiemene_resolved_hop_t> rh;
    std::string resolve_error;
    if (!cuwacunu::camahjucunu::resolve_hops(out.board.circuits[i], &rh, &resolve_error)) {
      out.ok = false;
      out.error = "circuit[" + std::to_string(i) + "] " + resolve_error;
      return out;
    }
    out.resolved_hops.push_back(std::move(rh));
  }

  out.ok = true;
  return out;
}

inline ConfigTabData make_text_tab(std::string id,
                                   std::string title,
                                   std::string path) {
  ConfigTabData tab{};
  tab.id = std::move(id);
  tab.title = std::move(title);
  tab.path = std::move(path);
  tab.ok = read_text_file_safe(tab.path, &tab.content, &tab.error);
  return tab;
}

inline ConfigTabData make_secrets_tab() {
  ConfigTabData tab{};
  tab.id = "secrets";
  tab.title = "secrets";
  tab.path = "(computed)";

  std::ostringstream oss;
  oss << "# secrets summary\n";
  oss << "# values are masked; file paths and sizes are shown\n\n";

  const std::vector<std::string> sections{"TEST_EXCHANGE", "REAL_EXCHANGE"};
  const std::vector<std::string> keys{"AES_salt", "Ed25519_pkey", "EXCHANGE_api_filename"};

  for (const std::string& sec : sections) {
    oss << "[" << sec << "]\n";
    for (const std::string& key : keys) {
      std::string path;
      if (!lookup_config_value(sec, key, &path)) {
        oss << "  " << key << ": <missing in config>\n";
        continue;
      }
      oss << "  " << key << ": " << format_file_status(path);
      if (key == "EXCHANGE_api_filename") {
        std::string content;
        std::string err;
        if (read_text_file_safe(path, &content, &err)) {
          std::string first_line;
          std::istringstream iss(content);
          std::getline(iss, first_line);
          oss << " preview=" << masked_preview(first_line);
        } else {
          oss << " preview=<unreadable>";
        }
      }
      oss << "\n";
    }
    oss << "\n";
  }

  tab.ok = true;
  tab.content = oss.str();
  return tab;
}

inline ConfigState load_config_view_from_config() {
  ConfigState out{};

  const std::string main_path = cuwacunu::piaabo::dconfig::config_space_t::config_file_path;
  out.tabs.push_back(make_text_tab("main", "main .config", main_path));

  struct TabSpec {
    const char* id;
    const char* title;
    const char* key;
  };
  static constexpr TabSpec specs[] = {
      {"observation_pipeline.bnf", "observation_pipeline.bnf", "observation_pipeline_bnf_filename"},
      {"observation_pipeline.instruction", "observation_pipeline.instruction", "observation_pipeline_instruction_filename"},
      {"training_components.bnf", "training_components.bnf", "training_components_bnf_filename"},
      {"training_components.instruction", "training_components.instruction", "training_components_instruction_filename"},
      {"tsiemene_board.bnf", "tsiemene_board.bnf", "tsiemene_board_bnf_filename"},
      {"tsiemene_board.instruction", "tsiemene_board.instruction", "tsiemene_board_instruction_filename"},
      {"canonical_path.bnf", "canonical_path.bnf", "canonical_path_bnf_filename"},
  };

  for (const auto& s : specs) {
    std::string path;
    if (!lookup_config_value("BNF", s.key, &path)) {
      ConfigTabData tab{};
      tab.id = s.id;
      tab.title = s.title;
      tab.ok = false;
      tab.error = "missing [BNF]." + std::string(s.key);
      out.tabs.push_back(std::move(tab));
      continue;
    }
    out.tabs.push_back(make_text_tab(s.id, s.title, path));
  }

  out.tabs.push_back(make_secrets_tab());
  out.ok = !out.tabs.empty();
  if (!out.ok) out.error = "no tabs";
  return out;
}

struct TsiDirectiveDoc {
  tsiemene::DirectiveDir dir{};
  std::string directive{};
  std::string kind{};
  std::string description{};
};

struct TsiNodeDoc {
  std::string id{};
  std::string title{};
  std::string type_name{};
  std::string role{};
  std::string determinism{};
  std::string notes{};
  std::vector<TsiDirectiveDoc> directives{};
};

inline const std::vector<TsiNodeDoc>& tsi_node_docs() {
  static const std::vector<TsiNodeDoc> docs = {
      {
          "wave.generator",
          "wave.generator",
          "tsi.wikimyei.wave.generator",
          "wave command entrypoint; forwards command payload into the circuit",
          "Deterministic",
          "Typical root instance. Emits @meta trace only when wired in the board.",
          {
              {tsiemene::DirectiveDir::In, "@payload", ":str", "generator command string"},
              {tsiemene::DirectiveDir::Out, "@payload", ":str", "wave payload string"},
              {tsiemene::DirectiveDir::Out, "@meta", ":str", "runtime trace/meta stream"},
          },
      },
      {
          "source.dataloader",
          "source.dataloader",
          "tsi.wikimyei.source.dataloader",
          "loads observation batches from instrument datasets",
          "SeededStochastic",
          "Supports command payloads like SYMBOL[dd.mm.yyyy,dd.mm.yyyy] and batches=N.",
          {
              {tsiemene::DirectiveDir::In, "@payload", ":str", "dataloader command string"},
              {tsiemene::DirectiveDir::Out, "@payload", ":tensor", "packed [B,C,T,D+1] batch"},
              {tsiemene::DirectiveDir::Out, "@meta", ":str", "dataloader mode/emit metadata"},
          },
      },
      {
          "representation.vicreg",
          "representation.vicreg",
          "tsi.wikimyei.representation.vicreg",
          "encodes packed tensors into representations; optional train-time loss emit",
          "SeededStochastic",
          "Consumes packed tensors and emits representation payload plus optional loss.",
          {
              {tsiemene::DirectiveDir::In, "@payload", ":tensor", "packed [B,C,T,D+1] input"},
              {tsiemene::DirectiveDir::Out, "@payload", ":tensor", "representation encoding"},
              {tsiemene::DirectiveDir::Out, "@loss", ":tensor", "loss scalar (train mode)"},
              {tsiemene::DirectiveDir::Out, "@meta", ":str", "shape / mode trace metadata"},
          },
      },
      {
          "sink.null",
          "sink.null",
          "tsi.sink.null",
          "terminal sink that consumes and discards tensor payloads",
          "Deterministic",
          "Useful to terminate branches while still exposing @meta routing visibility.",
          {
              {tsiemene::DirectiveDir::In, "@payload", ":tensor", "drop tensor payload"},
              {tsiemene::DirectiveDir::Out, "@meta", ":str", "runtime trace/meta stream"},
          },
      },
      {
          "sink.log.sys",
          "sink.log.sys",
          "tsi.sink.log.sys",
          "observability sink for payload/loss/meta events",
          "Deterministic",
          "Routes logs through piaabo/dlogs.h so they appear in terminal and F8 logs.",
          {
              {tsiemene::DirectiveDir::In, "@payload", ":str", "log string payload"},
              {tsiemene::DirectiveDir::In, "@loss", ":tensor", "log loss tensor"},
              {tsiemene::DirectiveDir::In, "@meta", ":str", "log runtime/system traces"},
          },
      },
  };
  return docs;
}

inline std::size_t tsi_tab_count() {
  return tsi_node_docs().size();
}

inline std::size_t clamp_tsi_tab_index(std::size_t idx) {
  const std::size_t n = tsi_tab_count();
  if (n == 0) return 0;
  return (idx < n) ? idx : 0;
}

inline std::string dir_token(tsiemene::DirectiveDir d) {
  return (d == tsiemene::DirectiveDir::In) ? "in" : "out";
}

inline constexpr char k_selected_line_marker = '\x1f';

inline std::string mark_selected_line(std::string line) {
  return std::string(1, k_selected_line_marker) + std::move(line);
}

template <class T>
inline std::shared_ptr<T> as(const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t>& obj) {
  if (!obj) return nullptr;
  return std::dynamic_pointer_cast<T>(obj->data);
}

inline void set_text_box(const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t>& box,
                         std::string text,
                         bool wrap) {
  auto tb = as<cuwacunu::iinuji::textBox_data_t>(box);
  if (!tb) return;
  tb->content = std::move(text);
  tb->wrap = wrap;
}

inline void append_log(const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t>& log_box,
                       std::string text,
                       std::string label = {},
                       std::string color = {}) {
  auto bb = as<cuwacunu::iinuji::bufferBox_data_t>(log_box);
  if (!bb) return;
  bb->push_line(std::move(text), std::move(label), std::move(color));
}

}  // namespace iinuji_cmd
}  // namespace iinuji
}  // namespace cuwacunu
