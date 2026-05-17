static void test_marshal_resume_rejects_mismatched_clarification_artifact() {
  temp_dir_t tmp("test_marshal_resume_rejects_bad_clarification");
  const fs::path cfg_dir = tmp.dir / "cfg";
  const fs::path runtime_root = tmp.dir / "runtime";
  const fs::path global_config = cfg_dir / "global.config";
  const fs::path marshal_dsl = cfg_dir / "default.hero.marshal.dsl";
  const fs::path human_dsl = cfg_dir / "default.hero.human.dsl";
  const fs::path codex_stub = cfg_dir / "codex_stub.sh";
  const fs::path human_identities = cfg_dir / "human_operator_identities";
  const fs::path bad_answer = cfg_dir / "bad_answer.json";
  const fs::path source_objective_dsl =
      "/cuwacunu/src/config/instructions/objectives/"
      "runtime.operative.vicreg.solo.train/"
      ".marshal.dsl";

  write_text_file(human_identities, "# smoke-only\n");
  const std::string codex_stub_text = std::string(
                                          R"CODEX(#!/usr/bin/env bash
set -euo pipefail
state_file="$(dirname "$0")/codex_stub.state"
decision_path=""
resume_mode=0
for ((i=1;i<=$#;i++)); do
  arg="${!i}"
  if [[ "$arg" == "resume" ]]; then resume_mode=1; fi
  if [[ "$arg" == "-o" ]]; then
    j=$((i+1))
    decision_path="${!j}"
  fi
done
if [[ -z "$decision_path" ]]; then
  echo "missing -o decision path" >&2
  exit 2
fi
printf 'session id: %s\n' ")CODEX") + kSmokeCodexSessionId +
                                      R"CODEX(" >&2
step=0
if [[ -f "$state_file" ]]; then
  step="$(cat "$state_file")"
fi
if [[ "$resume_mode" == "0" && "$step" == "0" ]]; then
  cat > "$decision_path" <<'JSON'
{"intent":"pause_for_clarification","reply_text":"","launch":null,"clarification_request":{"request":"Provide the missing clarification."},"governance":null,"reason":"need clarification","memory_note":"asked for clarification"}
JSON
  echo 1 > "$state_file"
  exit 0
fi
if [[ "$resume_mode" == "1" && "$step" == "1" ]]; then
  cat > "$decision_path" <<'JSON'
{"intent":"complete","reply_text":"","launch":null,"clarification_request":null,"governance":null,"reason":"clarification accepted","memory_note":"completed after valid clarification"}
JSON
  echo 2 > "$state_file"
  exit 0
fi
echo "unexpected codex stub invocation" >&2
exit 3
)CODEX";
  write_executable_file(codex_stub, codex_stub_text);
  write_text_file(
      global_config,
      std::string("[GENERAL]\n") + "runtime_root=" + runtime_root.string() +
          "\nrepo_root=/cuwacunu\n"
          "default_iitepi_campaign_dsl_filename=/cuwacunu/src/config/"
          "instructions/defaults/default.iitepi.campaign.dsl\n"
          "\n[BNF]\n"
          "iitepi_campaign_grammar_filename=/cuwacunu/src/config/bnf/"
          "iitepi.campaign.bnf\n"
          "marshal_objective_grammar_filename=/cuwacunu/src/config/bnf/"
          "objective.marshal.bnf\n"
          "\n[REAL_HERO]\n"
          "marshal_hero_dsl_filename=" +
          marshal_dsl.string() + "\n" +
          "human_hero_dsl_filename=" + human_dsl.string() + "\n");
  write_text_file(
      marshal_dsl,
      std::string("runtime_hero_binary:path = /bin/true\n") +
          "config_hero_binary:path = /bin/echo\n"
          "lattice_hero_binary:path = /bin/cat\n"
          "human_hero_binary:path = /cuwacunu/.build/hero/hero_human.mcp\n"
          "human_operator_identities:path = " +
          human_identities.string() + "\n" +
          "config_scope_root:path = /cuwacunu/src/config\n" +
          "marshal_codex_binary:path = " + codex_stub.string() + "\n" +
          "marshal_codex_model:str = gpt-5.3-codex-spark\n"
          "marshal_codex_reasoning_effort:str = xhigh\n"
          "tail_default_lines(1,+inf):int = 20\n"
          "poll_interval_ms(100,+inf):int = 100\n"
          "marshal_codex_timeout_sec(1,+inf):int = 5\n"
          "marshal_max_campaign_launches(1,+inf):int = 2\n");
  write_text_file(
      human_dsl,
      "marshal_hero_binary:path = /cuwacunu/.build/hero/hero_marshal.mcp\n"
      "operator_id:str = smoke@local\n"
      "operator_signing_ssh_identity:path = /tmp/unused-human-identity\n");

  const std::string start_args =
      std::string("{\"marshal_objective_dsl_path\":\"") +
      source_objective_dsl.string() + "\"}";
  const command_result_t start = run_command(
      direct_tool_command(kMarshalMcpBin, global_config, marshal_dsl,
                          "hero.marshal.start_session", start_args));
  REQUIRE(start.exit_code == 0);
  const std::string session_id =
      extract_json_string_field(start.output, "marshal_session_id");
  REQUIRE(!session_id.empty());

  const std::string get_args =
      std::string("{\"marshal_session_id\":\"") + session_id + "\"}";
  const std::string get_command =
      direct_tool_command(kMarshalMcpBin, global_config, marshal_dsl,
                          "hero.marshal.get_session", get_args);
  const command_result_t paused =
      wait_for_command(get_command, [](const command_result_t &result) {
        return result.exit_code == 0 &&
               contains(result.output, "\"lifecycle\":\"live\"") &&
               contains(result.output, "\"work_gate\":\"clarification\"");
      });
  REQUIRE(paused.exit_code == 0);

  write_text_file(bad_answer,
                  "{\"schema\":\"hero.human.clarification_answer.v3\","
                  "\"marshal_session_id\":\"marshal.wrong\","
                  "\"checkpoint_index\":999,"
                  "\"answer\":\"bad\"}");
  const std::string resume_args =
      std::string("{\"marshal_session_id\":\"") + session_id +
      "\",\"clarification_answer_path\":\"" + bad_answer.string() + "\"}";
  const command_result_t rejected = run_command(
      direct_tool_command(kMarshalMcpBin, global_config, marshal_dsl,
                          "hero.marshal.resume_session", resume_args));
  REQUIRE(rejected.exit_code != 0);
  REQUIRE(contains(rejected.output,
                   "clarification answer marshal_session_id does not match "
                   "paused session"));

  const command_result_t still_paused = run_command(get_command);
  REQUIRE(still_paused.exit_code == 0);
  REQUIRE(contains(still_paused.output, "\"lifecycle\":\"live\""));
  REQUIRE(contains(still_paused.output, "\"work_gate\":\"clarification\""));

  const std::string answer_args =
      std::string("{\"marshal_session_id\":\"") + session_id +
      "\",\"answer\":\"Here is the valid clarification.\"}";
  const command_result_t answered = run_command(
      direct_tool_command(kHumanMcpBin, global_config, human_dsl,
                          "hero.human.answer_request", answer_args));
  REQUIRE(answered.exit_code == 0);

  const command_result_t idled =
      wait_for_command(get_command, [](const command_result_t &result) {
        return result.exit_code == 0 &&
               contains(result.output, "\"lifecycle\":\"live\"") &&
               contains(result.output, "\"activity\":\"review\"") &&
               contains(result.output, "\"finish_reason\":\"success\"");
      });
  REQUIRE(idled.exit_code == 0);
}

static void test_marshal_session_failed_checkpoint_idles_and_continues() {
  temp_dir_t tmp("test_marshal_session_failed_checkpoint");
  const fs::path cfg_dir = tmp.dir / "cfg";
  const fs::path runtime_root = tmp.dir / "runtime";
  const fs::path global_config = cfg_dir / "global.config";
  const fs::path marshal_dsl = cfg_dir / "default.hero.marshal.dsl";
  const fs::path human_dsl = cfg_dir / "default.hero.human.dsl";
  const fs::path codex_stub = cfg_dir / "codex_stub.sh";
  const fs::path human_identities = cfg_dir / "human_operator_identities";
  const fs::path source_objective_dsl =
      "/cuwacunu/src/config/instructions/objectives/"
      "runtime.operative.vicreg.solo.train/"
      ".marshal.dsl";

  write_text_file(human_identities, "# smoke-only\n");
  const std::string codex_stub_text = std::string(
                                          R"CODEX(#!/usr/bin/env bash
set -euo pipefail
state_file="$(dirname "$0")/codex_stub.state"
decision_path=""
resume_mode=0
expect_model="gpt-5.3-codex-spark"
expect_effort='model_reasoning_effort="xhigh"'
have_model=0
have_effort=0
have_json=0
for ((i=1;i<=$#;i++)); do
  arg="${!i}"
  if [[ "$arg" == "resume" ]]; then resume_mode=1; fi
  if [[ "$arg" == "--json" ]]; then have_json=1; fi
  if [[ "$arg" == "-o" ]]; then
    j=$((i+1))
    decision_path="${!j}"
  fi
  if [[ "$arg" == "-m" ]]; then
    j=$((i+1))
    if [[ "${!j}" != "$expect_model" ]]; then
      echo "unexpected codex model: ${!j}" >&2
      exit 4
    fi
    have_model=1
  fi
  if [[ "$arg" == "-c" ]]; then
    j=$((i+1))
    if [[ "${!j}" == "$expect_effort" ]]; then
      have_effort=1
    fi
  fi
done
if [[ -z "$decision_path" ]]; then
  echo "missing -o decision path" >&2
  exit 2
fi
if [[ "$have_model" != "1" || "$have_effort" != "1" ]]; then
  echo "missing codex model/effort arguments" >&2
  exit 4
fi
if [[ "$have_json" != "1" ]]; then
  echo "missing codex --json argument" >&2
  exit 4
fi
printf 'session id: %s\n' ")CODEX") + kSmokeCodexSessionId +
                                      R"CODEX(" >&2
step=0
if [[ -f "$state_file" ]]; then
  step="$(cat "$state_file")"
fi
printf '{"type":"stub_event","step":"%s","resume_mode":%s}\n' "$step" "$resume_mode"
if [[ "$resume_mode" == "0" && "$step" == "0" ]]; then
  cat > "$decision_path" <<'JSON'
{"intent":"launch_campaign","reply_text":"","launch":{"mode":"run_plan","binding_id":null,"reset_runtime_state":false,"requires_objective_mutation":true},"clarification_request":null,"governance":null,"reason":"launch after same-checkpoint objective edit","memory_note":"need softer augmentation before the next launch"}
JSON
  echo 1 > "$state_file"
  exit 0
fi
if [[ "$resume_mode" == "1" && "$step" == "1" ]]; then
  sleep 2
  exit 0
fi
if [[ "$resume_mode" == "0" && "$step" == "1" ]]; then
  cat > "$decision_path" <<'JSON'
{"intent":"complete","reply_text":"","launch":null,"clarification_request":null,"governance":null,"reason":"operator continuation succeeded after reviewing the failed checkpoint","memory_note":"recovered after failed checkpoint continuation"}
JSON
  echo 2 > "$state_file"
  exit 0
fi
echo "unexpected codex stub invocation" >&2
exit 3
)CODEX";
  write_executable_file(codex_stub, codex_stub_text);
  write_text_file(
      global_config,
      std::string("[GENERAL]\n") + "runtime_root=" + runtime_root.string() +
          "\nrepo_root=/cuwacunu\n"
          "default_iitepi_campaign_dsl_filename=/cuwacunu/src/config/"
          "instructions/defaults/default.iitepi.campaign.dsl\n"
          "\n[BNF]\n"
          "iitepi_campaign_grammar_filename=/cuwacunu/src/config/bnf/"
          "iitepi.campaign.bnf\n"
          "marshal_objective_grammar_filename=/cuwacunu/src/config/bnf/"
          "objective.marshal.bnf\n"
          "\n[REAL_HERO]\n"
          "marshal_hero_dsl_filename=" +
          marshal_dsl.string() + "\n" +
          "human_hero_dsl_filename=" + human_dsl.string() + "\n");
  write_text_file(
      marshal_dsl,
      std::string("runtime_hero_binary:path = /bin/true\n") +
          "config_hero_binary:path = /bin/true\n"
          "lattice_hero_binary:path = /bin/true\n"
          "human_hero_binary:path = /cuwacunu/.build/hero/hero_human.mcp\n"
          "human_operator_identities:path = " +
          human_identities.string() + "\n" +
          "config_scope_root:path = /cuwacunu/src/config\n"
          "marshal_codex_binary:path = " +
          codex_stub.string() +
          "\n"
          "marshal_codex_model:str = gpt-5.3-codex-spark\n"
          "marshal_codex_reasoning_effort:str = xhigh\n"
          "tail_default_lines(1,+inf):int = 20\n"
          "poll_interval_ms(100,+inf):int = 100\n"
          "marshal_codex_timeout_sec(1,+inf):int = 1\n"
          "marshal_max_campaign_launches(1,+inf):int = 2\n");
  write_text_file(
      human_dsl,
      "marshal_hero_binary:path = /cuwacunu/.build/hero/hero_marshal.mcp\n"
      "operator_id:str = smoke@local\n"
      "operator_signing_ssh_identity:path = /tmp/unused-human-identity\n");

  const std::string start_args =
      std::string("{\"marshal_objective_dsl_path\":\"") +
      source_objective_dsl.string() + "\"}";
  const command_result_t start = run_command(
      direct_tool_command(kMarshalMcpBin, global_config, marshal_dsl,
                          "hero.marshal.start_session", start_args));
  REQUIRE(start.exit_code == 0);
  REQUIRE(contains(start.output, "\"isError\":false"));
  const std::string session_id =
      extract_json_string_field(start.output, "marshal_session_id");
  REQUIRE(!session_id.empty());

  const std::string get_args =
      std::string("{\"marshal_session_id\":\"") + session_id + "\"}";
  const std::string get_command =
      direct_tool_command(kMarshalMcpBin, global_config, marshal_dsl,
                          "hero.marshal.get_session", get_args);
  const command_result_t idled_failed =
      wait_for_command(get_command, [](const command_result_t &result) {
        return result.exit_code == 0 &&
               contains(result.output, "\"lifecycle\":\"live\"") &&
               contains(result.output, "\"activity\":\"review\"") &&
               contains(result.output, "\"finish_reason\":\"failed\"") &&
               contains(result.output, "\"checkpoint_count\":1");
      });
  REQUIRE(idled_failed.exit_code == 0);
  REQUIRE(contains(idled_failed.output,
                   "\"last_codex_action\":\"launch_campaign\""));
  const std::string failed_intent_path = extract_json_string_field(
      idled_failed.output, "last_intent_checkpoint_path");
  REQUIRE(!failed_intent_path.empty());
  REQUIRE(contains(idled_failed.output,
                   "launch.requires_objective_mutation=true but no "
                   "same-checkpoint objective mutation was materialized"));
  REQUIRE(contains(idled_failed.output,
                   "session parked for review so the operator can message it "
                   "after adjusting objective files or tooling"));

  const fs::path session_root = runtime_root / ".marshal_hero" / session_id;
  const fs::path codex_stdout_log =
      session_root / "logs" / "codex.session.stdout.jsonl";
  const fs::path codex_stderr_log =
      session_root / "logs" / "codex.session.stderr.jsonl";
  REQUIRE(fs::exists(codex_stdout_log));
  REQUIRE(fs::exists(codex_stderr_log));
  REQUIRE(!fs::exists(session_root / "logs" / "codex.session.log"));
  REQUIRE(
      contains(read_text_file(codex_stdout_log), "\"type\":\"stub_event\""));
  REQUIRE(contains(read_text_file(codex_stderr_log), "\"stream\":\"stderr\""));
  REQUIRE(contains(read_text_file(codex_stderr_log), kSmokeCodexSessionId));
  const std::string summary_text =
      read_text_file(session_root / "human" / "summary.latest.md");
  REQUIRE(contains(summary_text, "This report is actionable."));
  REQUIRE(contains(summary_text, "launch.requires_objective_mutation=true"));
  REQUIRE(contains(summary_text, "Checkpoints: 1"));

  const std::string continue_args =
      std::string("{\"marshal_session_id\":\"") + session_id +
      "\",\"message\":\"Continue after reviewing the failed mutation "
      "checkpoint.\"}";
  const command_result_t continued = run_command(
      direct_tool_command(kMarshalMcpBin, global_config, marshal_dsl,
                          "hero.marshal.message_session", continue_args));
  REQUIRE(continued.exit_code == 0);
  REQUIRE(contains(continued.output, "\"isError\":false"));

  const command_result_t idled_success =
      wait_for_command(get_command, [](const command_result_t &result) {
        return result.exit_code == 0 &&
               contains(result.output, "\"lifecycle\":\"live\"") &&
               contains(result.output, "\"activity\":\"review\"") &&
               contains(result.output, "\"finish_reason\":\"success\"") &&
               contains(result.output, "\"checkpoint_count\":2");
      });
  REQUIRE(idled_success.exit_code == 0);
  REQUIRE(contains(idled_success.output,
                   "codex resume degraded: checkpoint=2 "
                   "kind=resume_command_failed fallback=fresh_checkpoint"));
  REQUIRE(contains(idled_success.output, "resume_timeout=true"));

  const std::string events_text =
      wait_for_file_containing(session_root / "marshal.session.events.jsonl",
                               "\"type\":\"operator.message_handled\"");
  REQUIRE(contains(events_text, "\"type\":\"checkpoint.failed\""));
  REQUIRE(contains(events_text, "\"type\":\"codex.resume_failed\""));
  REQUIRE(contains(events_text, "\"type\":\"operator.message_received\""));
  REQUIRE(contains(events_text, "\"type\":\"operator.message_delivered\""));
  REQUIRE(contains(events_text, "\"type\":\"operator.message_handled\""));

  const std::string memory_text =
      read_text_file(session_root / "marshal.session.memory.md");
  REQUIRE(contains(memory_text, "## Checkpoint 1"));
  REQUIRE(contains(memory_text, "## Checkpoint 2"));
}

static void
test_marshal_pause_session_retains_active_campaign_until_runtime_stop_finishes() {
  temp_dir_t tmp("test_marshal_pause_session_retains_active_campaign");
  const fs::path cfg_dir = tmp.dir / "cfg";
  const fs::path runtime_root = tmp.dir / "runtime";
  const fs::path global_config = cfg_dir / "global.config";
  const fs::path marshal_dsl = cfg_dir / "default.hero.marshal.dsl";
  const fs::path human_identities = cfg_dir / "human_operator_identities";
  const fs::path runtime_stub = cfg_dir / "runtime_stub.sh";
  const fs::path runtime_trace = cfg_dir / "runtime_stop_args.json";
  const std::string session_id = "marshal.pause.pending";
  const std::string campaign_cursor = "campaign.pause.pending";
  const fs::path session_hero_marshal_dsl =
      runtime_root / ".marshal_hero" / session_id / "hero.marshal.dsl";

  write_text_file(human_identities, "# smoke-only\n");
  const std::string runtime_stub_text = std::string(
                                            R"RUNTIME(#!/usr/bin/env bash
set -euo pipefail
trace_file=")RUNTIME") + runtime_trace.string() +
                                        R"RUNTIME("
tool_name=""
args_json="{}"
for ((i=1;i<=$#;i++)); do
  arg="${!i}"
  if [[ "$arg" == "--tool" ]]; then
    j=$((i+1))
    tool_name="${!j}"
  fi
  if [[ "$arg" == "--args-json" ]]; then
    j=$((i+1))
    args_json="${!j}"
  fi
done
if [[ "$tool_name" != "hero.runtime.stop_campaign" ]]; then
  printf '{"content":[{"type":"text","text":"unsupported tool"}],"structuredContent":{},"isError":true}\n'
  exit 1
fi
printf '%s' "$args_json" > "$trace_file"
grep -F '"suppress_marshal_session_update":true' <<<"$args_json" >/dev/null || {
  printf '{"content":[{"type":"text","text":"missing suppress flag"}],"structuredContent":{},"isError":true}\n'
  exit 2
}
campaign_cursor="$(sed -n 's/.*"campaign_cursor":"\([^"]*\)".*/\1/p' <<<"$args_json" | head -n 1)"
if [[ -z "$campaign_cursor" ]]; then
  printf '{"content":[{"type":"text","text":"missing campaign cursor"}],"structuredContent":{},"isError":true}\n'
  exit 3
fi
printf '{"content":[{"type":"text","text":"ok"}],"structuredContent":{"campaign_cursor":"%s","campaign":{"schema":"hero.runtime.campaign.v2","campaign_cursor":"%s","state":"stopping","state_detail":"stub stop in progress","marshal_session_id":"marshal.pause.pending"},"warnings":[]},"isError":false}\n' \
  "$campaign_cursor" "$campaign_cursor"
)RUNTIME";
  write_executable_file(runtime_stub, runtime_stub_text);

  write_text_file(
      global_config,
      std::string("[GENERAL]\n") + "runtime_root=" + runtime_root.string() +
          "\nrepo_root=/cuwacunu\n"
          "default_iitepi_campaign_dsl_filename=/cuwacunu/src/config/"
          "instructions/defaults/default.iitepi.campaign.dsl\n"
          "\n[BNF]\n"
          "iitepi_campaign_grammar_filename=/cuwacunu/src/config/bnf/"
          "iitepi.campaign.bnf\n"
          "marshal_objective_grammar_filename=/cuwacunu/src/config/bnf/"
          "objective.marshal.bnf\n"
          "\n[REAL_HERO]\n"
          "marshal_hero_dsl_filename=" +
          marshal_dsl.string() + "\n");
  const std::string marshal_dsl_text =
      std::string("runtime_hero_binary:path = ") + runtime_stub.string() +
      "\n"
      "config_hero_binary:path = /bin/true\n"
      "lattice_hero_binary:path = /bin/true\n"
      "human_hero_binary:path = /bin/true\n"
      "human_operator_identities:path = " +
      human_identities.string() + "\n" +
      "config_scope_root:path = /cuwacunu/src/config\n"
      "marshal_codex_binary:path = /bin/true\n"
      "marshal_codex_model:str = gpt-5.3-codex-spark\n"
      "marshal_codex_reasoning_effort:str = xhigh\n"
      "tail_default_lines(1,+inf):int = 20\n"
      "poll_interval_ms(100,+inf):int = 100\n"
      "marshal_codex_timeout_sec(1,+inf):int = 5\n"
      "marshal_max_campaign_launches(1,+inf):int = 2\n";
  write_text_file(marshal_dsl, marshal_dsl_text);
  write_text_file(session_hero_marshal_dsl, marshal_dsl_text);

  const live_process_identity_t current_identity = current_process_identity();
  write_manual_running_campaign_fixture(runtime_root, global_config,
                                        session_hero_marshal_dsl, session_id,
                                        campaign_cursor, current_identity);
  {
    const held_file_lock_t session_runner_lock(
        cuwacunu::hero::marshal::marshal_session_runner_lock_path(
            runtime_root / ".marshal_hero", session_id));

    const std::string pause_args = std::string("{\"marshal_session_id\":\"") +
                                   session_id + "\",\"force\":false}";
    const command_result_t paused = run_command(
        direct_tool_command(kMarshalMcpBin, global_config, marshal_dsl,
                            "hero.marshal.pause_session", pause_args));
    REQUIRE(paused.exit_code == 0);
    REQUIRE(contains(paused.output, "\"isError\":false"));
    REQUIRE(contains(paused.output, "\"lifecycle\":\"live\""));
    REQUIRE(contains(paused.output, "\"work_gate\":\"operator_pause\""));
    REQUIRE(contains(paused.output, "\"status_detail\":\"pause_session "
                                    "requested by operator; active runtime "
                                    "campaign stop in progress\""));
    REQUIRE(contains(paused.output, std::string("\"campaign_cursor\":\"") +
                                        campaign_cursor + "\""));
    REQUIRE(contains(read_text_file(runtime_trace),
                     "\"suppress_marshal_session_update\":true"));

    const std::string resume_args =
        std::string("{\"marshal_session_id\":\"") + session_id + "\"}";
    const command_result_t rejected = run_command(
        direct_tool_command(kMarshalMcpBin, global_config, marshal_dsl,
                            "hero.marshal.resume_session", resume_args));
    REQUIRE(rejected.exit_code != 0);
    REQUIRE(contains(
        rejected.output,
        "resume_session cannot reopen an operator-paused session while the "
        "retained active runtime campaign stop is still in progress"));
  }

  mark_runtime_campaign_terminal(runtime_root, global_config, session_id,
                                 campaign_cursor);
  const std::string get_args =
      std::string("{\"marshal_session_id\":\"") + session_id + "\"}";
  const command_result_t reconciled = run_command(
      direct_tool_command(kMarshalMcpBin, global_config, marshal_dsl,
                          "hero.marshal.get_session", get_args));
  REQUIRE(reconciled.exit_code == 0);
  REQUIRE(contains(reconciled.output, "\"reconciled\":true"));
  REQUIRE(contains(reconciled.output, "\"lifecycle\":\"live\""));
  REQUIRE(contains(reconciled.output, "\"work_gate\":\"operator_pause\""));
  REQUIRE(
      contains(reconciled.output,
               "\"status_detail\":\"pause_session requested by operator\""));
  REQUIRE(contains(reconciled.output, "\"campaign_cursor\":\"\""));
}

static void
test_marshal_terminate_session_retains_active_campaign_until_runtime_stop_finishes() {
  temp_dir_t tmp("test_marshal_terminate_session_retains_active_campaign");
  const fs::path cfg_dir = tmp.dir / "cfg";
  const fs::path runtime_root = tmp.dir / "runtime";
  const fs::path global_config = cfg_dir / "global.config";
  const fs::path marshal_dsl = cfg_dir / "default.hero.marshal.dsl";
  const fs::path human_identities = cfg_dir / "human_operator_identities";
  const fs::path runtime_stub = cfg_dir / "runtime_stub.sh";
  const fs::path runtime_trace = cfg_dir / "runtime_stop_args.json";
  const std::string session_id = "marshal.terminate.pending";
  const std::string campaign_cursor = "campaign.terminate.pending";
  const fs::path session_hero_marshal_dsl =
      runtime_root / ".marshal_hero" / session_id / "hero.marshal.dsl";

  write_text_file(human_identities, "# smoke-only\n");
  const std::string runtime_stub_text = std::string(
                                            R"RUNTIME(#!/usr/bin/env bash
set -euo pipefail
trace_file=")RUNTIME") + runtime_trace.string() +
                                        R"RUNTIME("
tool_name=""
args_json="{}"
for ((i=1;i<=$#;i++)); do
  arg="${!i}"
  if [[ "$arg" == "--tool" ]]; then
    j=$((i+1))
    tool_name="${!j}"
  fi
  if [[ "$arg" == "--args-json" ]]; then
    j=$((i+1))
    args_json="${!j}"
  fi
done
if [[ "$tool_name" != "hero.runtime.stop_campaign" ]]; then
  printf '{"content":[{"type":"text","text":"unsupported tool"}],"structuredContent":{},"isError":true}\n'
  exit 1
fi
printf '%s' "$args_json" > "$trace_file"
grep -F '"suppress_marshal_session_update":true' <<<"$args_json" >/dev/null || {
  printf '{"content":[{"type":"text","text":"missing suppress flag"}],"structuredContent":{},"isError":true}\n'
  exit 2
}
campaign_cursor="$(sed -n 's/.*"campaign_cursor":"\([^"]*\)".*/\1/p' <<<"$args_json" | head -n 1)"
if [[ -z "$campaign_cursor" ]]; then
  printf '{"content":[{"type":"text","text":"missing campaign cursor"}],"structuredContent":{},"isError":true}\n'
  exit 3
fi
printf '{"content":[{"type":"text","text":"ok"}],"structuredContent":{"campaign_cursor":"%s","campaign":{"schema":"hero.runtime.campaign.v2","campaign_cursor":"%s","state":"stopping","state_detail":"stub stop in progress","marshal_session_id":"marshal.terminate.pending"},"warnings":[]},"isError":false}\n' \
  "$campaign_cursor" "$campaign_cursor"
)RUNTIME";
  write_executable_file(runtime_stub, runtime_stub_text);

  write_text_file(
      global_config,
      std::string("[GENERAL]\n") + "runtime_root=" + runtime_root.string() +
          "\nrepo_root=/cuwacunu\n"
          "default_iitepi_campaign_dsl_filename=/cuwacunu/src/config/"
          "instructions/defaults/default.iitepi.campaign.dsl\n"
          "\n[BNF]\n"
          "iitepi_campaign_grammar_filename=/cuwacunu/src/config/bnf/"
          "iitepi.campaign.bnf\n"
          "marshal_objective_grammar_filename=/cuwacunu/src/config/bnf/"
          "objective.marshal.bnf\n"
          "\n[REAL_HERO]\n"
          "marshal_hero_dsl_filename=" +
          marshal_dsl.string() + "\n");
  const std::string marshal_dsl_text =
      std::string("runtime_hero_binary:path = ") + runtime_stub.string() +
      "\n"
      "config_hero_binary:path = /bin/true\n"
      "lattice_hero_binary:path = /bin/true\n"
      "human_hero_binary:path = /bin/true\n"
      "human_operator_identities:path = " +
      human_identities.string() + "\n" +
      "config_scope_root:path = /cuwacunu/src/config\n"
      "marshal_codex_binary:path = /bin/true\n"
      "marshal_codex_model:str = gpt-5.3-codex-spark\n"
      "marshal_codex_reasoning_effort:str = xhigh\n"
      "tail_default_lines(1,+inf):int = 20\n"
      "poll_interval_ms(100,+inf):int = 100\n"
      "marshal_codex_timeout_sec(1,+inf):int = 5\n"
      "marshal_max_campaign_launches(1,+inf):int = 2\n";
  write_text_file(marshal_dsl, marshal_dsl_text);
  write_text_file(session_hero_marshal_dsl, marshal_dsl_text);

  const live_process_identity_t current_identity = current_process_identity();
  write_manual_running_campaign_fixture(runtime_root, global_config,
                                        session_hero_marshal_dsl, session_id,
                                        campaign_cursor, current_identity);
  {
    const held_file_lock_t session_runner_lock(
        cuwacunu::hero::marshal::marshal_session_runner_lock_path(
            runtime_root / ".marshal_hero", session_id));

    const std::string terminate_args =
        std::string("{\"marshal_session_id\":\"") + session_id +
        "\",\"force\":false}";
    const command_result_t terminated = run_command(
        direct_tool_command(kMarshalMcpBin, global_config, marshal_dsl,
                            "hero.marshal.terminate_session", terminate_args));
    REQUIRE(terminated.exit_code == 0);
    REQUIRE(contains(terminated.output, "\"isError\":false"));
    REQUIRE(contains(terminated.output, "\"lifecycle\":\"terminal\""));
    REQUIRE(contains(terminated.output, "\"finish_reason\":\"terminated\""));
    REQUIRE(contains(
        terminated.output,
        "\"status_detail\":\"terminate_session requested by operator; active "
        "runtime campaign stop in progress\""));
    REQUIRE(contains(terminated.output, std::string("\"campaign_cursor\":\"") +
                                            campaign_cursor + "\""));
    REQUIRE(contains(read_text_file(runtime_trace),
                     "\"suppress_marshal_session_update\":true"));
  }

  mark_runtime_campaign_terminal(runtime_root, global_config, session_id,
                                 campaign_cursor);
  const std::string get_args =
      std::string("{\"marshal_session_id\":\"") + session_id + "\"}";
  const command_result_t reconciled = run_command(
      direct_tool_command(kMarshalMcpBin, global_config, marshal_dsl,
                          "hero.marshal.get_session", get_args));
  REQUIRE(reconciled.exit_code == 0);
  REQUIRE(contains(reconciled.output, "\"reconciled\":true"));
  REQUIRE(contains(reconciled.output, "\"lifecycle\":\"terminal\""));
  REQUIRE(contains(reconciled.output, "\"finish_reason\":\"terminated\""));
  REQUIRE(contains(
      reconciled.output,
      "\"status_detail\":\"terminate_session requested by operator\""));
  REQUIRE(contains(reconciled.output, "\"campaign_cursor\":\"\""));
}

static void test_marshal_session_reconcile_rebooted_running_campaign() {
  temp_dir_t tmp("test_marshal_session_reconcile_rebooted_running_campaign");
  const fs::path cfg_dir = tmp.dir / "cfg";
  const fs::path runtime_root = tmp.dir / "runtime";
  const fs::path global_config = cfg_dir / "global.config";
  const fs::path marshal_dsl = cfg_dir / "default.hero.marshal.dsl";
  const fs::path human_identities = cfg_dir / "human_operator_identities";
  const fs::path marshal_root = runtime_root / ".marshal_hero";
  const fs::path campaigns_root = runtime_root / ".campaigns";
  const std::string session_id = "marshal.rebooted";
  const std::string campaign_cursor = "campaign.rebooted";
  const std::uint64_t now_ms = cuwacunu::hero::runtime::now_ms_utc();

  write_text_file(human_identities, "# smoke-only\n");
  write_text_file(
      global_config,
      std::string("[GENERAL]\n") + "runtime_root=" + runtime_root.string() +
          "\nrepo_root=/cuwacunu\n"
          "default_iitepi_campaign_dsl_filename=/cuwacunu/src/config/"
          "instructions/defaults/default.iitepi.campaign.dsl\n"
          "\n[BNF]\n"
          "iitepi_campaign_grammar_filename=/cuwacunu/src/config/bnf/"
          "iitepi.campaign.bnf\n"
          "marshal_objective_grammar_filename=/cuwacunu/src/config/bnf/"
          "objective.marshal.bnf\n"
          "\n[REAL_HERO]\n"
          "marshal_hero_dsl_filename=" +
          marshal_dsl.string() + "\n");
  write_text_file(marshal_dsl,
                  std::string("runtime_hero_binary:path = /bin/true\n") +
                      "config_hero_binary:path = /bin/true\n"
                      "lattice_hero_binary:path = /bin/true\n"
                      "human_hero_binary:path = /bin/true\n"
                      "human_operator_identities:path = " +
                      human_identities.string() + "\n" +
                      "config_scope_root:path = /cuwacunu/src/config\n"
                      "marshal_codex_binary:path = /bin/true\n"
                      "marshal_codex_model:str = gpt-5.3-codex-spark\n"
                      "marshal_codex_reasoning_effort:str = xhigh\n"
                      "tail_default_lines(1,+inf):int = 20\n"
                      "poll_interval_ms(100,+inf):int = 100\n"
                      "marshal_codex_timeout_sec(1,+inf):int = 5\n"
                      "marshal_max_campaign_launches(1,+inf):int = 2\n");

  fs::create_directories(marshal_root / session_id / "logs");
  fs::create_directories(marshal_root / session_id / "human");
  fs::create_directories(marshal_root / session_id / "checkpoints");
  fs::create_directories(campaigns_root / campaign_cursor / "jobs");

  cuwacunu::hero::marshal::marshal_session_record_t session{};
  session.schema =
      std::string(cuwacunu::hero::marshal::kMarshalSessionSchemaV6);
  session.marshal_session_id = session_id;
  session.lifecycle = "live";
  session.status_detail = "campaign launched from marshal planning checkpoint";
  session.work_gate = "open";
  session.activity = "awaiting_campaign_fact";
  session.finish_reason = "none";
  session.objective_name = "runtime.operative.vicreg.solo.train";
  session.global_config_path = global_config.string();
  session.boot_id = "stale-boot-id";
  session.runner_pid = 999999;
  session.runner_start_ticks = 1;
  session.source_marshal_objective_dsl_path =
      "/cuwacunu/src/config/instructions/objectives/"
      "runtime.operative.vicreg.solo.train/"
      ".marshal.dsl";
  session.source_campaign_dsl_path =
      "/cuwacunu/src/config/instructions/objectives/"
      "runtime.operative.vicreg.solo.train/"
      "iitepi.campaign.dsl";
  session.source_marshal_objective_md_path =
      "/cuwacunu/src/config/instructions/objectives/"
      "runtime.operative.vicreg.solo.train/"
      "vicreg.solo.train.objective.md";
  session.source_marshal_guidance_md_path =
      "/cuwacunu/src/config/instructions/defaults/"
      "default.runtime.operative.guidance.md";
  session.session_root = (marshal_root / session_id).string();
  session.objective_root = "/cuwacunu/src/config/instructions/objectives/"
                           "runtime.operative.vicreg.solo.train";
  session.campaign_dsl_path = session.source_campaign_dsl_path;
  session.marshal_objective_dsl_path =
      (marshal_root / session_id / "marshal.objective.dsl").string();
  session.marshal_objective_md_path =
      (marshal_root / session_id / "marshal.objective.md").string();
  session.marshal_guidance_md_path =
      (marshal_root / session_id / "marshal.guidance.md").string();
  session.hero_marshal_dsl_path =
      (marshal_root / session_id / "hero.marshal.dsl").string();
  session.config_policy_path =
      (marshal_root / session_id / "config.hero.policy.dsl").string();
  session.briefing_path =
      (marshal_root / session_id / "marshal.briefing.md").string();
  session.memory_path =
      (marshal_root / session_id / "marshal.session.memory.md").string();
  session.human_request_path =
      (marshal_root / session_id / "human" / "request.latest.md").string();
  session.events_path =
      (marshal_root / session_id / "marshal.session.events.jsonl").string();
  session.turns_path =
      (marshal_root / session_id / "marshal.session.turns.jsonl").string();
  session.codex_stdout_path =
      (marshal_root / session_id / "logs" / "codex.session.stdout.jsonl")
          .string();
  session.codex_stderr_path =
      (marshal_root / session_id / "logs" / "codex.session.stderr.jsonl")
          .string();
  session.resolved_marshal_codex_binary = "/bin/true";
  session.resolved_marshal_codex_model = "gpt-5.3-codex-spark";
  session.resolved_marshal_codex_reasoning_effort = "xhigh";
  session.started_at_ms = now_ms;
  session.updated_at_ms = now_ms;
  session.checkpoint_count = 1;
  session.launch_count = 1;
  session.max_campaign_launches = 4;
  session.remaining_campaign_launches = 3;
  session.authority_scope = "objective_only";
  session.campaign_status = "running";
  session.campaign_cursor = campaign_cursor;
  session.last_codex_action = "launch_campaign";
  session.last_input_checkpoint_path =
      (marshal_root / session_id / "checkpoints" / "input.0001.json").string();
  session.campaign_cursors.push_back(campaign_cursor);
  std::string session_error{};
  REQUIRE(cuwacunu::hero::marshal::write_marshal_session_record(
      marshal_root, session, &session_error));

  cuwacunu::hero::runtime::runtime_campaign_record_t campaign{};
  campaign.campaign_cursor = campaign_cursor;
  campaign.boot_id = "stale-boot-id";
  campaign.state = "running";
  campaign.state_detail = "campaign runner alive";
  campaign.marshal_session_id = session_id;
  campaign.global_config_path = global_config.string();
  campaign.source_campaign_dsl_path = session.source_campaign_dsl_path;
  campaign.campaign_dsl_path =
      (campaigns_root / campaign_cursor / "campaign.dsl").string();
  campaign.started_at_ms = now_ms;
  campaign.updated_at_ms = now_ms;
  campaign.runner_pid = 999998;
  campaign.runner_start_ticks = 1;
  std::string campaign_error{};
  REQUIRE(cuwacunu::hero::runtime::write_runtime_campaign_record(
      campaigns_root, campaign, &campaign_error));

  const std::string get_args =
      std::string("{\"marshal_session_id\":\"") + session_id + "\"}";
  const command_result_t get = run_command(
      direct_tool_command(kMarshalMcpBin, global_config, marshal_dsl,
                          "hero.marshal.get_session", get_args));
  REQUIRE(get.exit_code == 0);
  REQUIRE(contains(get.output, "\"isError\":false"));
  REQUIRE(contains(get.output, "\"reconciled\":true"));
  REQUIRE(contains(get.output, "\"schema\":\"hero.marshal.session.v6\""));
  REQUIRE(contains(get.output, "\"lifecycle\":\"live\""));
  REQUIRE(contains(get.output, "\"activity\":\"review\""));
  REQUIRE(contains(get.output, "\"finish_reason\":\"interrupted\""));
  REQUIRE(contains(get.output, campaign_cursor));
  REQUIRE(contains(get.output, "campaign runner is no longer alive"));

  const command_result_t list = run_command(
      direct_tool_command(kMarshalMcpBin, global_config, marshal_dsl,
                          "hero.marshal.list_sessions", "{}"));
  REQUIRE(list.exit_code == 0);
  REQUIRE(contains(list.output, "\"isError\":false"));
  REQUIRE(contains(list.output, session_id));
  REQUIRE(contains(list.output, "\"lifecycle\":\"live\""));
  REQUIRE(contains(list.output, "\"activity\":\"review\""));
  REQUIRE(contains(list.output, "\"finish_reason\":\"interrupted\""));

  const std::string manifest_text = read_text_file(
      marshal_root / session_id / "marshal.session.manifest.lls");
  REQUIRE(contains(manifest_text, "schema:str = hero.marshal.session.v6"));
  REQUIRE(contains(manifest_text, "lifecycle:str = live"));
  REQUIRE(contains(manifest_text, "activity:str = review"));
  REQUIRE(contains(manifest_text, "finish_reason:str = interrupted"));
  REQUIRE(!contains(manifest_text,
                    std::string("campaign_cursor:str = ") + campaign_cursor));
}

static void
test_marshal_session_canonicalize_clears_stale_campaign_status_without_cursor() {
  cuwacunu::hero::marshal::marshal_session_record_t review_ready{};
  review_ready.lifecycle = "live";
  review_ready.activity = "review";
  review_ready.work_gate = "open";
  review_ready.campaign_status = "running";
  cuwacunu::hero::marshal::canonicalize_marshal_session_record(&review_ready);
  REQUIRE(review_ready.campaign_status == "none");

  cuwacunu::hero::marshal::marshal_session_record_t work_blocked{};
  work_blocked.lifecycle = "live";
  work_blocked.activity = "quiet";
  work_blocked.work_gate = "clarification";
  work_blocked.campaign_status = "stopping";
  cuwacunu::hero::marshal::canonicalize_marshal_session_record(&work_blocked);
  REQUIRE(work_blocked.campaign_status == "none");

  cuwacunu::hero::marshal::marshal_session_record_t terminal{};
  terminal.lifecycle = "terminal";
  terminal.activity = "quiet";
  terminal.work_gate = "open";
  terminal.campaign_status = "running";
  cuwacunu::hero::marshal::canonicalize_marshal_session_record(&terminal);
  REQUIRE(terminal.campaign_status == "none");

  cuwacunu::hero::marshal::marshal_session_record_t planning{};
  planning.lifecycle = "live";
  planning.activity = "planning";
  planning.work_gate = "open";
  planning.campaign_status = "running";
  cuwacunu::hero::marshal::canonicalize_marshal_session_record(&planning);
  REQUIRE(planning.campaign_status == "running");
}

int main() {
  test_hashimyei_tools_list_surface();
  test_lattice_tools_list_surface();
  test_marshal_tools_list_surface();
  test_human_tools_list_surface();
  test_hashimyei_reset_catalog_smoke();
  test_hashimyei_list_direct_tool_skips_ingest_rebuild();
  test_lattice_reset_catalog_smoke();
  test_lattice_removed_tool_rejected();
  test_marshal_session_continue_smoke();
  test_marshal_run_plan_replans_between_bind_steps();
  test_marshal_resume_rejects_mismatched_clarification_artifact();
  test_marshal_session_failed_checkpoint_idles_and_continues();
  test_marshal_pause_session_retains_active_campaign_until_runtime_stop_finishes();
  test_marshal_terminate_session_retains_active_campaign_until_runtime_stop_finishes();
  test_marshal_session_reconcile_rebooted_running_campaign();
  test_marshal_session_canonicalize_clears_stale_campaign_status_without_cursor();
  std::cout << "[OK] hero mcp tool surface and smoke checks passed\n";
  return 0;
}
