#pragma once

#include "iinuji/iinuji_cmd/views/inbox/commands.h"
#include "iinuji/iinuji_cmd/views/lattice/commands.h"
#include "iinuji/iinuji_cmd/views/runtime/commands.h"

namespace cuwacunu {
namespace iinuji {
namespace iinuji_cmd {

struct IinujiStateFlow {
  CmdState& state;

  void reload_inbox() const { (void)queue_inbox_refresh(state); }

  void reload_runtime() const { (void)queue_runtime_refresh(state); }

  void reload_lattice(
      LatticeRefreshMode mode = LatticeRefreshMode::Snapshot) const {
    queue_lattice_refresh(state, mode);
  }

  void reload_config() const { (void)queue_config_refresh(state); }

  void reload_shell() const {
    reload_inbox();
    reload_config();
    reload_runtime();
    reload_lattice(LatticeRefreshMode::Snapshot);
  }

  void normalize_after_command() const {
    clamp_inbox_state(state);
    clamp_lattice_state(state);
    clamp_selected_config_file(state);
  }
};

}  // namespace iinuji_cmd
}  // namespace iinuji
}  // namespace cuwacunu
