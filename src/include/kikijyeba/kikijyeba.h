// SPDX-License-Identifier: MIT
#pragma once

// Kikijyeba public protocol/world surface.
//
// Runtime, Lattice, and Marshal are Hero-owned surfaces. Enter Kikijyeba
// through this header only for the current graph-first protocol, topology/dock
// validation, protocol contract compilation, and pipeline materialization.
// Component math remains in Wikimyei, source formation remains in Ujcamei, and
// optimizer/training loops remain in Jkimyei.

#include "kikijyeba/protocol/component_stream.h"
#include "kikijyeba/protocol/config_bundle.h"
#include "kikijyeba/protocol/pipeline_builder.h"
#include "kikijyeba/protocol/source_dock.h"
#include "kikijyeba/topology/dock_binding.h"
#include "kikijyeba/topology/graph/graph.h"
#include "kikijyeba/topology/node_value_chain.h"
#include "kikijyeba/topology/wikimyei_registry.h"
