param(
    [string]$Container = "unnamed_taoist",
    [string]$RuntimeRoot = "",
    [string]$PythonBin = ""
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

$ScriptDir = [IO.Path]::GetFullPath($PSScriptRoot)
$RepoRoot = [IO.Path]::GetFullPath(
    (Join-Path $ScriptDir "../../../../../../../")
).TrimEnd([IO.Path]::DirectorySeparatorChar)
$SchemaId = "wikimyei.mtf_jepa_mae_vicreg.srr3_activation_compatibility.v1"

if ([string]::IsNullOrWhiteSpace($RuntimeRoot)) {
    $RuntimeRoot = Join-Path $RepoRoot ".runtime/benchmarks/synthetic_continuous_graph_v1/$SchemaId.attempt_000002"
}
$RuntimeRoot = [IO.Path]::GetFullPath($RuntimeRoot)
if ([string]::IsNullOrWhiteSpace($PythonBin)) {
    if (-not [string]::IsNullOrWhiteSpace($env:SRR3_PYTHON_BIN)) {
        $PythonBin = $env:SRR3_PYTHON_BIN
    } else {
        $PythonBin = Join-Path $env:USERPROFILE ".cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe"
    }
}

$CaptureSource = Join-Path $RepoRoot "src/main/exec/cuwacunu_srr3_dual_readout_capture.cpp"
$CaptureBinary = Join-Path $RepoRoot ".build/exec/cuwacunu_srr3_dual_readout_capture"
$Evaluator = Join-Path $ScriptDir "evaluate_structured_readout_activation_compatibility.py"
$Protocol = Join-Path $ScriptDir "STRUCTURED_READOUT_ACTIVATION_COMPATIBILITY_PROTOCOL.md"
$ProtocolPin = Join-Path $ScriptDir "STRUCTURED_READOUT_ACTIVATION_COMPATIBILITY_PROTOCOL.sha256"
$Amendment = Join-Path $ScriptDir "STRUCTURED_READOUT_ACTIVATION_COMPATIBILITY_PROTOCOL_AMENDMENT_A1.md"
$AmendmentPin = Join-Path $ScriptDir "STRUCTURED_READOUT_ACTIVATION_COMPATIBILITY_PROTOCOL_AMENDMENT_A1.sha256"
$Runner = $MyInvocation.MyCommand.Path
$FrozenBaseConfig = Join-Path $RepoRoot "src/config/benchmarks/synthetic_continuous_graph_v1/synthetic_benchmark.config"
$BaseConfig = Join-Path $RepoRoot "src/config/benchmarks/synthetic_continuous_graph_v1/srr3_activation_compatibility.config"
$ActiveDsl = Join-Path $RepoRoot "src/config/wikimyei.representation.mtf_jepa_mae_vicreg.dsl"
$RepresentationCheckpoint = Join-Path $RepoRoot ".runtime/cuwacunu_exec/components/wikimyei.representation.encoding.mtf_jepa_mae_vicreg/spawns/diz_b8a87dee0c986487/jobs/train/train_core_mtf_jepa_mae_vicreg.train.channel_representation_mtf_jepa_mae_vicreg.attempt_000001/channel_representation.report.mtf_jepa_mae_vicreg.pt"
$MdnCheckpoint = Join-Path $RepoRoot ".runtime/cuwacunu_exec/components/wikimyei.inference.expected_value.mdn/spawns/syq_fd0cba7ed6f1feb8/jobs/train/train_core_channel_mdn.train.channel_inference_mdn.attempt_000001/channel_inference.report.channel_mdn.pt"
$HistoricalBaselineProbe = Join-Path $RepoRoot ".runtime/benchmarks/synthetic_continuous_graph_v1/synthetic_mdn_frozen_feature_capture.historical_760_1088.v1/anchor_760_1088/representation_edge_features.probe"
$DevelopmentBaselineProbe = Join-Path $RepoRoot ".runtime/benchmarks/synthetic_continuous_graph_v1/synthetic_mdn_frozen_feature_capture.v1/anchor_0_730/representation_edge_features.probe"

$Expected = @{
    Protocol = "1c24e92a49bb59b0f0a7db63917428399619a0783216f6f3c9049c5a46cbace3"
    Amendment = "68d4a96394faad9e8da736bf41a240f54474f92480056072c3a4d5456f4e5b4c"
    FrozenBaseConfig = "7c84cee94ecf839336c0383878298981b9ab362e80a570cefef20e9fed272fd6"
    Config = "23d94f9222527fcc14cbc3948861b42e06b0f55c992f8e3e01b8ebc1bd8149e0"
    Representation = "8a43cc9275954fa03dbd1d140fa74bd1c71f57d8941fe005ec258c8956b5c9de"
    Mdn = "eb5643b752994f4c3b1cc21202f1fec1a82bc3240ab578b5cf18127010155d8e"
    HistoricalBaseline = "8f6f72b78c0708b5f23512ada4ca8536ea8818f8e2d3d9bc501401d8ab0ce3c7"
    DevelopmentBaseline = "d3465e44ed15647e158b9cabf00f4b1670797fdddc5539c0dd4a067db7b193ed"
}

$Utf8NoBom = [Text.UTF8Encoding]::new($false)

function Invoke-Checked {
    param([string]$File, [string[]]$Arguments)
    & $File @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "command failed with exit code $LASTEXITCODE`: $File $($Arguments -join ' ')"
    }
}

function Require-File {
    param([string]$Path)
    $Item = Get-Item -LiteralPath $Path -Force
    if ($Item.PSIsContainer -or $null -ne $Item.LinkType) {
        throw "not a regular non-link file: $Path"
    }
}

function Get-Sha256 {
    param([string]$Path)
    return (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToLowerInvariant()
}

function Require-Sha256 {
    param([string]$Path, [string]$ExpectedHash)
    Require-File $Path
    $Actual = Get-Sha256 $Path
    if ($Actual -ne $ExpectedHash) {
        throw "SHA-256 mismatch for $Path`: expected $ExpectedHash, got $Actual"
    }
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
    $Values = [ordered]@{}
    foreach ($Line in [IO.File]::ReadAllLines($Path)) {
        $Index = $Line.IndexOf('=')
        if ($Index -le 0) { continue }
        $Key = $Line.Substring(0, $Index)
        if ($Values.Contains($Key)) { throw "duplicate key $Key in $Path" }
        $Values[$Key] = $Line.Substring($Index + 1)
    }
    return $Values
}

function Expect-Kv {
    param([hashtable]$Values, [string]$Key, [string]$ExpectedValue)
    if (-not $Values.Contains($Key) -or $Values[$Key] -ne $ExpectedValue) {
        $Actual = if ($Values.Contains($Key)) { $Values[$Key] } else { "<missing>" }
        throw "expected $Key=$ExpectedValue, got $Actual"
    }
}

function Require-Rows {
    param([string]$Path, [int]$ExpectedRows)
    Require-File $Path
    $Count = ([IO.File]::ReadLines($Path) | Measure-Object).Count - 1
    if ($Count -ne $ExpectedRows) {
        throw "expected $ExpectedRows rows in $Path, got $Count"
    }
}

function Write-HashManifest {
    param([string[]]$Paths, [string]$Output)
    $Lines = foreach ($Path in ($Paths | Sort-Object)) {
        $Relative = [IO.Path]::GetRelativePath($RepoRoot, $Path).Replace('\', '/')
        "$(Get-Sha256 $Path)  $Relative"
    }
    [IO.File]::WriteAllLines($Output, $Lines, $Utf8NoBom)
}

function Invoke-Capture {
    param([int]$Begin, [int]$End, [string]$OutputDirectory)
    $Arguments = @(
        "exec", "-e", "CUBLAS_WORKSPACE_CONFIG=:4096:8", $Container,
        (To-LinuxPath $CaptureBinary),
        "--config", (To-LinuxPath $BaseConfig),
        "--input-representation-checkpoint", (To-LinuxPath $RepresentationCheckpoint),
        "--input-mdn-checkpoint", (To-LinuxPath $MdnCheckpoint),
        "--output-dir", (To-LinuxPath $OutputDirectory),
        "--anchor-index-begin", $Begin.ToString(),
        "--anchor-index-end", $End.ToString()
    )
    Invoke-Checked "docker" $Arguments
}

function Validate-Capture {
    param(
        [string]$Directory,
        [string]$Range,
        [int]$Rows,
        [string]$ExpectedBaselineHash,
        [bool]$StageA
    )
    $ReceiptPath = Join-Path $Directory "mechanics.receipt"
    Require-File $ReceiptPath
    $Receipt = Read-Kv $ReceiptPath
    $Checks = [ordered]@{
        status = "mechanics_pass"
        anchor_range = $Range
        final_holdout_access = "false"
        active_production_policy = "all_tokens"
        baseline_policy = "all_tokens"
        candidate_policy = "structured_cdsb_v1"
        encoder_calls_equal_source_batches = "true"
        same_encoded_object = "true"
        pool_contract_exact = "true"
        pool_masks_exact = "true"
        invalid_zero_exact = "true"
        paired_adapter_contract_exact = "true"
        all_outputs_finite = "true"
        representation_parameters_unchanged = "true"
        representation_buffers_unchanged = "true"
        cpu_rng_unchanged = "true"
        cuda_rng_unchanged = "true"
        representation_eval_unchanged = "true"
        augmentations_enabled = "false"
        optimizer_steps = "0"
        backward_calls = "0"
        checkpoint_writes = "0"
        endpoint_metrics_computed = "false"
    }
    foreach ($Pair in $Checks.GetEnumerator()) {
        Expect-Kv $Receipt $Pair.Key $Pair.Value
    }
    $BaselineFeatures = Join-Path $Directory "all_tokens.features.csv"
    $CandidateFeatures = Join-Path $Directory "structured_cdsb_v1.features.csv"
    Require-Rows $BaselineFeatures $Rows
    Require-Rows $CandidateFeatures $Rows
    Require-Sha256 $BaselineFeatures $ExpectedBaselineHash
    if ($StageA) {
        foreach ($Pair in ([ordered]@{
            sigma_finite = "true"
            mdn_parameters_unchanged = "true"
            mdn_buffers_unchanged = "true"
            mdn_eval_unchanged = "true"
            all_tokens_identity_load_pass = "true"
            structured_identity_rejected = "true"
            structured_identity_attempt_model_unchanged = "true"
        }).GetEnumerator()) {
            Expect-Kv $Receipt $Pair.Key $Pair.Value
        }
        if ([int]$Receipt.encoder_calls -gt 6) { throw "Stage A exceeded six encoder calls" }
        Require-Rows (Join-Path $Directory "all_tokens.predictions.csv") $Rows
        Require-Rows (Join-Path $Directory "structured_cdsb_v1.predictions.csv") $Rows
    } else {
        Expect-Kv $Receipt "mdn_checkpoint_sha256" "not_accessed"
        Expect-Kv $Receipt "baseline_prediction_path" "not_emitted"
        Expect-Kv $Receipt "candidate_prediction_path" "not_emitted"
    }
}

foreach ($Path in @(
    $CaptureSource, $Evaluator, $Protocol, $ProtocolPin, $Amendment,
    $AmendmentPin, $Runner,
    $FrozenBaseConfig, $BaseConfig,
    $ActiveDsl, $RepresentationCheckpoint, $MdnCheckpoint,
    $HistoricalBaselineProbe, $DevelopmentBaselineProbe, $PythonBin
)) { Require-File $Path }
Require-Sha256 $Protocol $Expected.Protocol
Require-Sha256 $Amendment $Expected.Amendment
Require-Sha256 $FrozenBaseConfig $Expected.FrozenBaseConfig
Require-Sha256 $BaseConfig $Expected.Config
Require-Sha256 $RepresentationCheckpoint $Expected.Representation
Require-Sha256 $MdnCheckpoint $Expected.Mdn
Require-Sha256 $HistoricalBaselineProbe $Expected.HistoricalBaseline
Require-Sha256 $DevelopmentBaselineProbe $Expected.DevelopmentBaseline
$NormalizedBase = [IO.File]::ReadAllText($FrozenBaseConfig).Replace("`r`n", "`n").Replace(
    "runtime_wave_id = policy_training_ppo_v0",
    "runtime_wave_id = cwu_02v_certified_replay_eval_mdn"
)
$DerivedConfigText = [IO.File]::ReadAllText($BaseConfig).Replace("`r`n", "`n")
if ($NormalizedBase -ne $DerivedConfigText) {
    throw "derived SRR-3 config differs from the frozen base beyond runtime_wave_id"
}

$PolicyLines = @(
    Select-String -LiteralPath $ActiveDsl -Pattern '^\s*SERVING_POOL_POLICY\s*='
)
if ($PolicyLines.Count -ne 1 -or
    $PolicyLines[0].Line -notmatch '^\s*SERVING_POOL_POLICY\s*=\s*all_tokens\s*;\s*$') {
    throw "active serving policy is not exactly all_tokens"
}
if (Test-Path -LiteralPath $RuntimeRoot) {
    throw "authoritative runtime root already exists: $RuntimeRoot"
}

Invoke-Checked $PythonBin @($Evaluator, "--self-test")
Invoke-Checked "docker" @(
    "exec", $Container, "bash", "-lc",
    "cd /cuwacunu && make -C src/main/exec build-cuwacunu-srr3-dual-readout-capture PARALLEL_JOBS=4"
)
Require-File $CaptureBinary

[IO.Directory]::CreateDirectory($RuntimeRoot) | Out-Null
$AuthorityPaths = @(
    $CaptureSource, $CaptureBinary, $Evaluator, $Runner, $Protocol, $ProtocolPin,
    $Amendment, $AmendmentPin,
    $FrozenBaseConfig, $BaseConfig, $ActiveDsl, $RepresentationCheckpoint, $MdnCheckpoint,
    $HistoricalBaselineProbe, $DevelopmentBaselineProbe
)
Write-HashManifest $AuthorityPaths (Join-Path $RuntimeRoot "pre_metric_authority.sha256")
Write-HashManifest @(
    Get-ChildItem -LiteralPath (Join-Path $RepoRoot "src/include") -File -Recurse |
        ForEach-Object FullName
) (Join-Path $RuntimeRoot "pre_metric_include_tree.sha256")
Write-HashManifest @(
    Get-ChildItem -LiteralPath (Join-Path $RepoRoot "src/config") -File -Recurse |
        Where-Object FullName -NotMatch '[\\/]artifacts[\\/]' |
        ForEach-Object FullName
) (Join-Path $RuntimeRoot "pre_metric_config_tree.sha256")

$StageA = Join-Path $RuntimeRoot "stage_a"
Invoke-Capture 760 1088 $StageA
Validate-Capture $StageA "[760,1088)" 2952 $Expected.HistoricalBaseline $true
$StageAInputs = @(
    (Join-Path $StageA "mechanics.receipt"),
    (Join-Path $StageA "all_tokens.features.csv"),
    (Join-Path $StageA "structured_cdsb_v1.features.csv"),
    (Join-Path $StageA "all_tokens.predictions.csv"),
    (Join-Path $StageA "structured_cdsb_v1.predictions.csv")
)
Write-HashManifest $StageAInputs (Join-Path $StageA "capture_outputs.sha256")

$StageAReport = Join-Path $RuntimeRoot "stage_a.report"
$StageAReplay = Join-Path $RuntimeRoot "stage_a.replay.report"
$EvaluatorBaseArgs = @(
    "--baseline-predictions", (Join-Path $StageA "all_tokens.predictions.csv"),
    "--candidate-predictions", (Join-Path $StageA "structured_cdsb_v1.predictions.csv"),
    "--baseline-eval-probe", (Join-Path $StageA "all_tokens.features.csv"),
    "--candidate-eval-probe", (Join-Path $StageA "structured_cdsb_v1.features.csv")
)
Invoke-Checked $PythonBin ($EvaluatorBaseArgs + @("--output", $StageAReport))
Invoke-Checked $PythonBin ($EvaluatorBaseArgs + @("--output", $StageAReplay))
if ((Get-Sha256 $StageAReport) -ne (Get-Sha256 $StageAReplay)) {
    throw "Stage A evaluator replay is not byte exact"
}
$StageAValues = Read-Kv $StageAReport
Expect-Kv $StageAValues "mechanics.pass" "true"
$Classification = $StageAValues["stage_a.classification"]
$StageBExecuted = $false
$FinalReport = $StageAReport

switch ($Classification) {
    "frozen_head_compatible_and_useful" {
        $Decision = "activation_requires_versioned_head_checkpoint_migration"
    }
    "compatible_no_downstream_gain" {
        $Decision = "downstream_bottleneck_unresolved"
    }
    "frozen_head_incompatible" {
        $StageBExecuted = $true
        $Development = Join-Path $RuntimeRoot "stage_b_development"
        Invoke-Capture 0 730 $Development
        Validate-Capture $Development "[0,730)" 6570 $Expected.DevelopmentBaseline $false
        Write-HashManifest @(
            (Join-Path $Development "mechanics.receipt"),
            (Join-Path $Development "all_tokens.features.csv"),
            (Join-Path $Development "structured_cdsb_v1.features.csv")
        ) (Join-Path $Development "capture_outputs.sha256")
        $StageBReport = Join-Path $RuntimeRoot "stage_b.report"
        $StageBReplay = Join-Path $RuntimeRoot "stage_b.replay.report"
        $StageBArgs = $EvaluatorBaseArgs + @(
            "--baseline-dev-probe", (Join-Path $Development "all_tokens.features.csv"),
            "--candidate-dev-probe", (Join-Path $Development "structured_cdsb_v1.features.csv")
        )
        Invoke-Checked $PythonBin ($StageBArgs + @("--output", $StageBReport))
        Invoke-Checked $PythonBin ($StageBArgs + @("--output", $StageBReplay))
        if ((Get-Sha256 $StageBReport) -ne (Get-Sha256 $StageBReplay)) {
            throw "Stage B evaluator replay is not byte exact"
        }
        $FinalReport = $StageBReport
        $FinalValues = Read-Kv $FinalReport
        Expect-Kv $FinalValues "mechanics.pass" "true"
        $Decision = $FinalValues["final.decision"]
        if ($Decision -notin @(
            "activation_requires_versioned_head_checkpoint_migration",
            "downstream_bottleneck_unresolved"
        )) { throw "unexpected Stage B decision: $Decision" }
    }
    "invalid" { throw "Stage A evaluator classified the experiment invalid" }
    default { throw "unexpected Stage A classification: $Classification" }
}

$Completion = @(
    "schema_id=$SchemaId",
    "status=complete",
    "stage_a.classification=$Classification",
    "stage_b.executed=$($StageBExecuted.ToString().ToLowerInvariant())",
    "final.decision=$Decision",
    "final.report=$FinalReport",
    "rollback.policy=all_tokens",
    "active_policy_changed=false",
    "augmentation_attribution_started=false",
    "final_holdout_opened=false"
)
[IO.File]::WriteAllLines((Join-Path $RuntimeRoot "completion.receipt"), $Completion, $Utf8NoBom)
$FinalFiles = Get-ChildItem -LiteralPath $RuntimeRoot -File -Recurse |
    Where-Object Name -ne "final_outputs.sha256" |
    ForEach-Object FullName
Write-HashManifest $FinalFiles (Join-Path $RuntimeRoot "final_outputs.sha256")

Write-Output "srr3.status=complete"
Write-Output "srr3.stage_a.classification=$Classification"
Write-Output "srr3.stage_b.executed=$($StageBExecuted.ToString().ToLowerInvariant())"
Write-Output "srr3.final.decision=$Decision"
Write-Output "srr3.runtime_root=$RuntimeRoot"
