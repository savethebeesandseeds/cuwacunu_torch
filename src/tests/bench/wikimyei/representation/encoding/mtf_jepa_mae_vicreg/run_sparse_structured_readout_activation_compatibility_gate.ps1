param(
    [string]$Container = "unnamed_taoist",
    [string]$RuntimeRoot = "",
    [string]$PythonBin = ""
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest
if ($PSVersionTable.PSVersion.Major -lt 7) {
    throw "SRR-3R custody requires PowerShell 7+ for the frozen long checkpoint paths"
}

$ScriptDir = [IO.Path]::GetFullPath($PSScriptRoot)
$RepoRoot = [IO.Path]::GetFullPath(
    (Join-Path $ScriptDir "../../../../../../../")
).TrimEnd([IO.Path]::DirectorySeparatorChar)
$SchemaId = "wikimyei.mtf_jepa_mae_vicreg.srr3r_sparse_activation_compatibility.v1"

if ([string]::IsNullOrWhiteSpace($RuntimeRoot)) {
    $RuntimeRoot = Join-Path $RepoRoot ".runtime/benchmarks/synthetic_continuous_graph_v1/$SchemaId/attempt_000001"
}
$RuntimeRoot = [IO.Path]::GetFullPath($RuntimeRoot)
if ([string]::IsNullOrWhiteSpace($PythonBin)) {
    if (-not [string]::IsNullOrWhiteSpace($env:SRR3R_PYTHON_BIN)) {
        $PythonBin = $env:SRR3R_PYTHON_BIN
    } else {
        $PythonBin = Join-Path $env:USERPROFILE ".cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe"
    }
}
$PythonBin = [IO.Path]::GetFullPath($PythonBin)

$CaptureSource = Join-Path $RepoRoot "src/main/exec/cuwacunu_srr3r_sparse_activation_capture.cpp"
$CaptureBinary = Join-Path $RepoRoot ".build/exec/cuwacunu_srr3r_sparse_activation_capture"
$Evaluator = Join-Path $ScriptDir "evaluate_sparse_structured_readout_activation_compatibility.py"
$ImportedEvaluator = Join-Path $ScriptDir "evaluate_structured_readout_activation_compatibility.py"
$Protocol = Join-Path $ScriptDir "SPARSE_STRUCTURED_READOUT_ACTIVATION_COMPATIBILITY_PROTOCOL.md"
$ProtocolPin = Join-Path $ScriptDir "SPARSE_STRUCTURED_READOUT_ACTIVATION_COMPATIBILITY_PROTOCOL.sha256"
$Srr4Protocol = Join-Path $ScriptDir "SPARSE_SURFACE_STRUCTURED_READOUT_CONTRACT_REPAIR_PROTOCOL.md"
$Runner = [IO.Path]::GetFullPath($MyInvocation.MyCommand.Path)

$ProductionHeader = Join-Path $RepoRoot "src/include/wikimyei/representation/encoding/mtf_jepa_mae_vicreg/mtf_jepa_mae_vicreg.h"
$ProductionParser = Join-Path $RepoRoot "src/include/wikimyei/representation/encoding/mtf_jepa_mae_vicreg/mtf_jepa_mae_vicreg_spec.h"
$FocusedMechanicsTest = Join-Path $ScriptDir "test_structured_cdsb_sparse_v1.cpp"
$LegacyProductionTest = Join-Path $ScriptDir "test_production_structured_readout.cpp"
$LauncherIntegrationTest = Join-Path $RepoRoot "src/tests/bench/jkimyei/training/channel_graph_first_launchers/test_jkimyei_channel_graph_first_launchers.cpp"
$GraphSpecIntegrationTest = Join-Path $RepoRoot "src/tests/bench/wikimyei/config/graph_first_specs/test_wikimyei_graph_first_specs.cpp"
$FocusedMechanicsBinary = Join-Path $RepoRoot ".build/tests/test_structured_cdsb_sparse_v1"
$LegacyProductionBinary = Join-Path $RepoRoot ".build/tests/test_production_structured_readout"
$LauncherIntegrationBinary = Join-Path $RepoRoot ".build/tests/test_jkimyei_channel_graph_first_launchers"
$GraphSpecIntegrationBinary = Join-Path $RepoRoot ".build/tests/test_wikimyei_graph_first_specs"
$CaptureMakefile = Join-Path $RepoRoot "src/main/exec/Makefile"
$FocusedMakefile = Join-Path $ScriptDir "Makefile"
$LauncherMakefile = Join-Path $RepoRoot "src/tests/bench/jkimyei/training/channel_graph_first_launchers/Makefile"
$GraphSpecMakefile = Join-Path $RepoRoot "src/tests/bench/wikimyei/config/graph_first_specs/Makefile"

$FrozenBaseConfig = Join-Path $RepoRoot "src/config/benchmarks/synthetic_continuous_graph_v1/synthetic_benchmark.config"
$Config = Join-Path $RepoRoot "src/config/benchmarks/synthetic_continuous_graph_v1/srr3_activation_compatibility.config"
$ActiveDsl = Join-Path $RepoRoot "src/config/wikimyei.representation.mtf_jepa_mae_vicreg.dsl"
$RepresentationCheckpoint = Join-Path $RepoRoot ".runtime/cuwacunu_exec/components/wikimyei.representation.encoding.mtf_jepa_mae_vicreg/spawns/diz_b8a87dee0c986487/jobs/train/train_core_mtf_jepa_mae_vicreg.train.channel_representation_mtf_jepa_mae_vicreg.attempt_000001/channel_representation.report.mtf_jepa_mae_vicreg.pt"
$MdnCheckpoint = Join-Path $RepoRoot ".runtime/cuwacunu_exec/components/wikimyei.inference.expected_value.mdn/spawns/syq_fd0cba7ed6f1feb8/jobs/train/train_core_channel_mdn.train.channel_inference_mdn.attempt_000001/channel_inference.report.channel_mdn.pt"

$Srr4Root = Join-Path $RepoRoot ".runtime/benchmarks/synthetic_continuous_graph_v1/wikimyei.mtf_jepa_mae_vicreg.srr4_sparse_surface_contract_repair.v1/attempt_000001"
$Srr4Completion = Join-Path $Srr4Root "completion.receipt"
$Srr4Report = Join-Path $Srr4Root "representation_value.report"
$Srr4DevelopmentBaseline = Join-Path $Srr4Root "development_0_730/all_tokens.representation_edge_features.probe"
$Srr4DevelopmentCandidate = Join-Path $Srr4Root "development_0_730/structured_cdsb_sparse_v1.representation_edge_features.probe"
$Srr4ConfirmationBaseline = Join-Path $Srr4Root "confirmation_760_1088/all_tokens.representation_edge_features.probe"
$Srr4ConfirmationCandidate = Join-Path $Srr4Root "confirmation_760_1088/structured_cdsb_sparse_v1.representation_edge_features.probe"

$Expected = @{
    Protocol = "6deee9c2420e205828322cee34b8d5d43a83c98918670ede682d8e36e17de6da"
    ProtocolSize = 13639L
    FrozenBaseConfig = "7c84cee94ecf839336c0383878298981b9ab362e80a570cefef20e9fed272fd6"
    FrozenBaseConfigSize = 4287L
    Config = "23d94f9222527fcc14cbc3948861b42e06b0f55c992f8e3e01b8ebc1bd8149e0"
    ConfigSize = 4298L
    ActiveDsl = "68015c25d689227141ef62c94b8d0aa01549787f7454b0396ee4fee5c8aa61ba"
    ActiveDslSize = 639L
    ProductionHeader = "0664d062914971af58424037f92569fc0828ffc3bc2b240bb589670d69164b88"
    ProductionParser = "7d5e05da2fb64e2101074c9775b0209d69c831bb2d0f534a4c3ca15313e65d49"
    RepresentationCheckpoint = "8a43cc9275954fa03dbd1d140fa74bd1c71f57d8941fe005ec258c8956b5c9de"
    RepresentationCheckpointSize = 853867L
    MdnCheckpoint = "eb5643b752994f4c3b1cc21202f1fec1a82bc3240ab578b5cf18127010155d8e"
    MdnCheckpointSize = 3227665L
    Srr4Protocol = "a634a1ae386a5a0bebb10440cf0d45730e2dd652375aa3fb80a4d9277708ed30"
    Srr4ProtocolSize = 13186L
    Srr4Completion = "9058e77cf384ec33fbeced1d3aac2ac9288783be70fece8025f24cc5b9d6c8ac"
    Srr4CompletionSize = 406L
    Srr4Report = "47252fc1fc51ca8ab55db570e914a3c2f11d62bc3e6d5dc01359c4512d61fd9f"
    Srr4ReportSize = 21790L
    Evaluator = "4dc59092acfb46bfe39ef906d77ab6eb994ebec6505073967ba09a36507241bf"
    EvaluatorSize = 36125L
    ImportedEvaluator = "94d343284d3ce2d2272d31c6e5d24c8c34820356d5238434a85caecd8c423663"
    StageABaseline = "8f6f72b78c0708b5f23512ada4ca8536ea8818f8e2d3d9bc501401d8ab0ce3c7"
    StageABaselineSize = 6133066L
    StageACandidate = "dfac215b73b08525dcba90d8891c8dede328ed99ec0117e2e2efaea6a5afbd73"
    StageACandidateSize = 6112783L
    Srr4DevelopmentBaseline = "d3465e44ed15647e158b9cabf00f4b1670797fdddc5539c0dd4a067db7b193ed"
    Srr4DevelopmentBaselineSize = 13648442L
    Srr4DevelopmentCandidate = "2d65315536246e56f35fc8981c5f0f1157770d25964050e5265951589fd508d1"
    Srr4DevelopmentCandidateSize = 13601883L
}

$Utf8NoBom = [Text.UTF8Encoding]::new($false)

function Invoke-Checked {
    param([string]$File, [string[]]$Arguments, [switch]$Quiet)
    if ($Quiet) {
        & $File @Arguments | Out-Null
    } else {
        & $File @Arguments
    }
    if ($LASTEXITCODE -ne 0) {
        throw "command failed with exit code $LASTEXITCODE`: $File $($Arguments -join ' ')"
    }
}

function Invoke-CapturedChecked {
    param([string]$File, [string[]]$Arguments)
    $Text = (& $File @Arguments 2>&1 | Out-String)
    if ($LASTEXITCODE -ne 0) {
        throw "command failed with exit code $LASTEXITCODE`: $File $($Arguments -join ' ')`n$Text"
    }
    return $Text
}

function Require-File {
    param([string]$Path)
    $Item = Get-Item -LiteralPath $Path -Force
    if ($Item.PSIsContainer -or $null -ne $Item.LinkType) {
        throw "not a regular non-link file: $Path"
    }
}

function Require-Size {
    param([string]$Path, [int64]$ExpectedSize)
    Require-File $Path
    $ActualSize = (Get-Item -LiteralPath $Path -Force).Length
    if ($ActualSize -ne $ExpectedSize) {
        throw "size mismatch for $Path`: expected $ExpectedSize, got $ActualSize"
    }
}

function Get-Sha256 {
    param([string]$Path)
    return (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToLowerInvariant()
}

function Require-Sha256 {
    param([string]$Path, [string]$ExpectedHash)
    Require-File $Path
    $ActualHash = Get-Sha256 $Path
    if ($ActualHash -ne $ExpectedHash) {
        throw "SHA-256 mismatch for $Path`: expected $ExpectedHash, got $ActualHash"
    }
}

function Require-FrozenFile {
    param([string]$Path, [int64]$ExpectedSize, [string]$ExpectedHash)
    Require-Size $Path $ExpectedSize
    Require-Sha256 $Path $ExpectedHash
}

function To-LinuxPath {
    param([string]$Path)
    $Full = [IO.Path]::GetFullPath($Path)
    $Prefix = $RepoRoot + [IO.Path]::DirectorySeparatorChar
    if (-not $Full.StartsWith($Prefix, [StringComparison]::OrdinalIgnoreCase)) {
        throw "path is outside repository mount: $Full"
    }
    $Relative = [IO.Path]::GetRelativePath($RepoRoot, $Full).Replace('\', '/')
    return "/cuwacunu/$Relative"
}

function Read-Kv {
    param([string]$Path)
    Require-File $Path
    $Values = [ordered]@{}
    foreach ($Line in [IO.File]::ReadAllLines($Path)) {
        $Index = $Line.IndexOf('=')
        if ($Index -le 0) { continue }
        $Key = $Line.Substring(0, $Index)
        if ($Values.Contains($Key)) {
            throw "duplicate key $Key in $Path"
        }
        $Values[$Key] = $Line.Substring($Index + 1)
    }
    return $Values
}

function Expect-Kv {
    param([System.Collections.IDictionary]$Values, [string]$Key, [string]$ExpectedValue)
    if (-not $Values.Contains($Key) -or [string]$Values[$Key] -ne $ExpectedValue) {
        $Actual = if ($Values.Contains($Key)) { [string]$Values[$Key] } else { "<missing>" }
        throw "expected $Key=$ExpectedValue, got $Actual"
    }
}

function Require-Rows {
    param([string]$Path, [int]$ExpectedRows)
    Require-File $Path
    $Count = ([IO.File]::ReadLines($Path) | Measure-Object).Count - 1
    if ($Count -ne $ExpectedRows) {
        throw "expected $ExpectedRows data rows in $Path, got $Count"
    }
}

function Write-HashManifest {
    param([string[]]$Paths, [string]$Output)
    $Lines = foreach ($Path in ($Paths | Sort-Object -Unique)) {
        Require-File $Path
        $Full = [IO.Path]::GetFullPath($Path)
        $Prefix = $RepoRoot + [IO.Path]::DirectorySeparatorChar
        if (-not $Full.StartsWith($Prefix, [StringComparison]::OrdinalIgnoreCase)) {
            throw "cannot seal a path outside the repository: $Full"
        }
        $Relative = [IO.Path]::GetRelativePath($RepoRoot, $Full).Replace('\', '/')
        "$(Get-Sha256 $Full)  $Relative"
    }
    [IO.File]::WriteAllLines($Output, $Lines, $Utf8NoBom)
}

function Assert-HashManifest {
    param([string]$Manifest)
    Require-File $Manifest
    foreach ($Line in [IO.File]::ReadAllLines($Manifest)) {
        if ($Line -notmatch '^([0-9a-f]{64})  (.+)$') {
            throw "invalid hash-manifest line in $Manifest`: $Line"
        }
        $ExpectedHash = $Matches[1]
        $Path = [IO.Path]::GetFullPath((Join-Path $RepoRoot $Matches[2]))
        $Prefix = $RepoRoot + [IO.Path]::DirectorySeparatorChar
        if (-not $Path.StartsWith($Prefix, [StringComparison]::OrdinalIgnoreCase)) {
            throw "hash manifest escapes repository root: $Path"
        }
        Require-Sha256 $Path $ExpectedHash
    }
}

function Require-ByteExact {
    param([string]$Left, [string]$Right)
    Require-File $Left
    Require-File $Right
    $LeftStream = [IO.File]::OpenRead($Left)
    $RightStream = [IO.File]::OpenRead($Right)
    try {
        if ($LeftStream.Length -ne $RightStream.Length) {
            throw "byte-exact replay length mismatch: $Left vs $Right"
        }
        $LeftBuffer = [byte[]]::new(65536)
        $RightBuffer = [byte[]]::new(65536)
        while ($true) {
            $LeftCount = $LeftStream.Read($LeftBuffer, 0, $LeftBuffer.Length)
            $RightCount = $RightStream.Read($RightBuffer, 0, $RightBuffer.Length)
            if ($LeftCount -ne $RightCount) {
                throw "byte-exact replay read mismatch: $Left vs $Right"
            }
            if ($LeftCount -eq 0) { break }
            for ($Index = 0; $Index -lt $LeftCount; ++$Index) {
                if ($LeftBuffer[$Index] -ne $RightBuffer[$Index]) {
                    throw "byte-exact replay differs at a persisted byte: $Left vs $Right"
                }
            }
        }
    } finally {
        $LeftStream.Dispose()
        $RightStream.Dispose()
    }
}

function Invoke-StageACapture {
    param([string]$OutputDirectory)
    $Arguments = @(
        "exec", "-e", "CUBLAS_WORKSPACE_CONFIG=:4096:8", $Container,
        (To-LinuxPath $CaptureBinary),
        "--config", (To-LinuxPath $Config),
        "--input-representation-checkpoint", (To-LinuxPath $RepresentationCheckpoint),
        "--input-mdn-checkpoint", (To-LinuxPath $MdnCheckpoint),
        "--output-dir", (To-LinuxPath $OutputDirectory),
        "--anchor-index-begin", "760",
        "--anchor-index-end", "1088"
    )
    Invoke-Checked "docker" $Arguments
}

function Validate-StageACapture {
    param([string]$Directory)

    $ReceiptPath = Join-Path $Directory "mechanics.receipt"
    $BaselineFeatures = Join-Path $Directory "all_tokens.features.csv"
    $CandidateFeatures = Join-Path $Directory "structured_cdsb_sparse_v1.features.csv"
    $BaselinePredictions = Join-Path $Directory "all_tokens.predictions.csv"
    $CandidatePredictions = Join-Path $Directory "structured_cdsb_sparse_v1.predictions.csv"
    $Receipt = Read-Kv $ReceiptPath

    $Checks = [ordered]@{
        schema_id = "cuwacunu.srr3r.sparse_activation_capture.mechanics.v1"
        status = "mechanics_pass"
        stage = "stage_a"
        sealed_protocol_sha256 = $Expected.Protocol
        sealed_protocol_size = $Expected.ProtocolSize.ToString()
        range_id = "historical_confirmation_760_1088"
        anchor_range = "[760,1088)"
        anchor_count = "328"
        maximum_anchor_read = "1087"
        final_holdout_range = "[1088,1170)"
        final_holdout_access = "false"
        seed = "31"
        source_order = "contiguous_sequential_anchor_index"
        active_production_policy = "all_tokens"
        baseline_policy = "all_tokens"
        candidate_policy = "structured_cdsb_sparse_v1"
        loaded_mdn_checkpoint_policy = "all_tokens"
        sparse_identity_expected_policy = "structured_cdsb_sparse_v1"
        candidate_computation = "legacy_all_tokens_head_on_sparse_semantics"
        candidate_checkpoint_identity_relabelled = "false"
        candidate_checkpoint_copied = "false"
        candidate_checkpoint_rewritten = "false"
        candidate_checkpoint_identity_bypassed = "false"
        same_loaded_mdn_both_arms = "true"
        source_batch_ceiling = "6"
        encoder_calls_equal_source_batches = "true"
        mdn_model_constructions = "1"
        same_encoded_object = "true"
        same_retained_encoded_object = "true"
        encoded_bytes_stable = "true"
        input_data_unchanged = "true"
        input_mask_unchanged = "true"
        converted_data_bytes_stable = "true"
        converted_feature_mask_bytes_stable = "true"
        public_rows = "1312"
        public_mask_cells = "3936"
        context_mask_cells = "3936"
        baseline_valid_cells = "3936"
        baseline_context_valid_cells = "3936"
        candidate_valid_cells = "3936"
        candidate_context_valid_cells = "3936"
        public_masks_exact = "true"
        context_masks_exact = "true"
        public_contract_exact = "true"
        selector_contract_exact = "true"
        invalid_zero_exact = "true"
        selector_outputs_finite = "true"
        mdn_outputs_finite = "true"
        paired_adapter_contract_exact = "true"
        paired_rows_keys_targets_order_exact = "true"
        feature_count = "96"
        baseline_feature_rows = "2952"
        candidate_feature_rows = "2952"
        baseline_prediction_rows = "2952"
        candidate_prediction_rows = "2952"
        prediction_valid_bits_exact = "true"
        prediction_valid_coverage_exact = "true"
        sigma_finite = "true"
        representation_parameters_unchanged = "true"
        representation_buffers_unchanged = "true"
        representation_eval_unchanged = "true"
        mdn_parameters_unchanged = "true"
        mdn_buffers_unchanged = "true"
        mdn_eval_unchanged = "true"
        cpu_rng_unchanged = "true"
        cuda_rng_unchanged = "true"
        mdn_successful_weight_loads = "1"
        mdn_identity_authentication_attempts = "2"
        all_tokens_identity_load_pass = "true"
        sparse_identity_rejected = "true"
        sparse_identity_attempt_model_bytes_unchanged = "true"
        checkpoint_graph_identity_exact = "true"
        checkpoint_node_order_exact = "true"
        config_size = $Expected.ConfigSize.ToString()
        config_sha256 = $Expected.Config
        config_bytes_unchanged = "true"
        representation_checkpoint_size = $Expected.RepresentationCheckpointSize.ToString()
        representation_checkpoint_sha256 = $Expected.RepresentationCheckpoint
        representation_checkpoint_bytes_unchanged = "true"
        mdn_checkpoint_size = $Expected.MdnCheckpointSize.ToString()
        mdn_checkpoint_sha256 = $Expected.MdnCheckpoint
        mdn_checkpoint_bytes_unchanged = "true"
        baseline_feature_size = $Expected.StageABaselineSize.ToString()
        baseline_feature_sha256 = $Expected.StageABaseline
        baseline_feature_frozen_hash_match = "true"
        candidate_feature_size = $Expected.StageACandidateSize.ToString()
        candidate_feature_sha256 = $Expected.StageACandidate
        candidate_feature_frozen_hash_match = "true"
        endpoint_metrics_computed = "false"
        augmentations_enabled = "false"
        optimizer_steps = "0"
        backward_calls = "0"
        checkpoint_writes = "0"
        policy_activation_changed = "false"
    }
    foreach ($Pair in $Checks.GetEnumerator()) {
        Expect-Kv $Receipt $Pair.Key ([string]$Pair.Value)
    }

    if ([string]::IsNullOrWhiteSpace($Receipt.graph_order_fingerprint)) {
        throw "Stage A graph-order fingerprint is empty"
    }
    if ([string]::IsNullOrWhiteSpace($Receipt.sparse_identity_error) -or
        $Receipt.sparse_identity_error -notmatch 'serving pool policy') {
        throw "Stage A sparse identity rejection did not report the serving-policy mismatch"
    }
    $SourceBatches = [int]$Receipt.source_batches
    $EncoderCalls = [int]$Receipt.encoder_calls
    $MdnForwards = [int]$Receipt.mdn_forwards
    if ($SourceBatches -lt 1 -or $SourceBatches -gt 6 -or
        $EncoderCalls -ne $SourceBatches -or
        $MdnForwards -ne (2 * $SourceBatches)) {
        throw "Stage A source/encoder/MDN call accounting is not exact"
    }
    if ([int]$Receipt.baseline_prediction_valid_rows -ne
        [int]$Receipt.candidate_prediction_valid_rows -or
        [int]$Receipt.observed_paired_prediction_valid_rows -ne
        [int]$Receipt.baseline_prediction_valid_rows) {
        throw "Stage A prediction valid-row coverage differs between arms"
    }
    if ([int]$Receipt.observed_paired_prediction_valid_rows -lt 1 -or
        [int]$Receipt.observed_paired_prediction_valid_rows -gt 2952) {
        throw "Stage A observed prediction-valid count is outside the frozen surface"
    }

    foreach ($Pair in ([ordered]@{
        config_path = (To-LinuxPath $Config)
        representation_checkpoint_path = (To-LinuxPath $RepresentationCheckpoint)
        mdn_checkpoint_path = (To-LinuxPath $MdnCheckpoint)
        baseline_feature_path = (To-LinuxPath $BaselineFeatures)
        candidate_feature_path = (To-LinuxPath $CandidateFeatures)
        baseline_prediction_path = (To-LinuxPath $BaselinePredictions)
        candidate_prediction_path = (To-LinuxPath $CandidatePredictions)
    }).GetEnumerator()) {
        Expect-Kv $Receipt $Pair.Key ([string]$Pair.Value)
    }

    Require-Rows $BaselineFeatures 2952
    Require-Rows $CandidateFeatures 2952
    Require-Rows $BaselinePredictions 2952
    Require-Rows $CandidatePredictions 2952
    Require-FrozenFile $BaselineFeatures $Expected.StageABaselineSize $Expected.StageABaseline
    Require-FrozenFile $CandidateFeatures $Expected.StageACandidateSize $Expected.StageACandidate
    Require-Sha256 $BaselinePredictions $Receipt.baseline_prediction_sha256
    Require-Sha256 $CandidatePredictions $Receipt.candidate_prediction_sha256
    if ((Get-Item -LiteralPath $BaselinePredictions).Length -ne [int64]$Receipt.baseline_prediction_size -or
        (Get-Item -LiteralPath $CandidatePredictions).Length -ne [int64]$Receipt.candidate_prediction_size) {
        throw "Stage A prediction artifact size differs from its mechanics receipt"
    }

    return @(
        $ReceiptPath,
        $BaselineFeatures,
        $CandidateFeatures,
        $BaselinePredictions,
        $CandidatePredictions
    )
}

function Validate-Srr4Authorization {
    $Completion = Read-Kv $Srr4Completion
    foreach ($Pair in ([ordered]@{
        schema_id = "wikimyei.mtf_jepa_mae_vicreg.srr4_sparse_surface_contract_repair.v1"
        status = "complete"
        "final.decision" = "sparse_structured_repair_qualified"
        "quality_gate.pass" = "true"
        "authorization.fresh_srr3_stage_a" = "true"
        "rollback.policy" = "all_tokens"
        active_policy_changed = "false"
        augmentation_attribution_started = "false"
        final_holdout_opened = "false"
        endpoint_evaluator_replay_byte_exact = "true"
    }).GetEnumerator()) {
        Expect-Kv $Completion $Pair.Key ([string]$Pair.Value)
    }
}

function Admit-Srr4BoundedHeadEvidence {
    param([string]$Output)

    # These feature files are authenticated only on the authorized branch. No
    # encoder or ridge evaluator is called by SRR-3R.
    Require-FrozenFile $Srr4DevelopmentBaseline $Expected.Srr4DevelopmentBaselineSize $Expected.Srr4DevelopmentBaseline
    Require-FrozenFile $Srr4DevelopmentCandidate $Expected.Srr4DevelopmentCandidateSize $Expected.Srr4DevelopmentCandidate
    Require-FrozenFile $Srr4ConfirmationBaseline $Expected.StageABaselineSize $Expected.StageABaseline
    Require-FrozenFile $Srr4ConfirmationCandidate $Expected.StageACandidateSize $Expected.StageACandidate
    Require-FrozenFile $Srr4Report $Expected.Srr4ReportSize $Expected.Srr4Report

    $Srr4 = Read-Kv $Srr4Report
    foreach ($Pair in ([ordered]@{
        schema = "wikimyei.mtf_jepa_mae_vicreg.sparse_surface_structured_readout_contract_repair_evaluator.v1"
        protocol_sha256 = $Expected.Srr4Protocol
        status = "completed"
        "mechanics.feature_probe_boundary.pass" = "true"
        "mechanics.capture_contract_required_before_invocation" = "true"
        "mechanics.endpoint_metrics_inspected" = "true"
        "policy.baseline" = "all_tokens"
        "policy.candidate" = "structured_cdsb_sparse_v1"
        "policy.active" = "all_tokens"
        "policy.rollback" = "all_tokens"
        "policy.activation_changed" = "false"
        "execution.device" = "cpu"
        "execution.dtype" = "float64"
        "execution.optimizer_steps" = "0"
        "execution.backward_calls" = "0"
        "execution.rng_used_for_fit" = "false"
        "execution.final_holdout_opened" = "false"
        "execution.final_holdout_anchor_range" = "[1088,1170)"
        "split.selection_fit_anchor_range" = "[0,554)"
        "split.purge_anchor_range" = "[554,584)"
        "split.validation_anchor_range" = "[584,730)"
        "split.refit_anchor_range" = "[0,730)"
        "split.confirmation_anchor_range" = "[760,1088)"
        "pairing.development.identity_order_target.pass" = "true"
        "pairing.confirmation.identity_order_target.pass" = "true"
        "pairing.feature_width_equal" = "true"
        "pairing.full_confirmation_row_coverage" = "true"
        "compute.equal_selection.pass" = "true"
        "compute.equal_selected_refit.pass" = "true"
        "compute.equal_common_alpha_refit.pass" = "true"
        "compute.expected_selection_solves_per_arm" = "36"
        "compute.expected_selected_refit_solves_per_arm" = "3"
        "compute.expected_common_alpha_refit_solves_per_arm" = "3"
        "bootstrap.resamples" = "4096"
        "bootstrap.seed" = "8387496322364763509"
        "bootstrap.rng" = "numpy.PCG64"
        "bootstrap.unit" = "anchor_cluster"
        "gate.direction_delta_lower.minimum" = "-0.01"
        "gate.direction_delta_lower.pass" = "true"
        "gate.rank_delta_lower.minimum" = "-0.01"
        "gate.rank_delta_lower.pass" = "true"
        "gate.rmse_ratio_upper.maximum" = "1.05"
        "gate.rmse_ratio_upper.pass" = "true"
        "gate.noninferiority.pass" = "true"
        "material.required_count" = "2"
        "material.actual_count" = "3"
        "material.pass" = "true"
        "quality_gate.pass" = "true"
        "authorization.fresh_srr3_stage_a" = "true"
        "authorization.augmentation_attribution" = "false"
        "final.decision" = "sparse_structured_repair_qualified"
    }).GetEnumerator()) {
        Expect-Kv $Srr4 $Pair.Key ([string]$Pair.Value)
    }

    $ConditionalInputs = @(
        $Srr4Protocol,
        $Srr4Completion,
        $Srr4Report,
        $Srr4DevelopmentBaseline,
        $Srr4DevelopmentCandidate,
        $Srr4ConfirmationBaseline,
        $Srr4ConfirmationCandidate
    )
    $ConditionalManifest = Join-Path $RuntimeRoot "conditional_stage_b_authority.sha256"
    Write-HashManifest $ConditionalInputs $ConditionalManifest
    Assert-HashManifest $ConditionalManifest

    [IO.File]::WriteAllLines($Output, @(
        "schema_id=$SchemaId.conditional_stage_b_admission.v1",
        "status=admitted",
        "stage_a.required_classification=frozen_head_incompatible",
        "stage_a.actual_classification=frozen_head_incompatible",
        "evidence.kind=existing_srr4_bounded_equal_compute_ridge",
        "evidence.fresh=false",
        "evidence.independent_confirmation=false",
        "evidence.report=$Srr4Report",
        "evidence.report_sha256=$($Expected.Srr4Report)",
        "evidence.report_hash_match=true",
        "evidence.quality_gate.pass=true",
        "evidence.encoder_calls=0",
        "evidence.head_fits=0",
        "evidence.bootstrap_recomputed=false",
        "evidence.endpoint_metrics_recomputed=false",
        "production_mdn_trained=false",
        "production_mdn_checkpoint_written=false",
        "stage_b.pass=true",
        "final.decision=activation_requires_versioned_head_checkpoint_migration"
    ), $Utf8NoBom)
    return $ConditionalManifest
}

$RequiredPaths = @(
    $CaptureSource, $Evaluator, $ImportedEvaluator, $Protocol, $ProtocolPin,
    $Srr4Protocol, $Runner, $ProductionHeader, $ProductionParser,
    $FocusedMechanicsTest, $LegacyProductionTest,
    $LauncherIntegrationTest, $GraphSpecIntegrationTest,
    $CaptureMakefile, $FocusedMakefile, $LauncherMakefile, $GraphSpecMakefile,
    $FrozenBaseConfig, $Config, $ActiveDsl,
    $RepresentationCheckpoint, $MdnCheckpoint,
    $Srr4Completion, $Srr4Report, $PythonBin
)
foreach ($Path in $RequiredPaths) { Require-File $Path }

Require-FrozenFile $Protocol $Expected.ProtocolSize $Expected.Protocol
Require-FrozenFile $FrozenBaseConfig $Expected.FrozenBaseConfigSize $Expected.FrozenBaseConfig
Require-FrozenFile $Config $Expected.ConfigSize $Expected.Config
Require-FrozenFile $ActiveDsl $Expected.ActiveDslSize $Expected.ActiveDsl
Require-Sha256 $ProductionHeader $Expected.ProductionHeader
Require-Sha256 $ProductionParser $Expected.ProductionParser
Require-FrozenFile $RepresentationCheckpoint $Expected.RepresentationCheckpointSize $Expected.RepresentationCheckpoint
Require-FrozenFile $MdnCheckpoint $Expected.MdnCheckpointSize $Expected.MdnCheckpoint
Require-FrozenFile $Srr4Protocol $Expected.Srr4ProtocolSize $Expected.Srr4Protocol
Require-FrozenFile $Srr4Completion $Expected.Srr4CompletionSize $Expected.Srr4Completion
Require-FrozenFile $Srr4Report $Expected.Srr4ReportSize $Expected.Srr4Report
Require-FrozenFile $Evaluator $Expected.EvaluatorSize $Expected.Evaluator
Require-Sha256 $ImportedEvaluator $Expected.ImportedEvaluator

$ProtocolPinLine = [IO.File]::ReadAllText($ProtocolPin).Trim()
if ($ProtocolPinLine -ne "$($Expected.Protocol)  SPARSE_STRUCTURED_READOUT_ACTIVATION_COMPATIBILITY_PROTOCOL.md") {
    throw "protocol sidecar does not exactly pin the sealed SRR-3R protocol"
}
Validate-Srr4Authorization

$NormalizedBase = [IO.File]::ReadAllText($FrozenBaseConfig).Replace("`r`n", "`n").Replace(
    "runtime_wave_id = policy_training_ppo_v0",
    "runtime_wave_id = cwu_02v_certified_replay_eval_mdn"
)
$DerivedConfigText = [IO.File]::ReadAllText($Config).Replace("`r`n", "`n")
if ($NormalizedBase -ne $DerivedConfigText) {
    throw "derived SRR config differs from the frozen base beyond runtime_wave_id"
}
$PolicyLines = @(Select-String -LiteralPath $ActiveDsl -Pattern '^\s*SERVING_POOL_POLICY\s*=')
if ($PolicyLines.Count -ne 1 -or
    $PolicyLines[0].Line -notmatch '^\s*SERVING_POOL_POLICY\s*=\s*all_tokens\s*;\s*$') {
    throw "active serving policy is not exactly all_tokens"
}
$RuntimePrefix = $RepoRoot + [IO.Path]::DirectorySeparatorChar
if (-not $RuntimeRoot.StartsWith($RuntimePrefix, [StringComparison]::OrdinalIgnoreCase)) {
    throw "runtime root must stay inside the repository mount: $RuntimeRoot"
}
if (Test-Path -LiteralPath $RuntimeRoot) {
    throw "authoritative runtime root already exists: $RuntimeRoot"
}

Invoke-Checked $PythonBin @($Evaluator, "--self-test") -Quiet
Invoke-Checked "docker" @(
    "exec", $Container, "make", "-C",
    "/cuwacunu/src/tests/bench/wikimyei/representation/encoding/mtf_jepa_mae_vicreg",
    "-j2", "/cuwacunu/.build/tests/test_structured_cdsb_sparse_v1"
) -Quiet
Invoke-Checked "docker" @(
    "exec", $Container, "make", "-C",
    "/cuwacunu/src/tests/bench/wikimyei/representation/encoding/mtf_jepa_mae_vicreg",
    "-j2", "/cuwacunu/.build/tests/test_production_structured_readout"
) -Quiet
Invoke-Checked "docker" @(
    "exec", $Container, "make", "-C",
    "/cuwacunu/src/tests/bench/wikimyei/config/graph_first_specs",
    "-j2", "/cuwacunu/.build/tests/test_wikimyei_graph_first_specs"
) -Quiet
Invoke-Checked "docker" @(
    "exec", $Container, "make", "-C",
    "/cuwacunu/src/tests/bench/jkimyei/training/channel_graph_first_launchers",
    "-j2", "/cuwacunu/.build/tests/test_jkimyei_channel_graph_first_launchers"
) -Quiet

$FocusedMechanicsOutput = Invoke-CapturedChecked "docker" @(
    "exec", $Container, "/cuwacunu/.build/tests/test_structured_cdsb_sparse_v1"
)
if ($FocusedMechanicsOutput -notmatch '(?m)^cuda_float32_cases=passed\s*$' -or
    $FocusedMechanicsOutput -notmatch '(?m)^signed_zero_byte_canary=passed\s*$' -or
    $FocusedMechanicsOutput -notmatch '(?m)^complete_v1_bytes_exact=true\s*$') {
    throw "focused sparse mechanics did not prove CUDA/raw-byte/v1 contracts"
}
Invoke-Checked "docker" @(
    "exec", $Container, "/cuwacunu/.build/tests/test_production_structured_readout"
) -Quiet
Invoke-Checked "docker" @(
    "exec", $Container, "/cuwacunu/.build/tests/test_wikimyei_graph_first_specs"
) -Quiet
Invoke-Checked "docker" @(
    "exec", $Container, "/cuwacunu/.build/tests/test_jkimyei_channel_graph_first_launchers"
) -Quiet
Invoke-Checked "docker" @(
    "exec", $Container, "make", "-C", "/cuwacunu/src/main/exec",
    "build-cuwacunu-srr3r-sparse-activation-capture", "PARALLEL_JOBS=4"
)
foreach ($Path in @(
    $FocusedMechanicsBinary, $LegacyProductionBinary,
    $GraphSpecIntegrationBinary, $LauncherIntegrationBinary, $CaptureBinary
)) { Require-File $Path }

[IO.Directory]::CreateDirectory($RuntimeRoot) | Out-Null
$MechanicsGateReceipt = Join-Path $RuntimeRoot "pre_capture_mechanics.receipt"
[IO.File]::WriteAllLines($MechanicsGateReceipt, @(
    "schema_id=$SchemaId.pre_capture_mechanics.v1",
    "status=pass",
    "focused_sparse_mechanics=pass",
    "focused_cuda_float32=pass",
    "signed_zero_byte_canary=pass",
    "complete_v1_bytes_exact=pass",
    "legacy_policy_goldens=pass",
    "graph_spec_policy_fingerprint=pass",
    "downstream_adapter_checkpoint_identity=pass",
    "endpoint_capture_started_during_tests=false",
    "training_or_augmentation_used=false"
), $Utf8NoBom)

$AuthorityPaths = @(
    $CaptureSource, $CaptureBinary, $Evaluator, $ImportedEvaluator, $Runner,
    $Protocol, $ProtocolPin, $Srr4Protocol, $Srr4Completion, $Srr4Report,
    $ProductionHeader, $ProductionParser,
    $CaptureMakefile, $FocusedMakefile, $LauncherMakefile, $GraphSpecMakefile,
    $FocusedMechanicsTest, $FocusedMechanicsBinary,
    $LegacyProductionTest, $LegacyProductionBinary,
    $LauncherIntegrationTest, $LauncherIntegrationBinary,
    $GraphSpecIntegrationTest, $GraphSpecIntegrationBinary,
    $FrozenBaseConfig, $Config, $ActiveDsl,
    $RepresentationCheckpoint, $MdnCheckpoint
)
$PreCaptureAuthority = Join-Path $RuntimeRoot "pre_capture_authority.sha256"
$PreCaptureIncludeTree = Join-Path $RuntimeRoot "pre_capture_include_tree.sha256"
$PreCaptureConfigTree = Join-Path $RuntimeRoot "pre_capture_config_tree.sha256"
Write-HashManifest $AuthorityPaths $PreCaptureAuthority
Write-HashManifest @(
    Get-ChildItem -LiteralPath (Join-Path $RepoRoot "src/include") -File -Recurse |
        ForEach-Object FullName
) $PreCaptureIncludeTree
Write-HashManifest @(
    Get-ChildItem -LiteralPath (Join-Path $RepoRoot "src/config") -File -Recurse |
        Where-Object FullName -NotMatch '[\\/]artifacts[\\/]' |
        ForEach-Object FullName
) $PreCaptureConfigTree

$StageA = Join-Path $RuntimeRoot "stage_a_760_1088"
Invoke-StageACapture $StageA
$StageAOutputs = Validate-StageACapture $StageA
$StageAManifest = Join-Path $StageA "capture_outputs.sha256"
Write-HashManifest $StageAOutputs $StageAManifest

Assert-HashManifest $PreCaptureAuthority
Assert-HashManifest $PreCaptureIncludeTree
Assert-HashManifest $PreCaptureConfigTree
Assert-HashManifest $StageAManifest

$PreEndpointAuthority = Join-Path $RuntimeRoot "pre_endpoint_authority.sha256"
$EndpointInputs = $AuthorityPaths + $StageAOutputs + @(
    $MechanicsGateReceipt,
    $PreCaptureAuthority,
    $PreCaptureIncludeTree,
    $PreCaptureConfigTree,
    $StageAManifest
)
Write-HashManifest $EndpointInputs $PreEndpointAuthority
Assert-HashManifest $PreEndpointAuthority

$StageAReport = Join-Path $RuntimeRoot "stage_a.report"
$StageAReplay = Join-Path $RuntimeRoot "stage_a.replay.report"
$EvaluatorArguments = @(
    "--baseline-predictions", (Join-Path $StageA "all_tokens.predictions.csv"),
    "--candidate-predictions", (Join-Path $StageA "structured_cdsb_sparse_v1.predictions.csv"),
    "--baseline-eval-probe", (Join-Path $StageA "all_tokens.features.csv"),
    "--candidate-eval-probe", (Join-Path $StageA "structured_cdsb_sparse_v1.features.csv")
)
Invoke-Checked $PythonBin (@($Evaluator) + $EvaluatorArguments + @("--output", $StageAReport)) -Quiet
Assert-HashManifest $PreEndpointAuthority
Assert-HashManifest $PreCaptureIncludeTree
Assert-HashManifest $PreCaptureConfigTree
Assert-HashManifest $StageAManifest
Invoke-Checked $PythonBin (@($Evaluator) + $EvaluatorArguments + @("--output", $StageAReplay)) -Quiet
Assert-HashManifest $PreEndpointAuthority
Assert-HashManifest $PreCaptureIncludeTree
Assert-HashManifest $PreCaptureConfigTree
Assert-HashManifest $StageAManifest
Require-ByteExact $StageAReport $StageAReplay

$StageAValues = Read-Kv $StageAReport
$StageAReceiptValues = Read-Kv (Join-Path $StageA "mechanics.receipt")
foreach ($Pair in ([ordered]@{
    schema = "wikimyei.mtf_jepa_mae_vicreg.sparse_structured_readout_activation_compatibility_evaluator.v1"
    protocol_sha256 = $Expected.Protocol
    protocol_size_bytes = $Expected.ProtocolSize.ToString()
    status = "completed"
    scope = "stage_a_only"
    baseline_policy = "all_tokens"
    candidate_policy = "structured_cdsb_sparse_v1"
    active_policy = "all_tokens"
    "head.saved_checkpoint_policy" = "all_tokens"
    "head.counterfactual_computation" = "legacy_all_tokens_head_on_sparse_semantics"
    "reused_srr3.source_sha256" = $Expected.ImportedEvaluator
    "reused_srr3.contract_guard.pass" = "true"
    "mechanics.endpoint_metrics_inspected" = "true"
    "mechanics.development_inputs_supported" = "false"
    "mechanics.stage_b_evidence_opened" = "false"
    "stage_a.anchor_range" = "[760,1088)"
    "stage_a.anchor_count" = "328"
    "stage_a.expected_row_count" = "2952"
    "stage_a.source_batch_ceiling" = "6"
    "stage_a.bootstrap.resamples" = "4096"
    "stage_a.bootstrap.seed" = "8387496322364763509"
    "stage_a.bootstrap.cluster_unit" = "anchor"
    "stage_b.evidence.loaded_by_evaluator" = "false"
    "stage_b.evidence.admitted_by_evaluator" = "false"
    "stage_b.evidence.recomputed_by_evaluator" = "false"
    "stage_b.evidence.new_encoder_calls" = "0"
    "stage_b.evidence.new_head_fits" = "0"
    "final.recommendation.safe_direct_activation_authorized" = "false"
    "final.recommendation.policy_activation_authorized" = "false"
    "final.recommendation.rollback_policy" = "all_tokens"
}).GetEnumerator()) {
    Expect-Kv $StageAValues $Pair.Key ([string]$Pair.Value)
}
Expect-Kv $StageAValues "mechanics.pass" "true"
Expect-Kv $StageAValues "stage_a.source_batch_count" $StageAReceiptValues.source_batches
foreach ($Input in @(
    @("input.baseline_predictions", (Join-Path $StageA "all_tokens.predictions.csv"), $StageAReceiptValues.baseline_prediction_size, $StageAReceiptValues.baseline_prediction_sha256),
    @("input.candidate_predictions", (Join-Path $StageA "structured_cdsb_sparse_v1.predictions.csv"), $StageAReceiptValues.candidate_prediction_size, $StageAReceiptValues.candidate_prediction_sha256),
    @("input.baseline_eval_probe", (Join-Path $StageA "all_tokens.features.csv"), $Expected.StageABaselineSize.ToString(), $Expected.StageABaseline),
    @("input.candidate_eval_probe", (Join-Path $StageA "structured_cdsb_sparse_v1.features.csv"), $Expected.StageACandidateSize.ToString(), $Expected.StageACandidate)
)) {
    Expect-Kv $StageAValues "$($Input[0]).path" ([string]$Input[1])
    Expect-Kv $StageAValues "$($Input[0]).size_bytes" ([string]$Input[2])
    Expect-Kv $StageAValues "$($Input[0]).sha256" ([string]$Input[3])
    Expect-Kv $StageAValues "$($Input[0]).row_count" "2952"
    Expect-Kv $StageAValues "$($Input[0]).batch_count" $StageAReceiptValues.source_batches
}
$Classification = [string]$StageAValues["stage_a.classification"]
$StageBAdmitted = $false
$StageBPass = "not_applicable"
$MigrationKind = "none"
$FinalDecision = ""
$ConditionalAdmission = Join-Path $RuntimeRoot "conditional_stage_b_admission.receipt"

switch ($Classification) {
    "frozen_head_compatible_and_useful" {
        Expect-Kv $StageAValues "stage_a.compatibility.pass" "true"
        if ([int]$StageAValues["stage_a.material.count"] -lt 1) {
            throw "useful Stage A classification has no material flag"
        }
        Expect-Kv $StageAValues "stage_b.status" "not_authorized"
        Expect-Kv $StageAValues "stage_b.evidence.eligible_for_runner_admission" "false"
        Expect-Kv $StageAValues "final.recommendation.status" "complete"
        Expect-Kv $StageAValues "final.recommendation.decision" "activation_requires_versioned_head_checkpoint_migration"
        $MigrationKind = "versioned_identity_migration_with_frozen_tensor_bytes"
        $FinalDecision = "activation_requires_versioned_head_checkpoint_migration"
    }
    "compatible_no_downstream_gain" {
        Expect-Kv $StageAValues "stage_a.compatibility.pass" "true"
        Expect-Kv $StageAValues "stage_a.material.count" "0"
        Expect-Kv $StageAValues "stage_b.status" "not_authorized"
        Expect-Kv $StageAValues "stage_b.evidence.eligible_for_runner_admission" "false"
        Expect-Kv $StageAValues "final.recommendation.status" "complete"
        Expect-Kv $StageAValues "final.recommendation.decision" "downstream_bottleneck_remains_unresolved"
        $FinalDecision = "downstream_bottleneck_remains_unresolved"
    }
    "frozen_head_incompatible" {
        Expect-Kv $StageAValues "stage_a.compatibility.pass" "false"
        Expect-Kv $StageAValues "stage_b.status" "eligible_for_authenticated_srr4_admission"
        Expect-Kv $StageAValues "stage_b.evidence.eligible_for_runner_admission" "true"
        Expect-Kv $StageAValues "stage_b.evidence.loaded_by_evaluator" "false"
        Expect-Kv $StageAValues "stage_b.evidence.admitted_by_evaluator" "false"
        Expect-Kv $StageAValues "stage_b.evidence.recomputed_by_evaluator" "false"
        Expect-Kv $StageAValues "stage_b.evidence.required_report_size_bytes" $Expected.Srr4ReportSize.ToString()
        Expect-Kv $StageAValues "stage_b.evidence.required_report_sha256" $Expected.Srr4Report
        Expect-Kv $StageAValues "stage_b.evidence.required_decision" "sparse_structured_repair_qualified"
        Expect-Kv $StageAValues "final.recommendation.status" "pending_authenticated_srr4_bounded_head_evidence"
        Expect-Kv $StageAValues "final.recommendation.decision" "not_emitted"
        Expect-Kv $StageAValues "final.recommendation.if_stage_b_pass" "activation_requires_versioned_head_checkpoint_migration"
        Expect-Kv $StageAValues "final.recommendation.if_stage_b_fail" "downstream_bottleneck_remains_unresolved"
        $ConditionalManifest = Admit-Srr4BoundedHeadEvidence $ConditionalAdmission
        Assert-HashManifest $ConditionalManifest
        $StageBAdmitted = $true
        $StageBPass = "true"
        $MigrationKind = "fresh_or_adapted_versioned_production_head_required"
        $FinalDecision = "activation_requires_versioned_head_checkpoint_migration"
    }
    "invalid" {
        throw "Stage A evaluator classified the experiment invalid"
    }
    default {
        throw "unexpected Stage A classification: $Classification"
    }
}

if ($FinalDecision -notin @(
    "activation_requires_versioned_head_checkpoint_migration",
    "downstream_bottleneck_remains_unresolved"
)) {
    throw "unexpected canonical final decision: $FinalDecision"
}

Assert-HashManifest $PreEndpointAuthority
Assert-HashManifest $PreCaptureIncludeTree
Assert-HashManifest $PreCaptureConfigTree
Assert-HashManifest $StageAManifest
if ($StageBAdmitted) {
    Assert-HashManifest $ConditionalManifest
}

$CompletionPath = Join-Path $RuntimeRoot "completion.receipt"
[IO.File]::WriteAllLines($CompletionPath, @(
    "schema_id=$SchemaId",
    "status=complete",
    "stage_a.classification=$Classification",
    "stage_a.report=$StageAReport",
    "stage_a.evaluator_replay_byte_exact=true",
    "stage_b.status=$(if ($StageBAdmitted) { 'authenticated_existing_evidence_admitted' } else { 'not_authorized' })",
    "stage_b.evidence.admitted=$($StageBAdmitted.ToString().ToLowerInvariant())",
    "stage_b.evidence.fresh=false",
    "stage_b.evidence.recomputed=false",
    "stage_b.encoder_calls=0",
    "stage_b.head_fits=0",
    "stage_b.pass=$StageBPass",
    "stage_b.evidence.report_sha256=$(if ($StageBAdmitted) { $Expected.Srr4Report } else { 'not_admitted' })",
    "historical_checkpoint.saved_policy=all_tokens",
    "historical_checkpoint.successful_weight_loads=1",
    "historical_checkpoint.sparse_identity_accepted=false",
    "historical_checkpoint.sparse_identity_rejected=true",
    "historical_checkpoint.identity_bypass_used=false",
    "candidate.computation=legacy_all_tokens_head_on_sparse_semantics",
    "safe_direct_activation.eligible=false",
    "migration.kind=$MigrationKind",
    "migration.checkpoint_written=false",
    "production_mdn_trained=false",
    "final.report=$StageAReport",
    "final.decision=$FinalDecision",
    "rollback.policy=all_tokens",
    "active_policy_changed=false",
    "augmentation_attribution_started=false",
    "final_holdout_opened=false",
    "optimizer_steps=0",
    "backward_calls=0",
    "checkpoint_writes=0"
), $Utf8NoBom)

Assert-HashManifest $PreEndpointAuthority
Assert-HashManifest $PreCaptureIncludeTree
Assert-HashManifest $PreCaptureConfigTree
Assert-HashManifest $StageAManifest
if ($StageBAdmitted) {
    Assert-HashManifest $ConditionalManifest
}

$FinalFiles = Get-ChildItem -LiteralPath $RuntimeRoot -File -Recurse |
    Where-Object Name -ne "final_outputs.sha256" |
    ForEach-Object FullName
$FinalOutputsManifest = Join-Path $RuntimeRoot "final_outputs.sha256"
Write-HashManifest $FinalFiles $FinalOutputsManifest
Assert-HashManifest $FinalOutputsManifest

Write-Output "srr3r.status=complete"
Write-Output "srr3r.stage_a.classification=$Classification"
Write-Output "srr3r.stage_b.evidence.admitted=$($StageBAdmitted.ToString().ToLowerInvariant())"
Write-Output "srr3r.final.decision=$FinalDecision"
Write-Output "srr3r.runtime_root=$RuntimeRoot"
