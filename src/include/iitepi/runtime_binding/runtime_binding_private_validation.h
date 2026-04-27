[[nodiscard]] inline bool validate_runtime_hashimyei_component_compatibility(
    std::string_view expected_family, std::string_view hashimyei,
    std::string_view contract_hash, std::string_view binding_id,
    bool require_registered_manifest, std::string *error = nullptr) {
  if (error)
    error->clear();

  std::string selected_hashimyei(hashimyei);
  cuwacunu::hashimyei::normalize_hex_hash_name_inplace(&selected_hashimyei);
  const std::string selected_contract_hash(contract_hash);
  if (selected_hashimyei.empty() || selected_contract_hash.empty())
    return true;

  const auto contract_snapshot =
      cuwacunu::iitepi::contract_space_t::contract_itself(
          selected_contract_hash);
  if (!contract_snapshot) {
    if (error) {
      *error = "contract snapshot is missing for contract hash: " +
               selected_contract_hash;
    }
    return false;
  }
  const auto *component_compatibility =
      cuwacunu::iitepi::find_component_compatibility_signature(
          *contract_snapshot, binding_id);
  if (!component_compatibility) {
    if (error) {
      *error = "contract snapshot is missing component compatibility "
               "signature for binding id: " +
               std::string(binding_id);
    }
    return false;
  }
  const std::string expected_component_compatibility =
      lowercase_copy(component_compatibility->sha256_hex);
  if (expected_component_compatibility.empty()) {
    if (error) {
      *error = "contract component compatibility signature is empty for "
               "binding id: " +
               std::string(binding_id);
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
      *error = "failed to open hashimyei catalog for component compatibility "
               "validation: " +
               catalog_error;
    }
    return false;
  }

  cuwacunu::hero::hashimyei::component_state_t component{};
  std::string resolve_error{};
  const std::string selected_canonical_path =
      !std::string(expected_family).empty()
          ? std::string(expected_family) + "." + selected_hashimyei
          : std::string{};
  if (!catalog.resolve_component(selected_canonical_path, selected_hashimyei,
                                 &component, &resolve_error)) {
    // Bootstrap runs are allowed to target a fresh hashimyei slot that has no
    // manifest yet. In that case we should not force a full catalog ingest
    // just to confirm absence before training has founded the component.
    if (!require_registered_manifest) {
      return true;
    }
    if (!std::filesystem::exists(hashimyei_store_root)) {
      if (error) {
        *error = "hashimyei store root does not exist for configured "
                 "hashimyei: " +
                 hashimyei_store_root.string();
      }
      return false;
    }
    std::string ingest_error{};
    if (!catalog.ingest_filesystem(hashimyei_store_root, &ingest_error)) {
      if (error) {
        *error =
            "failed to refresh hashimyei catalog for component compatibility "
            "validation: " +
            ingest_error;
      }
      return false;
    }
    if (!catalog.resolve_component(selected_canonical_path, selected_hashimyei,
                                   &component, &resolve_error)) {
      if (error) {
        *error =
            "configured hashimyei manifest lookup failed: " + resolve_error;
      }
      return false;
    }
  }

  const std::string actual_component_compatibility =
      lowercase_copy(component.manifest.component_compatibility_sha256_hex);
  if (actual_component_compatibility.empty()) {
    if (error) {
      *error = "configured hashimyei manifest is missing "
               "component_compatibility_sha256_hex: " +
               selected_hashimyei;
    }
    return false;
  }
  if (actual_component_compatibility != expected_component_compatibility) {
    const std::string component_contract_hash =
        cuwacunu::hero::hashimyei::contract_hash_from_identity(
            component.manifest.contract_identity);
    if (error) {
      *error = "configured hashimyei component compatibility mismatch: "
               "hashimyei=" +
               selected_hashimyei +
               " canonical_path=" + component.manifest.canonical_path +
               " component_compatibility=" + actual_component_compatibility +
               " expected_component_compatibility=" +
               expected_component_compatibility +
               " component_contract=" + component_contract_hash +
               " requested_contract=" + selected_contract_hash;
    }
    return false;
  }

  return true;
}

[[nodiscard]] inline bool
validate_runtime_component_manifest_component_compatibility(
    const cuwacunu::hero::hashimyei::component_manifest_t &manifest,
    std::string_view requested_contract_hash, std::string_view binding_id,
    std::string *error = nullptr) {
  if (error)
    error->clear();
  const auto contract_snapshot =
      cuwacunu::iitepi::contract_space_t::contract_itself(
          std::string(requested_contract_hash));
  if (!contract_snapshot) {
    if (error) {
      *error = "requested contract snapshot is missing";
    }
    return false;
  }
  const auto *component_compatibility =
      cuwacunu::iitepi::find_component_compatibility_signature(
          *contract_snapshot, binding_id);
  if (!component_compatibility) {
    if (error) {
      *error = "requested contract is missing component compatibility "
               "signature for binding id: " +
               std::string(binding_id);
    }
    return false;
  }
  const std::string expected_signature =
      lowercase_copy(component_compatibility->sha256_hex);
  const std::string actual_signature =
      lowercase_copy(manifest.component_compatibility_sha256_hex);
  if (actual_signature.empty()) {
    if (error) {
      *error =
          "component manifest is missing component_compatibility_sha256_hex";
    }
    return false;
  }
  if (actual_signature != expected_signature) {
    if (error) {
      *error = "component manifest compatibility signature mismatch: "
               "canonical_path=" +
               manifest.canonical_path +
               " component_compatibility=" + actual_signature +
               " requested_component_compatibility=" + expected_signature +
               " component_contract=" +
               cuwacunu::hero::hashimyei::contract_hash_from_identity(
                   manifest.contract_identity) +
               " requested_contract=" + std::string(requested_contract_hash);
    }
    return false;
  }
  return true;
}

[[nodiscard]] inline bool
validate_runtime_binding_circuit(const RuntimeBindingContract &c,
                                 CircuitIssue *issue = nullptr) noexcept {
  return validate(c.view(), issue);
}

struct RuntimeBindingIssue {
  std::string_view what{};
  std::size_t contract_index{};
  std::size_t circuit_index{};
  CircuitIssue circuit_issue{};
};

[[nodiscard]] inline bool
is_known_canonical_component_type(std::string_view canonical_type,
                                  TsiTypeId *out = nullptr) noexcept {
  const auto id = parse_tsi_type_id(canonical_type);
  if (!id)
    return false;
  const auto *desc = find_tsi_type(*id);
  if (!desc)
    return false;
  if (desc->canonical != canonical_type)
    return false;
  if (out)
    *out = *id;
  return true;
}

[[nodiscard]] inline bool
runtime_node_canonical_type(const Tsi &node, std::string *out) noexcept {
  if (!out)
    return false;
  const std::string_view raw = node.type_name();
  const auto id = parse_tsi_type_id(raw);
  if (!id)
    return false;
  const auto *desc = find_tsi_type(*id);
  if (!desc)
    return false;
  *out = std::string(desc->canonical);
  return true;
}

[[nodiscard]] inline bool
validate_runtime_binding(const RuntimeBinding &b,
                         RuntimeBindingIssue *issue = nullptr) noexcept {
  if (b.contracts.empty()) {
    if (issue) {
      issue->what = "empty runtime binding";
      issue->contract_index = 0;
      issue->circuit_index = 0;
      issue->circuit_issue =
          CircuitIssue{.what = "empty runtime binding", .hop_index = 0};
    }
    return false;
  }

  for (std::size_t i = 0; i < b.contracts.size(); ++i) {
    const RuntimeBindingContract &c = b.contracts[i];
    auto fail = [&](std::string_view what, std::size_t hop_index) noexcept {
      if (issue) {
        issue->what = what;
        issue->contract_index = i;
        issue->circuit_index = i;
        issue->circuit_issue =
            CircuitIssue{.what = what, .hop_index = hop_index};
      }
      return false;
    };

    if (c.name.empty())
      return fail("contract circuit name is empty", 0);
    if (c.invoke_name.empty())
      return fail("contract invoke_name is empty", 0);
    if (c.invoke_payload.empty())
      return fail("contract invoke_payload is empty", 0);
    if (c.nodes.empty())
      return fail("contract has no nodes", 0);
    std::string_view missing_dsl{};
    if (!c.has_required_dsl_segments(&missing_dsl)) {
      if (missing_dsl == kRuntimeBindingContractCircuitDslKey) {
        return fail("contract missing runtime_binding.contract.circuit@DSL:str",
                    0);
      }
      if (missing_dsl == kRuntimeBindingContractWaveDslKey) {
        return fail("contract missing runtime_binding.contract.wave@DSL:str",
                    0);
      }
      return fail("contract missing required DSL segment", 0);
    }

    std::unordered_set<const Tsi *> owned_nodes;
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
    for (const auto &n : c.nodes) {
      if (!n)
        return fail("null node in contract nodes", 0);
      if (!owned_nodes.insert(n.get()).second)
        return fail("duplicated node pointer in contract nodes", 0);
      if (!node_ids.insert(n->id()).second)
        return fail("duplicated tsi id in contract nodes", 0);

      std::string canonical_runtime_type;
      if (runtime_node_canonical_type(*n, &canonical_runtime_type)) {
        runtime_component_types.insert(canonical_runtime_type);
        if (n->domain() == TsiDomain::Source)
          runtime_source_types.insert(canonical_runtime_type);
        if (n->domain() == TsiDomain::Wikimyei)
          runtime_representation_types.insert(canonical_runtime_type);
      }
      if (n->domain() == TsiDomain::Source)
        ++runtime_source_count;
      if (n->domain() == TsiDomain::Wikimyei)
        ++runtime_representation_count;
    }

    std::unordered_set<const Tsi *> wired_nodes;
    wired_nodes.reserve(c.nodes.size());
    for (std::size_t hi = 0; hi < c.hops.size(); ++hi) {
      const Hop &h = c.hops[hi];
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

    if (c.seed_ingress.directive.empty())
      return fail("contract seed_ingress.directive is empty", 0);

    const DirectiveSpec *start_in = find_directive(
        *cv.hops[0].from.tsi, c.seed_ingress.directive, DirectiveDir::In);
    if (!start_in)
      return fail("contract seed_ingress directive not found on root tsi", 0);

    if (start_in->kind.kind != c.seed_ingress.signal.kind) {
      return fail("contract seed_ingress kind mismatch with root tsi input", 0);
    }

    if (!c.spec.sourced_from_config)
      continue;

    if (c.spec.sample_type.empty())
      return fail("contract spec.sample_type is empty", 0);
    if (runtime_source_count > 0 && c.spec.instrument.empty()) {
      return fail("contract spec.instrument is empty", 0);
    }
    if (runtime_source_count > 0) {
      std::string signature_error{};
      if (!cuwacunu::camahjucunu::instrument_signature_validate(
              c.spec.runtime_instrument_signature, /*allow_any=*/false,
              "contract spec.runtime_instrument_signature", &signature_error)) {
        return fail("contract spec.runtime_instrument_signature is invalid: " +
                        signature_error,
                    0);
      }
    }
    if (runtime_source_count > 0 && c.spec.source_type.empty()) {
      return fail("contract spec.source_type is empty", 0);
    }
    if (runtime_representation_count > 0 &&
        c.spec.representation_type.empty()) {
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
    for (const auto &type_name : c.spec.component_types) {
      if (type_name.empty())
        return fail("contract spec.component_types has empty type", 0);
      if (!spec_component_types.insert(type_name).second) {
        return fail("contract spec.component_types has duplicate type", 0);
      }
      if (!is_known_canonical_component_type(type_name, nullptr)) {
        return fail("contract spec.component_types has unknown canonical type",
                    0);
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
          runtime_source_types.find(c.spec.source_type) ==
              runtime_source_types.end()) {
        return fail(
            "contract spec.source_type does not match runtime source nodes", 0);
      }
      if (c.spec.source_type ==
              std::string(tsi_type_token(TsiTypeId::SourceDataloader)) &&
          !c.spec.has_positive_shape_hints()) {
        return fail("contract spec dataloader shape hints are incomplete", 0);
      }
    }

    if (!c.spec.representation_type.empty()) {
      TsiTypeId rep_id{};
      if (!is_known_canonical_component_type(c.spec.representation_type,
                                             &rep_id)) {
        return fail("contract spec.representation_type is not canonical/known",
                    0);
      }
      if (tsi_type_domain(rep_id) != TsiDomain::Wikimyei) {
        return fail("contract spec.representation_type domain mismatch", 0);
      }
      if (!runtime_representation_types.empty() &&
          runtime_representation_types.find(c.spec.representation_type) ==
              runtime_representation_types.end()) {
        return fail("contract spec.representation_type does not match runtime "
                    "wikimyei nodes",
                    0);
      }
      if (tsi_type_instance_policy(rep_id) ==
              TsiInstancePolicy::HashimyeiInstances &&
          runtime_representation_count > 0 &&
          c.spec.representation_hashimyei.empty()) {
        return fail("contract spec.representation_hashimyei is empty for "
                    "hashimyei type",
                    0);
      }
    }

    if (runtime_representation_count > 0) {
      if (c.spec.wikimyei_bindings.empty()) {
        return fail("contract spec.wikimyei_bindings is empty", 0);
      }

      std::unordered_map<
          std::string, const RuntimeBindingContract::WikimyeiBindingIdentity *>
          bindings_by_id;
      bindings_by_id.reserve(c.spec.wikimyei_bindings.size());
      for (const auto &binding : c.spec.wikimyei_bindings) {
        if (binding.binding_id.empty()) {
          return fail("contract spec.wikimyei_bindings has empty binding_id",
                      0);
        }
        if (binding.canonical_type.empty()) {
          return fail(
              "contract spec.wikimyei_bindings has empty canonical_type", 0);
        }
        TsiTypeId binding_type_id{};
        if (!is_known_canonical_component_type(binding.canonical_type,
                                               &binding_type_id)) {
          return fail("contract spec.wikimyei_bindings has unknown canonical "
                      "type",
                      0);
        }
        if (tsi_type_domain(binding_type_id) != TsiDomain::Wikimyei) {
          return fail("contract spec.wikimyei_bindings type domain mismatch",
                      0);
        }
        if (tsi_type_instance_policy(binding_type_id) ==
                TsiInstancePolicy::HashimyeiInstances &&
            binding.hashimyei.empty()) {
          return fail("contract spec.wikimyei_bindings is missing hashimyei "
                      "for hashimyei type",
                      0);
        }
        if (!bindings_by_id.emplace(binding.binding_id, &binding).second) {
          return fail("contract spec.wikimyei_bindings has duplicate "
                      "binding_id",
                      0);
        }
      }

      std::size_t matched_runtime_wikimyei = 0;
      for (const auto &n : c.nodes) {
        if (!n || n->domain() != TsiDomain::Wikimyei)
          continue;
        const std::string binding_id = std::string(n->instance_name());
        const auto binding_it = bindings_by_id.find(binding_id);
        if (binding_it == bindings_by_id.end()) {
          return fail("runtime wikimyei node is missing from "
                      "spec.wikimyei_bindings",
                      0);
        }
        std::string runtime_type{};
        if (!runtime_node_canonical_type(*n, &runtime_type) ||
            runtime_type.empty()) {
          return fail("runtime wikimyei node has unknown canonical type", 0);
        }
        if (binding_it->second->canonical_type != runtime_type) {
          return fail("spec.wikimyei_bindings canonical_type does not match "
                      "runtime wikimyei node",
                      0);
        }
        ++matched_runtime_wikimyei;
      }
      if (matched_runtime_wikimyei != c.spec.wikimyei_bindings.size()) {
        return fail("spec.wikimyei_bindings contains entry absent from "
                    "runtime graph",
                    0);
      }
    }

    if (!c.spec.source_type.empty() &&
        spec_component_types.find(c.spec.source_type) ==
            spec_component_types.end()) {
      return fail("contract spec.source_type missing in spec.component_types",
                  0);
    }
    if (!c.spec.representation_type.empty() &&
        spec_component_types.find(c.spec.representation_type) ==
            spec_component_types.end()) {
      return fail(
          "contract spec.representation_type missing in spec.component_types",
          0);
    }

    if (!runtime_component_types.empty()) {
      for (const auto &runtime_type : runtime_component_types) {
        if (spec_component_types.find(runtime_type) ==
            spec_component_types.end()) {
          return fail(
              "runtime canonical component missing from spec.component_types",
              0);
        }
      }
      for (const auto &spec_type : spec_component_types) {
        if (runtime_component_types.find(spec_type) ==
            runtime_component_types.end()) {
          return fail(
              "spec.component_types contains type absent from runtime graph",
              0);
        }
      }
    }
  }
  return true;
}
