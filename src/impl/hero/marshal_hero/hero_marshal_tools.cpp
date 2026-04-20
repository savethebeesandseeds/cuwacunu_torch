#include "hero/marshal_hero/hero_marshal_tools.h"

#include <algorithm>
#include <array>
#include <cerrno>
#include <charconv>
#include <chrono>
#include <csignal>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <system_error>
#include <tuple>
#include <unistd.h>
#include <unordered_map>
#include <vector>

#include "camahjucunu/dsl/iitepi_campaign/iitepi_campaign.h"
#include "camahjucunu/dsl/latent_lineage_state/latent_lineage_state_lhs.h"
#include "camahjucunu/dsl/marshal_objective/marshal_objective.h"
#include "hero/human_hero/human_attestation.h"
#include "hero/marshal_hero/marshal_session.h"
#include "hero/marshal_hero/marshal_session_workspace.h"
#include "hero/runtime_hero/runtime_campaign.h"
#include "hero/runtime_hero/runtime_job.h"
#include "hero/wave_contract_binding_runtime.h"

namespace cuwacunu {
namespace hero {
namespace marshal_mcp {

namespace {

#include "hero_marshal_tools_private_core.cpp"
#include "hero_marshal_tools_private_session.cpp"
#include "hero_marshal_tools_private_objective.cpp"
#include "hero_marshal_tools_private_checkpoints.cpp"
#include "hero_marshal_tools_private_runner.cpp"
#include "hero_marshal_tools_private_handlers.cpp"

} // namespace

#include "hero_marshal_tools_public.cpp"

} // namespace marshal_mcp
} // namespace hero
} // namespace cuwacunu
