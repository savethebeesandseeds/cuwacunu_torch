std::string quote_if_needed(const std::string& v) {
  if (v.find(',') == std::string::npos && v.find(' ') == std::string::npos) return v;
  std::string out = "\"";
  out += v;
  out += '"';
  return out;
}

std::string options_kv_string(const kv_list_t& kv) {
  std::ostringstream oss;
  for (std::size_t i = 0; i < kv.entries.size(); ++i) {
    if (i > 0) oss << ',';
    oss << kv.entries[i].first << '=' << quote_if_needed(kv.entries[i].second);
  }
  return oss.str();
}

void append_kv_to_row(const kv_list_t& src,
                      cuwacunu::camahjucunu::jkimyei_specs_t::row_t* dst) {
  if (!dst) return;
  for (const auto& kv : src.entries) {
    (*dst)[kv.first] = kv.second;
  }
}

void push_row(cuwacunu::camahjucunu::jkimyei_specs_t* out,
              const std::string& table_name,
              cuwacunu::camahjucunu::jkimyei_specs_t::row_t row) {
  out->tables[table_name].push_back(std::move(row));
}

const profile_t* find_profile(const component_t& c, const std::string& profile_name) {
  for (const auto& p : c.profiles) {
    if (p.name == profile_name) return &p;
  }
  return nullptr;
}

void materialize_profile_tables(const component_t& component,
                                const profile_t& profile,
                                bool active,
                                cuwacunu::camahjucunu::jkimyei_specs_t* out) {
  const std::string profile_id = component.id + "@" + profile.name;
  const std::string optimizer_id = profile_id + "::optimizer";
  const std::string scheduler_id = profile_id + "::scheduler";
  const std::string loss_id = profile_id + "::loss";

  {
    cuwacunu::camahjucunu::jkimyei_specs_t::row_t row{};
    row[ROW_ID_COLUMN_HEADER] = optimizer_id;
    row["type"] = profile.optimizer.name;
    row["options"] = options_kv_string(profile.optimizer.kv);
    push_row(out, "optimizers_table", std::move(row));
  }

  {
    cuwacunu::camahjucunu::jkimyei_specs_t::row_t row{};
    row[ROW_ID_COLUMN_HEADER] = scheduler_id;
    row["type"] = profile.lr_scheduler.name;
    row["options"] = options_kv_string(profile.lr_scheduler.kv);
    push_row(out, "lr_schedulers_table", std::move(row));
  }

  {
    cuwacunu::camahjucunu::jkimyei_specs_t::row_t row{};
    row[ROW_ID_COLUMN_HEADER] = loss_id;
    row["type"] = profile.loss.name;
    row["options"] = options_kv_string(profile.loss.kv);
    push_row(out, "loss_functions_table", std::move(row));
  }

  {
    cuwacunu::camahjucunu::jkimyei_specs_t::row_t row{};
    row[ROW_ID_COLUMN_HEADER] = profile_id;
    row["component_id"] = component.id;
    row["component_type"] = component.canonical_type;
    row["profile_id"] = profile.name;
    row["optimizer"] = optimizer_id;
    row["lr_scheduler"] = scheduler_id;
    row["loss_function"] = loss_id;
    row["active"] = active ? "true" : "false";
    append_kv_to_row(profile.component_params, &row);
    push_row(out, "component_profiles_table", std::move(row));
  }

  if (profile.numerics_present) {
    cuwacunu::camahjucunu::jkimyei_specs_t::row_t row{};
    row[ROW_ID_COLUMN_HEADER] = profile_id;
    row["component_id"] = component.id;
    append_kv_to_row(profile.numerics, &row);
    push_row(out, "component_numerics_table", std::move(row));
  }

  if (profile.gradient_present) {
    cuwacunu::camahjucunu::jkimyei_specs_t::row_t row{};
    row[ROW_ID_COLUMN_HEADER] = profile_id;
    row["component_id"] = component.id;
    append_kv_to_row(profile.gradient, &row);
    push_row(out, "component_gradient_table", std::move(row));
  }

  if (profile.checkpoint_present) {
    cuwacunu::camahjucunu::jkimyei_specs_t::row_t row{};
    row[ROW_ID_COLUMN_HEADER] = profile_id;
    row["component_id"] = component.id;
    append_kv_to_row(profile.checkpoint, &row);
    push_row(out, "component_checkpoint_table", std::move(row));
  }

  if (profile.metrics_present) {
    cuwacunu::camahjucunu::jkimyei_specs_t::row_t row{};
    row[ROW_ID_COLUMN_HEADER] = profile_id;
    row["component_id"] = component.id;
    append_kv_to_row(profile.metrics, &row);
    push_row(out, "component_metrics_table", std::move(row));
  }

  if (profile.data_ref_present) {
    cuwacunu::camahjucunu::jkimyei_specs_t::row_t row{};
    row[ROW_ID_COLUMN_HEADER] = profile_id;
    row["component_id"] = component.id;
    append_kv_to_row(profile.data_ref, &row);
    push_row(out, "component_data_ref_table", std::move(row));
  }

  if (active) {
    cuwacunu::camahjucunu::jkimyei_specs_t::row_t row{};
    row[ROW_ID_COLUMN_HEADER] = component.id;
    row["component_type"] = component.canonical_type;
    row["active_profile"] = profile.name;
    row["optimizer"] = optimizer_id;
    row["lr_scheduler"] = scheduler_id;
    row["loss_function"] = loss_id;
    append_kv_to_row(profile.component_params, &row);
    push_row(out, "components_table", std::move(row));
  }
}

void materialize_profile_augmentations(const component_t& component,
                                       const profile_t& profile,
                                       cuwacunu::camahjucunu::jkimyei_specs_t* out) {
  if (!profile.augmentations_present) return;
  const std::string profile_row_id = component.id + "@" + profile.name;
  std::size_t curve_index = 0;
  for (const auto& curve : profile.augmentations) {
    const std::string curve_context =
        profile_row_id + " AUGMENTATIONS CURVE '" + curve.name + "'";
    const kv_list_t curve_kv =
        canonicalize_augmentation_curve_kv(curve.kv, curve_context);
    cuwacunu::camahjucunu::jkimyei_specs_t::row_t row{};
    row[ROW_ID_COLUMN_HEADER] = cuwacunu::piaabo::string_format(
        "%s::augmentation::%zu",
        profile_row_id.c_str(),
        curve_index);
    row["component_id"] = component.id;
    row["component_type"] = component.canonical_type;
    row["profile_id"] = profile.name;
    row["profile_row_id"] = profile_row_id;
    append_kv_to_row(curve_kv, &row);
    row[kAugmentationCurveKey] = curve.name;
    push_row(out, "vicreg_augmentations", std::move(row));
    ++curve_index;
  }
}

void materialize_document(const document_t& doc,
                          cuwacunu::camahjucunu::jkimyei_specs_t* out) {
  for (const auto& component : doc.components) {
    for (const auto& profile : component.profiles) {
      materialize_profile_tables(component, profile, profile.name == component.active_profile, out);
      materialize_profile_augmentations(component, profile, out);
    }
  }
}

} // namespace
