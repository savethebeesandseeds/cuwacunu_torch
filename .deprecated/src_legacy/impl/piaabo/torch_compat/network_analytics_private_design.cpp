network_design_analytics_report_t summarize_network_design_analytics(
    const cuwacunu::camahjucunu::network_design_instruction_t& instruction) {
  network_design_analytics_report_t out{};
  out.network_id = instruction.network_id;
  out.assembly_tag = instruction.assembly_tag;
  out.node_count = static_cast<std::uint64_t>(instruction.nodes.size());
  out.export_count = static_cast<std::uint64_t>(instruction.exports.size());

  const std::size_t n = instruction.nodes.size();
  if (n == 0) {
    out.topological_order_count = 0;
    out.topological_order_ratio = 0.0;
    out.active_fanout_mean = 0.0;
    out.edge_surplus = 0.0;
    out.explicit_edge_count = 0;
    out.inferred_edge_count = out.internal_edge_count;
    return out;
  }

  std::unordered_map<std::string, std::size_t> node_index;
  node_index.reserve(n);
  for (std::size_t i = 0; i < n; ++i) {
    const auto [it, inserted] = node_index.emplace(instruction.nodes[i].id, i);
    if (!inserted) {
      out.analysis_valid = false;
      out.analysis_error = "duplicate_node_id";
      ++out.duplicate_node_id_count;
      if (out.duplicate_node_id_example.empty()) {
        out.duplicate_node_id_example = instruction.nodes[i].id;
      }
      continue;
    }
  }
  if (!out.analysis_valid) return out;

  std::vector<std::unordered_set<std::size_t>> internal_out(n);
  std::vector<std::uint64_t> in_degree(n, 0ULL);
  std::vector<std::uint64_t> out_degree_internal(n, 0ULL);
  std::vector<std::uint64_t> out_degree_export(n, 0ULL);

  std::unordered_map<std::string, std::uint64_t> node_kind_counts;
  std::unordered_map<std::string, std::uint64_t> edge_transition_counts;

  node_kind_counts.reserve(n);
  for (const auto& node : instruction.nodes) {
    ++node_kind_counts[node.kind];
  }
  out.distinct_node_kind_count =
      static_cast<std::uint64_t>(node_kind_counts.size());

  out.unresolved_identifier_token_count = 0;
  out.self_reference_token_count = 0;

  for (std::size_t dst = 0; dst < n; ++dst) {
    const auto& node = instruction.nodes[dst];
    for (const auto& param : node.params) {
      ++out.parameter_count;
      const auto tokens = extract_identifier_tokens_(param.value);
      std::unordered_set<std::size_t> source_candidates{};
      for (const auto& token : tokens) {
        const auto it = node_index.find(token);
        if (it == node_index.end()) {
          ++out.unresolved_identifier_token_count;
          continue;
        }
        if (it->second == dst) {
          ++out.self_reference_token_count;
        }
        source_candidates.insert(it->second);
      }
      for (const std::size_t src : source_candidates) {
        if (!internal_out[src].insert(dst).second) continue;
        ++in_degree[dst];
        ++out_degree_internal[src];
      }
    }
  }

  out.internal_edge_count = 0;
  for (const auto& targets : internal_out) {
    out.internal_edge_count += static_cast<std::uint64_t>(targets.size());
  }

  out.export_edge_count = 0;
  for (const auto& ex : instruction.exports) {
    const auto it = node_index.find(ex.node_id);
    if (it == node_index.end()) continue;
    ++out_degree_export[it->second];
    ++out.export_edge_count;
    const std::string relation = instruction.nodes[it->second].kind + "->EXPORT";
    ++edge_transition_counts[relation];
  }

  for (std::size_t src = 0; src < n; ++src) {
    for (const std::size_t dst : internal_out[src]) {
      const std::string relation =
          instruction.nodes[src].kind + "->" + instruction.nodes[dst].kind;
      ++edge_transition_counts[relation];
    }
  }

  out.total_edge_count = out.internal_edge_count + out.export_edge_count;
  out.explicit_edge_count = 0;
  out.inferred_edge_count = out.internal_edge_count;

  std::vector<std::uint64_t> out_degree_total(n, 0ULL);
  std::uint64_t isolated = 0;
  std::uint64_t max_in = 0;
  std::uint64_t max_out = 0;
  long double sum_in = 0.0L;
  long double sum_out = 0.0L;
  long double branch_sum = 0.0L;
  std::uint64_t branch_count = 0;

  out.source_count = 0;
  out.internal_sink_count = 0;
  out.branch_node_count = 0;
  out.merge_node_count = 0;

  for (std::size_t i = 0; i < n; ++i) {
    out_degree_total[i] = out_degree_internal[i] + out_degree_export[i];
    if (in_degree[i] == 0 && out_degree_total[i] == 0) ++isolated;
    if (in_degree[i] == 0) ++out.source_count;
    if (out_degree_internal[i] == 0) ++out.internal_sink_count;
    if (out_degree_internal[i] > 1) ++out.branch_node_count;
    if (in_degree[i] > 1) ++out.merge_node_count;

    max_in = std::max(max_in, in_degree[i]);
    max_out = std::max(max_out, out_degree_total[i]);
    sum_in += static_cast<long double>(in_degree[i]);
    sum_out += static_cast<long double>(out_degree_total[i]);
    if (out_degree_total[i] > 0) {
      branch_sum += static_cast<long double>(out_degree_total[i]);
      ++branch_count;
    }
  }

  out.isolated_node_count = isolated;
  out.max_in_degree = max_in;
  out.max_out_degree = max_out;
  out.mean_in_degree = safe_ratio(static_cast<double>(sum_in), static_cast<double>(n));
  out.mean_out_degree = safe_ratio(static_cast<double>(sum_out), static_cast<double>(n));
  out.active_fanout_mean =
      safe_ratio(static_cast<double>(branch_sum), static_cast<double>(branch_count));

  if (n > 1) {
    const long double max_edges =
        static_cast<long double>(n) * static_cast<long double>(n - 1);
    out.internal_edge_density = clamp01(safe_ratio(
        static_cast<double>(out.internal_edge_count),
        static_cast<double>(max_edges)));
  }

  std::vector<std::vector<std::size_t>> internal_adj(n);
  for (std::size_t src = 0; src < n; ++src) {
    internal_adj[src].reserve(internal_out[src].size());
    for (const std::size_t dst : internal_out[src]) {
      internal_adj[src].push_back(dst);
    }
  }

  std::vector<std::vector<std::size_t>> undirected(n);
  std::vector<std::vector<std::size_t>> reverse_adj(n);
  for (std::size_t src = 0; src < n; ++src) {
    for (const std::size_t dst : internal_adj[src]) {
      undirected[src].push_back(dst);
      undirected[dst].push_back(src);
      reverse_adj[dst].push_back(src);
    }
  }

  std::vector<unsigned char> visited(n, static_cast<unsigned char>(0));
  std::uint64_t components = 0;
  for (std::size_t i = 0; i < n; ++i) {
    if (visited[i] != 0) continue;
    ++components;
    std::queue<std::size_t> q;
    q.push(i);
    visited[i] = 1;
    while (!q.empty()) {
      const std::size_t u = q.front();
      q.pop();
      for (const auto v : undirected[u]) {
        if (visited[v] != 0) continue;
        visited[v] = 1;
        q.push(v);
      }
    }
  }
  out.weakly_connected_components = components;
  out.edge_surplus = static_cast<double>(
      static_cast<long double>(out.internal_edge_count) -
      static_cast<long double>(n) +
      static_cast<long double>(components));

  std::vector<std::uint64_t> in_degree_work = in_degree;
  std::queue<std::size_t> topo_queue;
  for (std::size_t i = 0; i < n; ++i) {
    if (in_degree_work[i] == 0) topo_queue.push(i);
  }
  std::vector<std::size_t> topo_order;
  topo_order.reserve(n);
  while (!topo_queue.empty()) {
    const std::size_t u = topo_queue.front();
    topo_queue.pop();
    topo_order.push_back(u);
    for (const auto v : internal_adj[u]) {
      if (in_degree_work[v] == 0) continue;
      --in_degree_work[v];
      if (in_degree_work[v] == 0) topo_queue.push(v);
    }
  }

  out.topological_order_count = static_cast<std::uint64_t>(topo_order.size());
  out.topological_order_ratio = safe_ratio(
      static_cast<double>(out.topological_order_count),
      static_cast<double>(n));
  out.has_internal_cycle = (topo_order.size() != n);
  if (!out.has_internal_cycle) {
    std::vector<std::uint64_t> longest_to(n, static_cast<std::uint64_t>(1));
    for (const std::size_t u : topo_order) {
      for (const auto v : internal_adj[u]) {
        longest_to[v] =
            std::max(longest_to[v], longest_to[u] + static_cast<std::uint64_t>(1));
      }
    }
    out.acyclic_longest_path_nodes =
        *std::max_element(longest_to.begin(), longest_to.end());
  }

  {
    std::vector<unsigned char> export_reachable(n, static_cast<unsigned char>(0));
    std::queue<std::size_t> q;
    for (std::size_t i = 0; i < n; ++i) {
      if (out_degree_export[i] == 0) continue;
      if (export_reachable[i] != 0) continue;
      export_reachable[i] = 1;
      q.push(i);
    }
    while (!q.empty()) {
      const std::size_t node = q.front();
      q.pop();
      for (const std::size_t pred : reverse_adj[node]) {
        if (export_reachable[pred] != 0) continue;
        export_reachable[pred] = 1;
        q.push(pred);
      }
    }

    std::uint64_t reachable_count = 0;
    for (const auto flag : export_reachable) {
      if (flag != 0) ++reachable_count;
    }
    out.export_reachable_node_count = reachable_count;
    out.export_reachable_ratio = safe_ratio(
        static_cast<double>(reachable_count),
        static_cast<double>(n));
    out.dead_end_node_count = static_cast<std::uint64_t>(n) - reachable_count;
  }

  {
    std::vector<unsigned char> source_reachable(n, static_cast<unsigned char>(0));
    std::queue<std::size_t> q;
    for (std::size_t i = 0; i < n; ++i) {
      if (in_degree[i] != 0) continue;
      if (source_reachable[i] != 0) continue;
      source_reachable[i] = 1;
      q.push(i);
    }
    while (!q.empty()) {
      const std::size_t u = q.front();
      q.pop();
      for (const std::size_t v : internal_adj[u]) {
        if (source_reachable[v] != 0) continue;
        source_reachable[v] = 1;
        q.push(v);
      }
    }

    std::uint64_t orphan_count = 0;
    for (const auto flag : source_reachable) {
      if (flag == 0) ++orphan_count;
    }
    out.orphan_node_count = orphan_count;
  }

  std::vector<int> tarjan_index(n, -1);
  std::vector<int> tarjan_low(n, -1);
  std::vector<unsigned char> tarjan_on_stack(n, static_cast<unsigned char>(0));
  std::vector<std::size_t> tarjan_stack{};
  std::vector<int> node_to_scc(n, -1);
  std::vector<std::vector<std::size_t>> scc_nodes{};
  int next_tarjan_index = 0;

  std::function<void(std::size_t)> strong_connect = [&](std::size_t v) {
    tarjan_index[v] = next_tarjan_index;
    tarjan_low[v] = next_tarjan_index;
    ++next_tarjan_index;

    tarjan_stack.push_back(v);
    tarjan_on_stack[v] = 1;

    for (const std::size_t w : internal_adj[v]) {
      if (tarjan_index[w] < 0) {
        strong_connect(w);
        tarjan_low[v] = std::min(tarjan_low[v], tarjan_low[w]);
      } else if (tarjan_on_stack[w] != 0) {
        tarjan_low[v] = std::min(tarjan_low[v], tarjan_index[w]);
      }
    }

    if (tarjan_low[v] != tarjan_index[v]) return;

    const int scc_id = static_cast<int>(scc_nodes.size());
    std::vector<std::size_t> component{};
    while (!tarjan_stack.empty()) {
      const std::size_t w = tarjan_stack.back();
      tarjan_stack.pop_back();
      tarjan_on_stack[w] = 0;
      node_to_scc[w] = scc_id;
      component.push_back(w);
      if (w == v) break;
    }
    scc_nodes.push_back(std::move(component));
  };

  for (std::size_t v = 0; v < n; ++v) {
    if (tarjan_index[v] < 0) strong_connect(v);
  }

  out.scc_count = static_cast<std::uint64_t>(scc_nodes.size());
  out.largest_scc_size = 0;
  out.cyclic_node_count = 0;
  for (const auto& component : scc_nodes) {
    out.largest_scc_size =
        std::max(out.largest_scc_size, static_cast<std::uint64_t>(component.size()));
    bool has_self_loop = false;
    if (component.size() == 1) {
      const std::size_t u = component.front();
      for (const std::size_t v : internal_adj[u]) {
        if (v == u) {
          has_self_loop = true;
          break;
        }
      }
    }
    if (component.size() > 1 || has_self_loop) {
      out.cyclic_node_count += static_cast<std::uint64_t>(component.size());
    }
  }
  out.cyclic_node_ratio = safe_ratio(
      static_cast<double>(out.cyclic_node_count),
      static_cast<double>(n));

  const std::size_t scc_n = scc_nodes.size();
  std::vector<std::unordered_set<std::size_t>> dag_out_sets(scc_n);
  std::vector<std::vector<std::size_t>> dag_out(scc_n);
  std::vector<std::uint64_t> dag_in_degree(scc_n, 0ULL);

  for (std::size_t src = 0; src < n; ++src) {
    const std::size_t src_scc = static_cast<std::size_t>(node_to_scc[src]);
    for (const std::size_t dst : internal_adj[src]) {
      const std::size_t dst_scc = static_cast<std::size_t>(node_to_scc[dst]);
      if (src_scc == dst_scc) continue;
      if (!dag_out_sets[src_scc].insert(dst_scc).second) continue;
      dag_out[src_scc].push_back(dst_scc);
      ++dag_in_degree[dst_scc];
    }
  }

  std::vector<unsigned char> source_scc(scc_n, static_cast<unsigned char>(0));
  std::vector<unsigned char> export_scc(scc_n, static_cast<unsigned char>(0));
  for (std::size_t sid = 0; sid < scc_n; ++sid) {
    if (dag_in_degree[sid] == 0) source_scc[sid] = 1;
  }
  for (std::size_t i = 0; i < n; ++i) {
    const std::size_t sid = static_cast<std::size_t>(node_to_scc[i]);
    if (out_degree_export[i] > 0) export_scc[sid] = 1;
  }

  std::vector<std::uint64_t> dag_in_work = dag_in_degree;
  std::queue<std::size_t> dag_queue;
  for (std::size_t sid = 0; sid < scc_n; ++sid) {
    if (dag_in_work[sid] == 0) dag_queue.push(sid);
  }
  std::vector<std::size_t> dag_topo_order{};
  dag_topo_order.reserve(scc_n);
  while (!dag_queue.empty()) {
    const std::size_t sid = dag_queue.front();
    dag_queue.pop();
    dag_topo_order.push_back(sid);
    for (const std::size_t tid : dag_out[sid]) {
      if (dag_in_work[tid] == 0) continue;
      --dag_in_work[tid];
      if (dag_in_work[tid] == 0) dag_queue.push(tid);
    }
  }

  std::vector<unsigned char> path_reachable(scc_n, static_cast<unsigned char>(0));
  std::vector<std::uint64_t> path_nodes(scc_n, 0ULL);
  for (std::size_t sid = 0; sid < scc_n; ++sid) {
    if (source_scc[sid] == 0) continue;
    path_reachable[sid] = 1;
    path_nodes[sid] = static_cast<std::uint64_t>(scc_nodes[sid].size());
  }

  for (const std::size_t sid : dag_topo_order) {
    for (const std::size_t tid : dag_out[sid]) {
      if (path_reachable[sid] == 0) continue;
      const std::uint64_t candidate =
          path_nodes[sid] + static_cast<std::uint64_t>(scc_nodes[tid].size());
      if (path_reachable[tid] == 0 || candidate > path_nodes[tid]) {
        path_reachable[tid] = 1;
        path_nodes[tid] = candidate;
      }
    }
  }

  out.longest_source_to_export_path_nodes = 0;
  for (std::size_t sid = 0; sid < scc_n; ++sid) {
    if (export_scc[sid] == 0) continue;
    if (path_reachable[sid] == 0) continue;
    out.longest_source_to_export_path_nodes =
        std::max(out.longest_source_to_export_path_nodes, path_nodes[sid]);
  }

  {
    std::vector<std::uint64_t> export_lengths{};
    export_lengths.reserve(instruction.exports.size());
    for (const auto& ex : instruction.exports) {
      const auto it = node_index.find(ex.node_id);
      if (it == node_index.end()) continue;
      const std::size_t sid = static_cast<std::size_t>(node_to_scc[it->second]);
      if (path_reachable[sid] == 0) continue;
      export_lengths.push_back(path_nodes[sid]);
    }

    if (!export_lengths.empty()) {
      std::sort(export_lengths.begin(), export_lengths.end());
      const std::size_t mid = export_lengths.size() / 2;
      if ((export_lengths.size() % 2) == 1) {
        out.median_source_to_export_path_nodes = export_lengths[mid];
      } else {
        out.median_source_to_export_path_nodes =
            (export_lengths[mid - 1] + export_lengths[mid]) / 2;
      }
    }
  }

  {
    std::vector<unsigned char> depth_reachable(scc_n, static_cast<unsigned char>(0));
    std::vector<std::uint64_t> depth(scc_n, 0ULL);
    for (std::size_t sid = 0; sid < scc_n; ++sid) {
      if (source_scc[sid] == 0) continue;
      depth_reachable[sid] = 1;
      depth[sid] = 0;
    }

    for (const std::size_t sid : dag_topo_order) {
      for (const std::size_t tid : dag_out[sid]) {
        if (depth_reachable[sid] == 0) continue;
        const std::uint64_t candidate = depth[sid] + 1;
        if (depth_reachable[tid] == 0 || candidate > depth[tid]) {
          depth_reachable[tid] = 1;
          depth[tid] = candidate;
        }
      }
    }

    std::uint64_t skip_count = 0;
    long double skip_span_sum = 0.0L;
    std::uint64_t max_skip_span = 0;
    for (std::size_t sid = 0; sid < scc_n; ++sid) {
      if (depth_reachable[sid] == 0) continue;
      for (const std::size_t tid : dag_out[sid]) {
        if (depth_reachable[tid] == 0) continue;
        if (depth[tid] <= depth[sid]) continue;
        const std::uint64_t span = depth[tid] - depth[sid];
        if (span <= 1) continue;
        ++skip_count;
        skip_span_sum += static_cast<long double>(span);
        max_skip_span = std::max(max_skip_span, span);
      }
    }

    out.skip_edge_count = skip_count;
    out.skip_edge_ratio = safe_ratio(
        static_cast<double>(out.skip_edge_count),
        static_cast<double>(out.internal_edge_count));
    out.mean_skip_span = safe_ratio(
        static_cast<double>(skip_span_sum),
        static_cast<double>(out.skip_edge_count));
    out.max_skip_span = max_skip_span;
  }

  std::unordered_map<std::uint64_t, std::uint64_t> in_degree_counts;
  std::unordered_map<std::uint64_t, std::uint64_t> out_degree_counts;
  for (std::size_t i = 0; i < n; ++i) {
    ++in_degree_counts[in_degree[i]];
    ++out_degree_counts[out_degree_total[i]];
  }

  out.node_kind_entropy = normalized_shannon_entropy_from_map_(node_kind_counts);
  out.in_degree_entropy = normalized_shannon_entropy_from_map_(in_degree_counts);
  out.out_degree_entropy = normalized_shannon_entropy_from_map_(out_degree_counts);
  out.edge_kind_transition_entropy =
      normalized_shannon_entropy_from_map_(edge_transition_counts);

  return out;
}

bool resolve_network_analytics_options_from_network_design(
    const cuwacunu::camahjucunu::network_design_instruction_t& instruction,
    network_analytics_options_t* out_options,
    std::string* error) {
  if (error) error->clear();
  if (out_options == nullptr) {
    if (error) {
      *error =
          "resolve_network_analytics_options_from_network_design requires non-null output";
    }
    return false;
  }

  const auto policy_nodes =
      instruction.find_nodes_by_kind("NETWORK_ANALYTICS_POLICY");
  if (policy_nodes.empty()) {
    if (error) {
      *error =
          "network_design is missing required node kind NETWORK_ANALYTICS_POLICY";
    }
    return false;
  }
  if (policy_nodes.size() != 1) {
    if (error) {
      *error =
          "network_design must declare exactly one NETWORK_ANALYTICS_POLICY node";
    }
    return false;
  }

  const auto& node = *policy_nodes.front();
  std::unordered_map<std::string, std::string> kv{};
  kv.reserve(node.params.size());

  const auto is_known_key = [](const std::string& key) {
    return key == "near_zero_epsilon" ||
           key == "log10_abs_histogram_bins" ||
           key == "log10_abs_histogram_min" ||
           key == "log10_abs_histogram_max" || key == "include_buffers" ||
           key == "enable_spectral_metrics" ||
           key == "spectral_max_elements" || key == "anomaly_top_k";
  };

  for (const auto& param : node.params) {
    const std::string key =
        lower_ascii_copy_(std::string(trim_ascii_ws_view_(param.key)));
    const std::string value =
        std::string(trim_ascii_ws_view_(param.value));
    if (key.empty()) {
      if (error) {
        *error =
            "NETWORK_ANALYTICS_POLICY contains an empty parameter key";
      }
      return false;
    }
    if (!is_known_key(key)) {
      if (error) {
        *error =
            "NETWORK_ANALYTICS_POLICY has unknown key `" + key + "`";
      }
      return false;
    }
    if (!kv.emplace(key, value).second) {
      if (error) {
        *error =
            "NETWORK_ANALYTICS_POLICY duplicates key `" + key + "`";
      }
      return false;
    }
  }

  const auto require_value = [&](const char* key, std::string* out_value) {
    if (out_value) out_value->clear();
    const auto it = kv.find(key);
    if (it == kv.end()) {
      if (error) {
        *error =
            "NETWORK_ANALYTICS_POLICY missing required key `" +
            std::string(key) + "`";
      }
      return false;
    }
    if (out_value) *out_value = it->second;
    return true;
  };

  std::string raw{};
  network_analytics_options_t parsed{};

  if (!require_value("near_zero_epsilon", &raw) ||
      !parse_double_strict_(raw, &parsed.near_zero_epsilon)) {
    if (error && error->empty()) {
      *error =
          "NETWORK_ANALYTICS_POLICY.near_zero_epsilon must be finite float";
    }
    return false;
  }
  if (parsed.near_zero_epsilon < 0.0) {
    if (error) {
      *error =
          "NETWORK_ANALYTICS_POLICY.near_zero_epsilon must be >= 0";
    }
    return false;
  }

  if (!require_value("log10_abs_histogram_bins", &raw) ||
      !parse_i64_strict_(raw, &parsed.log10_abs_histogram_bins)) {
    if (error && error->empty()) {
      *error =
          "NETWORK_ANALYTICS_POLICY.log10_abs_histogram_bins must be int";
    }
    return false;
  }
  if (parsed.log10_abs_histogram_bins < 1) {
    if (error) {
      *error =
          "NETWORK_ANALYTICS_POLICY.log10_abs_histogram_bins must be >= 1";
    }
    return false;
  }

  if (!require_value("log10_abs_histogram_min", &raw) ||
      !parse_double_strict_(raw, &parsed.log10_abs_histogram_min)) {
    if (error && error->empty()) {
      *error =
          "NETWORK_ANALYTICS_POLICY.log10_abs_histogram_min must be finite float";
    }
    return false;
  }
  if (!require_value("log10_abs_histogram_max", &raw) ||
      !parse_double_strict_(raw, &parsed.log10_abs_histogram_max)) {
    if (error && error->empty()) {
      *error =
          "NETWORK_ANALYTICS_POLICY.log10_abs_histogram_max must be finite float";
    }
    return false;
  }
  if (parsed.log10_abs_histogram_max <= parsed.log10_abs_histogram_min) {
    if (error) {
      *error =
          "NETWORK_ANALYTICS_POLICY.log10_abs_histogram_max must be > log10_abs_histogram_min";
    }
    return false;
  }

  if (!require_value("include_buffers", &raw) ||
      !parse_bool_strict_(raw, &parsed.include_buffers)) {
    if (error && error->empty()) {
      *error =
          "NETWORK_ANALYTICS_POLICY.include_buffers must be bool";
    }
    return false;
  }

  if (!require_value("enable_spectral_metrics", &raw) ||
      !parse_bool_strict_(raw, &parsed.enable_spectral_metrics)) {
    if (error && error->empty()) {
      *error =
          "NETWORK_ANALYTICS_POLICY.enable_spectral_metrics must be bool";
    }
    return false;
  }

  if (!require_value("spectral_max_elements", &raw) ||
      !parse_i64_strict_(raw, &parsed.spectral_max_elements)) {
    if (error && error->empty()) {
      *error =
          "NETWORK_ANALYTICS_POLICY.spectral_max_elements must be int";
    }
    return false;
  }
  if (parsed.spectral_max_elements < 1) {
    if (error) {
      *error =
          "NETWORK_ANALYTICS_POLICY.spectral_max_elements must be >= 1";
    }
    return false;
  }

  if (!require_value("anomaly_top_k", &raw) ||
      !parse_i64_strict_(raw, &parsed.anomaly_top_k)) {
    if (error && error->empty()) {
      *error = "NETWORK_ANALYTICS_POLICY.anomaly_top_k must be int";
    }
    return false;
  }
  if (parsed.anomaly_top_k < 0) {
    if (error) {
      *error =
          "NETWORK_ANALYTICS_POLICY.anomaly_top_k must be >= 0";
    }
    return false;
  }

  *out_options = parsed;
  return true;
}

