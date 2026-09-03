param(
    [string]$Container = "unnamed_taoist",
    [string]$RuntimeRoot = "",
    [string]$PythonBin = ""
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest
if ($PSVersionTable.PSVersion.Major -lt 7) {
    throw "SRR-4 custody requires PowerShell 7+ for the frozen long checkpoint path"
}

$ScriptDir = [IO.Path]::GetFullPath($PSScriptRoot)
$RepoRoot = [IO.Path]::GetFullPath(
    (Join-Path $ScriptDir "../../../../../../../")
).TrimEnd([IO.Path]::DirectorySeparatorChar)
$SchemaId = "wikimyei.mtf_jepa_mae_vicreg.srr4_sparse_surface_contract_repair.v1"

if ([string]::IsNullOrWhiteSpace($RuntimeRoot)) {
    $RuntimeRoot = Join-Path $RepoRoot ".runtime/benchmarks/synthetic_continuous_graph_v1/$SchemaId/attempt_000001"
}
$RuntimeRoot = [IO.Path]::GetFullPath($RuntimeRoot)
if ([string]::IsNullOrWhiteSpace($PythonBin)) {
    if (-not [string]::IsNullOrWhiteSpace($env:SRR4_PYTHON_BIN)) {
        $PythonBin = $env:SRR4_PYTHON_BIN
    } else {
        $PythonBin = Join-Path $env:USERPROFILE ".cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe"
    }
}
$PythonBin = [IO.Path]::GetFullPath($PythonBin)

$CaptureSource = Join-Path $RepoRoot "src/main/exec/cuwacunu_srr4_sparse_readout_capture.cpp"
$CaptureBinary = Join-Path $RepoRoot ".build/exec/cuwacunu_srr4_sparse_readout_capture"
$Evaluator = Join-Path $ScriptDir "evaluate_sparse_surface_structured_readout_contract_repair.py"
$ImportedEvaluator = Join-Path $ScriptDir "evaluate_structured_readout_activation_compatibility.py"
$Protocol = Join-Path $ScriptDir "SPARSE_SURFACE_STRUCTURED_READOUT_CONTRACT_REPAIR_PROTOCOL.md"
$ProtocolPin = Join-Path $ScriptDir "SPARSE_SURFACE_STRUCTURED_READOUT_CONTRACT_REPAIR_PROTOCOL.sha256"
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
$DevelopmentFrozenProbe = Join-Path $RepoRoot ".runtime/benchmarks/synthetic_continuous_graph_v1/synthetic_mdn_frozen_feature_capture.v1/anchor_0_730/representation_edge_features.probe"
$ConfirmationFrozenProbe = Join-Path $RepoRoot ".runtime/benchmarks/synthetic_continuous_graph_v1/synthetic_mdn_frozen_feature_capture.historical_760_1088.v1/anchor_760_1088/representation_edge_features.probe"

$Expected = @{
    Protocol = "a634a1ae386a5a0bebb10440cf0d45730e2dd652375aa3fb80a4d9277708ed30"
    FrozenBaseConfig = "7c84cee94ecf839336c0383878298981b9ab362e80a570cefef20e9fed272fd6"
    Config = "23d94f9222527fcc14cbc3948861b42e06b0f55c992f8e3e01b8ebc1bd8149e0"
    Representation = "8a43cc9275954fa03dbd1d140fa74bd1c71f57d8941fe005ec258c8956b5c9de"
    DevelopmentBaseline = "d3465e44ed15647e158b9cabf00f4b1670797fdddc5539c0dd4a067db7b193ed"
    ConfirmationBaseline = "8f6f72b78c0708b5f23512ada4ca8536ea8818f8e2d3d9bc501401d8ab0ce3c7"
}
$ExpectedCellCardinality = @(2, 3, 2, 1, 1, 1, 1, 1, 2, 3, 2, 1, 1, 1, 1, 1)
$ExpectedObservedByChannel = @(
    @(0, 0, 2, 0, 0, 1, 1, 1, 0, 0, 2, 0, 0, 1, 1, 1),
    @(0, 1, 2, 0, 1, 1, 1, 1, 0, 1, 2, 0, 1, 1, 1, 1),
    @(2, 3, 2, 1, 1, 1, 1, 1, 2, 3, 2, 1, 1, 1, 1, 1)
)
$ExpectedSourceTokens = @(10, 14, 24)
$ExpectedSupportedCells = @(8, 12, 16)
$ExpectedRepeatedSupport = @(10, 18, 24)
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
    Require-File $Path
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
    param([System.Collections.IDictionary]$Values, [string]$Key, [string]$ExpectedValue)
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
        throw "expected $ExpectedRows data rows in $Path, got $Count"
    }
}

function Write-HashManifest {
    param([string[]]$Paths, [string]$Output)
    $Lines = foreach ($Path in ($Paths | Sort-Object -Unique)) {
        Require-File $Path
        $Relative = [IO.Path]::GetRelativePath($RepoRoot, $Path).Replace('\', '/')
        "$(Get-Sha256 $Path)  $Relative"
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
        Require-Sha256 $Path $ExpectedHash
    }
}

function Invoke-Capture {
    param([int]$Begin, [int]$End, [string]$OutputDirectory)
    $Arguments = @(
        "exec", "-e", "CUBLAS_WORKSPACE_CONFIG=:4096:8", $Container,
        (To-LinuxPath $CaptureBinary),
        "--config", (To-LinuxPath $Config),
        "--input-representation-checkpoint", (To-LinuxPath $RepresentationCheckpoint),
        "--output-dir", (To-LinuxPath $OutputDirectory),
        "--anchor-index-begin", $Begin.ToString(),
        "--anchor-index-end", $End.ToString()
    )
    Invoke-Checked "docker" $Arguments
}

function Validate-Capture {
    param(
        [string]$Directory,
        [string]$RangeId,
        [string]$AnchorRange,
        [int]$Begin,
        [int]$AnchorCount,
        [int]$BatchCeiling,
        [string]$ExpectedBaselineHash,
        [int64]$ExpectedBaselineSize
    )
    $PublicRows = $AnchorCount * 4
    $FeatureRows = $AnchorCount * 9
    $MaskCells = $PublicRows * 3
    $ReceiptPath = Join-Path $Directory "mechanics.receipt"
    $BaselineProbe = Join-Path $Directory "all_tokens.representation_edge_features.probe"
    $CandidateProbe = Join-Path $Directory "structured_cdsb_sparse_v1.representation_edge_features.probe"
    $Receipt = Read-Kv $ReceiptPath
    $Checks = [ordered]@{
        schema_id = "cuwacunu.srr4.sparse_readout_capture.mechanics.v1"
        status = "mechanics_pass"
        sealed_protocol_sha256 = $Expected.Protocol
        range_id = $RangeId
        anchor_range = $AnchorRange
        anchor_count = $AnchorCount.ToString()
        maximum_anchor_read = (($Begin + $AnchorCount) - 1).ToString()
        final_holdout_range = "[1088,1170)"
        final_holdout_access = "false"
        seed = "31"
        source_order = "contiguous_sequential_anchor_index"
        active_production_policy = "all_tokens"
        baseline_policy = "all_tokens"
        candidate_policy = "structured_cdsb_sparse_v1"
        source_batch_ceiling = $BatchCeiling.ToString()
        encoder_calls_equal_source_batches = "true"
        same_retained_encoded_object = "true"
        encoded_bytes_stable = "true"
        public_rows = $PublicRows.ToString()
        public_mask_cells = $MaskCells.ToString()
        public_contract_exact = "true"
        public_masks_exact = "true"
        baseline_valid_cells = $MaskCells.ToString()
        candidate_valid_cells = $MaskCells.ToString()
        invalid_zero_exact = "true"
        outputs_finite = "true"
        paired_adapter_contract_exact = "true"
        paired_rows_keys_targets_order_exact = "true"
        input_data_unchanged = "true"
        input_mask_unchanged = "true"
        converted_data_bytes_stable = "true"
        converted_feature_mask_bytes_stable = "true"
        representation_parameters_unchanged = "true"
        representation_buffers_unchanged = "true"
        representation_eval_unchanged = "true"
        cpu_rng_unchanged = "true"
        cuda_rng_unchanged = "true"
        "support.expected_cell_cardinality_exact" = "true"
        "support.cell_support_semantics_exact" = "true"
        "support.repeated_support_semantics_exact" = "true"
        "support.domain_scale_coverage_exact" = "true"
        feature_count = "96"
        baseline_feature_rows = $FeatureRows.ToString()
        candidate_feature_rows = $FeatureRows.ToString()
        config_size = "4298"
        config_sha256 = $Expected.Config
        representation_checkpoint_size = "853867"
        representation_checkpoint_sha256 = $Expected.Representation
        baseline_feature_size = $ExpectedBaselineSize.ToString()
        baseline_feature_sha256 = $ExpectedBaselineHash
        baseline_frozen_hash_match = "true"
        mdn_checkpoint_access = "false"
        mdn_constructions = "0"
        mdn_forwards = "0"
        prediction_artifacts_written = "false"
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
        throw "capture graph-order fingerprint is empty"
    }
    if ([int]$Receipt.source_batches -gt $BatchCeiling -or
        [int]$Receipt.encoder_calls -ne [int]$Receipt.source_batches) {
        throw "capture exceeded encoder budget or call pairing"
    }
    for ($Channel = 0; $Channel -lt 3; ++$Channel) {
        $Prefix = "channel_$Channel."
        $CompleteRows = if ($Channel -eq 2) { $PublicRows } else { 0 }
        foreach ($Pair in ([ordered]@{
            "${Prefix}rows" = $PublicRows.ToString()
            "${Prefix}complete_rows" = $CompleteRows.ToString()
            "${Prefix}expected_source_token_count" = $ExpectedSourceTokens[$Channel].ToString()
            "${Prefix}source_token_count_min" = $ExpectedSourceTokens[$Channel].ToString()
            "${Prefix}source_token_count_max" = $ExpectedSourceTokens[$Channel].ToString()
            "${Prefix}expected_supported_cell_count" = $ExpectedSupportedCells[$Channel].ToString()
            "${Prefix}supported_cell_count_min" = $ExpectedSupportedCells[$Channel].ToString()
            "${Prefix}supported_cell_count_max" = $ExpectedSupportedCells[$Channel].ToString()
            "${Prefix}expected_repeated_support_position_count" = $ExpectedRepeatedSupport[$Channel].ToString()
            "${Prefix}repeated_support_position_count_min" = $ExpectedRepeatedSupport[$Channel].ToString()
            "${Prefix}repeated_support_position_count_max" = $ExpectedRepeatedSupport[$Channel].ToString()
        }).GetEnumerator()) {
            Expect-Kv $Receipt $Pair.Key ([string]$Pair.Value)
        }
        for ($Cell = 0; $Cell -lt 16; ++$Cell) {
            Expect-Kv $Receipt "${Prefix}cell_$Cell.expected_token_count" $ExpectedCellCardinality[$Cell].ToString()
            Expect-Kv $Receipt "${Prefix}cell_$Cell.source_token_count_min" $ExpectedObservedByChannel[$Channel][$Cell].ToString()
            Expect-Kv $Receipt "${Prefix}cell_$Cell.source_token_count_max" $ExpectedObservedByChannel[$Channel][$Cell].ToString()
        }
    }
    Require-Rows $BaselineProbe $FeatureRows
    Require-Rows $CandidateProbe $FeatureRows
    Require-Sha256 $BaselineProbe $ExpectedBaselineHash
    Require-Sha256 $CandidateProbe $Receipt.candidate_feature_sha256
    if ((Get-Item -LiteralPath $BaselineProbe).Length -ne $ExpectedBaselineSize -or
        (Get-Item -LiteralPath $CandidateProbe).Length -ne [int64]$Receipt.candidate_feature_size) {
        throw "capture probe size differs from receipt"
    }
    return @($ReceiptPath, $BaselineProbe, $CandidateProbe)
}

$RequiredPaths = @(
    $CaptureSource, $Evaluator, $ImportedEvaluator, $Protocol, $ProtocolPin,
    $Runner, $ProductionHeader, $ProductionParser, $FocusedMechanicsTest,
    $LegacyProductionTest, $CaptureMakefile, $FocusedMakefile,
    $LauncherMakefile, $GraphSpecMakefile,
    $LauncherIntegrationTest, $GraphSpecIntegrationTest, $FrozenBaseConfig,
    $Config, $ActiveDsl, $RepresentationCheckpoint, $DevelopmentFrozenProbe,
    $ConfirmationFrozenProbe, $PythonBin
)
foreach ($Path in $RequiredPaths) { Require-File $Path }
Require-Sha256 $Protocol $Expected.Protocol
Require-Sha256 $FrozenBaseConfig $Expected.FrozenBaseConfig
Require-Sha256 $Config $Expected.Config
Require-Sha256 $RepresentationCheckpoint $Expected.Representation
Require-Sha256 $DevelopmentFrozenProbe $Expected.DevelopmentBaseline
Require-Sha256 $ConfirmationFrozenProbe $Expected.ConfirmationBaseline
$PinnedProtocolHash = ([IO.File]::ReadAllText($ProtocolPin).Trim() -split '\s+')[0]
if ($PinnedProtocolHash -ne $Expected.Protocol) {
    throw "protocol sidecar does not pin the sealed SRR-4 protocol"
}

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
    throw "focused SRR-4 mechanics did not prove CUDA/raw-byte/v1 contracts"
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
    "build-cuwacunu-srr4-sparse-readout-capture", "PARALLEL_JOBS=4"
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
    $Protocol, $ProtocolPin, $ProductionHeader, $ProductionParser,
    $CaptureMakefile, $FocusedMakefile, $LauncherMakefile, $GraphSpecMakefile,
    $FocusedMechanicsTest, $FocusedMechanicsBinary,
    $LegacyProductionTest, $LegacyProductionBinary,
    $LauncherIntegrationTest, $LauncherIntegrationBinary,
    $GraphSpecIntegrationTest, $GraphSpecIntegrationBinary,
    $FrozenBaseConfig, $Config, $ActiveDsl, $RepresentationCheckpoint,
    $DevelopmentFrozenProbe, $ConfirmationFrozenProbe
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

$Development = Join-Path $RuntimeRoot "development_0_730"
Invoke-Capture 0 730 $Development
$DevelopmentOutputs = Validate-Capture $Development "development_refit_0_730" "[0,730)" 0 730 12 $Expected.DevelopmentBaseline 13648442
$DevelopmentManifest = Join-Path $Development "capture_outputs.sha256"
Write-HashManifest $DevelopmentOutputs $DevelopmentManifest

$Confirmation = Join-Path $RuntimeRoot "confirmation_760_1088"
Invoke-Capture 760 1088 $Confirmation
$ConfirmationOutputs = Validate-Capture $Confirmation "historical_confirmation_760_1088" "[760,1088)" 760 328 6 $Expected.ConfirmationBaseline 6133066
$ConfirmationManifest = Join-Path $Confirmation "capture_outputs.sha256"
Write-HashManifest $ConfirmationOutputs $ConfirmationManifest

Assert-HashManifest $PreCaptureAuthority
Assert-HashManifest $PreCaptureIncludeTree
Assert-HashManifest $PreCaptureConfigTree
Assert-HashManifest $DevelopmentManifest
Assert-HashManifest $ConfirmationManifest

$PreEndpointAuthority = Join-Path $RuntimeRoot "pre_endpoint_authority.sha256"
$EndpointInputs = $AuthorityPaths + $DevelopmentOutputs + $ConfirmationOutputs + @(
    $MechanicsGateReceipt,
    $PreCaptureAuthority, $PreCaptureIncludeTree, $PreCaptureConfigTree,
    $DevelopmentManifest, $ConfirmationManifest
)
Write-HashManifest $EndpointInputs $PreEndpointAuthority
Assert-HashManifest $PreEndpointAuthority

$DevelopmentBaseline = Join-Path $Development "all_tokens.representation_edge_features.probe"
$DevelopmentCandidate = Join-Path $Development "structured_cdsb_sparse_v1.representation_edge_features.probe"
$ConfirmationBaseline = Join-Path $Confirmation "all_tokens.representation_edge_features.probe"
$ConfirmationCandidate = Join-Path $Confirmation "structured_cdsb_sparse_v1.representation_edge_features.probe"
$EvaluationReport = Join-Path $RuntimeRoot "representation_value.report"
$EvaluationReplay = Join-Path $RuntimeRoot "representation_value.replay.report"
$EvaluatorArguments = @(
    "--baseline-dev-probe", $DevelopmentBaseline,
    "--candidate-dev-probe", $DevelopmentCandidate,
    "--baseline-confirmation-probe", $ConfirmationBaseline,
    "--candidate-confirmation-probe", $ConfirmationCandidate
)
Invoke-Checked $PythonBin (@($Evaluator) + $EvaluatorArguments + @("--output", $EvaluationReport)) -Quiet
Assert-HashManifest $PreEndpointAuthority
Assert-HashManifest $PreCaptureIncludeTree
Assert-HashManifest $PreCaptureConfigTree
Invoke-Checked $PythonBin (@($Evaluator) + $EvaluatorArguments + @("--output", $EvaluationReplay)) -Quiet
Assert-HashManifest $PreEndpointAuthority
Assert-HashManifest $PreCaptureIncludeTree
Assert-HashManifest $PreCaptureConfigTree
if ((Get-Sha256 $EvaluationReport) -ne (Get-Sha256 $EvaluationReplay)) {
    throw "SRR-4 evaluator replay is not byte exact"
}

$Evaluation = Read-Kv $EvaluationReport
foreach ($Pair in ([ordered]@{
    schema = "wikimyei.mtf_jepa_mae_vicreg.sparse_surface_structured_readout_contract_repair_evaluator.v1"
    protocol_sha256 = $Expected.Protocol
    status = "completed"
    "mechanics.feature_probe_boundary.pass" = "true"
    "mechanics.capture_contract_required_before_invocation" = "true"
    "mechanics.endpoint_metrics_inspected" = "true"
    "policy.baseline" = "all_tokens"
    "policy.candidate" = "structured_cdsb_sparse_v1"
    "policy.active" = "all_tokens"
    "policy.rollback" = "all_tokens"
    "policy.activation_changed" = "false"
    "execution.optimizer_steps" = "0"
    "execution.backward_calls" = "0"
    "execution.final_holdout_opened" = "false"
    "pairing.development.identity_order_target.pass" = "true"
    "pairing.confirmation.identity_order_target.pass" = "true"
    "compute.equal_selection.pass" = "true"
    "compute.equal_selected_refit.pass" = "true"
    "compute.equal_common_alpha_refit.pass" = "true"
    "authorization.augmentation_attribution" = "false"
}).GetEnumerator()) {
    Expect-Kv $Evaluation $Pair.Key ([string]$Pair.Value)
}
$Decision = $Evaluation["final.decision"]
if ($Decision -notin @(
    "sparse_structured_repair_qualified",
    "sparse_surface_value_gate_not_passed"
)) {
    throw "unexpected SRR-4 decision: $Decision"
}

$Completion = @(
    "schema_id=$SchemaId",
    "status=complete",
    "final.decision=$Decision",
    "quality_gate.pass=$($Evaluation['quality_gate.pass'])",
    "authorization.fresh_srr3_stage_a=$($Evaluation['authorization.fresh_srr3_stage_a'])",
    "rollback.policy=all_tokens",
    "active_policy_changed=false",
    "augmentation_attribution_started=false",
    "final_holdout_opened=false",
    "encoder_batches.maximum=18",
    "endpoint_evaluator_replay_byte_exact=true"
)
[IO.File]::WriteAllLines((Join-Path $RuntimeRoot "completion.receipt"), $Completion, $Utf8NoBom)
Assert-HashManifest $PreEndpointAuthority
Assert-HashManifest $PreCaptureIncludeTree
Assert-HashManifest $PreCaptureConfigTree
$FinalFiles = Get-ChildItem -LiteralPath $RuntimeRoot -File -Recurse |
    Where-Object Name -ne "final_outputs.sha256" |
    ForEach-Object FullName
Write-HashManifest $FinalFiles (Join-Path $RuntimeRoot "final_outputs.sha256")

Write-Output "srr4.status=complete"
Write-Output "srr4.final.decision=$Decision"
Write-Output "srr4.quality_gate.pass=$($Evaluation['quality_gate.pass'])"
Write-Output "srr4.authorization.fresh_srr3_stage_a=$($Evaluation['authorization.fresh_srr3_stage_a'])"
Write-Output "srr4.runtime_root=$RuntimeRoot"
