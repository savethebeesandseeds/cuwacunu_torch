#pragma once

#include <algorithm>
#include <cstddef>
#include <string>
#include <vector>

#include "iinuji/iinuji_cmd/views/ui/dialogs.h"

namespace cuwacunu {
namespace iinuji {
namespace iinuji_cmd {

template <class Action>
inline bool prompt_action_choice_panel(const std::string &title,
                                       const std::string &body,
                                       const std::vector<Action> &actions,
                                       std::size_t *selected,
                                       bool *cancelled) {
  std::vector<ui_choice_panel_option_t> options{};
  options.reserve(actions.size());
  for (const auto &action : actions) {
    options.push_back(ui_choice_panel_option_t{.label = action.label,
                                               .enabled = action.enabled,
                                               .disabled_status =
                                                   action.disabled_status});
  }

  std::size_t default_index = 0u;
  for (std::size_t i = 0; i < actions.size(); ++i) {
    if (actions[i].enabled) {
      default_index = i;
      break;
    }
  }

  std::size_t choice = selected ? *selected : default_index;
  if (choice >= actions.size())
    choice = default_index;

  const bool ok = ui_prompt_choice_panel(title, body, options, &choice, cancelled);
  if (selected)
    *selected = choice;
  return ok;
}

template <class Action>
inline const Action &selected_action_or_last(const std::vector<Action> &actions,
                                             std::size_t selected) {
  return actions[std::min(selected, actions.size() - 1)];
}

} // namespace iinuji_cmd
} // namespace iinuji
} // namespace cuwacunu
