#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd "${script_dir}/../../../.." && pwd)"
benchmark_root="${repo_root}/src/config/benchmarks/synthetic_continuous_graph_v1"
artifacts_dir="${benchmark_root}/artifacts"
manifest_path="${artifacts_dir}/synthetic_periodic_chart_manifest.v1.report"
data_root="${benchmark_root}/data/raw"
report_path="${SYNTHETIC_EDGE_PROBE_REPORT_PATH:-${artifacts_dir}/synthetic_direct_edge_return_supervised_probe.v1.report}"

margin_eps="${SYNTHETIC_EDGE_PROBE_MARGIN_EPS:-0.001}"
rank_margin_eps="${SYNTHETIC_EDGE_PROBE_RANK_MARGIN_EPS:-0.001}"
ridge_lambda="${SYNTHETIC_EDGE_PROBE_RIDGE_LAMBDA:-0.00000001}"
edge_feature_probe_path="${SYNTHETIC_EDGE_FEATURE_PROBE_PATH:-${SYNTHETIC_REPRESENTATION_EDGE_FEATURE_PROBE_PATH:-}}"
edge_feature_probe_label="${SYNTHETIC_EDGE_FEATURE_PROBE_LABEL:-frozen_representation}"
edge_feature_probe_max_features="${SYNTHETIC_EDGE_PROBE_MAX_FEATURES:-${SYNTHETIC_REPRESENTATION_EDGE_PROBE_MAX_FEATURES:-0}}"

kv() {
  local key="$1"
  local file="$2"
  awk -F= -v k="${key}" '$1 == k { print $2; exit }' "${file}" 2>/dev/null || true
}

mkdir -p "${artifacts_dir}"
"${script_dir}/generate_synthetic_klines.sh" >/dev/null

if [[ ! -f "${manifest_path}" ]]; then
  echo "missing synthetic chart manifest: ${manifest_path}" >&2
  exit 2
fi

input_length="$(kv input_length "${manifest_path}")"
train_begin="$(kv train_anchor_begin "${manifest_path}")"
train_end="$(kv train_anchor_end_exclusive "${manifest_path}")"
eval_begin="$(kv eval_anchor_begin "${manifest_path}")"
eval_end="$(kv eval_anchor_end_exclusive "${manifest_path}")"
accepted_count="$(kv accepted_anchor_count "${manifest_path}")"
source_function_digest="$(kv source_function_digest "${manifest_path}")"

alpha_csv="${data_root}/SYNALPHASYNUSD/1d/SYNALPHASYNUSD-1d-all-years.csv"
beta_csv="${data_root}/SYNBETASYNUSD/1d/SYNBETASYNUSD-1d-all-years.csv"
gamma_csv="${data_root}/SYNGAMMASYNUSD/1d/SYNGAMMASYNUSD-1d-all-years.csv"
for csv in "${alpha_csv}" "${beta_csv}" "${gamma_csv}"; do
  if [[ ! -f "${csv}" ]]; then
    echo "missing synthetic CSV: ${csv}" >&2
    exit 2
  fi
done

tmp_report="$(mktemp)"
awk -F, \
  -v alpha_file="${alpha_csv}" \
  -v beta_file="${beta_csv}" \
  -v gamma_file="${gamma_csv}" \
  -v input_length="${input_length}" \
  -v train_begin="${train_begin}" \
  -v train_end="${train_end}" \
  -v eval_begin="${eval_begin}" \
  -v eval_end="${eval_end}" \
  -v accepted_count="${accepted_count}" \
  -v source_digest="${source_function_digest}" \
  -v margin_eps="${margin_eps}" \
  -v rank_margin_eps="${rank_margin_eps}" \
  -v ridge_lambda="${ridge_lambda}" \
  -v report_path="${tmp_report}" '
function abs(x) { return x < 0.0 ? -x : x; }
function sqr(x) { return x * x; }
function sgn(x) { return x > 0.0 ? 1 : (x < 0.0 ? -1 : 0); }
function max3(a, b, c) { return a > b ? (a > c ? a : c) : (b > c ? b : c); }
function period_feature_count() { return 6; }
function set_feature_vector(raw,    pi, idx, prev_raw, p) {
  pi = atan2(0, -1);
  idx = 0;
  feat[++idx] = 1.0;
  split("12 6 10 5 8 4", periods, " ");
  for (p = 1; p <= period_feature_count(); ++p) {
    feat[++idx] = sin(2.0 * pi * raw / (periods[p] + 0.0));
    feat[++idx] = cos(2.0 * pi * raw / (periods[p] + 0.0));
  }
  prev_raw = raw - 1;
  feat[++idx] = edge_return_at("SYNALPHA", prev_raw);
  feat[++idx] = edge_return_at("SYNBETA", prev_raw);
  feat[++idx] = edge_return_at("SYNGAMMA", prev_raw);
  return idx;
}
function edge_return_at(node, raw,    a, b) {
  a = close_px[node, raw];
  b = close_px[node, raw + 1];
  if (a <= 0.0 || b <= 0.0) {
    return 0.0;
  }
  return log(b / a);
}
function projection_sanity(set_name, begin, end, anchor, raw, r1, r2, r3, y0, y1, y2, y3, e, count) {
  for (anchor = begin; anchor < end; ++anchor) {
    raw = anchor + input_length - 1;
    r1 = edge_return_at("SYNALPHA", raw);
    r2 = edge_return_at("SYNBETA", raw);
    r3 = edge_return_at("SYNGAMMA", raw);
    y0 = -(r1 + r2 + r3) / 4.0;
    y1 = r1 + y0;
    y2 = r2 + y0;
    y3 = r3 + y0;
    e = abs((y1 - y0) - r1);
    if (e > sanity_max[set_name]) sanity_max[set_name] = e;
    e = abs((y2 - y0) - r2);
    if (e > sanity_max[set_name]) sanity_max[set_name] = e;
    e = abs((y3 - y0) - r3);
    if (e > sanity_max[set_name]) sanity_max[set_name] = e;
    count += 3;
  }
  sanity_count[set_name] = count;
}
function accumulate_training(begin, end,    anchor, raw, j, k, e, nfeat, y) {
  nfeat_global = 0;
  for (anchor = begin; anchor < end; ++anchor) {
    raw = anchor + input_length - 1;
    nfeat = set_feature_vector(raw);
    if (nfeat_global == 0) nfeat_global = nfeat;
    for (j = 1; j <= nfeat; ++j) {
      for (k = 1; k <= nfeat; ++k) {
        xtx[j, k] += feat[j] * feat[k];
      }
    }
    for (e = 1; e <= 3; ++e) {
      y = target_by_edge_index(e, raw);
      for (j = 1; j <= nfeat; ++j) {
        xty[e, j] += feat[j] * y;
      }
    }
    train_sample_count += 1;
  }
}
function target_by_edge_index(edge, raw) {
  if (edge == 1) return edge_return_at("SYNALPHA", raw);
  if (edge == 2) return edge_return_at("SYNBETA", raw);
  return edge_return_at("SYNGAMMA", raw);
}
function solve_edge(edge,    i, j, k, pivot, pivot_abs, row, tmp, factor, diag) {
  for (i = 1; i <= nfeat_global; ++i) {
    for (j = 1; j <= nfeat_global; ++j) {
      mat[i, j] = xtx[i, j];
    }
    diag = (i == 1) ? 0.0 : ridge_lambda;
    mat[i, i] += diag;
    rhs[i] = xty[edge, i];
  }
  for (i = 1; i <= nfeat_global; ++i) {
    pivot = i;
    pivot_abs = abs(mat[i, i]);
    for (row = i + 1; row <= nfeat_global; ++row) {
      if (abs(mat[row, i]) > pivot_abs) {
        pivot = row;
        pivot_abs = abs(mat[row, i]);
      }
    }
    if (pivot_abs < 1e-14) {
      solve_failed[edge] = 1;
      return;
    }
    if (pivot != i) {
      for (j = i; j <= nfeat_global; ++j) {
        tmp = mat[i, j];
        mat[i, j] = mat[pivot, j];
        mat[pivot, j] = tmp;
      }
      tmp = rhs[i];
      rhs[i] = rhs[pivot];
      rhs[pivot] = tmp;
    }
    tmp = mat[i, i];
    for (j = i; j <= nfeat_global; ++j) mat[i, j] /= tmp;
    rhs[i] /= tmp;
    for (row = 1; row <= nfeat_global; ++row) {
      if (row == i) continue;
      factor = mat[row, i];
      if (factor == 0.0) continue;
      for (j = i; j <= nfeat_global; ++j) {
        mat[row, j] -= factor * mat[i, j];
      }
      rhs[row] -= factor * rhs[i];
    }
  }
  for (i = 1; i <= nfeat_global; ++i) {
    w[edge, i] = rhs[i];
  }
}
function linear_prediction(edge,    j, out) {
  out = 0.0;
  for (j = 1; j <= nfeat_global; ++j) out += w[edge, j] * feat[j];
  return out;
}
function best_base(a, b, c) {
  if (a >= b && a >= c) return 1;
  if (b >= a && b >= c) return 2;
  return 3;
}
function best_with_numeraire(a, b, c) {
  if (a <= 0.0 && b <= 0.0 && c <= 0.0) return 0;
  return best_base(a, b, c);
}
function observe_metric(prefix, pred1, pred2, pred3, y1, y2, y3, e, pred, y, pbest, ybest, pnum, ynum, pd, yd) {
  metric_count[prefix] += 3;
  edge_y[1] = y1; edge_y[2] = y2; edge_y[3] = y3;
  edge_pred[1] = pred1; edge_pred[2] = pred2; edge_pred[3] = pred3;
  for (e = 1; e <= 3; ++e) {
    pred = edge_pred[e];
    y = edge_y[e];
    metric_abs_error[prefix] += abs(pred - y);
    metric_sq_error[prefix] += sqr(pred - y);
    metric_pred_sum[prefix] += pred;
    metric_y_sum[prefix] += y;
    metric_pred_sq_sum[prefix] += pred * pred;
    metric_y_sq_sum[prefix] += y * y;
    metric_cross_sum[prefix] += pred * y;
    if (sgn(pred) == sgn(y)) metric_sign_correct[prefix] += 1;
    if (abs(y) > margin_eps) {
      metric_margin_count[prefix] += 1;
      if (sgn(pred) == sgn(y)) metric_margin_sign_correct[prefix] += 1;
    }
  }
  pbest = best_base(pred1, pred2, pred3);
  ybest = best_base(y1, y2, y3);
  metric_best_count[prefix] += 1;
  if (pbest == ybest) metric_best_correct[prefix] += 1;
  pnum = best_with_numeraire(pred1, pred2, pred3);
  ynum = best_with_numeraire(y1, y2, y3);
  metric_best_num_count[prefix] += 1;
  if (pnum == ynum) metric_best_num_correct[prefix] += 1;
  for (e = 1; e <= 2; ++e) {
    for (j = e + 1; j <= 3; ++j) {
      pd = edge_pred[e] - edge_pred[j];
      yd = edge_y[e] - edge_y[j];
      metric_pair_count[prefix] += 1;
      if (sgn(pd) == sgn(yd)) metric_pair_correct[prefix] += 1;
      if (abs(yd) > rank_margin_eps) {
        metric_pair_margin_count[prefix] += 1;
        if (sgn(pd) == sgn(yd)) metric_pair_margin_correct[prefix] += 1;
      }
    }
  }
}
function evaluate_range(prefix, begin, end, anchor, raw, y1, y2, y3, p1, p2, p3) {
  for (anchor = begin; anchor < end; ++anchor) {
    raw = anchor + input_length - 1;
    y1 = edge_return_at("SYNALPHA", raw);
    y2 = edge_return_at("SYNBETA", raw);
    y3 = edge_return_at("SYNGAMMA", raw);
    p1 = edge_return_at("SYNALPHA", raw - 1);
    p2 = edge_return_at("SYNBETA", raw - 1);
    p3 = edge_return_at("SYNGAMMA", raw - 1);
    observe_metric(prefix ".previous_return", p1, p2, p3, y1, y2, y3);
    set_feature_vector(raw);
    p1 = linear_prediction(1);
    p2 = linear_prediction(2);
    p3 = linear_prediction(3);
    observe_metric(prefix ".phase_lag_linear", p1, p2, p3, y1, y2, y3);
  }
}
function corr(prefix,    n, denom_left, denom_right, numerator) {
  n = metric_count[prefix];
  if (n <= 1) return 0.0;
  numerator = metric_cross_sum[prefix] - metric_pred_sum[prefix] * metric_y_sum[prefix] / n;
  denom_left = metric_pred_sq_sum[prefix] - metric_pred_sum[prefix] * metric_pred_sum[prefix] / n;
  denom_right = metric_y_sq_sum[prefix] - metric_y_sum[prefix] * metric_y_sum[prefix] / n;
  if (denom_left <= 0.0 || denom_right <= 0.0) return 0.0;
  return numerator / sqrt(denom_left * denom_right);
}
function emit_metric(prefix) {
  printf("%s.count=%d\n", prefix, metric_count[prefix]) >> report_path;
  if (metric_count[prefix] > 0) {
    printf("%s.mae=%.12f\n", prefix, metric_abs_error[prefix] / metric_count[prefix]) >> report_path;
    printf("%s.rmse=%.12f\n", prefix, sqrt(metric_sq_error[prefix] / metric_count[prefix])) >> report_path;
    printf("%s.directional_accuracy=%.12f\n", prefix, metric_sign_correct[prefix] / metric_count[prefix]) >> report_path;
    printf("%s.correlation=%.12f\n", prefix, corr(prefix)) >> report_path;
  }
  printf("%s.margin_count=%d\n", prefix, metric_margin_count[prefix]) >> report_path;
  if (metric_margin_count[prefix] > 0) {
    printf("%s.margin_directional_accuracy=%.12f\n", prefix, metric_margin_sign_correct[prefix] / metric_margin_count[prefix]) >> report_path;
  }
  printf("%s.pairwise_rank_count=%d\n", prefix, metric_pair_count[prefix]) >> report_path;
  if (metric_pair_count[prefix] > 0) {
    printf("%s.pairwise_rank_accuracy=%.12f\n", prefix, metric_pair_correct[prefix] / metric_pair_count[prefix]) >> report_path;
  }
  printf("%s.margin_pairwise_rank_count=%d\n", prefix, metric_pair_margin_count[prefix]) >> report_path;
  if (metric_pair_margin_count[prefix] > 0) {
    printf("%s.margin_pairwise_rank_accuracy=%.12f\n", prefix, metric_pair_margin_correct[prefix] / metric_pair_margin_count[prefix]) >> report_path;
  }
  if (metric_best_count[prefix] > 0) {
    printf("%s.best_asset_agreement=%.12f\n", prefix, metric_best_correct[prefix] / metric_best_count[prefix]) >> report_path;
  }
  if (metric_best_num_count[prefix] > 0) {
    printf("%s.best_with_numeraire_agreement=%.12f\n", prefix, metric_best_num_correct[prefix] / metric_best_num_count[prefix]) >> report_path;
  }
}
BEGIN {
  nodes[1] = "SYNALPHA";
  nodes[2] = "SYNBETA";
  nodes[3] = "SYNGAMMA";
}
FILENAME == alpha_file { close_px["SYNALPHA", FNR - 1] = $5 + 0.0; }
FILENAME == beta_file { close_px["SYNBETA", FNR - 1] = $5 + 0.0; }
FILENAME == gamma_file { close_px["SYNGAMMA", FNR - 1] = $5 + 0.0; }
END {
  print "schema_id=synthetic_direct_edge_return_supervised_probe.v1" > report_path;
  print "status=complete" >> report_path;
  print "benchmark_id=synthetic_continuous_graph_v1" >> report_path;
  print "record_type=kline" >> report_path;
  print "input_length=" input_length >> report_path;
  print "accepted_anchor_count=" accepted_count >> report_path;
  print "train_anchor_range=[" train_begin "," train_end ")" >> report_path;
  print "eval_anchor_range=[" eval_begin "," eval_end ")" >> report_path;
  print "source_function_digest=" source_digest >> report_path;
  print "margin_eps=" margin_eps >> report_path;
  print "rank_margin_eps=" rank_margin_eps >> report_path;
  print "ridge_lambda=" ridge_lambda >> report_path;
  print "feature_set=phase_periods_12_6_10_5_8_4_plus_previous_edge_returns" >> report_path;
  print "supervised_probe_kind=closed_form_ridge_linear_regression" >> report_path;

  projection_sanity("train", train_begin, train_end);
  projection_sanity("eval", eval_begin, eval_end);
  printf("target_transform.train.projected_edge_max_abs_error=%.17g\n",
         sanity_max["train"]) >> report_path;
  print "target_transform.train.projected_edge_count=" sanity_count["train"] >> report_path;
  printf("target_transform.eval.projected_edge_max_abs_error=%.17g\n",
         sanity_max["eval"]) >> report_path;
  print "target_transform.eval.projected_edge_count=" sanity_count["eval"] >> report_path;
  projection_identity_passed = ((sanity_max["train"] <= 1e-12 && sanity_max["eval"] <= 1e-12) ? "true" : "false");
  print "target_transform.projection_identity_passed=" projection_identity_passed >> report_path;

  accumulate_training(train_begin, train_end);
  print "train_sample_count=" train_sample_count >> report_path;
  print "linear_feature_count=" nfeat_global >> report_path;
  solve_edge(1);
  solve_edge(2);
  solve_edge(3);
  linear_solve_failed = ((solve_failed[1] || solve_failed[2] || solve_failed[3]) ? "true" : "false");
  print "linear_solve_failed=" linear_solve_failed >> report_path;

  evaluate_range("train", train_begin, train_end);
  evaluate_range("eval", eval_begin, eval_end);
  emit_metric("train.previous_return");
  emit_metric("eval.previous_return");
  emit_metric("train.phase_lag_linear");
  emit_metric("eval.phase_lag_linear");

  print "interpretation.target_transform=projected_node_potentials_should_match_raw_base_minus_quote_edge_returns" >> report_path;
  print "interpretation.previous_return=causal_no_training_baseline_from_last_edge_return" >> report_path;
  print "interpretation.phase_lag_linear=small_supervised_upper_bound_using_known_periodic_basis_and_previous_returns" >> report_path;
  print "interpretation.edge_feature_probe=uses_runtime_edge_feature_probe_when_SYNTHETIC_EDGE_FEATURE_PROBE_PATH_or_legacy_SYNTHETIC_REPRESENTATION_EDGE_FEATURE_PROBE_PATH_is_provided" >> report_path;
}
' "${alpha_csv}" "${beta_csv}" "${gamma_csv}"

mv "${tmp_report}" "${report_path}"

if [[ -n "${edge_feature_probe_path}" && -f "${edge_feature_probe_path}" ]]; then
  awk -F, \
    -v train_begin="${train_begin}" \
    -v train_end="${train_end}" \
    -v eval_begin="${eval_begin}" \
    -v eval_end="${eval_end}" \
    -v margin_eps="${margin_eps}" \
    -v rank_margin_eps="${rank_margin_eps}" \
    -v ridge_lambda="${ridge_lambda}" \
    -v max_features="${edge_feature_probe_max_features}" \
    -v probe_label="${edge_feature_probe_label}" \
    -v source_path="${edge_feature_probe_path}" '
function abs(x) { return x < 0.0 ? -x : x; }
function sqr(x) { return x * x; }
function sgn(x) { return x > 0.0 ? 1 : (x < 0.0 ? -1 : 0); }
function best_base(a, b, c) {
  if (a >= b && a >= c) return 1;
  if (b >= a && b >= c) return 2;
  return 3;
}
function read_features(text,    i, raw_count, use_count) {
  raw_count = split(text, raw_feat, ";");
  use_count = raw_count;
  if (max_features + 0 > 0 && use_count > max_features + 0) {
    use_count = max_features + 0;
  }
  feat[1] = 1.0;
  for (i = 1; i <= use_count; ++i) {
    feat[i + 1] = raw_feat[i] + 0.0;
  }
  return use_count + 1;
}
function accumulate(edge, target, nfeat,    j, k) {
  if (nfeat_global == 0) nfeat_global = nfeat;
  if (nfeat != nfeat_global) {
    feature_count_mismatch = 1;
    return;
  }
  for (j = 1; j <= nfeat; ++j) {
    for (k = 1; k <= nfeat; ++k) {
      xtx[edge, j, k] += feat[j] * feat[k];
    }
    xty[edge, j] += feat[j] * target;
  }
  train_rows_by_edge[edge] += 1;
  train_rows += 1;
}
function solve_edge(edge,    i, j, row, pivot, pivot_abs, tmp, factor, diag) {
  if (train_rows_by_edge[edge] <= 0 || nfeat_global <= 0) {
    solve_failed[edge] = 1;
    return;
  }
  for (i = 1; i <= nfeat_global; ++i) {
    for (j = 1; j <= nfeat_global; ++j) {
      mat[i, j] = xtx[edge, i, j];
    }
    diag = (i == 1) ? 0.0 : ridge_lambda;
    mat[i, i] += diag;
    rhs[i] = xty[edge, i];
  }
  for (i = 1; i <= nfeat_global; ++i) {
    pivot = i;
    pivot_abs = abs(mat[i, i]);
    for (row = i + 1; row <= nfeat_global; ++row) {
      if (abs(mat[row, i]) > pivot_abs) {
        pivot = row;
        pivot_abs = abs(mat[row, i]);
      }
    }
    if (pivot_abs < 1e-14) {
      solve_failed[edge] = 1;
      return;
    }
    if (pivot != i) {
      for (j = i; j <= nfeat_global; ++j) {
        tmp = mat[i, j]; mat[i, j] = mat[pivot, j]; mat[pivot, j] = tmp;
      }
      tmp = rhs[i]; rhs[i] = rhs[pivot]; rhs[pivot] = tmp;
    }
    tmp = mat[i, i];
    for (j = i; j <= nfeat_global; ++j) mat[i, j] /= tmp;
    rhs[i] /= tmp;
    for (row = 1; row <= nfeat_global; ++row) {
      if (row == i) continue;
      factor = mat[row, i];
      if (factor == 0.0) continue;
      for (j = i; j <= nfeat_global; ++j) mat[row, j] -= factor * mat[i, j];
      rhs[row] -= factor * rhs[i];
    }
  }
  for (i = 1; i <= nfeat_global; ++i) w[edge, i] = rhs[i];
}
function predict(edge,    j, out) {
  out = 0.0;
  for (j = 1; j <= nfeat_global; ++j) out += w[edge, j] * feat[j];
  return out;
}
function observe(prefix, anchor, channel, edge, pred, target,    group) {
  metric_count[prefix] += 1;
  metric_abs_error[prefix] += abs(pred - target);
  metric_sq_error[prefix] += sqr(pred - target);
  metric_pred_sum[prefix] += pred;
  metric_y_sum[prefix] += target;
  metric_pred_sq_sum[prefix] += pred * pred;
  metric_y_sq_sum[prefix] += target * target;
  metric_cross_sum[prefix] += pred * target;
  if (sgn(pred) == sgn(target)) metric_sign_correct[prefix] += 1;
  if (abs(target) > margin_eps) {
    metric_margin_count[prefix] += 1;
    if (sgn(pred) == sgn(target)) metric_margin_sign_correct[prefix] += 1;
  }
  group = prefix SUBSEP anchor SUBSEP channel;
  group_seen[group] = 1;
  group_pred[group, edge] = pred;
  group_target[group, edge] = target;
}
function finalize_groups(    g, pbest, ybest, e, j, pd, yd, split_name) {
  for (g in group_seen) {
    split(g, group_parts, SUBSEP);
    split_name = group_parts[1];
    if (!((g SUBSEP 1) in group_pred) ||
        !((g SUBSEP 2) in group_pred) ||
        !((g SUBSEP 3) in group_pred)) {
      continue;
    }
    pbest = best_base(group_pred[g, 1], group_pred[g, 2], group_pred[g, 3]);
    ybest = best_base(group_target[g, 1], group_target[g, 2], group_target[g, 3]);
    metric_best_count[split_name] += 1;
    if (pbest == ybest) metric_best_correct[split_name] += 1;
    for (e = 1; e <= 2; ++e) {
      for (j = e + 1; j <= 3; ++j) {
        pd = group_pred[g, e] - group_pred[g, j];
        yd = group_target[g, e] - group_target[g, j];
        metric_pair_count[split_name] += 1;
        if (sgn(pd) == sgn(yd)) metric_pair_correct[split_name] += 1;
        if (abs(yd) > rank_margin_eps) {
          metric_pair_margin_count[split_name] += 1;
          if (sgn(pd) == sgn(yd)) metric_pair_margin_correct[split_name] += 1;
        }
      }
    }
  }
}
function corr(prefix,    n, numerator, denom_left, denom_right) {
  n = metric_count[prefix];
  if (n <= 1) return 0.0;
  numerator = metric_cross_sum[prefix] - metric_pred_sum[prefix] * metric_y_sum[prefix] / n;
  denom_left = metric_pred_sq_sum[prefix] - metric_pred_sum[prefix] * metric_pred_sum[prefix] / n;
  denom_right = metric_y_sq_sum[prefix] - metric_y_sum[prefix] * metric_y_sum[prefix] / n;
  if (denom_left <= 0.0 || denom_right <= 0.0) return 0.0;
  return numerator / sqrt(denom_left * denom_right);
}
function emit(prefix) {
  printf("%s.%s.count=%d\n", probe_label, prefix, metric_count[prefix]);
  if (metric_count[prefix] > 0) {
    printf("%s.%s.mae=%.12f\n", probe_label, prefix, metric_abs_error[prefix] / metric_count[prefix]);
    printf("%s.%s.rmse=%.12f\n", probe_label, prefix, sqrt(metric_sq_error[prefix] / metric_count[prefix]));
    printf("%s.%s.directional_accuracy=%.12f\n", probe_label, prefix, metric_sign_correct[prefix] / metric_count[prefix]);
    printf("%s.%s.correlation=%.12f\n", probe_label, prefix, corr(prefix));
  }
  printf("%s.%s.margin_count=%d\n", probe_label, prefix, metric_margin_count[prefix]);
  if (metric_margin_count[prefix] > 0) {
    printf("%s.%s.margin_directional_accuracy=%.12f\n", probe_label, prefix, metric_margin_sign_correct[prefix] / metric_margin_count[prefix]);
  }
  printf("%s.%s.pairwise_rank_count=%d\n", probe_label, prefix, metric_pair_count[prefix]);
  if (metric_pair_count[prefix] > 0) {
    printf("%s.%s.pairwise_rank_accuracy=%.12f\n", probe_label, prefix, metric_pair_correct[prefix] / metric_pair_count[prefix]);
  }
  printf("%s.%s.margin_pairwise_rank_count=%d\n", probe_label, prefix, metric_pair_margin_count[prefix]);
  if (metric_pair_margin_count[prefix] > 0) {
    printf("%s.%s.margin_pairwise_rank_accuracy=%.12f\n", probe_label, prefix, metric_pair_margin_correct[prefix] / metric_pair_margin_count[prefix]);
  }
  if (metric_best_count[prefix] > 0) {
    printf("%s.%s.best_asset_agreement=%.12f\n", probe_label, prefix, metric_best_correct[prefix] / metric_best_count[prefix]);
  }
}
NR == 1 { next; }
{
  anchor = $3 + 0;
  edge = ($5 + 0) + 1;
  channel = $9 + 0;
  target = $10 + 0.0;
  nfeat = read_features($12);
  if (edge < 1 || edge > 3) next;
  total_rows += 1;
  if (anchor >= train_begin + 0 && anchor < train_end + 0) {
    accumulate(edge, target, nfeat);
  }
}
END {
  solve_edge(1); solve_edge(2); solve_edge(3);
  close(source_path);
  while ((getline line < source_path) > 0) {
    if (line ~ /^record_schema,/) continue;
    split(line, cols, ",");
    anchor = cols[3] + 0;
    edge = (cols[5] + 0) + 1;
    channel = cols[9] + 0;
    target = cols[10] + 0.0;
    nfeat = read_features(cols[12]);
    if (edge < 1 || edge > 3 || nfeat != nfeat_global || solve_failed[edge]) continue;
    pred = predict(edge);
    if (anchor >= train_begin + 0 && anchor < train_end + 0) {
      observe("train", anchor, channel, edge, pred, target);
    } else if (anchor >= eval_begin + 0 && anchor < eval_end + 0) {
      observe("eval", anchor, channel, edge, pred, target);
    }
  }
  close(source_path);
  finalize_groups();
  print probe_label "_feature_probe_status=complete";
  print probe_label "_feature_probe_path=" source_path;
  print probe_label "_feature_probe_total_rows=" total_rows;
  print probe_label "_feature_probe_train_rows=" train_rows;
  print probe_label "_feature_probe_feature_count=" nfeat_global;
  print probe_label "_feature_probe_max_features=" max_features;
  print probe_label "_feature_probe_feature_count_mismatch=" (feature_count_mismatch ? "true" : "false");
  print probe_label "_feature_probe_linear_solve_failed=" ((solve_failed[1] || solve_failed[2] || solve_failed[3]) ? "true" : "false");
  emit("train");
  emit("eval");
}
' "${edge_feature_probe_path}" >> "${report_path}"
else
  {
    echo "${edge_feature_probe_label}_feature_probe_status=not_available"
    echo "${edge_feature_probe_label}_feature_probe_reason=runtime_edge_feature_probe_artifact_not_provided"
    echo "${edge_feature_probe_label}_feature_probe_expected_env=SYNTHETIC_EDGE_FEATURE_PROBE_PATH"
  } >> "${report_path}"
fi

cat "${report_path}"
