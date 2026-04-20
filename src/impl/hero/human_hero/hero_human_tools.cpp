// SPDX-License-Identifier: MIT
#include "hero/human_hero/hero_human_tools.h"

#include <algorithm>
#include <array>
#include <cctype>
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
#include <iostream>
#include <optional>
#include <pwd.h>
#include <sstream>
#include <string>
#include <string_view>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <unordered_map>
#include <vector>

#include "camahjucunu/dsl/iitepi_campaign/iitepi_campaign.h"
#include "camahjucunu/dsl/latent_lineage_state/latent_lineage_state_lhs.h"
#include "hero/human_hero/human_attestation.h"
#include "hero/marshal_hero/marshal_session.h"
#include "iinuji/ncurses/iinuji_app_ncurses.h"

namespace cuwacunu {
namespace hero {
namespace human_mcp {

#include "hero_human_tools_private_core.cpp"
#include "hero_human_tools_private_ncurses.cpp"
#include "hero_human_tools_private_ops.cpp"
#include "hero_human_tools_private_handlers.cpp"
#include "hero_human_tools_public.cpp"

} // namespace human_mcp
} // namespace hero
} // namespace cuwacunu
