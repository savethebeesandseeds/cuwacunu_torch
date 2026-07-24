(() => {
  "use strict";

  const API = Object.freeze({
    benchmarks: "/api/v1/benchmarks",
    catalog: "/api/v1/catalog",
    chart: "/api/v1/chart",
  });
  const COLOR = Object.freeze({
    bg: "#17171d",
    grid: "rgba(255,255,255,.075)",
    axis: "#85858e",
    text: "#b9b9b3",
    green: "#4fcf6b",
    cyan: "#61c9d8",
    red: "#e87979",
    amber: "#e6b85c",
    blue: "#6c9cff",
  });
  const SPLIT_COLORS = Object.freeze({
    train: COLOR.green,
    embargo: COLOR.amber,
    evaluation: COLOR.blue,
    validation: COLOR.blue,
    certified_development: COLOR.blue,
    purge_1: COLOR.amber,
    purge_2: COLOR.amber,
    purge_3: COLOR.amber,
    test_holdout: COLOR.red,
  });
  const FIELD_COUNT = 9;
  const POINT_FIELDS = Object.freeze([
    "anchor",
    "open_time_ms",
    "close_time_ms",
    "open",
    "high",
    "low",
    "close",
    "volume",
    "log_return",
  ]);
  const MAX_ACF_LAG = 48;

  const byId = (id) => document.getElementById(id);
  const el = {
    benchmark: byId("benchmark-select"),
    benchmarkContext: byId("benchmark-context"),
    instrument: byId("instrument-select"),
    interval: byId("interval-select"),
    compareToggle: byId("compare-toggle"),
    compare: byId("compare-select"),
    reload: byId("reload-button"),
    retry: byId("error-retry"),
    errorPanel: byId("error-panel"),
    errorMessage: byId("error-message"),
    serviceDot: byId("service-dot"),
    serviceStatus: byId("service-status"),
    sealedBadge: byId("sealed-badge"),
    sealedDetail: byId("sealed-detail"),
    warning: byId("duplication-warning"),
    live: byId("live-region"),
    period: byId("summary-period"),
    history: byId("summary-history"),
    historyDetail: byId("summary-history-detail"),
    cycles: byId("summary-cycles"),
    noise: byId("summary-noise"),
    drift: byId("summary-drift"),
    digest: byId("summary-digest"),
    closeSubtitle: byId("close-subtitle"),
    primaryLegend: byId("primary-legend"),
    compareLegend: byId("compare-legend"),
    compareLegendWrap: byId("compare-legend-wrap"),
    closeCanvas: byId("close-chart"),
    returnCanvas: byId("return-chart"),
    acfCanvas: byId("acf-chart"),
    closeTooltip: byId("close-tooltip"),
    returnTooltip: byId("return-tooltip"),
    acfTooltip: byId("acf-tooltip"),
    prevDirection: byId("metric-prev-direction"),
    acfMetric: byId("metric-acf"),
    acfDetail: byId("metric-acf-detail"),
    rowMatch: byId("metric-row-match"),
    rowDetail: byId("metric-row-detail"),
    timeDiscrepancy: byId("metric-time-discrepancy"),
    timeDetail: byId("metric-time-detail"),
    splitKey: byId("split-key"),
    boundaryDescription: byId("boundary-description"),
    footer: byId("footer-provenance"),
    loading: [byId("close-loading"), byId("return-loading"), byId("acf-loading")],
  };

  const state = {
    benchmarkCatalog: null,
    benchmark: null,
    catalog: null,
    primary: null,
    comparison: null,
    axis: "row",
    hoverAnchor: null,
    hoverLag: null,
    acf: [],
    request: null,
    renderFrame: 0,
    plot: {},
  };

  function finite(value, label) {
    const number = Number(value);
    if (!Number.isFinite(number)) throw new Error(`${label} is not finite`);
    return number;
  }

  function safeId(value, label) {
    if (typeof value !== "string" || !/^[A-Za-z0-9_.-]+$/.test(value)) {
      throw new Error(`${label} is not a safe identifier`);
    }
    return value;
  }

  function assert(condition, message) {
    if (!condition) throw new Error(message);
  }

  function validateBenchmarkCatalog(raw) {
    assert(raw && raw.schema_id === "iinuji.synthetic_benchmark_catalog.v1", "Unexpected benchmark catalog schema");
    safeId(raw.default_benchmark_id, "default benchmark id");
    assert(Array.isArray(raw.benchmarks) && raw.benchmarks.length === 2, "Benchmark catalog is incomplete");
    raw.benchmarks.forEach((item) => {
      safeId(item.id, "benchmark id");
      assert(typeof item.label === "string" && item.label.length > 0, "Benchmark label is missing");
      finite(item.served_anchor_end_exclusive, "benchmark served boundary");
      assert(item.test_holdout_served === false, "Benchmark unexpectedly serves a test holdout");
    });
    assert(raw.benchmarks.some((item) => item.id === raw.default_benchmark_id), "Default benchmark is unavailable");
    return raw;
  }

  function validateCatalog(raw) {
    const expectedSchema = state.benchmark === "synthetic_continuous_graph_v2"
      ? "iinuji.synthetic_chart_catalog.v2"
      : "iinuji.synthetic_chart_catalog.v1";
    assert(raw && raw.schema_id === expectedSchema, "Unexpected catalog schema");
    assert(raw.benchmark_id === state.benchmark, "Catalog benchmark identity does not match selection");
    assert(raw.test_holdout_served === false, "Catalog unexpectedly serves the test holdout");
    if (state.benchmark === "synthetic_continuous_graph_v2") {
      assert(raw.data_scope === "development_prefix_only", "V2 catalog is not development-prefix scoped");
      assert(raw.raw_source_served === false, "V2 catalog unexpectedly serves raw source data");
      assert(raw.served_anchor_end_exclusive === 3264, "V2 development boundary changed");
    }
    assert(Array.isArray(raw.instruments) && raw.instruments.length > 0, "Catalog has no instruments");
    assert(Array.isArray(raw.intervals) && raw.intervals.length > 0, "Catalog has no intervals");
    assert(Array.isArray(raw.splits), "Catalog split list is missing");
    raw.instruments.forEach((item) => {
      safeId(item.id, "instrument id");
      finite(item.fundamental_period_anchors, "fundamental period");
    });
    raw.intervals.forEach((item) => {
      safeId(item.id, "interval id");
      finite(item.input_length, "interval input length");
      finite(item.step_days, "interval step days");
    });
    raw.splits.forEach((split) => {
      safeId(split.id, "split id");
      finite(split.begin, "split begin");
      finite(split.end_exclusive, "split end");
    });
    finite(raw.served_anchor_end_exclusive, "served anchor boundary");
    return raw;
  }

  function validateChart(raw, instrument, interval) {
    const expectedSchema = state.benchmark === "synthetic_continuous_graph_v2"
      ? "iinuji.synthetic_chart.v2"
      : "iinuji.synthetic_chart.v1";
    assert(raw && raw.schema_id === expectedSchema, "Unexpected chart schema");
    assert(raw.benchmark_id === state.benchmark, "Chart benchmark identity does not match selection");
    assert(raw.instrument === instrument && raw.interval === interval, "Chart identity does not match request");
    assert(raw.test_holdout_served === false, "Chart response unexpectedly serves the test holdout");
    if (state.benchmark === "synthetic_continuous_graph_v2") {
      assert(raw.data_scope === "development_prefix_only", "V2 chart is not development-prefix scoped");
      assert(raw.raw_source_served === false, "V2 chart unexpectedly serves raw source data");
    }
    assert(
      Array.isArray(raw.point_fields) &&
        raw.point_fields.length === FIELD_COUNT &&
        raw.point_fields.every((field, index) => field === POINT_FIELDS[index]),
      "Unexpected point schema",
    );
    assert(Array.isArray(raw.points) && raw.points.length > 1, "Chart has too few points");
    const servedEnd = state.catalog.served_anchor_end_exclusive;
    const points = raw.points.map((row, index) => {
      assert(Array.isArray(row) && row.length === FIELD_COUNT, `Malformed point at row ${index}`);
      const point = {
        anchor: finite(row[0], "anchor"),
        openTime: finite(row[1], "open time"),
        closeTime: finite(row[2], "close time"),
        open: finite(row[3], "open"),
        high: finite(row[4], "high"),
        low: finite(row[5], "low"),
        close: finite(row[6], "close"),
        volume: finite(row[7], "volume"),
        return: finite(row[8], "log return"),
      };
      assert(point.anchor < servedEnd, "Chart crossed the served-data boundary");
      if (index > 0) {
        assert(point.anchor > finite(raw.points[index - 1][0], "previous anchor"), "Anchors are not increasing");
        assert(point.closeTime > finite(raw.points[index - 1][2], "previous close time"), "Timestamps are not increasing");
      }
      return point;
    });
    return { ...raw, points };
  }

  async function fetchJson(url, signal) {
    const response = await fetch(url, {
      signal,
      cache: "no-store",
      credentials: "same-origin",
      headers: { Accept: "application/json" },
    });
    if (!response.ok) throw new Error(`Local API returned HTTP ${response.status}`);
    const type = response.headers.get("content-type") || "";
    if (!type.toLowerCase().includes("application/json")) throw new Error("Local API did not return JSON");
    return response.json();
  }

  function setOptions(select, values, selected) {
    select.replaceChildren();
    values.forEach(({ value, label }) => {
      const option = document.createElement("option");
      option.value = value;
      option.textContent = label;
      option.selected = value === selected;
      select.append(option);
    });
  }

  function intervalRecord(id) {
    return state.catalog.intervals.find((item) => item.id === id);
  }

  function instrumentRecord(id) {
    return state.catalog.instruments.find((item) => item.id === id);
  }

  function syncComparisonOptions() {
    const primary = el.interval.value;
    const choices = state.catalog.intervals.filter((item) => item.id !== primary);
    const current = choices.some((item) => item.id === el.compare.value) ? el.compare.value : null;
    const preferred = primary !== "1d" && choices.some((item) => item.id === "1d") ? "1d" : choices[0]?.id;
    setOptions(
      el.compare,
      choices.map((item) => ({ value: item.id, label: `${item.id} · ${item.step_days} day step` })),
      current || preferred,
    );
    const available = choices.length > 0;
    el.compareToggle.disabled = !available;
    el.compare.disabled = !available || !el.compareToggle.checked;
  }

  function setBusy(busy) {
    el.benchmark.disabled = busy || !state.benchmarkCatalog;
    el.instrument.disabled = busy || !state.catalog;
    el.interval.disabled = busy || !state.catalog;
    el.reload.disabled = busy || !state.catalog;
    el.compareToggle.disabled = busy || !state.catalog || state.catalog.intervals.length < 2;
    el.compare.disabled = busy || !state.catalog || !el.compareToggle.checked;
    el.loading.forEach((node) => node.classList.toggle("is-complete", !busy));
    if (busy) {
      el.serviceDot.className = "service-dot";
      el.serviceStatus.textContent = "Loading served chart rows";
    }
  }

  function showError(error) {
    const message = error instanceof Error ? error.message : String(error);
    el.errorMessage.textContent = message;
    el.errorPanel.hidden = false;
    el.serviceDot.className = "service-dot error";
    el.serviceStatus.textContent = "Local evidence unavailable";
    el.live.textContent = `Chart loading failed: ${message}`;
  }

  function showReady() {
    el.errorPanel.hidden = true;
    el.serviceDot.className = "service-dot ready";
    el.serviceStatus.textContent = "Served evidence verified";
  }

  function benchmarkQuery(id) {
    safeId(id, "benchmark id");
    return `benchmark=${encodeURIComponent(id)}`;
  }

  function selectedBenchmarkFromLocation() {
    const value = new URL(window.location.href).searchParams.get("benchmark");
    return value || null;
  }

  function updateLocationBenchmark() {
    const url = new URL(window.location.href);
    if (state.benchmark === state.benchmarkCatalog.default_benchmark_id) url.searchParams.delete("benchmark");
    else url.searchParams.set("benchmark", state.benchmark);
    window.history.replaceState(null, "", `${url.pathname}${url.search}${url.hash}`);
  }

  async function loadBenchmarkCatalog() {
    setBusy(true);
    try {
      const controller = new AbortController();
      state.request = controller;
      state.benchmarkCatalog = validateBenchmarkCatalog(await fetchJson(API.benchmarks, controller.signal));
      const requested = selectedBenchmarkFromLocation();
      const selected = requested || state.benchmarkCatalog.default_benchmark_id;
      assert(
        state.benchmarkCatalog.benchmarks.some((item) => item.id === selected),
        `Unknown benchmark selection: ${selected}`,
      );
      state.benchmark = selected;
      setOptions(
        el.benchmark,
        state.benchmarkCatalog.benchmarks.map((item) => ({ value: item.id, label: item.label })),
        state.benchmark,
      );
      await loadCatalog();
    } catch (error) {
      if (error.name !== "AbortError") showError(error);
    } finally {
      setBusy(false);
      state.request = null;
    }
  }

  async function loadCatalog() {
    setBusy(true);
    state.catalog = null;
    state.primary = null;
    state.comparison = null;
    try {
      const controller = new AbortController();
      state.request = controller;
      state.catalog = validateCatalog(
        await fetchJson(`${API.catalog}?${benchmarkQuery(state.benchmark)}`, controller.signal),
      );
      setOptions(
        el.instrument,
        state.catalog.instruments.map((item) => ({ value: item.id, label: item.id })),
        state.catalog.instruments[0].id,
      );
      setOptions(
        el.interval,
        state.catalog.intervals.map((item) => ({ value: item.id, label: `${item.id} · ${item.input_length} anchors` })),
        state.catalog.intervals[0].id,
      );
      el.compareToggle.checked = state.catalog.intervals.length > 1;
      syncComparisonOptions();
      renderCatalogBoundary();
      await loadCharts();
    } catch (error) {
      if (error.name !== "AbortError") showError(error);
    } finally {
      setBusy(false);
      state.request = null;
    }
  }

  function chartUrl(instrument, interval) {
    safeId(instrument, "instrument id");
    safeId(interval, "interval id");
    return `${API.chart}/${instrument}/${interval}?${benchmarkQuery(state.benchmark)}`;
  }

  async function loadCharts() {
    if (!state.catalog) return;
    if (state.request) state.request.abort();
    const controller = new AbortController();
    state.request = controller;
    setBusy(true);
    el.errorPanel.hidden = true;
    state.hoverAnchor = null;
    try {
      const instrument = el.instrument.value;
      const interval = el.interval.value;
      const compareInterval = el.compareToggle.checked ? el.compare.value : null;
      const requests = [fetchJson(chartUrl(instrument, interval), controller.signal)];
      if (compareInterval) requests.push(fetchJson(chartUrl(instrument, compareInterval), controller.signal));
      const responses = await Promise.all(requests);
      state.primary = validateChart(responses[0], instrument, interval);
      state.comparison = compareInterval ? validateChart(responses[1], instrument, compareInterval) : null;
      state.acf = computeAcf(state.primary.points.map((point) => point.return), MAX_ACF_LAG);
      renderSummary();
      scheduleRender();
      showReady();
      el.live.textContent = `${instrument} ${interval} loaded with ${state.primary.points.length} served points`;
    } catch (error) {
      if (error.name !== "AbortError") showError(error);
    } finally {
      if (state.request === controller) {
        state.request = null;
        setBusy(false);
      }
    }
  }

  function renderCatalogBoundary() {
    const catalog = state.catalog;
    const benchmark = state.benchmarkCatalog.benchmarks.find((item) => item.id === state.benchmark);
    const holdout = catalog.splits.find((split) => split.id === "test_holdout");
    const sealed = catalog.test_holdout_served === false && (!holdout || holdout.served === false);
    el.sealedBadge.classList.toggle("warning", !sealed);
    el.sealedBadge.querySelector("strong").textContent = sealed ? "Test holdout sealed" : "Holdout boundary warning";
    el.sealedDetail.textContent = holdout
      ? `[${holdout.begin}, ${holdout.end_exclusive}) ${sealed ? "absent from API" : "catalog reports served"}`
      : sealed ? "Catalog reports it is not served" : "Review catalog response";
    el.benchmarkContext.textContent = `${benchmark?.label || state.benchmark} · anchors [0, ${catalog.served_anchor_end_exclusive}) visible`;
    el.boundaryDescription.textContent = sealed
      ? `${state.benchmark} returns only anchors before ${catalog.served_anchor_end_exclusive}. The test holdout remains absent from both chart responses and calculations.`
      : "The catalog does not confirm a sealed holdout. Treat this page as unsafe for diagnostic selection.";
    el.splitKey.replaceChildren();
    catalog.splits.forEach((split) => {
      const item = document.createElement("div");
      item.className = `split-item split-${split.id}`;
      const title = document.createElement("strong");
      title.textContent = split.id.replaceAll("_", " ");
      const range = document.createElement("span");
      range.textContent = `[${split.begin}, ${split.end_exclusive})`;
      const status = document.createElement("small");
      status.textContent = split.served ? "served" : "sealed · not served";
      item.append(title, range, status);
      el.splitKey.append(item);
    });
  }

  function formatNumber(value, digits = 4) {
    if (!Number.isFinite(value)) return "—";
    return new Intl.NumberFormat("en", { maximumFractionDigits: digits }).format(value);
  }

  function formatPercent(value, digits = 2) {
    if (!Number.isFinite(value)) return "—";
    return `${(value * 100).toFixed(digits)}%`;
  }

  function shortDigest(value) {
    if (typeof value !== "string" || !value) return "not listed";
    return value.length > 18 ? `${value.slice(0, 10)}…${value.slice(-6)}` : value;
  }

  function previousDirection(points) {
    let valid = 0;
    let correct = 0;
    for (let index = 1; index < points.length; index += 1) {
      const previous = Math.sign(points[index - 1].return);
      const current = Math.sign(points[index].return);
      if (!Number.isFinite(previous) || !Number.isFinite(current)) continue;
      valid += 1;
      if (previous === current) correct += 1;
    }
    return { accuracy: valid ? correct / valid : NaN, correct, valid };
  }

  function pearson(left, right) {
    const count = Math.min(left.length, right.length);
    if (count < 2) return NaN;
    let meanLeft = 0;
    let meanRight = 0;
    for (let index = 0; index < count; index += 1) {
      meanLeft += left[index];
      meanRight += right[index];
    }
    meanLeft /= count;
    meanRight /= count;
    let covariance = 0;
    let varianceLeft = 0;
    let varianceRight = 0;
    for (let index = 0; index < count; index += 1) {
      const a = left[index] - meanLeft;
      const b = right[index] - meanRight;
      covariance += a * b;
      varianceLeft += a * a;
      varianceRight += b * b;
    }
    const denominator = Math.sqrt(varianceLeft * varianceRight);
    return denominator > 0 ? covariance / denominator : NaN;
  }

  function computeAcf(values, requestedLags) {
    const maxLag = Math.min(requestedLags, Math.floor(values.length / 4));
    const result = [];
    for (let lag = 1; lag <= maxLag; lag += 1) {
      result.push({ lag, value: pearson(values.slice(lag), values.slice(0, -lag)) });
    }
    return result;
  }

  function compareRows(primary, comparison) {
    if (!comparison) return null;
    const other = new Map(comparison.points.map((point) => [point.anchor, point]));
    let count = 0;
    let exact = 0;
    let maxDelta = 0;
    for (const point of primary.points) {
      const match = other.get(point.anchor);
      if (!match) continue;
      count += 1;
      const deltas = ["open", "high", "low", "close"].map((field) => Math.abs(point[field] - match[field]));
      maxDelta = Math.max(maxDelta, ...deltas);
      if (deltas.every((delta) => delta === 0)) exact += 1;
    }
    return { count, exact, maxDelta };
  }

  function compareMatchedTimes(primary, comparison) {
    if (!comparison) return null;
    const oneDay = primary.interval === "1d" ? primary : comparison.interval === "1d" ? comparison : primary;
    const other = oneDay === primary ? comparison : primary;
    const baseline = new Map(oneDay.points.map((point) => [point.closeTime, point.close]));
    const differences = [];
    for (const point of other.points) {
      if (!baseline.has(point.closeTime)) continue;
      const reference = baseline.get(point.closeTime);
      differences.push(Math.abs(point.close - reference) / Math.max(Math.abs(reference), Number.EPSILON));
    }
    if (!differences.length) return { count: 0, mean: NaN, max: NaN, baseline: oneDay.interval };
    return {
      count: differences.length,
      mean: differences.reduce((sum, value) => sum + value, 0) / differences.length,
      max: Math.max(...differences),
      baseline: oneDay.interval,
    };
  }

  function renderSummary() {
    const chart = state.primary;
    const catalog = state.catalog;
    const instrument = instrumentRecord(chart.instrument);
    const interval = intervalRecord(chart.interval);
    const period = finite(instrument.fundamental_period_anchors, "period");
    const inputLength = finite(interval.input_length, "input length");
    const inputSpanInPeriodUnits = state.benchmark === "synthetic_continuous_graph_v2"
      ? inputLength * finite(interval.step_days, "step days")
      : inputLength;
    const source = catalog.source || {};
    el.warning.hidden = instrument.cross_interval_ohlc?.identical_by_row !== true;
    el.period.textContent = formatNumber(period, 3);
    el.history.textContent = formatNumber(inputLength, 0);
    el.historyDetail.textContent = `${chart.points.length} interval observations served`;
    el.cycles.textContent = formatNumber(inputSpanInPeriodUnits / period, 3);
    el.noise.textContent = `not listed / ${source.chart_drift_model || "not listed"}`;
    el.drift.textContent = "noise / drift from catalog metadata";
    el.digest.textContent = shortDigest(source.source_function_digest);
    el.digest.title = source.source_function_digest || "Source digest not listed";
    el.footer.textContent = source.source_function_digest ? `source ${source.source_function_digest}` : "Source digest not listed";
    el.primaryLegend.textContent = `${chart.instrument} · ${chart.interval}`;
    el.compareLegendWrap.hidden = !state.comparison;
    if (state.comparison) el.compareLegend.textContent = `${state.comparison.instrument} · ${state.comparison.interval}`;
    el.closeSubtitle.textContent = `${chart.points.length} observations · visible anchor ceiling ${catalog.served_anchor_end_exclusive} · ${interval.step_days}-day timestamp step · ${state.axis === "row" ? "anchor-index" : "wall-clock"} axis`;

    const direction = previousDirection(chart.points);
    el.prevDirection.textContent = formatPercent(direction.accuracy);
    el.prevDirection.nextElementSibling.textContent = `${direction.correct}/${direction.valid} adjacent signs matched`;
    const strongest = state.acf.reduce((best, item) => !best || Math.abs(item.value) > Math.abs(best.value) ? item : best, null);
    el.acfMetric.textContent = strongest ? formatNumber(strongest.value, 4) : "—";
    el.acfDetail.textContent = strongest ? `lag ${strongest.lag} · absolute ${formatNumber(Math.abs(strongest.value), 4)}` : "no finite ACF lags";

    const rows = compareRows(chart, state.comparison);
    if (rows && rows.count) {
      el.rowMatch.textContent = formatPercent(rows.exact / rows.count);
      el.rowDetail.textContent = `${rows.exact}/${rows.count} OHLC rows · max Δ ${formatNumber(rows.maxDelta, 8)}`;
    } else {
      el.rowMatch.textContent = "—";
      el.rowDetail.textContent = "enable interval comparison";
    }
    const matched = compareMatchedTimes(chart, state.comparison);
    if (matched && matched.count) {
      el.timeDiscrepancy.textContent = `${formatPercent(matched.mean, 3)} mean`;
      el.timeDetail.textContent = `${formatPercent(matched.max, 3)} max · ${matched.count} exact close-time matches · ${matched.baseline} baseline`;
    } else {
      el.timeDiscrepancy.textContent = "—";
      el.timeDetail.textContent = matched ? "no exact close-time overlap" : "enable interval comparison";
    }
  }

  function canvasFrame(canvas) {
    const bounds = canvas.getBoundingClientRect();
    const width = Math.max(280, bounds.width);
    const height = Math.max(180, bounds.height);
    const ratio = Math.min(window.devicePixelRatio || 1, 2.5);
    const pixelWidth = Math.round(width * ratio);
    const pixelHeight = Math.round(height * ratio);
    if (canvas.width !== pixelWidth || canvas.height !== pixelHeight) {
      canvas.width = pixelWidth;
      canvas.height = pixelHeight;
    }
    const context = canvas.getContext("2d");
    context.setTransform(ratio, 0, 0, ratio, 0, 0);
    context.clearRect(0, 0, width, height);
    return { context, width, height, ratio };
  }

  function plotLayout(width, height) {
    const compact = width < 520;
    const left = compact ? 50 : 62;
    const right = compact ? 12 : 20;
    const top = 20;
    const bottom = 34;
    return { left, right, top, bottom, width: width - left - right, height: height - top - bottom };
  }

  function pointX(point) {
    return state.axis === "row" ? point.anchor : point.closeTime;
  }

  function estimateTimeAtAnchor(chart, anchor) {
    const points = chart.points;
    if (anchor <= points[0].anchor) {
      const timePerAnchor = (points[1].closeTime - points[0].closeTime) / (points[1].anchor - points[0].anchor);
      return points[0].closeTime + (anchor - points[0].anchor) * timePerAnchor;
    }
    const last = points[points.length - 1];
    if (anchor >= last.anchor) {
      const previous = points[points.length - 2];
      const timePerAnchor = (last.closeTime - previous.closeTime) / (last.anchor - previous.anchor);
      return last.closeTime + (anchor - last.anchor) * timePerAnchor;
    }
    let low = 0;
    let high = points.length - 1;
    while (low + 1 < high) {
      const middle = Math.floor((low + high) / 2);
      if (points[middle].anchor <= anchor) low = middle;
      else high = middle;
    }
    const first = points[low];
    const second = points[high];
    const fraction = (anchor - first.anchor) / (second.anchor - first.anchor);
    return first.closeTime + fraction * (second.closeTime - first.closeTime);
  }

  function xDomain() {
    const charts = [state.primary, state.comparison].filter(Boolean);
    if (state.axis === "row") {
      return [state.primary.points[0].anchor, state.catalog.served_anchor_end_exclusive];
    }
    const values = charts.flatMap((chart) => [chart.points[0].closeTime, estimateTimeAtAnchor(chart, state.catalog.served_anchor_end_exclusive)]);
    return [Math.min(...values), Math.max(...values)];
  }

  function expandDomain(minimum, maximum, symmetric = false) {
    if (symmetric) {
      const extent = Math.max(Math.abs(minimum), Math.abs(maximum), 1e-9) * 1.12;
      return [-extent, extent];
    }
    const span = maximum - minimum;
    const pad = span > 0 ? span * 0.08 : Math.max(Math.abs(minimum) * 0.02, 1e-6);
    return [minimum - pad, maximum + pad];
  }

  function formatDate(milliseconds, short = false) {
    const options = short
      ? { year: "2-digit", month: "short", timeZone: "UTC" }
      : { year: "numeric", month: "short", day: "2-digit", timeZone: "UTC" };
    return new Intl.DateTimeFormat("en", options).format(new Date(milliseconds));
  }

  function splitCoordinate(splitAnchor) {
    return state.axis === "row" ? splitAnchor : estimateTimeAtAnchor(state.primary, splitAnchor);
  }

  function drawSplitBands(context, layout, xScale) {
    context.save();
    context.beginPath();
    context.rect(layout.left, layout.top, layout.width, layout.height);
    context.clip();
    state.catalog.splits.forEach((split) => {
      const start = xScale(splitCoordinate(split.begin));
      const end = xScale(splitCoordinate(split.end_exclusive));
      const left = Math.max(layout.left, Math.min(start, end));
      const right = Math.min(layout.left + layout.width, Math.max(start, end));
      if (right <= left) return;
      const color = SPLIT_COLORS[split.id] || COLOR.axis;
      context.globalAlpha = split.served ? 0.055 : 0.025;
      context.fillStyle = color;
      context.fillRect(left, layout.top, right - left, layout.height);
      context.globalAlpha = 0.62;
      context.strokeStyle = color;
      context.setLineDash(split.served ? [] : [4, 4]);
      context.beginPath();
      context.moveTo(left, layout.top);
      context.lineTo(left, layout.top + layout.height);
      context.stroke();
      if (right - left > 68) {
        context.globalAlpha = 0.68;
        context.fillStyle = color;
        context.font = "600 9px ui-sans-serif, system-ui";
        context.fillText(split.id.replace("_holdout", ""), left + 7, layout.top + 13);
      }
    });
    context.restore();
  }

  function drawGrid(context, layout, xDomainValues, yDomainValues, xScale, yScale, isReturn) {
    context.save();
    context.font = "10px ui-monospace, monospace";
    context.fillStyle = COLOR.axis;
    context.strokeStyle = COLOR.grid;
    context.lineWidth = 1;
    context.textBaseline = "middle";
    for (let index = 0; index <= 4; index += 1) {
      const value = yDomainValues[0] + (yDomainValues[1] - yDomainValues[0]) * (index / 4);
      const y = yScale(value);
      context.beginPath();
      context.moveTo(layout.left, y);
      context.lineTo(layout.left + layout.width, y);
      context.stroke();
      context.textAlign = "right";
      context.fillText(isReturn ? `${(value * 100).toFixed(2)}%` : formatNumber(value, 4), layout.left - 8, y);
    }
    context.textBaseline = "top";
    for (let index = 0; index <= 5; index += 1) {
      const value = xDomainValues[0] + (xDomainValues[1] - xDomainValues[0]) * (index / 5);
      const x = xScale(value);
      context.beginPath();
      context.moveTo(x, layout.top);
      context.lineTo(x, layout.top + layout.height);
      context.stroke();
      context.textAlign = index === 0 ? "left" : index === 5 ? "right" : "center";
      context.fillText(state.axis === "row" ? String(Math.round(value)) : formatDate(value, true), x, layout.top + layout.height + 9);
    }
    context.restore();
  }

  function drawSeries(context, chart, field, color, xScale, yScale, layout) {
    context.save();
    context.beginPath();
    context.rect(layout.left, layout.top, layout.width, layout.height);
    context.clip();
    context.beginPath();
    chart.points.forEach((point, index) => {
      const x = xScale(pointX(point));
      const y = yScale(point[field]);
      if (index === 0) context.moveTo(x, y);
      else context.lineTo(x, y);
    });
    context.strokeStyle = color;
    context.lineWidth = field === "return" ? 1.2 : 1.7;
    context.lineJoin = "round";
    context.lineCap = "round";
    context.stroke();
    context.restore();
  }

  function pointForAnchor(chart, anchor) {
    if (!chart) return null;
    const index = Math.round(anchor - chart.points[0].anchor);
    const point = chart.points[index];
    return point && point.anchor === anchor ? point : chart.points.reduce((best, candidate) =>
      !best || Math.abs(candidate.anchor - anchor) < Math.abs(best.anchor - anchor) ? candidate : best, null);
  }

  function drawTimeSeries(canvas, field) {
    if (!state.primary) return null;
    const { context, width, height } = canvasFrame(canvas);
    const layout = plotLayout(width, height);
    const domainX = xDomain();
    const values = [state.primary, state.comparison].filter(Boolean).flatMap((chart) => chart.points.map((point) => point[field]));
    const domainY = expandDomain(Math.min(...values), Math.max(...values), field === "return");
    const xScale = (value) => layout.left + ((value - domainX[0]) / (domainX[1] - domainX[0])) * layout.width;
    const yScale = (value) => layout.top + (1 - (value - domainY[0]) / (domainY[1] - domainY[0])) * layout.height;
    context.fillStyle = COLOR.bg;
    context.fillRect(0, 0, width, height);
    drawSplitBands(context, layout, xScale);
    drawGrid(context, layout, domainX, domainY, xScale, yScale, field === "return");
    if (field === "return") {
      context.strokeStyle = "rgba(255,255,255,.28)";
      context.beginPath();
      context.moveTo(layout.left, yScale(0));
      context.lineTo(layout.left + layout.width, yScale(0));
      context.stroke();
    }
    if (state.comparison) drawSeries(context, state.comparison, field, COLOR.cyan, xScale, yScale, layout);
    drawSeries(context, state.primary, field, COLOR.green, xScale, yScale, layout);

    if (state.hoverAnchor !== null) {
      const point = pointForAnchor(state.primary, state.hoverAnchor);
      if (point) {
        const x = xScale(pointX(point));
        const y = yScale(point[field]);
        context.save();
        context.strokeStyle = "rgba(244,244,241,.45)";
        context.setLineDash([3, 4]);
        context.beginPath();
        context.moveTo(x, layout.top);
        context.lineTo(x, layout.top + layout.height);
        context.stroke();
        context.setLineDash([]);
        context.fillStyle = COLOR.green;
        context.beginPath();
        context.arc(x, y, 3.5, 0, Math.PI * 2);
        context.fill();
        context.restore();
      }
    }
    return { layout, domainX, domainY, xScale, yScale };
  }

  function drawAcf() {
    if (!state.primary) return null;
    const { context, width, height } = canvasFrame(el.acfCanvas);
    const layout = plotLayout(width, height);
    const values = state.acf.map((item) => item.value).filter(Number.isFinite);
    const extent = Math.max(0.1, ...values.map(Math.abs)) * 1.12;
    const yScale = (value) => layout.top + (1 - (value + extent) / (extent * 2)) * layout.height;
    const step = layout.width / Math.max(state.acf.length, 1);
    context.fillStyle = COLOR.bg;
    context.fillRect(0, 0, width, height);
    context.font = "10px ui-monospace, monospace";
    context.fillStyle = COLOR.axis;
    context.strokeStyle = COLOR.grid;
    context.textAlign = "right";
    context.textBaseline = "middle";
    for (let index = 0; index <= 4; index += 1) {
      const value = -extent + (extent * 2 * index) / 4;
      const y = yScale(value);
      context.beginPath();
      context.moveTo(layout.left, y);
      context.lineTo(layout.left + layout.width, y);
      context.stroke();
      context.fillText(value.toFixed(2), layout.left - 8, y);
    }
    const bars = [];
    state.acf.forEach((item, index) => {
      const x = layout.left + index * step + step * 0.16;
      const barWidth = Math.max(1, step * 0.68);
      const yZero = yScale(0);
      const yValue = yScale(item.value);
      context.fillStyle = item.lag === state.hoverLag ? COLOR.cyan : item.value >= 0 ? COLOR.green : COLOR.red;
      context.globalAlpha = item.lag === state.hoverLag ? 1 : 0.78;
      context.fillRect(x, Math.min(yZero, yValue), barWidth, Math.max(1, Math.abs(yValue - yZero)));
      bars.push({ x, width: barWidth, item });
    });
    context.globalAlpha = 1;
    context.strokeStyle = "rgba(255,255,255,.35)";
    context.beginPath();
    context.moveTo(layout.left, yScale(0));
    context.lineTo(layout.left + layout.width, yScale(0));
    context.stroke();
    context.fillStyle = COLOR.axis;
    context.textAlign = "center";
    context.textBaseline = "top";
    const tickEvery = width < 520 ? 12 : 6;
    state.acf.forEach((item, index) => {
      if (item.lag === 1 || item.lag % tickEvery === 0) {
        context.fillText(String(item.lag), layout.left + (index + 0.5) * step, layout.top + layout.height + 9);
      }
    });
    return { layout, bars };
  }

  function scheduleRender() {
    if (state.renderFrame) return;
    state.renderFrame = window.requestAnimationFrame(() => {
      state.renderFrame = 0;
      state.plot.close = drawTimeSeries(el.closeCanvas, "close");
      state.plot.return = drawTimeSeries(el.returnCanvas, "return");
      state.plot.acf = drawAcf();
      positionTooltips();
    });
  }

  function nearestPrimaryFromEvent(canvas, event, plot) {
    if (!plot || !state.primary) return null;
    const bounds = canvas.getBoundingClientRect();
    const pixelX = event.clientX - bounds.left;
    const dataX = plot.domainX[0] + ((pixelX - plot.layout.left) / plot.layout.width) * (plot.domainX[1] - plot.domainX[0]);
    let low = 0;
    let high = state.primary.points.length - 1;
    while (low < high) {
      const middle = Math.floor((low + high) / 2);
      if (pointX(state.primary.points[middle]) < dataX) low = middle + 1;
      else high = middle;
    }
    const candidates = [state.primary.points[low], state.primary.points[Math.max(0, low - 1)]].filter(Boolean);
    return candidates.reduce((best, point) => !best || Math.abs(pointX(point) - dataX) < Math.abs(pointX(best) - dataX) ? point : best, null);
  }

  function tooltipText(point, field) {
    const label = field === "return" ? "return" : "close";
    const value = field === "return" ? `${point.return >= 0 ? "+" : ""}${(point.return * 100).toFixed(5)}%` : formatNumber(point.close, 8);
    const lines = [`anchor  ${point.anchor}`, `UTC     ${formatDate(point.closeTime)}`, `${state.primary.interval.padEnd(7)} ${label} ${value}`];
    if (state.comparison) {
      const comparison = state.axis === "row"
        ? pointForAnchor(state.comparison, point.anchor)
        : state.comparison.points.reduce((best, candidate) => !best || Math.abs(candidate.closeTime - point.closeTime) < Math.abs(best.closeTime - point.closeTime) ? candidate : best, null);
      if (comparison) {
        const comparisonValue = field === "return" ? `${comparison.return >= 0 ? "+" : ""}${(comparison.return * 100).toFixed(5)}%` : formatNumber(comparison.close, 8);
        lines.push(`${state.comparison.interval.padEnd(7)} ${label} ${comparisonValue}`);
        if (state.axis === "time") lines.push(`Δ time  ${formatNumber((comparison.closeTime - point.closeTime) / 86400000, 1)} days`);
      }
    }
    return lines.join("\n");
  }

  function placeTooltip(node, _canvas, plot, point, text) {
    if (!point || !plot) {
      node.hidden = true;
      return;
    }
    node.textContent = text;
    node.hidden = false;
  }

  function positionTooltips() {
    const point = state.hoverAnchor === null ? null : pointForAnchor(state.primary, state.hoverAnchor);
    placeTooltip(el.closeTooltip, el.closeCanvas, state.plot.close, point, point ? tooltipText(point, "close") : "");
    placeTooltip(el.returnTooltip, el.returnCanvas, state.plot.return, point, point ? tooltipText(point, "return") : "");
    if (state.hoverLag === null || !state.plot.acf) {
      el.acfTooltip.hidden = true;
    } else {
      const item = state.acf.find((candidate) => candidate.lag === state.hoverLag);
      const bar = state.plot.acf.bars.find((candidate) => candidate.item.lag === state.hoverLag);
      if (item && bar) {
        el.acfTooltip.textContent = `lag ${item.lag}\nautocorrelation ${formatNumber(item.value, 6)}`;
        el.acfTooltip.hidden = false;
      }
    }
  }

  function bindEvents() {
    el.benchmark.addEventListener("change", () => {
      state.benchmark = el.benchmark.value;
      updateLocationBenchmark();
      loadCatalog();
    });
    el.instrument.addEventListener("change", loadCharts);
    el.interval.addEventListener("change", () => {
      syncComparisonOptions();
      loadCharts();
    });
    el.compareToggle.addEventListener("change", () => {
      syncComparisonOptions();
      loadCharts();
    });
    el.compare.addEventListener("change", loadCharts);
    el.reload.addEventListener("click", loadCharts);
    el.retry.addEventListener("click", () => {
      if (state.catalog) loadCharts();
      else if (state.benchmarkCatalog) loadCatalog();
      else loadBenchmarkCatalog();
    });
    document.querySelectorAll("input[name=axis]").forEach((input) => {
      input.addEventListener("change", () => {
        state.axis = input.value;
        if (state.primary) {
          renderSummary();
          scheduleRender();
          el.live.textContent = `${input.value === "row" ? "Row-index" : "Wall-clock"} axis selected`;
        }
      });
    });
    [[el.closeCanvas, "close"], [el.returnCanvas, "return"]].forEach(([canvas, key]) => {
      canvas.addEventListener("pointermove", (event) => {
        const point = nearestPrimaryFromEvent(canvas, event, state.plot[key]);
        if (point && state.hoverAnchor !== point.anchor) {
          state.hoverAnchor = point.anchor;
          scheduleRender();
        }
      });
      canvas.addEventListener("pointerleave", () => {
        state.hoverAnchor = null;
        scheduleRender();
      });
    });
    el.acfCanvas.addEventListener("pointermove", (event) => {
      if (!state.plot.acf) return;
      const x = event.clientX - el.acfCanvas.getBoundingClientRect().left;
      const bar = state.plot.acf.bars.find((candidate) => x >= candidate.x && x <= candidate.x + candidate.width);
      const lag = bar ? bar.item.lag : null;
      if (lag !== state.hoverLag) {
        state.hoverLag = lag;
        scheduleRender();
      }
    });
    el.acfCanvas.addEventListener("pointerleave", () => {
      state.hoverLag = null;
      scheduleRender();
    });
    const resizeObserver = new ResizeObserver(scheduleRender);
    [el.closeCanvas, el.returnCanvas, el.acfCanvas].forEach((canvas) => resizeObserver.observe(canvas));
  }

  bindEvents();
  loadBenchmarkCatalog();
})();
