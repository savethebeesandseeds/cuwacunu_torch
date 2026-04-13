// test_iinuji_cmd_terminal.cpp
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <string>
#include <vector>

#include "camahjucunu/dsl/canonical_path/canonical_path.h"
#include "iinuji/iinuji_cmd/app.h"
#include "iinuji/iinuji_cmd/commands.h"
#include "iinuji/iinuji_cmd/views/common.h"
#include "iinuji/iinuji_cmd/views/inbox/app.h"
#include "iinuji/iinuji_cmd/views/inbox/commands.h"
#include "iinuji/iinuji_cmd/views/logs/view.h"
#include "iinuji/iinuji_cmd/views/runtime/commands.h"
#include "piaabo/dconfig.h"

namespace {

namespace fs = std::filesystem;

using cuwacunu::camahjucunu::canonical_path_kind_e;

bool require(bool cond, const std::string &msg) {
  if (!cond) {
    std::cerr << "[FAIL] " << msg << "\n";
    return false;
  }
  return true;
}

void print_decoded(const cuwacunu::camahjucunu::canonical_path_t &d) {
  std::cout << "raw:       " << d.raw << "\n";
  std::cout << "canonical: " << d.canonical << "\n";
  std::cout << "kind:      "
            << (d.path_kind == canonical_path_kind_e::Call
                    ? "Call"
                    : (d.path_kind == canonical_path_kind_e::Endpoint
                           ? "Endpoint"
                           : "Node"))
            << "\n";
  std::cout << "idhash:    " << d.identity_hash_name << "\n";
}

bool write_text_file(const fs::path &path, const std::string &content) {
  std::error_code ec{};
  fs::create_directories(path.parent_path(), ec);
  std::ofstream out(path);
  if (!out)
    return false;
  out << content;
  return out.good();
}

} // namespace

int main() {
  try {
    const char *global_config_path = "/cuwacunu/src/config/.config";
    cuwacunu::iitepi::config_space_t::change_config_file(global_config_path);
    cuwacunu::iitepi::config_space_t::update_config();

    const std::vector<std::string> valid_samples = {
        "iinuji.refresh()",
        "iinuji.screen.human()",
        "iinuji.screen.runtime()",
        "iinuji.config.file.index.n1()",
    };

    std::set<std::string> identity_hashes;
    std::set<std::string> canonical_identities;
    bool ok = true;
    for (const auto &sample : valid_samples) {
      auto decoded = cuwacunu::camahjucunu::decode_canonical_path(sample);
      print_decoded(decoded);
      std::cout << "---\n";

      ok = ok && require(decoded.ok, "sample should parse: " + sample);
      ok =
          ok && require(cuwacunu::camahjucunu::validate_canonical_path(decoded),
                        "sample should validate: " + sample);
      ok = ok && require(!decoded.identity_hash_name.empty(),
                         "identity hash should not be empty: " + sample);
      identity_hashes.insert(decoded.identity_hash_name);
      canonical_identities.insert(decoded.canonical_identity);
    }

    {
      auto a =
          cuwacunu::camahjucunu::decode_canonical_path(valid_samples.front());
      auto b =
          cuwacunu::camahjucunu::decode_canonical_path(valid_samples.front());
      ok = ok && require(a.identity_hash_name == b.identity_hash_name,
                         "identity hash must be deterministic");
    }

    ok = ok && require(identity_hashes.size() == canonical_identities.size(),
                       "expected one identity hash per canonical identity");

    using namespace cuwacunu::iinuji::iinuji_cmd;

    ok = ok &&
         require(desired_input_timeout_for_screen(ScreenMode::Inbox) == 250,
                 "human screen should use a polling input timeout");
    ok = ok &&
         require(idle_refresh_period_ms_for_screen(ScreenMode::Inbox) == 2000,
                 "human screen should auto-refresh on a short idle cadence");
    ok = ok &&
         require(desired_input_timeout_for_screen(ScreenMode::ShellLogs) == 50,
                 "shell logs should keep the faster polling timeout");

    {
      ShellLogsState logs_settings{};
      cuwacunu::piaabo::dlog_entry_t entry{};
      entry.level = "INFO";

      entry.message = "{\"warnings\":[]}";
      ok = ok && require(logs_line_emphasis(logs_settings, entry) ==
                             cuwacunu::iinuji::text_line_emphasis_t::Info,
                         "empty warnings payload should keep info emphasis");

      entry.message = "{\"warnings\":[\"runtime launch warning\"]}";
      ok = ok &&
           require(
               logs_line_emphasis(logs_settings, entry) ==
                   cuwacunu::iinuji::text_line_emphasis_t::MutedWarning,
               "non-empty warnings payload should use muted warning emphasis");

      entry.message = "{\"dev_warnings\":[\"developer note\"]}";
      ok = ok &&
           require(logs_line_emphasis(logs_settings, entry) ==
                       cuwacunu::iinuji::text_line_emphasis_t::MutedWarning,
                   "non-empty dev_warnings payload should use muted warning "
                   "emphasis");
    }

    {
      cuwacunu::hero::marshal::marshal_session_record_t failed_session{};
      failed_session.finish_reason = "failed";
      ok = ok &&
           require(runtime_session_row_emphasis(failed_session) ==
                       cuwacunu::iinuji::text_line_emphasis_t::MutedError,
                   "failed runtime sessions should use muted error emphasis");

      cuwacunu::hero::runtime::runtime_campaign_record_t failed_campaign{};
      failed_campaign.state = "error";
      ok = ok &&
           require(runtime_campaign_row_emphasis(failed_campaign) ==
                       cuwacunu::iinuji::text_line_emphasis_t::MutedError,
                   "failed runtime campaigns should use muted error emphasis");

      cuwacunu::hero::runtime::runtime_job_record_t failed_job{};
      failed_job.state = "failed";
      ok = ok && require(runtime_job_row_emphasis(failed_job) ==
                             cuwacunu::iinuji::text_line_emphasis_t::MutedError,
                         "failed runtime jobs should use muted error emphasis");
    }

    CmdState st{};
    st.inbox.app.global_config_path = global_config_path;
    st.inbox.app.hero_config_path =
        cuwacunu::hero::human_mcp::resolve_human_hero_dsl_path(
            global_config_path);
    st.inbox.app.self_binary_path =
        cuwacunu::hero::human_mcp::current_executable_path();
    std::string human_error{};
    (void)cuwacunu::hero::human_mcp::load_human_defaults(
        st.inbox.app.hero_config_path, global_config_path,
        &st.inbox.app.defaults, &human_error);
    (void)refresh_inbox_state(st);

    st.runtime.app.global_config_path = global_config_path;
    st.runtime.app.hero_config_path =
        cuwacunu::hero::runtime_mcp::resolve_runtime_hero_dsl_path(
            global_config_path);
    st.runtime.app.self_binary_path =
        cuwacunu::hero::runtime_mcp::current_executable_path();
    std::string runtime_error{};
    (void)cuwacunu::hero::runtime_mcp::load_runtime_defaults(
        st.runtime.app.hero_config_path, global_config_path,
        &st.runtime.app.defaults, &runtime_error);
    (void)refresh_runtime_state(st);
    st.config = load_config_view_from_state(st);
    clamp_selected_config_file(st);

    {
      const fs::path temp_root = "/tmp/test_iinuji_cmd_terminal_config_scope";
      std::error_code ec{};
      fs::remove_all(temp_root, ec);
      fs::create_directories(temp_root / "backups", ec);
      const fs::path session_policy_path =
          temp_root / "session.hero.config.dsl";
      const std::string session_policy_text =
          "protocol_layer[STDIO|HTTPS/SSE]:str = STDIO\n"
          "config_scope_root:str = /cuwacunu/src/config\n"
          "allow_local_read:bool = true\n"
          "allow_local_write:bool = true\n"
          "default_roots:str = /cuwacunu/src/config/instructions/defaults\n"
          "objective_roots:str = "
          "/cuwacunu/src/config/instructions/objectives/vicreg.solo.train\n"
          "allowed_extensions:str = .dsl,.md\n"
          "write_roots:str = "
          "/cuwacunu/src/config/instructions/objectives/vicreg.solo.train\n"
          "backup_enabled:bool = true\n"
          "backup_dir:str = "
          "/tmp/test_iinuji_cmd_terminal_config_scope/backups\n"
          "backup_max_entries(1,+inf):int = 4\n";
      ok = ok &&
           require(write_text_file(session_policy_path, session_policy_text),
                   "should write synthetic session config policy");

      cuwacunu::hero::marshal::marshal_session_record_t selected_session{};
      selected_session.session_id = "session.synthetic.config.scope";
      selected_session.config_policy_path = session_policy_path.string();
      selected_session.objective_root =
          "/cuwacunu/src/config/instructions/objectives/vicreg.solo.train";

      const ConfigState scoped = load_config_view_from_global_config(
          global_config_path, &selected_session);
      const auto find_config_file =
          [&](std::string_view relative_path) -> const ConfigFileEntry * {
        for (const auto &file : scoped.files) {
          if (file.relative_path == relative_path)
            return &file;
        }
        return nullptr;
      };

      const ConfigFileEntry *train_file =
          find_config_file("vicreg.solo.train/iitepi.campaign.dsl");
      const ConfigFileEntry *optimize_file =
          find_config_file("vicreg.solo.settings_optimize/iitepi.campaign.dsl");

      ok = ok &&
           require(scoped.ok,
                   "session-scoped config browser should load successfully");
      ok = ok && require(scoped.using_session_policy,
                         "selected session should activate the write scope");
      ok =
          ok && require(train_file != nullptr,
                        "selected session objective files should stay visible");
      ok = ok && require(optimize_file != nullptr,
                         "global config browser should keep sibling objective "
                         "folders visible");
      ok = ok &&
           require(train_file != nullptr && train_file->editable,
                   "selected session objective files should stay editable");
      ok = ok && require(optimize_file != nullptr && !optimize_file->editable,
                         "sibling objective folders should stay read-only "
                         "outside the selected session scope");

      fs::remove_all(temp_root, ec);
    }

    run_command(st, "help", nullptr);
    ok = ok &&
         require(st.help_view,
                 "help command should enable help overlay without log box");
    st.help_view = false;

    run_command(st, "h", nullptr);
    ok = ok && require(st.help_view,
                       "h alias should enable help overlay without log box");
    st.help_view = false;

    run_command(st, "iinuji.help", nullptr);
    ok = ok && require(st.help_view,
                       "help canonical shorthand should enable help overlay");
    st.help_view = false;

    run_command(st, "marshal", nullptr);
    ok = ok && require(st.screen == ScreenMode::Inbox,
                       "marshal alias should switch to Inbox screen");

    run_command(st, "runtime", nullptr);
    ok = ok && require(st.screen == ScreenMode::Runtime,
                       "runtime alias should switch to Runtime screen");

    run_command(st, "logs", nullptr);
    ok = ok && require(st.screen == ScreenMode::ShellLogs,
                       "logs alias should switch to Shell Logs screen");

    run_command(st, "config", nullptr);
    ok = ok && require(st.screen == ScreenMode::Config,
                       "config alias should switch to Config screen");

    const ScreenMode before_removed = st.screen;
    const auto removed_training_alias = command_aliases::resolve("training");
    ok = ok && require(!removed_training_alias.matched,
                       "removed training alias should stay unresolved");
    ok = ok && require(st.screen == before_removed,
                       "removed training alias should not switch screens");

    run_command(st, "refresh", nullptr);
    ok = ok && require(!st.inbox.status_is_error,
                       "refresh alias should reload the Hero shell state");
    ok = ok &&
         require(
             st.inbox.error.empty(),
             "refresh alias should leave the Inbox screen without an error");

    run_command(st, "quit", nullptr);
    ok = ok && require(!st.running, "quit alias should work without log box");
    st.running = true;

    run_command(st, "q", nullptr);
    ok = ok && require(!st.running, "q alias should work without log box");
    st.running = true;

    run_command(st, "exit", nullptr);
    ok = ok && require(!st.running, "exit alias should work without log box");

    st.running = true;
    st.screen = ScreenMode::Inbox;
    st.inbox.focus = kInboxMenuFocus;
    st.inbox.view = kInboxView;

    ok = ok && require(!handle_inbox_key(st, '\t'),
                       "tab should no longer cycle Marshal lanes");
    ok = ok &&
         require(!handle_inbox_key(st, KEY_DOWN),
                 "down in the human menu should no-op with inbox-only Marshal");
    ok = ok &&
         require(!handle_inbox_key(st, KEY_LEFT),
                 "left in the human menu should no-op with inbox-only Marshal");
    ok = ok && require(st.inbox.view == kInboxView,
                       "human menu should remain on Inbox");
    ok = ok && require(st.inbox.focus == kInboxMenuFocus,
                       "human menu navigation should keep menu focus");

    cuwacunu::hero::marshal::marshal_session_record_t request_a{};
    request_a.session_id = "session.request.a";
    request_a.phase = "paused";
    request_a.pause_kind = "clarification";
    request_a.objective_name = "Synthetic Request A";
    cuwacunu::hero::marshal::marshal_session_record_t request_b{};
    request_b.session_id = "session.request.b";
    request_b.phase = "paused";
    request_b.pause_kind = "governance";
    request_b.objective_name = "Synthetic Request B";
    st.inbox.operator_inbox.all_sessions = {request_a, request_b};
    st.inbox.operator_inbox.actionable_requests = {request_a, request_b};
    st.inbox.view = kInboxView;
    st.inbox.selected_inbox_session = 0;

    ok = ok && require(handle_inbox_key(st, '\n'),
                       "enter in the human menu should focus the inbox");
    ok = ok && require(st.inbox.focus == kInboxFocus,
                       "enter in the human menu should enter inbox focus");
    ok = ok && require(!handle_inbox_key(st, 'a'),
                       "a should no longer trigger Marshal actions directly");
    ok = ok && require(!handle_inbox_key(st, 'r'),
                       "r should no longer trigger a manual Marshal refresh");
    ok = ok && require(handle_inbox_key(st, KEY_DOWN),
                       "down in the inbox should move the selected request");
    ok = ok && require(st.inbox.selected_inbox_session == 1,
                       "inbox down should advance request selection");
    ok = ok && require(handle_inbox_key(st, 27),
                       "escape in the inbox should return to menu focus");
    ok = ok && require(st.inbox.focus == kInboxMenuFocus,
                       "escape in the inbox should restore menu focus");

    if (!ok)
      return 1;
    std::cout << "[ok] iinuji cmd terminal Hero shell smoke passed\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << "[test_iinuji_cmd_terminal] exception: " << e.what() << "\n";
    return 1;
  }
}
