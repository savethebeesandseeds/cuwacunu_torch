[[nodiscard]] static snapshot_ptr_t
snapshot_ptr_or_fail(const contract_hash_t &hash) {
  LOCK_GUARD(contract_config_mutex);
  auto &snapshots = snapshots_by_hash();
  const auto it = snapshots.find(hash);
  if (it == snapshots.end() || !it->second) {
    log_fatal("[dconfig] contract hash lookup failed: hash=%s is not "
              "registered in runtime registry\n",
              hash.c_str());
  }
  return it->second;
}

} // namespace

contract_space_t::_init contract_space_t::_initializer{};

void contract_space_t::lifecycle_init() {
  log_dbg(
      "[contract_space_t] initializing static-global contract registry "
      "(hash-keyed snapshots loaded lazily; no single locked runtime hash)\n");
}

void contract_space_t::lifecycle_finit() {
  std::size_t registered_count = 0;
  std::string last_registered_hash;
  std::string last_registered_path;
  {
    LOCK_GUARD(contract_config_mutex);
    registered_count = g_registered_contract_count;
    last_registered_hash = g_last_registered_contract_hash;
    last_registered_path = g_last_registered_contract_path_canonical;
  }

  log_dbg("[contract_space_t] finalizing static-global contract registry "
           "(registered_contracts=%zu, last_registered_hash=%s, "
           "last_registered_path=%s)\n",
           registered_count, non_empty_or(last_registered_hash, "<none>"),
           non_empty_or(last_registered_path, "<none>"));
}

/*──────────────────────── contract registry space ─────────────────────────*/
contract_hash_t
contract_space_t::register_contract_file(const std::string &path) {
  const std::string canonical_path = canonicalize_path_best_effort(path);
  if (!has_non_ws_ascii(canonical_path)) {
    log_fatal(
        "[dconfig] register_contract_file received empty/invalid path: %s\n",
        path.c_str());
  }

  std::optional<contract_hash_t> existing_hash;
  {
    LOCK_GUARD(contract_config_mutex);
    auto &path_to_hash = hash_by_contract_path();
    auto &snapshots = snapshots_by_hash();
    const auto existing = path_to_hash.find(canonical_path);
    if (existing != path_to_hash.end()) {
      const auto snap_it = snapshots.find(existing->second);
      if (snap_it == snapshots.end() || !snap_it->second) {
        log_fatal("[dconfig] contract registry corruption: path is mapped but "
                  "snapshot is missing (%s)\n",
                  canonical_path.c_str());
      }
      existing_hash = existing->second;
      g_registered_contract_count = snapshots.size();
      g_last_registered_contract_hash = existing->second;
      g_last_registered_contract_path_canonical = canonical_path;
    }
  }
  if (existing_hash.has_value()) {
    assert_intact_or_fail_fast(*existing_hash);
    return *existing_hash;
  }

  const auto built_snapshot =
      build_contract_record_from_contract_path(canonical_path);
  if (!built_snapshot) {
    log_fatal("[dconfig] failed to build contract snapshot from: %s\n",
              canonical_path.c_str());
  }
  const std::string signature_json =
      canonical_contract_signature_json(built_snapshot->signature);
  const contract_hash_t built_hash =
      sha256_hex_from_bytes(signature_json.data(), signature_json.size());
  if (!has_non_ws_ascii(built_hash)) {
    log_fatal(
        "[dconfig] built contract snapshot has empty signature hash for: %s\n",
        canonical_path.c_str());
  }

  {
    LOCK_GUARD(contract_config_mutex);
    auto &path_to_hash = hash_by_contract_path();
    auto &snapshots = snapshots_by_hash();
    const auto existing = path_to_hash.find(canonical_path);
    if (existing != path_to_hash.end()) {
      if (existing->second != built_hash) {
        log_fatal("[dconfig] immutable contract lock violation: attempted to "
                  "rebind contract path %s from hash %s to %s\n",
                  canonical_path.c_str(), existing->second.c_str(),
                  built_hash.c_str());
      }
      const auto snap_it = snapshots.find(existing->second);
      if (snap_it == snapshots.end() || !snap_it->second) {
        log_fatal("[dconfig] contract registry corruption: path is mapped but "
                  "snapshot is missing (%s)\n",
                  canonical_path.c_str());
      }
      existing_hash = existing->second;
    } else {
      auto by_hash = snapshots.find(built_hash);
      if (by_hash == snapshots.end()) {
        snapshots.emplace(built_hash, built_snapshot);
      } else if (!by_hash->second) {
        by_hash->second = built_snapshot;
      }
      path_to_hash[canonical_path] = built_hash;
    }
    g_registered_contract_count = snapshots.size();
    g_last_registered_contract_hash =
        existing_hash.has_value() ? *existing_hash : built_hash;
    g_last_registered_contract_path_canonical = canonical_path;
  }

  if (existing_hash.has_value()) {
    assert_intact_or_fail_fast(*existing_hash);
    return *existing_hash;
  }
  return built_hash;
}

std::shared_ptr<const contract_record_t>
contract_space_t::contract_itself(const contract_hash_t &hash) {
  return snapshot_ptr_or_fail(hash);
}

bool contract_space_t::validate_contract_file_isolated(
    const std::string &path, const std::string &global_config_path,
    std::string *error) {
  if (error)
    error->clear();
  const std::string trimmed_path = trim_ascii_copy(path);
  const std::string trimmed_global_config_path =
      trim_ascii_copy(global_config_path);
  if (!has_non_ws_ascii(trimmed_path)) {
    if (error)
      *error = "empty contract path";
    return false;
  }

  int pipe_fds[2]{-1, -1};
  if (::pipe(pipe_fds) != 0) {
    if (error) {
      *error = "failed to create isolated contract validation pipe";
    }
    return false;
  }

  const pid_t pid = ::fork();
  if (pid < 0) {
    ::close(pipe_fds[0]);
    ::close(pipe_fds[1]);
    if (error)
      *error = "failed to fork isolated contract validator";
    return false;
  }

  if (pid == 0) {
    ::close(pipe_fds[0]);
    (void)::dup2(pipe_fds[1], STDOUT_FILENO);
    (void)::dup2(pipe_fds[1], STDERR_FILENO);
    ::close(pipe_fds[1]);
    cuwacunu::piaabo::dlog_clear_buffer();
    cuwacunu::piaabo::dlog_set_terminal_output_enabled(false);
    cuwacunu::piaabo::dlog_buffer_capture_scope capture_logs(true);
    auto flush_captured_logs = []() {
      const auto lines = cuwacunu::piaabo::dlog_snapshot_lines(64);
      for (const auto &line : lines) {
        if (!has_non_ws_ascii(line))
          continue;
        std::cerr << line;
        if (line.back() != '\n')
          std::cerr << '\n';
      }
      std::cerr.flush();
    };
    try {
      if (has_non_ws_ascii(trimmed_global_config_path)) {
        cuwacunu::iitepi::config_space_t::change_config_file(
            trimmed_global_config_path.c_str());
      }
      (void)contract_space_t::register_contract_file(trimmed_path);
      _exit(0);
    } catch (const std::exception &e) {
      flush_captured_logs();
      if (has_non_ws_ascii(e.what())) {
        std::cerr << e.what() << '\n';
        std::cerr.flush();
      }
      _exit(1);
    } catch (...) {
      flush_captured_logs();
      std::cerr
          << "isolated contract validator caught non-standard exception\n";
      std::cerr.flush();
      _exit(1);
    }
  }

  ::close(pipe_fds[1]);
  std::string child_output{};
  const bool read_ok = read_all_fd(pipe_fds[0], &child_output);
  ::close(pipe_fds[0]);

  int status = 0;
  const pid_t waited = ::waitpid(pid, &status, 0);
  if (!read_ok) {
    if (error) {
      *error = "failed to read isolated contract validator output";
    }
    return false;
  }
  if (waited != pid) {
    if (error)
      *error = "failed to wait for isolated contract validator";
    return false;
  }
  if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
    return true;
  }

  std::string message = trim_ascii_copy(child_output);
  if (message.empty()) {
    if (WIFSIGNALED(status)) {
      message = "isolated contract validator terminated by signal " +
                std::to_string(WTERMSIG(status));
    } else if (WIFEXITED(status)) {
      message = "isolated contract validator exited with code " +
                std::to_string(WEXITSTATUS(status));
    } else {
      message = "isolated contract validator failed";
    }
  }
  if (error)
    *error = message;
  return false;
}

std::string contract_space_t::sha256_hex_for_file(const std::string &path) {
  const std::string canonical_path = canonicalize_path_best_effort(path);
  const std::string target_path =
      has_non_ws_ascii(canonical_path) ? canonical_path : path;
  if (!has_non_ws_ascii(target_path))
    return {};

  std::error_code ec{};
  if (!std::filesystem::exists(target_path, ec) ||
      !std::filesystem::is_regular_file(target_path, ec)) {
    return {};
  }

  try {
    const std::string payload = piaabo::dfiles::readFileToString(target_path);
    return sha256_hex_from_bytes(payload.data(), payload.size());
  } catch (...) {
    return {};
  }
}

void contract_space_t::assert_intact_or_fail_fast(const contract_hash_t &hash) {
  const auto snap = snapshot_ptr_or_fail(hash);

  std::vector<contract_file_fingerprint_t> refreshed;
  refreshed.reserve(snap->dependency_manifest.files.size());

  for (const auto &expected : snap->dependency_manifest.files) {
    const std::filesystem::path p(expected.canonical_path);
    if (!std::filesystem::exists(p) || !std::filesystem::is_regular_file(p)) {
      log_fatal("[dconfig] immutable contract lock violation: dependency "
                "missing or invalid: %s\n",
                expected.canonical_path.c_str());
    }

    contract_file_fingerprint_t current = expected;
    current.file_size_bytes = std::filesystem::file_size(p);
    current.mtime_ticks = file_mtime_ticks(p);
    current.sha256_hex = sha256_hex_from_file(expected.canonical_path);
    if (current.sha256_hex != expected.sha256_hex) {
      log_fatal("[dconfig] immutable contract lock violation: contract "
                "dependency changed "
                "mid-run: %s\n",
                expected.canonical_path.c_str());
    }
    refreshed.push_back(std::move(current));
  }

  const std::string digest = compute_manifest_digest_hex(refreshed);
  if (digest !=
      snap->dependency_manifest.dependency_manifest_aggregate_sha256_hex) {
    log_fatal("[dconfig] immutable contract lock violation: dependency "
              "manifest digest "
              "mismatch mid-run\n");
  }
}

void contract_space_t::assert_registry_intact_or_fail_fast() {
  std::vector<std::pair<contract_hash_t, snapshot_ptr_t>> snapshots;
  {
    LOCK_GUARD(contract_config_mutex);
    const auto &by_hash = snapshots_by_hash();
    snapshots.reserve(by_hash.size());
    for (const auto &[hash, ptr] : by_hash) {
      if (!ptr)
        continue;
      snapshots.emplace_back(hash, ptr);
    }
  }
  for (const auto &[hash, ptr] : snapshots) {
    if (!ptr)
      continue;
    assert_intact_or_fail_fast(hash);
  }
}

bool contract_space_t::has_contract(const contract_hash_t &hash) noexcept {
  LOCK_GUARD(contract_config_mutex);
  const auto &snapshots = snapshots_by_hash();
  const auto it = snapshots.find(hash);
  return it != snapshots.end() && static_cast<bool>(it->second);
}

std::vector<contract_hash_t> contract_space_t::registered_hashes() {
  LOCK_GUARD(contract_config_mutex);
  const auto &snapshots = snapshots_by_hash();
  std::vector<contract_hash_t> out{};
  out.reserve(snapshots.size());
  for (const auto &[hash, ptr] : snapshots) {
    if (ptr)
      out.push_back(hash);
  }
  std::sort(out.begin(), out.end());
  return out;
}

std::string contract_record_t::raw(const std::string &section,
                                   const std::string &key) const {
  if (section_supports_instruction_file(section)) {
    if (const auto instruction_value =
            contract_instruction_section_value_or_empty(*this, section, key);
        instruction_value.has_value()) {
      return *instruction_value;
    }
  }

  const auto s = config.find(section);
  if (s != config.end()) {
    const auto k = s->second.find(key);
    if (k != s->second.end())
      return k->second;
  }
  if (const auto instruction_value =
          contract_instruction_section_value_or_empty(*this, section, key);
      instruction_value.has_value()) {
    return *instruction_value;
  }
  if (s == config.end()) {
    if (module_config_path_for_section(*this, section).has_value() ||
        module_section_exists(*this, section)) {
      throw bad_config_access("Missing key <" + key + "> in [" + section + "]");
    }
    throw bad_config_access("Missing section [" + section + "]");
  }
  throw bad_config_access("Missing key <" + key + "> in [" + section + "]");
}

template <class T> T contract_record_t::from_string(const std::string &s) {
  return parse_scalar_from_string<T>(s);
}

template <class T>
T contract_record_t::get(const std::string &section, const std::string &key,
                         std::optional<T> fallback) const {
  try {
    return from_string<T>(raw(section, key));
  } catch (const bad_config_access &) {
    if (fallback)
      return *fallback;
    throw;
  }
}

template <class T>
std::vector<T>
contract_record_t::get_arr(const std::string &section, const std::string &key,
                           std::optional<std::vector<T>> fallback) const {
  try {
    const std::string s = raw(section, key);
    const std::vector<std::string> items = split_string_items(s);
    std::vector<T> result;
    result.reserve(items.size());
    for (const auto &it : items)
      result.emplace_back(from_string<T>(it));
    return result;
  } catch (const bad_config_access &) {
    if (fallback)
      return *fallback;
    throw;
  }
}

const cuwacunu::camahjucunu::tsiemene_circuit_instruction_t &
contract_record::circuit_blob_t::decoded() const {
  std::call_once(decode_once_, [&]() {
    try {
      auto parser = cuwacunu::camahjucunu::dsl::tsiemeneCircuits(grammar);
      std::string effective_dsl = strip_comments_from_text_preserve_lines(dsl);
      const std::size_t first_non_ws =
          effective_dsl.find_first_not_of(" \t\r\n");
      if (first_non_ws == std::string::npos) {
        throw std::runtime_error(
            "circuit DSL is empty after comment stripping");
      }
      if (first_non_ws > 0)
        effective_dsl.erase(0, first_non_ws);
      auto inst = parser.decode(effective_dsl);
      std::string semantic_error;
      if (!cuwacunu::camahjucunu::validate_circuit_instruction(
              inst, &semantic_error)) {
        throw std::runtime_error("circuit semantic validation failed: " +
                                 semantic_error);
      }
      decoded_cache_ = std::make_shared<
          cuwacunu::camahjucunu::tsiemene_circuit_instruction_t>(
          std::move(inst));
    } catch (const std::exception &e) {
      throw std::runtime_error(std::string("failed to decode circuit DSL: ") +
                               e.what());
    }
  });
  if (!decoded_cache_) {
    throw std::runtime_error(
        "failed to decode circuit DSL: empty decoded cache");
  }
  return *decoded_cache_;
}

const cuwacunu::camahjucunu::network_design_instruction_t &
contract_record::network_design_blob_t::decoded() const {
  std::call_once(decode_once_, [&]() {
    try {
      if (!has_payload()) {
        throw std::runtime_error("network design payload is empty");
      }
      auto inst = cuwacunu::camahjucunu::dsl::decode_network_design_from_dsl(
          grammar, dsl);
      decoded_cache_ =
          std::make_shared<cuwacunu::camahjucunu::network_design_instruction_t>(
              std::move(inst));
    } catch (const std::exception &e) {
      throw std::runtime_error(
          std::string("failed to decode network design DSL: ") + e.what());
    }
  });
  if (!decoded_cache_) {
    throw std::runtime_error(
        "failed to decode network design DSL: empty decoded cache");
  }
  return *decoded_cache_;
}

template int64_t contract_record_t::get<int64_t>(const std::string &,
                                                 const std::string &,
                                                 std::optional<int64_t>) const;
template int contract_record_t::get<int>(const std::string &,
                                         const std::string &,
                                         std::optional<int>) const;
template double contract_record_t::get<double>(const std::string &,
                                               const std::string &,
                                               std::optional<double>) const;
template float contract_record_t::get<float>(const std::string &,
                                             const std::string &,
                                             std::optional<float>) const;
template bool contract_record_t::get<bool>(const std::string &,
                                           const std::string &,
                                           std::optional<bool>) const;
template std::string
contract_record_t::get<std::string>(const std::string &, const std::string &,
                                    std::optional<std::string>) const;

template std::vector<int64_t>
contract_record_t::get_arr<int64_t>(const std::string &, const std::string &,
                                    std::optional<std::vector<int64_t>>) const;
template std::vector<int>
contract_record_t::get_arr<int>(const std::string &, const std::string &,
                                std::optional<std::vector<int>>) const;
template std::vector<double>
contract_record_t::get_arr<double>(const std::string &, const std::string &,
                                   std::optional<std::vector<double>>) const;
template std::vector<float>
contract_record_t::get_arr<float>(const std::string &, const std::string &,
                                  std::optional<std::vector<float>>) const;
template std::vector<bool>
contract_record_t::get_arr<bool>(const std::string &, const std::string &,
                                 std::optional<std::vector<bool>>) const;
template std::vector<std::string> contract_record_t::get_arr<std::string>(
    const std::string &, const std::string &,
    std::optional<std::vector<std::string>>) const;

} // namespace iitepi
} // namespace cuwacunu
