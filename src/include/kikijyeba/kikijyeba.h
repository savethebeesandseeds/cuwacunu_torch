// SPDX-License-Identifier: MIT
#pragma once

// Kikijyeba public runtime-planning surface.
//
// Runtime mains and Hero-facing tools should enter Kikijyeba through this
// header when they need the current graph-first protocol: wave settings,
// topology/dock validation, protocol contract compilation, and pipeline
// materialization.
// Component math remains in Wikimyei, source formation remains in Ujcamei, and
// optimizer/training loops remain in Jkimyei.

#include "kikijyeba/lattice/exposure/exposure_ledger.h"
#include "kikijyeba/lattice/target/lattice_target.h"
#include "kikijyeba/lattice/target/lattice_target_evaluator.h"
#include "kikijyeba/protocol/component_stream.h"
#include "kikijyeba/protocol/config_bundle.h"
#include "kikijyeba/protocol/pipeline_builder.h"
#include "kikijyeba/protocol/source_dock.h"
#include "kikijyeba/runtime/job_manifest.h"
#include "kikijyeba/runtime/job_runner.h"
#include "kikijyeba/runtime/job_state.h"
#include "kikijyeba/runtime/wave_plan.h"
#include "kikijyeba/settings/wave.h"
#include "kikijyeba/topology/dock_binding.h"
#include "kikijyeba/topology/graph/graph.h"
#include "kikijyeba/topology/node_value_chain.h"
#include "kikijyeba/topology/wikimyei_registry.h"
