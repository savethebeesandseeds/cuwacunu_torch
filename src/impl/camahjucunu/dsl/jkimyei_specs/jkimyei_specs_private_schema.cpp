struct owner_schema_t {
  std::unordered_map<std::string, cuwacunu::jkimyei::specs::value_kind_e> key_kinds{};
  std::unordered_set<std::string> required_keys{};
};

struct component_schema_t {
  cuwacunu::jkimyei::specs::component_type_e type{};
  std::string canonical_type{};
  std::string kind_token{};
  std::vector<cuwacunu::jkimyei::specs::family_rule_t> family_rules{};
};

struct schema_index_t {
  std::unordered_map<std::string, owner_schema_t> owners{};
  std::unordered_map<std::string, component_schema_t> components{};
};

std::string lower_ascii_copy(std::string s) {
  for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  return s;
}

std::string trim_ascii_copy(const std::string& raw) {
  std::size_t begin = 0;
  while (begin < raw.size() && std::isspace(static_cast<unsigned char>(raw[begin]))) ++begin;
  std::size_t end = raw.size();
  while (end > begin && std::isspace(static_cast<unsigned char>(raw[end - 1]))) --end;
  return raw.substr(begin, end - begin);
}

std::vector<std::string> split_csv(const std::string& raw) {
  std::vector<std::string> out;
  if (raw.empty()) return out;
  std::size_t cursor = 0;
  while (cursor <= raw.size()) {
    const std::size_t comma = raw.find(',', cursor);
    std::string item = (comma == std::string::npos) ? raw.substr(cursor)
                                                     : raw.substr(cursor, comma - cursor);
    item = trim_ascii_copy(item);
    if (item.empty()) {
      throw std::runtime_error(
          cuwacunu::piaabo::string_format("invalid empty list element in value '%s'", raw.c_str()));
    }
    out.push_back(std::move(item));
    if (comma == std::string::npos) break;
    cursor = comma + 1;
  }
  return out;
}

bool try_parse_int64(const std::string& raw, long long* out) {
  if (!out) return false;
  const std::string text = trim_ascii_copy(raw);
  if (text.empty()) return false;
  char* end = nullptr;
  errno = 0;
  const long long value = std::strtoll(text.c_str(), &end, 10);
  if (errno != 0 || end == text.c_str() || *end != '\0') return false;
  *out = value;
  return true;
}

bool try_parse_f64(const std::string& raw, double* out) {
  if (!out) return false;
  const std::string text = trim_ascii_copy(raw);
  if (text.empty()) return false;
  char* end = nullptr;
  errno = 0;
  const double value = std::strtod(text.c_str(), &end);
  if (errno != 0 || end == text.c_str() || *end != '\0') return false;
  if (!std::isfinite(value)) return false;
  *out = value;
  return true;
}

const char* to_string(cuwacunu::jkimyei::specs::value_kind_e kind) {
  using value_kind_e = cuwacunu::jkimyei::specs::value_kind_e;
  switch (kind) {
    case value_kind_e::Bool: return "Bool";
    case value_kind_e::Int: return "Int";
    case value_kind_e::Float: return "Float";
    case value_kind_e::String: return "String";
    case value_kind_e::IntList: return "IntList";
    case value_kind_e::FloatList: return "FloatList";
    case value_kind_e::StringList: return "StringList";
  }
  return "Unknown";
}

const char* component_kind_token(cuwacunu::jkimyei::specs::component_type_e type) {
  using component_type_e = cuwacunu::jkimyei::specs::component_type_e;
  switch (type) {
#define JK_COMPONENT_TYPE(kind_token, canonical_type, description) \
  case component_type_e::kind_token: return #kind_token;
#include "jkimyei/specs/jkimyei_schema.def"
#undef JK_COMPONENT_TYPE
  }
  return "UNKNOWN";
}

bool is_value_kind_valid(cuwacunu::jkimyei::specs::value_kind_e kind, const std::string& raw_value) {
  const std::string value = trim_ascii_copy(raw_value);
  switch (kind) {
    case cuwacunu::jkimyei::specs::value_kind_e::Bool: {
      const std::string low = lower_ascii_copy(value);
      return low == "true" || low == "false";
    }
    case cuwacunu::jkimyei::specs::value_kind_e::Int: {
      long long tmp = 0;
      return try_parse_int64(value, &tmp);
    }
    case cuwacunu::jkimyei::specs::value_kind_e::Float: {
      double tmp = 0.0;
      return try_parse_f64(value, &tmp);
    }
    case cuwacunu::jkimyei::specs::value_kind_e::String: {
      return true;
    }
    case cuwacunu::jkimyei::specs::value_kind_e::IntList: {
      const std::vector<std::string> elems = split_csv(value);
      for (const auto& e : elems) {
        long long tmp = 0;
        if (!try_parse_int64(e, &tmp)) return false;
      }
      return true;
    }
    case cuwacunu::jkimyei::specs::value_kind_e::FloatList: {
      const std::vector<std::string> elems = split_csv(value);
      for (const auto& e : elems) {
        double tmp = 0.0;
        if (!try_parse_f64(e, &tmp)) return false;
      }
      return true;
    }
    case cuwacunu::jkimyei::specs::value_kind_e::StringList: {
      (void)split_csv(value);
      return true;
    }
  }
  return false;
}

const schema_index_t& schema_index() {
  static const schema_index_t index = []() -> schema_index_t {
    schema_index_t out{};

    for (const auto& d : cuwacunu::jkimyei::specs::kTypedParams) {
      owner_schema_t& owner = out.owners[std::string(d.owner)];
      const std::string key(d.key);
      auto it = owner.key_kinds.find(key);
      if (it != owner.key_kinds.end() && it->second != d.kind) {
        throw std::logic_error(
            cuwacunu::piaabo::string_format("schema duplicate key '%s' with different types",
                                            key.c_str()));
      }
      owner.key_kinds[key] = d.kind;
      if (d.required) owner.required_keys.insert(key);
    }

    for (const auto& comp : cuwacunu::jkimyei::specs::kComponents) {
      component_schema_t schema{};
      schema.type = comp.type;
      schema.canonical_type = std::string(comp.canonical_type);
      schema.kind_token = std::string(component_kind_token(comp.type));
      out.components[schema.canonical_type] = std::move(schema);
    }

    for (const auto& rule : cuwacunu::jkimyei::specs::kFamilyRules) {
      std::string canonical;
      for (const auto& comp : cuwacunu::jkimyei::specs::kComponents) {
        if (comp.type == rule.type) {
          canonical = std::string(comp.canonical_type);
          break;
        }
      }
      if (canonical.empty()) continue;
      out.components[canonical].family_rules.push_back(rule);
    }

    return out;
  }();
  return index;
}

const component_schema_t& resolve_component_schema(const component_t& component) {
  const auto& idx = schema_index();
  auto it = idx.components.find(component.canonical_type);
  if (it == idx.components.end()) {
    throw std::runtime_error(cuwacunu::piaabo::string_format(
        "unknown COMPONENT canonical type '%s' for id '%s'",
        component.canonical_type.c_str(),
        component.id.c_str()));
  }
  return it->second;
}

const std::string* find_kv(const kv_list_t& kv, const std::string& key) {
  for (const auto& entry : kv.entries) {
    if (entry.first == key) return &entry.second;
  }
  return nullptr;
}

const profile_t* find_profile(const component_t& c, const std::string& profile_name);

bool family_present_for_profile(const component_t& component,
                                const profile_t& profile,
                                const std::string& family) {
  (void)component;
  if (family == "Optimizer") return profile.optimizer.present;
  if (family == "Scheduler") return profile.lr_scheduler.present;
  if (family == "Loss") return profile.loss.present;
  if (family == "ComponentParams") return profile.component_params_present;
  if (family == "Reproducibility") return false;
  if (family == "Numerics") return profile.numerics_present;
  if (family == "Gradient") return profile.gradient_present;
  if (family == "Checkpoint") return profile.checkpoint_present;
  if (family == "Metrics") return profile.metrics_present;
  if (family == "DataRef") return profile.data_ref_present;
  if (family == "Augmentations") return profile.augmentations_present;
  throw std::runtime_error(cuwacunu::piaabo::string_format(
      "unsupported schema family token '%s'", family.c_str()));
}

void validate_kv_with_owner_schema(const kv_list_t& kv,
                                   const std::string& owner,
                                   const std::string& context) {
  const auto& idx = schema_index();
  auto owner_it = idx.owners.find(owner);
  if (owner_it == idx.owners.end()) {
    throw std::runtime_error(cuwacunu::piaabo::string_format(
        "%s references unknown schema owner '%s'",
        context.c_str(),
        owner.c_str()));
  }

  const owner_schema_t& owner_schema = owner_it->second;
  std::unordered_set<std::string> seen_keys;
  for (const auto& [key, value] : kv.entries) {
    if (!seen_keys.insert(key).second) {
      throw std::runtime_error(cuwacunu::piaabo::string_format(
          "%s repeats key '%s' in owner '%s'",
          context.c_str(),
          key.c_str(),
          owner.c_str()));
    }

    auto kind_it = owner_schema.key_kinds.find(key);
    if (kind_it == owner_schema.key_kinds.end()) {
      throw std::runtime_error(cuwacunu::piaabo::string_format(
          "%s uses unknown key '%s' for owner '%s'",
          context.c_str(),
          key.c_str(),
          owner.c_str()));
    }
    if (!is_value_kind_valid(kind_it->second, value)) {
      throw std::runtime_error(cuwacunu::piaabo::string_format(
          "%s key '%s' expects %s but got '%s'",
          context.c_str(),
          key.c_str(),
          to_string(kind_it->second),
          value.c_str()));
    }
  }

  for (const auto& req_key : owner_schema.required_keys) {
    if (seen_keys.find(req_key) == seen_keys.end()) {
      throw std::runtime_error(cuwacunu::piaabo::string_format(
          "%s is missing required key '%s' for owner '%s'",
          context.c_str(),
          req_key.c_str(),
          owner.c_str()));
    }
  }
}

void validate_component(const component_t& component) {
  const component_schema_t& schema = resolve_component_schema(component);
  if (component.profiles.empty()) {
    throw std::runtime_error(cuwacunu::piaabo::string_format(
        "COMPONENT '%s' has no PROFILE blocks",
        component.id.c_str()));
  }

  if (component.active_profile.empty()) {
    throw std::runtime_error(cuwacunu::piaabo::string_format(
        "COMPONENT '%s' missing ACTIVE_PROFILE",
        component.id.c_str()));
  }
  const profile_t* active_profile = find_profile(component, component.active_profile);
  if (!active_profile) {
    throw std::runtime_error(cuwacunu::piaabo::string_format(
        "COMPONENT '%s' ACTIVE_PROFILE '%s' does not match any PROFILE",
        component.id.c_str(),
        component.active_profile.c_str()));
  }

  for (const auto& profile : component.profiles) {
    const std::string context = cuwacunu::piaabo::string_format(
        "COMPONENT '%s' PROFILE '%s'",
        component.id.c_str(),
        profile.name.c_str());

    for (const auto& family_rule : schema.family_rules) {
      const std::string family(family_rule.family);
      const bool present = family_present_for_profile(component, profile, family);
      if (family_rule.required && !present) {
        throw std::runtime_error(cuwacunu::piaabo::string_format(
            "%s missing required family '%s'",
            context.c_str(),
            family.c_str()));
      }
      if (!family_rule.required && present) {
        throw std::runtime_error(cuwacunu::piaabo::string_format(
            "%s has forbidden family '%s'",
            context.c_str(),
            family.c_str()));
      }
    }

    if (profile.optimizer.present) {
      validate_kv_with_owner_schema(
          profile.optimizer.kv,
          "optimizer." + profile.optimizer.name,
          context + " OPTIMIZER");
    }
    if (profile.lr_scheduler.present) {
      validate_kv_with_owner_schema(
          profile.lr_scheduler.kv,
          "scheduler." + profile.lr_scheduler.name,
          context + " LR_SCHEDULER");
    }
    if (profile.loss.present) {
      validate_kv_with_owner_schema(
          profile.loss.kv,
          "loss." + profile.loss.name,
          context + " LOSS");
    }
    if (profile.component_params_present) {
      validate_kv_with_owner_schema(
          profile.component_params,
          "component." + schema.kind_token,
          context + " COMPONENT_PARAMS");
    }
    if (profile.numerics_present) {
      validate_kv_with_owner_schema(profile.numerics, "numerics", context + " NUMERICS");
    }
    if (profile.gradient_present) {
      validate_kv_with_owner_schema(profile.gradient, "gradient", context + " GRADIENT");
    }
    if (profile.checkpoint_present) {
      validate_kv_with_owner_schema(profile.checkpoint, "checkpoint", context + " CHECKPOINT");
    }
    if (profile.metrics_present) {
      validate_kv_with_owner_schema(profile.metrics, "metrics", context + " METRICS");
    }
    if (profile.data_ref_present) {
      validate_kv_with_owner_schema(profile.data_ref, "data_ref", context + " DATA_REF");
    }
    if (profile.augmentations_present) {
      for (const auto& curve : profile.augmentations) {
        const std::string curve_context =
            context + " AUGMENTATIONS CURVE '" + curve.name + "'";
        kv_list_t curve_kv =
            canonicalize_augmentation_curve_kv(curve.kv, curve_context);
        const std::string* explicit_curve_name =
            find_kv(curve_kv, kAugmentationCurveKey);
        if (explicit_curve_name && *explicit_curve_name != curve.name) {
          throw std::runtime_error(cuwacunu::piaabo::string_format(
              "%s defines mismatched %s '%s'",
              context.c_str(),
              kAugmentationCurveKey,
              explicit_curve_name->c_str()));
        }
        if (!explicit_curve_name) {
          curve_kv.entries.emplace_back(kAugmentationCurveKey, curve.name);
        }
        validate_kv_with_owner_schema(
            curve_kv,
            "augmentation.curve",
            curve_context);
      }
    }
  }

  (void)active_profile;
}

void validate_document(const document_t& doc) {
  for (const auto& component : doc.components) {
    validate_component(component);
  }
}
