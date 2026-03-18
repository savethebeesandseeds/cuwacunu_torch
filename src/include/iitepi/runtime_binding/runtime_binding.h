// ./include/iitepi/runtime_binding/runtime_binding.h
// SPDX-License-Identifier: MIT
#pragma once

#include <algorithm>
#include <chrono>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <limits>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
#include <unordered_set>
#include <vector>

#include "hero/hashimyei_hero/hashimyei_catalog.h"
#include "hero/hashimyei_hero/hashimyei_report_fragments.h"
#include "iitepi/runtime_binding/runtime_binding.contract.h"
#include "iitepi/contract_space_t.h"
#include "tsiemene/tsi.type.registry.h"
#include "tsiemene/tsi.wikimyei.h"

namespace tsiemene {

struct RuntimeBinding {
  std::string campaign_hash{};
  std::string runtime_binding_path{};
  std::string binding_id{};
  std::string contract_hash{};
  std::string wave_hash{};
  std::vector<RuntimeBindingContract> contracts{};

  RuntimeBinding() = default;
  RuntimeBinding(RuntimeBinding&& other) noexcept
      : campaign_hash(std::move(other.campaign_hash)),
        runtime_binding_path(std::move(other.runtime_binding_path)),
        binding_id(std::move(other.binding_id)),
        contract_hash(std::move(other.contract_hash)),
        wave_hash(std::move(other.wave_hash)),
        contracts(std::move(other.contracts)) {}
  RuntimeBinding& operator=(RuntimeBinding&& other) noexcept {
    if (this != &other) {
      campaign_hash = std::move(other.campaign_hash);
      runtime_binding_path = std::move(other.runtime_binding_path);
      binding_id = std::move(other.binding_id);
      contract_hash = std::move(other.contract_hash);
      wave_hash = std::move(other.wave_hash);
      contracts = std::move(other.contracts);
    }
    return *this;
  }

  RuntimeBinding(const RuntimeBinding&) = delete;
  RuntimeBinding& operator=(const RuntimeBinding&) = delete;
};

[[nodiscard]] inline DirectiveId pick_start_directive(const Circuit& c) noexcept {
  if (!c.hops || c.hop_count == 0 || !c.hops[0].from.tsi) {
    return directive_id::Step;
  }

  const Tsi& t = *c.hops[0].from.tsi;
  for (const auto& d : t.directives()) {
    if (d.dir == DirectiveDir::In && d.kind.kind == PayloadKind::String) return d.id;
  }
  for (const auto& d : t.directives()) {
    if (d.dir == DirectiveDir::In) return d.id;
  }
  return directive_id::Step;
}

[[nodiscard]] inline std::string make_runtime_run_id(
    const RuntimeBindingContract& contract) {
  const std::uint64_t now_ms = static_cast<std::uint64_t>(
      std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::system_clock::now().time_since_epoch())
          .count());
  std::ostringstream out;
  out << "run."
      << (contract.spec.contract_hash.empty() ? "unknown" : contract.spec.contract_hash)
      << "."
      << now_ms;
  return out.str();
}

[[nodiscard]] inline std::string lowercase_copy(std::string s) {
  std::transform(s.begin(), s.end(), s.begin(), [](unsigned char ch) {
    return static_cast<char>(std::tolower(ch));
  });
  return s;
}

[[nodiscard]] inline std::uint64_t now_ms_utc_runtime_binding() {
  return static_cast<std::uint64_t>(
      std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::system_clock::now().time_since_epoch())
          .count());
}

[[nodiscard]] inline bool write_runtime_text_file_atomic(
    const std::filesystem::path& target, std::string_view content,
    std::string* error = nullptr) {
  if (error) error->clear();
  if (target.empty()) {
    if (error) *error = "target path is empty";
    return false;
  }
  std::error_code ec{};
  if (target.has_parent_path()) {
    std::filesystem::create_directories(target.parent_path(), ec);
    if (ec) {
      if (error) {
        *error = "cannot create parent directory: " +
                 target.parent_path().string();
      }
      return false;
    }
  }
  const auto tmp = target.string() + ".tmp";
  std::ofstream out(tmp, std::ios::binary | std::ios::trunc);
  if (!out) {
    if (error) *error = "cannot open temporary file: " + tmp;
    return false;
  }
  out << content;
  out.flush();
  if (!out.good()) {
    if (error) *error = "cannot write temporary file: " + tmp;
    return false;
  }
  out.close();
  std::filesystem::rename(tmp, target, ec);
  if (ec) {
    std::filesystem::remove(tmp, ec);
    if (error) *error = "cannot replace target file: " + target.string();
    return false;
  }
  return true;
}

[[nodiscard]] inline std::string sanitize_bundle_snapshot_filename(
    std::string_view filename) {
  std::string out{};
  out.reserve(filename.size());
  for (const char ch : filename) {
    const bool ok = (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') ||
                    (ch >= '0' && ch <= '9') || ch == '.' || ch == '_' ||
                    ch == '-';
    out.push_back(ok ? ch : '_');
  }
  if (out.empty()) out = "dsl";
  return out;
}

[[nodiscard]] inline std::optional<cuwacunu::hero::hashimyei::component_manifest_t>
build_runtime_component_manifest(const RuntimeBindingContract& c,
                                 const TsiWikimyei&,
                                 std::string* error = nullptr) {
  if (error) error->clear();
  if (c.spec.representation_hashimyei.empty()) {
    if (error) *error = "representation_hashimyei is empty";
    return std::nullopt;
  }

  std::string family = c.spec.representation_type;
  if (family.empty()) {
    if (error) *error = "wikimyei family is empty";
    return std::nullopt;
  }

  const RuntimeBindingContract::ComponentDslFingerprint* selected_fp = nullptr;
  for (const auto& fp : c.spec.component_dsl_fingerprints) {
    if (!fp.hashimyei.empty() && fp.hashimyei == c.spec.representation_hashimyei) {
      selected_fp = &fp;
      break;
    }
    if (!selected_fp && fp.tsi_type == family) selected_fp = &fp;
  }
  if (!selected_fp) {
    if (error) {
      *error = "missing component DSL fingerprint for runtime bootstrap family: " +
               family;
    }
    return std::nullopt;
  }

  const auto contract_snapshot =
      cuwacunu::iitepi::contract_space_t::contract_itself(c.spec.contract_hash);
  if (!contract_snapshot) {
    if (error) *error = "missing contract snapshot for runtime component manifest";
    return std::nullopt;
  }

  std::uint64_t component_ordinal = 0;
  if (!cuwacunu::hashimyei::parse_hex_hash_name_ordinal(
          c.spec.representation_hashimyei, &component_ordinal)) {
    if (error) {
      *error = "representation_hashimyei is not a valid lowercase 0x... ordinal";
    }
    return std::nullopt;
  }
  const std::string contract_hash = lowercase_copy(c.spec.contract_hash);
  const std::uint64_t contract_ordinal =
      cuwacunu::hashimyei::fnv1a64(contract_hash) & 0xffffull;

  cuwacunu::hero::hashimyei::component_manifest_t manifest{};
  manifest.schema = cuwacunu::hashimyei::kComponentManifestSchemaV2;
  manifest.canonical_path = family + "." + c.spec.representation_hashimyei;
  manifest.family = family;
  manifest.hashimyei_identity =
      cuwacunu::hashimyei::make_identity(
          cuwacunu::hashimyei::hashimyei_kind_e::TSIEMENE, component_ordinal);
  manifest.contract_identity =
      cuwacunu::hashimyei::make_identity(
          cuwacunu::hashimyei::hashimyei_kind_e::CONTRACT, contract_ordinal,
          contract_hash);
  manifest.parent_identity = std::nullopt;
  manifest.revision_reason = "runtime_bootstrap";
  manifest.founding_revision_id = "runtime_bootstrap:" + contract_hash;
  manifest.founding_dsl_provenance_path = selected_fp->tsi_dsl_path;
  manifest.founding_dsl_provenance_sha256_hex = lowercase_copy(
      selected_fp->tsi_dsl_sha256_hex.empty()
          ? cuwacunu::iitepi::contract_space_t::sha256_hex_for_file(
                selected_fp->tsi_dsl_path)
          : selected_fp->tsi_dsl_sha256_hex);
  manifest.docking_signature_sha256_hex =
      lowercase_copy(contract_snapshot->signature.docking_signature_sha256_hex);
  manifest.lineage_state = "active";
  manifest.replaced_by.clear();
  manifest.created_at_ms = now_ms_utc_runtime_binding();
  manifest.updated_at_ms = manifest.created_at_ms;

  std::string validate_error{};
  if (!cuwacunu::hero::hashimyei::validate_component_manifest(
          manifest, &validate_error)) {
    if (error) {
      *error = "runtime component manifest invalid: " + validate_error;
    }
    return std::nullopt;
  }
  return manifest;
}

[[nodiscard]] inline bool persist_runtime_component_lineage(
    const RuntimeBindingContract& c, const TsiWikimyei& wik,
    std::string* error = nullptr) {
  if (error) error->clear();
  const auto manifest_opt = build_runtime_component_manifest(c, wik, error);
  if (!manifest_opt.has_value()) return false;
  const auto& manifest = *manifest_opt;

  const auto store_root = cuwacunu::hashimyei::store_root();
  const std::string component_id =
      cuwacunu::hero::hashimyei::compute_component_manifest_id(manifest);
  const auto manifest_path =
      cuwacunu::hero::hashimyei::component_manifest_path(store_root, component_id);
  if (!std::filesystem::exists(manifest_path)) {
    if (!cuwacunu::hero::hashimyei::save_component_manifest(
            store_root, manifest, nullptr, error)) {
      return false;
    }
  }

  const auto bundle_manifest_path =
      cuwacunu::hero::hashimyei::founding_dsl_bundle_manifest_path(store_root,
                                                                   component_id);
  if (std::filesystem::exists(bundle_manifest_path)) return true;

  const auto contract_snapshot =
      cuwacunu::iitepi::contract_space_t::contract_itself(c.spec.contract_hash);
  if (!contract_snapshot) {
    if (error) *error = "missing contract snapshot for founding DSL bundle";
    return false;
  }

  std::vector<std::filesystem::path> source_paths{};
  std::unordered_set<std::string> seen{};
  const auto add_source_path = [&](std::string_view raw_path) {
    const std::string trimmed = std::string(raw_path);
    if (trimmed.empty()) return;
    const std::filesystem::path p(trimmed);
    const std::string key = p.lexically_normal().string();
    if (!seen.insert(key).second) return;
    source_paths.push_back(p);
  };

  add_source_path(contract_snapshot->config_file_path_canonical);
  add_source_path(manifest.founding_dsl_provenance_path);
  for (const auto& fp : c.spec.component_dsl_fingerprints) {
    add_source_path(fp.tsi_dsl_path);
  }
  for (const auto& surface : contract_snapshot->docking_signature.surfaces) {
    add_source_path(surface.canonical_path);
  }
  for (const auto& entry : contract_snapshot->signature.module_dsl_entries) {
    add_source_path(entry.module_dsl_path);
  }

  const auto bundle_dir =
      cuwacunu::hero::hashimyei::founding_dsl_bundle_directory(store_root,
                                                               component_id);
  std::error_code ec{};
  std::filesystem::create_directories(bundle_dir, ec);
  if (ec) {
    if (error) {
      *error = "cannot create founding DSL bundle directory: " +
               bundle_dir.string();
    }
    return false;
  }

  cuwacunu::hero::hashimyei::founding_dsl_bundle_manifest_t bundle_manifest{};
  bundle_manifest.component_id = component_id;
  bundle_manifest.canonical_path = manifest.canonical_path;
  bundle_manifest.hashimyei_name = manifest.hashimyei_identity.name;
  bundle_manifest.files.reserve(source_paths.size());

  for (std::size_t i = 0; i < source_paths.size(); ++i) {
    const auto& source_path = source_paths[i];
    std::ifstream in(source_path, std::ios::binary);
    if (!in) {
      if (error) {
        *error = "cannot read founding DSL source: " + source_path.string();
      }
      return false;
    }
    std::ostringstream payload;
    payload << in.rdbuf();
    if (!in.good() && !in.eof()) {
      if (error) {
        *error = "cannot read founding DSL source: " + source_path.string();
      }
      return false;
    }
    std::ostringstream name;
    name << std::setfill('0') << std::setw(4) << i << "_"
         << sanitize_bundle_snapshot_filename(source_path.filename().string());
    const auto snapshot_path = bundle_dir / name.str();
    if (!write_runtime_text_file_atomic(snapshot_path, payload.str(), error)) {
      return false;
    }
    bundle_manifest.files.push_back(
        cuwacunu::hero::hashimyei::founding_dsl_bundle_manifest_file_t{
            .source_path = source_path.string(),
            .snapshot_relpath = name.str(),
            .sha256_hex =
                lowercase_copy(cuwacunu::iitepi::contract_space_t::sha256_hex_for_file(
                    snapshot_path.string())),
        });
  }
  bundle_manifest.founding_dsl_bundle_aggregate_sha256_hex =
      cuwacunu::hero::hashimyei::compute_founding_dsl_bundle_aggregate_sha256(
          bundle_manifest);
  if (bundle_manifest.founding_dsl_bundle_aggregate_sha256_hex.empty()) {
    if (error) *error = "cannot compute founding DSL bundle aggregate digest";
    return false;
  }

  return cuwacunu::hero::hashimyei::write_founding_dsl_bundle_manifest(
      store_root, bundle_manifest, error);
}

[[nodiscard]] inline bool validate_runtime_hashimyei_contract_docking(
    std::string_view hashimyei,
    std::string_view contract_hash,
    bool require_registered_manifest,
    std::string* error = nullptr) {
  if (error) error->clear();

  const std::string selected_hashimyei(hashimyei);
  const std::string selected_contract_hash(contract_hash);
  if (selected_hashimyei.empty() || selected_contract_hash.empty()) return true;

  const auto contract_snapshot =
      cuwacunu::iitepi::contract_space_t::contract_itself(
          selected_contract_hash);
  const std::string expected_docking_signature = lowercase_copy(
      contract_snapshot->signature.docking_signature_sha256_hex);
  if (expected_docking_signature.empty()) {
    if (error) {
      *error = "contract snapshot is missing docking_signature_sha256_hex";
    }
    return false;
  }

  cuwacunu::hero::hashimyei::hashimyei_catalog_store_t catalog{};
  cuwacunu::hero::hashimyei::hashimyei_catalog_store_t::options_t options{};
  const auto hashimyei_store_root = cuwacunu::hashimyei::store_root();
  options.catalog_path = cuwacunu::hashimyei::catalog_db_path();
  options.encrypted = false;

  std::string catalog_error{};
  if (!catalog.open(options, &catalog_error)) {
    if (error) {
      *error = "failed to open hashimyei catalog for docking validation: " +
               catalog_error;
    }
    return false;
  }

  cuwacunu::hero::hashimyei::component_state_t component{};
  std::string resolve_error{};
  if (!catalog.resolve_component("", selected_hashimyei, &component,
                                 &resolve_error)) {
    if (!std::filesystem::exists(hashimyei_store_root)) {
      if (require_registered_manifest) {
        if (error) {
          *error = "hashimyei store root does not exist for configured "
                   "hashimyei: " +
                   hashimyei_store_root.string();
        }
        return false;
      }
      return true;
    }
    std::string ingest_error{};
    if (!catalog.ingest_filesystem(hashimyei_store_root, &ingest_error)) {
      if (error) {
        *error =
            "failed to refresh hashimyei catalog for docking validation: " +
            ingest_error;
      }
      return false;
    }
    if (!catalog.resolve_component("", selected_hashimyei, &component,
                                   &resolve_error)) {
      if (require_registered_manifest) {
        if (error) {
          *error = "configured hashimyei manifest lookup failed: " + resolve_error;
        }
        return false;
      }
      return true;
    }
  }

  const std::string component_contract_hash =
      cuwacunu::hero::hashimyei::contract_hash_from_identity(
          component.manifest.contract_identity);
  if (component_contract_hash != selected_contract_hash) {
    if (error) {
      *error =
          "configured hashimyei belongs to a different contract: hashimyei=" +
          selected_hashimyei + " component_contract=" +
          component_contract_hash +
          " requested_contract=" + selected_contract_hash;
    }
    return false;
  }

  const std::string actual_docking_signature =
      lowercase_copy(component.manifest.docking_signature_sha256_hex);
  if (actual_docking_signature.empty()) {
    if (error) {
      *error = "configured hashimyei manifest is missing "
               "docking_signature_sha256_hex: " +
               selected_hashimyei;
    }
    return false;
  }
  if (actual_docking_signature != expected_docking_signature) {
    if (error) {
      *error =
          "configured hashimyei docking signature mismatch: hashimyei=" +
          selected_hashimyei + " canonical_path=" +
          component.manifest.canonical_path + " component_docking=" +
          actual_docking_signature + " contract_docking=" +
          expected_docking_signature;
    }
    return false;
  }

  return true;
}

[[nodiscard]] inline bool validate_runtime_binding_circuit(
    const RuntimeBindingContract& c,
    CircuitIssue* issue = nullptr) noexcept {
  return validate(c.view(), issue);
}

struct RuntimeBindingIssue {
  std::string_view what{};
  std::size_t contract_index{};
  std::size_t circuit_index{};
  CircuitIssue circuit_issue{};
};

[[nodiscard]] inline bool is_known_canonical_component_type(
    std::string_view canonical_type,
    TsiTypeId* out = nullptr) noexcept {
  const auto id = parse_tsi_type_id(canonical_type);
  if (!id) return false;
  const auto* desc = find_tsi_type(*id);
  if (!desc) return false;
  if (desc->canonical != canonical_type) return false;
  if (out) *out = *id;
  return true;
}

[[nodiscard]] inline bool runtime_node_canonical_type(
    const Tsi& node,
    std::string* out) noexcept {
  if (!out) return false;
  const std::string_view raw = node.type_name();
  const auto id = parse_tsi_type_id(raw);
  if (!id) return false;
  const auto* desc = find_tsi_type(*id);
  if (!desc) return false;
  *out = std::string(desc->canonical);
  return true;
}

[[nodiscard]] inline bool validate_runtime_binding(
    const RuntimeBinding& b,
    RuntimeBindingIssue* issue = nullptr) noexcept {
  if (b.contracts.empty()) {
    if (issue) {
      issue->what = "empty runtime binding";
      issue->contract_index = 0;
      issue->circuit_index = 0;
      issue->circuit_issue = CircuitIssue{.what = "empty runtime binding", .hop_index = 0};
    }
    return false;
  }

  for (std::size_t i = 0; i < b.contracts.size(); ++i) {
    const RuntimeBindingContract& c = b.contracts[i];
    auto fail = [&](std::string_view what, std::size_t hop_index) noexcept {
      if (issue) {
        issue->what = what;
        issue->contract_index = i;
        issue->circuit_index = i;
        issue->circuit_issue = CircuitIssue{.what = what, .hop_index = hop_index};
      }
      return false;
    };

    if (c.name.empty()) return fail("contract circuit name is empty", 0);
    if (c.invoke_name.empty()) return fail("contract invoke_name is empty", 0);
    if (c.invoke_payload.empty()) return fail("contract invoke_payload is empty", 0);
    if (c.nodes.empty()) return fail("contract has no nodes", 0);
    std::string_view missing_dsl{};
    if (!c.has_required_dsl_segments(&missing_dsl)) {
      if (missing_dsl == kRuntimeBindingContractCircuitDslKey) {
        return fail("contract missing runtime_binding.contract.circuit@DSL:str", 0);
      }
      if (missing_dsl == kRuntimeBindingContractWaveDslKey) {
        return fail("contract missing runtime_binding.contract.wave@DSL:str", 0);
      }
      return fail("contract missing required DSL segment", 0);
    }

    std::unordered_set<const Tsi*> owned_nodes;
    std::unordered_set<TsiId> node_ids;
    owned_nodes.reserve(c.nodes.size());
    node_ids.reserve(c.nodes.size());
    std::unordered_set<std::string> runtime_component_types;
    std::unordered_set<std::string> runtime_source_types;
    std::unordered_set<std::string> runtime_representation_types;
    runtime_component_types.reserve(c.nodes.size());
    runtime_source_types.reserve(c.nodes.size());
    runtime_representation_types.reserve(c.nodes.size());
    std::size_t runtime_source_count = 0;
    std::size_t runtime_representation_count = 0;
    for (const auto& n : c.nodes) {
      if (!n) return fail("null node in contract nodes", 0);
      if (!owned_nodes.insert(n.get()).second) return fail("duplicated node pointer in contract nodes", 0);
      if (!node_ids.insert(n->id()).second) return fail("duplicated tsi id in contract nodes", 0);

      std::string canonical_runtime_type;
      if (runtime_node_canonical_type(*n, &canonical_runtime_type)) {
        runtime_component_types.insert(canonical_runtime_type);
        if (n->domain() == TsiDomain::Source) runtime_source_types.insert(canonical_runtime_type);
        if (n->domain() == TsiDomain::Wikimyei) runtime_representation_types.insert(canonical_runtime_type);
      }
      if (n->domain() == TsiDomain::Source) ++runtime_source_count;
      if (n->domain() == TsiDomain::Wikimyei) ++runtime_representation_count;
    }

    std::unordered_set<const Tsi*> wired_nodes;
    wired_nodes.reserve(c.nodes.size());
    for (std::size_t hi = 0; hi < c.hops.size(); ++hi) {
      const Hop& h = c.hops[hi];
      if (!owned_nodes.count(h.from.tsi) || !owned_nodes.count(h.to.tsi)) {
        return fail("hop endpoint is not owned by contract nodes", hi);
      }
      wired_nodes.insert(h.from.tsi);
      wired_nodes.insert(h.to.tsi);
    }
    if (wired_nodes.size() != owned_nodes.size()) {
      return fail("orphan node not referenced by any contract hop", 0);
    }

    CircuitIssue ci{};
    if (!validate_runtime_binding_circuit(c, &ci)) {
      if (issue) {
        issue->what = "invalid circuit";
        issue->contract_index = i;
        issue->circuit_index = i;
        issue->circuit_issue = ci;
      }
      return false;
    }

    const Circuit cv = c.view();
    if (!cv.hops || cv.hop_count == 0 || !cv.hops[0].from.tsi) {
      return fail("contract has no start tsi", 0);
    }

    if (c.seed_ingress.directive.empty()) return fail("contract seed_ingress.directive is empty", 0);

    const DirectiveSpec* start_in =
        find_directive(*cv.hops[0].from.tsi, c.seed_ingress.directive, DirectiveDir::In);
    if (!start_in) return fail("contract seed_ingress directive not found on root tsi", 0);

    if (start_in->kind.kind != c.seed_ingress.signal.kind) {
      return fail("contract seed_ingress kind mismatch with root tsi input", 0);
    }

    if (!c.spec.sourced_from_config) continue;

    if (c.spec.sample_type.empty()) return fail("contract spec.sample_type is empty", 0);
    if (runtime_source_count > 0 && c.spec.instrument.empty()) {
      return fail("contract spec.instrument is empty", 0);
    }
    if (runtime_source_count > 0 && c.spec.source_type.empty()) {
      return fail("contract spec.source_type is empty", 0);
    }
    if (runtime_representation_count > 0 && c.spec.representation_type.empty()) {
      return fail("contract spec.representation_type is empty", 0);
    }
    if (c.spec.component_types.empty()) {
      return fail("contract spec.component_types is empty", 0);
    }
    if (c.spec.future_timesteps < 0) {
      return fail("contract spec.future_timesteps must be >= 0", 0);
    }

    std::unordered_set<std::string> spec_component_types;
    spec_component_types.reserve(c.spec.component_types.size());
    for (const auto& type_name : c.spec.component_types) {
      if (type_name.empty()) return fail("contract spec.component_types has empty type", 0);
      if (!spec_component_types.insert(type_name).second) {
        return fail("contract spec.component_types has duplicate type", 0);
      }
      if (!is_known_canonical_component_type(type_name, nullptr)) {
        return fail("contract spec.component_types has unknown canonical type", 0);
      }
    }

    if (!c.spec.source_type.empty()) {
      TsiTypeId source_id{};
      if (!is_known_canonical_component_type(c.spec.source_type, &source_id)) {
        return fail("contract spec.source_type is not canonical/known", 0);
      }
      if (tsi_type_domain(source_id) != TsiDomain::Source) {
        return fail("contract spec.source_type domain mismatch", 0);
      }
      if (!runtime_source_types.empty() &&
          runtime_source_types.find(c.spec.source_type) == runtime_source_types.end()) {
        return fail("contract spec.source_type does not match runtime source nodes", 0);
      }
      if (c.spec.source_type == std::string(tsi_type_token(TsiTypeId::SourceDataloader)) &&
          !c.spec.has_positive_shape_hints()) {
        return fail("contract spec dataloader shape hints are incomplete", 0);
      }
    }

    if (!c.spec.representation_type.empty()) {
      TsiTypeId rep_id{};
      if (!is_known_canonical_component_type(c.spec.representation_type, &rep_id)) {
        return fail("contract spec.representation_type is not canonical/known", 0);
      }
      if (tsi_type_domain(rep_id) != TsiDomain::Wikimyei) {
        return fail("contract spec.representation_type domain mismatch", 0);
      }
      if (!runtime_representation_types.empty() &&
          runtime_representation_types.find(c.spec.representation_type) ==
              runtime_representation_types.end()) {
        return fail("contract spec.representation_type does not match runtime wikimyei nodes", 0);
      }
      if (tsi_type_instance_policy(rep_id) == TsiInstancePolicy::HashimyeiInstances &&
          runtime_representation_count > 0 &&
          c.spec.representation_hashimyei.empty()) {
        return fail("contract spec.representation_hashimyei is empty for hashimyei type", 0);
      }
    }

    if (!c.spec.source_type.empty() &&
        spec_component_types.find(c.spec.source_type) == spec_component_types.end()) {
      return fail("contract spec.source_type missing in spec.component_types", 0);
    }
    if (!c.spec.representation_type.empty() &&
        spec_component_types.find(c.spec.representation_type) == spec_component_types.end()) {
      return fail("contract spec.representation_type missing in spec.component_types", 0);
    }

    if (!runtime_component_types.empty()) {
      for (const auto& runtime_type : runtime_component_types) {
        if (spec_component_types.find(runtime_type) == spec_component_types.end()) {
          return fail("runtime canonical component missing from spec.component_types", 0);
        }
      }
      for (const auto& spec_type : spec_component_types) {
        if (runtime_component_types.find(spec_type) == runtime_component_types.end()) {
          return fail("spec.component_types contains type absent from runtime graph", 0);
        }
      }
    }
  }
  return true;
}

inline bool load_contract_wikimyei_report_fragments(RuntimeBindingContract& c,
                                             const RuntimeContext& ctx,
                                             std::string* error = nullptr);
inline bool save_contract_wikimyei_report_fragments(RuntimeBindingContract& c,
                                             const RuntimeContext& ctx,
                                             std::string* error = nullptr);

class runtime_binding_null_emitter_t final : public Emitter {
 public:
  void emit(const Wave&, DirectiveId, Signal) override {}
};

inline void broadcast_contract_runtime_event(RuntimeBindingContract& c,
                                             RuntimeEventKind kind,
                                             const Wave* wave,
                                             RuntimeContext& ctx) {
  runtime_binding_null_emitter_t emitter{};
  for (auto& node : c.nodes) {
    if (!node) continue;
    RuntimeEvent event{};
    event.kind = kind;
    event.wave = wave;
    event.source = node.get();
    event.target = node.get();
    (void)node->on_event(event, ctx, emitter);
  }
}

[[nodiscard]] inline bool validate_contract_component_dsl_fingerprints(
    const RuntimeBindingContract& c,
    std::string* error = nullptr) {
  if (error) error->clear();
  for (const auto& fp : c.spec.component_dsl_fingerprints) {
    if (fp.tsi_dsl_path.empty() && fp.tsi_dsl_sha256_hex.empty()) continue;
    if (fp.tsi_dsl_path.empty()) {
      if (error) {
        *error = "component DSL fingerprint missing path for canonical_path='" +
                 fp.canonical_path + "'";
      }
      return false;
    }
    const std::string current_sha256 =
        cuwacunu::iitepi::contract_space_t::sha256_hex_for_file(fp.tsi_dsl_path);
    if (current_sha256.empty()) {
      if (error) {
        *error = "component DSL fingerprint file is missing/unreadable path='" +
                 fp.tsi_dsl_path + "'";
      }
      return false;
    }
    if (!fp.tsi_dsl_sha256_hex.empty() && current_sha256 != fp.tsi_dsl_sha256_hex) {
      if (error) {
        *error = "component DSL drift detected canonical_path='" + fp.canonical_path +
                 "' path='" + fp.tsi_dsl_path + "' expected_sha256='" +
                 fp.tsi_dsl_sha256_hex + "' actual_sha256='" + current_sha256 + "'";
      }
      return false;
    }
  }
  return true;
}

inline std::uint64_t run_runtime_binding_circuit(RuntimeBindingContract& c,
                                 RuntimeContext& ctx,
                                 std::string* error = nullptr) {
  if (error) error->clear();
  ctx.wave_mode_flags = c.execution.wave_mode_flags;
  ctx.debug_enabled = c.execution.debug_enabled;
  if (ctx.run_id.empty()) ctx.run_id = make_runtime_run_id(c);
  if (ctx.source_runtime_cursor.empty()) {
    ctx.source_runtime_cursor = c.spec.source_runtime_cursor;
  }
  CircuitIssue issue{};
  if (!c.ensure_compiled(&issue)) {
    if (error) {
      *error = std::string("compile_circuit failed: ");
      error->append(issue.what.data(), issue.what.size());
    }
    return 0;
  }
  std::string dsl_guard_error;
  if (!validate_contract_component_dsl_fingerprints(c, &dsl_guard_error)) {
    if (error) *error = dsl_guard_error;
    log_warn("[runtime_binding.contract.run] abort contract=%s reason=%s\n",
             c.name.empty() ? "<empty>" : c.name.c_str(),
             dsl_guard_error.empty() ? "<empty>" : dsl_guard_error.c_str());
    return 0;
  }
  std::string report_fragment_error;
  if (!load_contract_wikimyei_report_fragments(c, ctx, &report_fragment_error)) {
    if (error) *error = report_fragment_error;
    return 0;
  }
  broadcast_contract_runtime_event(
      c, RuntimeEventKind::RunStart, &c.seed_wave, ctx);
  broadcast_contract_runtime_event(
      c, RuntimeEventKind::EpochStart, &c.seed_wave, ctx);
  const std::uint64_t steps =
      run_wave_compiled(
          c.compiled_runtime,
          c.seed_wave,
          c.seed_ingress,
          ctx,
          c.execution.runtime);
  if (!validate_contract_component_dsl_fingerprints(c, &dsl_guard_error)) {
    if (error) *error = dsl_guard_error;
    log_warn("[runtime_binding.contract.run] abort contract=%s reason=%s\n",
             c.name.empty() ? "<empty>" : c.name.c_str(),
             dsl_guard_error.empty() ? "<empty>" : dsl_guard_error.c_str());
    broadcast_contract_runtime_event(
        c, RuntimeEventKind::RunEnd, &c.seed_wave, ctx);
    return 0;
  }
  broadcast_contract_runtime_event(
      c, RuntimeEventKind::EpochEnd, &c.seed_wave, ctx);
  if (!save_contract_wikimyei_report_fragments(c, ctx, &report_fragment_error)) {
    if (error) *error = report_fragment_error;
    broadcast_contract_runtime_event(
        c, RuntimeEventKind::RunEnd, &c.seed_wave, ctx);
    return 0;
  }
  broadcast_contract_runtime_event(
      c, RuntimeEventKind::RunEnd, &c.seed_wave, ctx);
  return steps;
}

[[nodiscard]] inline Wave wave_for_epoch(const Wave& seed, std::uint64_t epoch_index) {
  Wave out = seed;
  if (epoch_index >
      (std::numeric_limits<std::uint64_t>::max() - seed.cursor.episode)) {
    out.cursor.episode = std::numeric_limits<std::uint64_t>::max();
  } else {
    out.cursor.episode = seed.cursor.episode + epoch_index;
  }
  return out;
}

inline bool load_contract_wikimyei_report_fragments(RuntimeBindingContract& c,
                                             const RuntimeContext& ctx,
                                             std::string* error) {
  if (error) error->clear();
  if (c.spec.representation_hashimyei.empty()) return true;

  for (auto& node : c.nodes) {
    auto* wik = dynamic_cast<TsiWikimyei*>(node.get());
    if (!wik) continue;
    if (!wik->supports_init_report_fragments()) continue;
    if (!wik->runtime_autoload_report_fragments()) continue;

    TsiWikimyeiRuntimeIoContext io_ctx{};
    io_ctx.enable_debug_outputs = c.execution.debug_enabled;
    io_ctx.binding_id = ctx.binding_id;
    io_ctx.run_id = ctx.run_id;
    io_ctx.source_runtime_cursor = ctx.source_runtime_cursor;
    std::string local_error;
    if (!wik->runtime_load_from_hashimyei(c.spec.representation_hashimyei,
                                          &local_error,
                                          &io_ctx)) {
      // Training-enabled wikimyei are allowed to bootstrap from scratch when
      // the configured hashimyei has not been founded yet. The first
      // successful run will persist the freshly constructed state as the
      // initial report_fragment set.
      const bool missing_report_fragment =
          local_error.find("not found") != std::string::npos;
      if (missing_report_fragment && wik->runtime_autosave_report_fragments()) {
        const std::string instance_name(wik->instance_name());
        log_warn(
            "[runtime_binding.contract.run] configured hashimyei missing for "
            "node=%s hashimyei=%s; bootstrapping a new hashimyei from the "
            "active founding_dsl_bundle/runtime state\n",
            instance_name.c_str(), c.spec.representation_hashimyei.c_str());
        continue;
      }
      if (error) {
        *error = "failed to load wikimyei report fragments for node '" +
                 std::string(wik->instance_name()) + "': " + local_error;
      }
      return false;
    }
    if (!validate_runtime_hashimyei_contract_docking(
            c.spec.representation_hashimyei,
            c.spec.contract_hash,
            /*require_registered_manifest=*/true,
            &local_error)) {
      if (error) {
        *error = "configured hashimyei failed contract docking validation for "
                 "node '" +
                 std::string(wik->instance_name()) + "': " + local_error;
      }
      return false;
    }
  }
  return true;
}

inline bool save_contract_wikimyei_report_fragments(RuntimeBindingContract& c,
                                             const RuntimeContext& ctx,
                                             std::string* error) {
  if (error) error->clear();
  if (c.spec.representation_hashimyei.empty()) return true;

  // DEV_WARNING: concurrent campaigns targeting the same representation_hashimyei
  // still have no per-hashimyei lease/ownership protocol here. Keep Runtime
  // Hero campaign concurrency conservative until report-fragment writers gain a
  // cross-process coordination mechanism with staleness recovery.
  for (auto& node : c.nodes) {
    auto* wik = dynamic_cast<TsiWikimyei*>(node.get());
    if (!wik) continue;
    if (!wik->supports_init_report_fragments()) continue;
    if (!wik->runtime_autosave_report_fragments()) continue;

    TsiWikimyeiRuntimeIoContext io_ctx{};
    io_ctx.enable_debug_outputs = c.execution.debug_enabled;
    io_ctx.binding_id = ctx.binding_id;
    io_ctx.run_id = ctx.run_id;
    io_ctx.source_runtime_cursor = ctx.source_runtime_cursor;
    std::string local_error;
    if (!wik->runtime_save_to_hashimyei(c.spec.representation_hashimyei,
                                        &local_error,
                                        &io_ctx)) {
      if (error) {
        *error = "failed to save wikimyei report fragments for node '" +
                 std::string(wik->instance_name()) + "': " + local_error;
      }
      return false;
    }
    if (!persist_runtime_component_lineage(c, *wik, &local_error)) {
      if (error) {
        *error = "failed to persist wikimyei component lineage for node '" +
                 std::string(wik->instance_name()) + "': " + local_error;
      }
      return false;
    }
  }
  return true;
}

inline std::uint64_t run_runtime_binding_contract(RuntimeBindingContract& c,
                                  RuntimeContext& ctx,
                                  std::string* error = nullptr) {
  // Contract runtime initialization: compile + report_fragment preload.
  if (error) error->clear();
  ctx.wave_mode_flags = c.execution.wave_mode_flags;
  ctx.debug_enabled = c.execution.debug_enabled;
  if (ctx.run_id.empty()) ctx.run_id = make_runtime_run_id(c);
  if (ctx.source_runtime_cursor.empty()) {
    ctx.source_runtime_cursor = c.spec.source_runtime_cursor;
  }
  log_info("[runtime_binding.contract.run] start contract=%s epochs=%llu\n",
           c.name.empty() ? "<empty>" : c.name.c_str(),
           static_cast<unsigned long long>(
               std::max<std::uint64_t>(1, c.execution.epochs)));
  CircuitIssue issue{};
  if (!c.ensure_compiled(&issue)) {
    if (error) {
      *error = std::string("compile_circuit failed: ");
      error->append(issue.what.data(), issue.what.size());
    }
    log_err("[runtime_binding.contract.run] compile failed contract=%s error=%s\n",
            c.name.empty() ? "<empty>" : c.name.c_str(),
            error && !error->empty() ? error->c_str() : "<empty>");
    return 0;
  }
  std::string dsl_guard_error;
  if (!validate_contract_component_dsl_fingerprints(c, &dsl_guard_error)) {
    if (error) *error = dsl_guard_error;
    log_warn("[runtime_binding.contract.run] abort contract=%s reason=%s\n",
             c.name.empty() ? "<empty>" : c.name.c_str(),
             dsl_guard_error.empty() ? "<empty>" : dsl_guard_error.c_str());
    return 0;
  }
  std::string report_fragment_error;
  if (!load_contract_wikimyei_report_fragments(c, ctx, &report_fragment_error)) {
    if (error) *error = report_fragment_error;
    log_err("[runtime_binding.contract.run] preload failed contract=%s error=%s\n",
            c.name.empty() ? "<empty>" : c.name.c_str(),
            report_fragment_error.empty() ? "<empty>" : report_fragment_error.c_str());
    return 0;
  }
  broadcast_contract_runtime_event(
      c, RuntimeEventKind::RunStart, &c.seed_wave, ctx);
  Wave run_end_wave = c.seed_wave;

  // Epoch execution + callbacks.
  const std::uint64_t epochs = std::max<std::uint64_t>(1, c.execution.epochs);
  std::uint64_t total_steps = 0;
  for (std::uint64_t epoch = 0; epoch < epochs; ++epoch) {
    if (!validate_contract_component_dsl_fingerprints(c, &dsl_guard_error)) {
      if (error) *error = dsl_guard_error;
      log_warn("[runtime_binding.contract.run] abort contract=%s epoch=%llu reason=%s\n",
               c.name.empty() ? "<empty>" : c.name.c_str(),
               static_cast<unsigned long long>(epoch + 1),
               dsl_guard_error.empty() ? "<empty>" : dsl_guard_error.c_str());
      broadcast_contract_runtime_event(
          c, RuntimeEventKind::RunEnd, &run_end_wave, ctx);
      return 0;
    }
    const Wave start_wave = wave_for_epoch(c.seed_wave, epoch);
    run_end_wave = start_wave;
    broadcast_contract_runtime_event(
        c, RuntimeEventKind::EpochStart, &start_wave, ctx);
    const std::uint64_t epoch_steps =
        run_wave_compiled(
            c.compiled_runtime,
            start_wave,
            c.seed_ingress,
            ctx,
            c.execution.runtime);
    if (!validate_contract_component_dsl_fingerprints(c, &dsl_guard_error)) {
      if (error) *error = dsl_guard_error;
      log_warn("[runtime_binding.contract.run] abort contract=%s epoch=%llu reason=%s\n",
               c.name.empty() ? "<empty>" : c.name.c_str(),
               static_cast<unsigned long long>(epoch + 1),
               dsl_guard_error.empty() ? "<empty>" : dsl_guard_error.c_str());
      broadcast_contract_runtime_event(
          c, RuntimeEventKind::RunEnd, &run_end_wave, ctx);
      return 0;
    }
    total_steps += epoch_steps;
    log_info("[runtime_binding.contract.run] epoch done contract=%s epoch=%llu/%llu steps=%llu cumulative=%llu\n",
             c.name.empty() ? "<empty>" : c.name.c_str(),
             static_cast<unsigned long long>(epoch + 1),
             static_cast<unsigned long long>(epochs),
             static_cast<unsigned long long>(epoch_steps),
             static_cast<unsigned long long>(total_steps));
    broadcast_contract_runtime_event(
        c, RuntimeEventKind::EpochEnd, &start_wave, ctx);
  }

  // Finalization: persist autosave report fragments after the execution loop.
  if (!validate_contract_component_dsl_fingerprints(c, &dsl_guard_error)) {
    if (error) *error = dsl_guard_error;
    log_warn("[runtime_binding.contract.run] abort contract=%s reason=%s\n",
             c.name.empty() ? "<empty>" : c.name.c_str(),
             dsl_guard_error.empty() ? "<empty>" : dsl_guard_error.c_str());
    broadcast_contract_runtime_event(
        c, RuntimeEventKind::RunEnd, &run_end_wave, ctx);
    return 0;
  }
  if (!save_contract_wikimyei_report_fragments(c, ctx, &report_fragment_error)) {
    if (error) *error = report_fragment_error;
    log_err("[runtime_binding.contract.run] finalize failed contract=%s error=%s\n",
            c.name.empty() ? "<empty>" : c.name.c_str(),
            report_fragment_error.empty() ? "<empty>" : report_fragment_error.c_str());
    broadcast_contract_runtime_event(
        c, RuntimeEventKind::RunEnd, &run_end_wave, ctx);
    return 0;
  }
  broadcast_contract_runtime_event(
      c, RuntimeEventKind::RunEnd, &run_end_wave, ctx);
  log_info("[runtime_binding.contract.run] done contract=%s total_steps=%llu\n",
           c.name.empty() ? "<empty>" : c.name.c_str(),
           static_cast<unsigned long long>(total_steps));
  return total_steps;
}

} // namespace tsiemene
