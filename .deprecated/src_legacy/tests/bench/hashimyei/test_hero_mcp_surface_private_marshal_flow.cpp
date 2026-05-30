static void test_marshal_session_continue_smoke() {
  temp_dir_t tmp("test_marshal_session_continue");
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
schema_path=""
resume_mode=0
expect_model="gpt-5.4-mini"
expect_effort='model_reasoning_effort="high"'
expect_runtime='mcp_servers.hero-runtime.command="/bin/true"'
expect_config='mcp_servers.hero-config.command="/bin/echo"'
expect_lattice='mcp_servers.hero-lattice.command="/bin/cat"'
have_model=0
have_effort=0
have_json=0
have_runtime=0
have_config=0
have_lattice=0
for ((i=1;i<=$#;i++)); do
  arg="${!i}"
  if [[ "$arg" == "resume" ]]; then resume_mode=1; fi
  if [[ "$arg" == "--json" ]]; then have_json=1; fi
  if [[ "$arg" == "-o" ]]; then
    j=$((i+1))
    decision_path="${!j}"
  fi
  if [[ "$arg" == "--output-schema" ]]; then
    j=$((i+1))
    schema_path="${!j}"
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
    if [[ "${!j}" == "$expect_runtime" ]]; then
      have_runtime=1
    fi
    if [[ "${!j}" == "$expect_config" ]]; then
      have_config=1
    fi
    if [[ "${!j}" == "$expect_lattice" ]]; then
      have_lattice=1
    fi
  fi
done
if [[ -z "$decision_path" ]]; then
  echo "missing -o decision path" >&2
  exit 2
fi
if [[ "$have_model" != "1" || "$have_effort" != "1" || "$have_runtime" != "1" || "$have_config" != "1" || "$have_lattice" != "1" ]]; then
  echo "missing pinned codex or MCP command arguments" >&2
  exit 4
fi
if [[ "$have_json" != "1" ]]; then
  echo "missing codex --json argument" >&2
  exit 4
fi
if [[ -n "$schema_path" ]] && grep -Eq ',[[:space:]]*[}\]]' "$schema_path"; then
  echo "invalid output schema json" >&2
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
{"intent":"pause_for_clarification","reply_text":"","launch":null,"clarification_request":{"request":"Provide the missing clarification."},"governance":null,"reason":"need clarification","memory_note":"asked for clarification"}
JSON
  echo 1 > "$state_file"
  exit 0
fi
if [[ "$resume_mode" == "1" && "$step" == "1" ]]; then
  cat > "$decision_path" <<'JSON'
{"intent":"complete","reply_text":"","launch":null,"clarification_request":null,"governance":null,"reason":"objective satisfied after clarification","memory_note":"completed after clarification"}
JSON
  echo 2 > "$state_file"
  exit 0
fi
if [[ "$resume_mode" == "1" && "$step" == "2" ]]; then
  sleep 2
  cat > "$decision_path" <<'JSON'
{"intent":"complete","reply_text":"","launch":null,"clarification_request":null,"governance":null,"reason":"objective satisfied after continuation","memory_note":"completed after continuation"}
JSON
  echo 3 > "$state_file"
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
          "config_scope_root:path = /cuwacunu/src/config\n"
          "marshal_codex_binary:path = " +
          codex_stub.string() +
          "\n"
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
      source_objective_dsl.string() +
      "\",\"marshal_codex_model\":\"gpt-5.4-mini\","
      "\"marshal_codex_reasoning_effort\":\"high\"}";
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
  const command_result_t paused =
      wait_for_command(get_command, [](const command_result_t &result) {
        return result.exit_code == 0 &&
               contains(result.output, "\"lifecycle\":\"live\"") &&
               contains(result.output, "\"work_gate\":\"clarification\"");
      });
  REQUIRE(paused.exit_code == 0);
  const std::string paused_codex_session =
      extract_json_string_field(paused.output, "current_thread_id");
  REQUIRE(paused_codex_session == kSmokeCodexSessionId);
  REQUIRE(extract_json_string_field(
              paused.output, "resolved_marshal_codex_model") == "gpt-5.4-mini");
  REQUIRE(extract_json_string_field(
              paused.output, "resolved_marshal_codex_reasoning_effort") ==
          "high");

  const command_result_t pending_requests =
      run_command(direct_tool_command(kHumanMcpBin, global_config, human_dsl,
                                      "hero.human.list_requests", "{}"));
  REQUIRE(pending_requests.exit_code == 0);
  REQUIRE(contains(pending_requests.output, "\"isError\":false"));
  REQUIRE(contains(pending_requests.output, session_id));

  const command_result_t human_request =
      run_command(direct_tool_command(kHumanMcpBin, global_config, human_dsl,
                                      "hero.human.get_request", get_args));
  REQUIRE(human_request.exit_code == 0);
  REQUIRE(contains(human_request.output, "\"isError\":false"));
  REQUIRE(
      contains(human_request.output,
               std::string("\"marshal_session_id\":\"") + session_id + "\""));

  const std::string answer_args =
      std::string("{\"marshal_session_id\":\"") + session_id +
      "\",\"answer\":\"Here is the clarification.\"}";
  const command_result_t answered = run_command(
      direct_tool_command(kHumanMcpBin, global_config, human_dsl,
                          "hero.human.answer_request", answer_args));
  REQUIRE(answered.exit_code == 0);
  REQUIRE(contains(answered.output, "\"isError\":false"));

  const command_result_t idled =
      wait_for_command(get_command, [](const command_result_t &result) {
        return result.exit_code == 0 &&
               contains(result.output, "\"lifecycle\":\"live\"") &&
               contains(result.output, "\"activity\":\"review\"") &&
               contains(result.output, "\"finish_reason\":\"success\"");
      });
  REQUIRE(idled.exit_code == 0);
  REQUIRE(contains(idled.output, "\"checkpoint_count\":2"));
  const std::string idled_codex_session =
      extract_json_string_field(idled.output, "current_thread_id");
  REQUIRE(idled_codex_session == paused_codex_session);

  const command_result_t pending_summaries =
      run_command(direct_tool_command(kHumanMcpBin, global_config, human_dsl,
                                      "hero.human.list_summaries", "{}"));
  REQUIRE(pending_summaries.exit_code == 0);
  REQUIRE(contains(pending_summaries.output, "\"isError\":false"));
  REQUIRE(contains(pending_summaries.output, session_id));

  const command_result_t human_get =
      run_command(direct_tool_command(kHumanMcpBin, global_config, human_dsl,
                                      "hero.human.get_summary", get_args));
  REQUIRE(human_get.exit_code == 0);
  REQUIRE(contains(human_get.output, "\"isError\":false"));
  REQUIRE(contains(human_get.output, "\"lifecycle\":\"live\""));
  REQUIRE(contains(human_get.output, "\"activity\":\"review\""));

  write_text_file(
      marshal_dsl,
      std::string("runtime_hero_binary:path = /bin/false\n") +
          "config_hero_binary:path = /bin/sh\n"
          "lattice_hero_binary:path = /usr/bin/env\n"
          "human_hero_binary:path = /cuwacunu/.build/hero/hero_human.mcp\n"
          "human_operator_identities:path = " +
          human_identities.string() + "\n" +
          "config_scope_root:path = /cuwacunu/src/config\n"
          "marshal_codex_binary:path = /bin/false\n"
          "marshal_codex_model:str = broken-model\n"
          "marshal_codex_reasoning_effort:str = low\n"
          "tail_default_lines(1,+inf):int = 3\n"
          "poll_interval_ms(100,+inf):int = 9000\n"
          "marshal_codex_timeout_sec(1,+inf):int = 1\n"
          "marshal_max_campaign_launches(1,+inf):int = 9\n");

  const std::string continue_args = std::string("{\"marshal_session_id\":\"") +
                                    session_id +
                                    "\",\"message\":\"Train for 10 more "
                                    "epochs from the latest checkpoint.\"}";
  const command_result_t continued = run_command(
      direct_tool_command(kMarshalMcpBin, global_config, marshal_dsl,
                          "hero.marshal.message_session", continue_args));
  REQUIRE(continued.exit_code == 0);
  REQUIRE(contains(continued.output, "\"isError\":false"));

  const command_result_t idled_again =
      wait_for_command(get_command, [](const command_result_t &result) {
        return result.exit_code == 0 &&
               contains(result.output, "\"lifecycle\":\"live\"") &&
               contains(result.output, "\"activity\":\"review\"") &&
               contains(result.output, "\"finish_reason\":\"success\"") &&
               contains(result.output, "\"checkpoint_count\":3");
      });
  REQUIRE(idled_again.exit_code == 0);
  const std::string continued_codex_session =
      extract_json_string_field(idled_again.output, "current_thread_id");
  REQUIRE(continued_codex_session == idled_codex_session);

  const fs::path session_root = runtime_root / ".marshal_hero" / session_id;
  const fs::path hero_marshal_dsl = session_root / "hero.marshal.dsl";
  const fs::path codex_stdout_log =
      session_root / "logs" / "codex.session.stdout.jsonl";
  const fs::path codex_stderr_log =
      session_root / "logs" / "codex.session.stderr.jsonl";
  REQUIRE(fs::exists(hero_marshal_dsl));
  REQUIRE(contains(read_text_file(hero_marshal_dsl), "repo_root:path = "));
  REQUIRE(contains(read_text_file(hero_marshal_dsl),
                   "runtime_hero_binary:path = /bin/true"));
  REQUIRE(contains(read_text_file(hero_marshal_dsl),
                   "config_hero_binary:path = /bin/echo"));
  REQUIRE(contains(read_text_file(hero_marshal_dsl),
                   "lattice_hero_binary:path = /bin/cat"));
  REQUIRE(contains(read_text_file(hero_marshal_dsl),
                   "marshal_codex_model:str = gpt-5.4-mini"));
  REQUIRE(contains(read_text_file(hero_marshal_dsl),
                   "marshal_codex_reasoning_effort:str = high"));
  REQUIRE(contains(read_text_file(hero_marshal_dsl),
                   "marshal_codex_timeout_sec(1,+inf):int = 5"));
  REQUIRE(fs::exists(codex_stdout_log));
  REQUIRE(fs::exists(codex_stderr_log));
  REQUIRE(!fs::exists(session_root / "logs" / "codex.session.log"));
  REQUIRE(
      contains(read_text_file(codex_stdout_log), "\"type\":\"stub_event\""));
  REQUIRE(contains(read_text_file(codex_stderr_log), "\"stream\":\"stderr\""));
  REQUIRE(contains(read_text_file(codex_stderr_log), kSmokeCodexSessionId));
  const std::string events_text =
      wait_for_file_containing(session_root / "marshal.session.events.jsonl",
                               "\"type\":\"operator.message_handled\"");
  REQUIRE(contains(events_text, "\"type\":\"operator.message_received\""));
  REQUIRE(contains(events_text, "\"type\":\"operator.message_delivered\""));
  REQUIRE(contains(events_text, "\"type\":\"operator.message_handled\""));
  REQUIRE(count_occurrences(events_text, "\"type\":\"checkpoint.staged\"") >=
          3);

  const std::string memory_text =
      read_text_file(session_root / "marshal.session.memory.md");
  REQUIRE(contains(memory_text, "## Checkpoint 2"));
  REQUIRE(contains(memory_text, "## Checkpoint 3"));
}

static void test_marshal_run_plan_replans_between_bind_steps() {
  temp_dir_t tmp("test_marshal_run_plan_replans_between_bind_steps");
  const fs::path cfg_dir = tmp.dir / "cfg";
  const fs::path runtime_root = tmp.dir / "runtime";
  const fs::path global_config = cfg_dir / "global.config";
  const fs::path marshal_dsl = cfg_dir / "default.hero.marshal.dsl";
  const fs::path human_dsl = cfg_dir / "default.hero.human.dsl";
  const fs::path codex_stub = cfg_dir / "codex_stub.sh";
  const fs::path runtime_stub = cfg_dir / "runtime_stub.sh";
  const fs::path human_identities = cfg_dir / "human_operator_identities";
  const fs::path source_objective_dsl =
      "/cuwacunu/src/config/instructions/objectives/"
      "runtime.operative.vicreg.solo.train/"
      ".marshal.dsl";

  write_text_file(human_identities, "# smoke-only\n");
  const std::string runtime_stub_text = std::string(
                                            R"RUNTIME(#!/usr/bin/env bash
set -euo pipefail
campaigns_root=")RUNTIME") + (runtime_root / ".campaigns").string() +
                                        R"RUNTIME("
tool_name=""
args_json="{}"
global_config=""
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
  if [[ "$arg" == "--global-config" ]]; then
    j=$((i+1))
    global_config="${!j}"
  fi
done
if [[ "$tool_name" != "hero.runtime.start_campaign" ]]; then
  printf '{"content":[{"type":"text","text":"unsupported tool"}],"structuredContent":{},"isError":true}\n'
  exit 1
fi
binding_id="$(sed -n 's/.*"binding_id":"\([^"]*\)".*/\1/p' <<<"$args_json" | head -n 1)"
marshal_session_id="$(sed -n 's/.*"marshal_session_id":"\([^"]*\)".*/\1/p' <<<"$args_json" | head -n 1)"
campaign_dsl_path="$(sed -n 's/.*"campaign_dsl_path":"\([^"]*\)".*/\1/p' <<<"$args_json" | head -n 1)"
reset_runtime_state=false
grep -F '"reset_runtime_state":true' <<<"$args_json" >/dev/null && reset_runtime_state=true
if [[ -z "$binding_id" || -z "$marshal_session_id" || -z "$campaign_dsl_path" || -z "$global_config" ]]; then
  printf '{"content":[{"type":"text","text":"missing required start_campaign field"}],"structuredContent":{},"isError":true}\n'
  exit 1
fi
state_file="$(dirname "$0")/runtime_stub.state"
count=0
if [[ -f "$state_file" ]]; then
  count="$(cat "$state_file")"
fi
count=$((count+1))
printf '%s' "$count" > "$state_file"
campaign_cursor="$(printf 'campaign.stub.%04d' "$count")"
campaign_dir="$campaigns_root/$campaign_cursor"
mkdir -p "$campaign_dir"
cp "$campaign_dsl_path" "$campaign_dir/campaign.dsl"
started_at="$((1000 + count))"
cat > "$campaign_dir/runtime.campaign.manifest.lls" <<EOF
schema:str = hero.runtime.campaign.v2
campaign_cursor:str = $campaign_cursor
boot_id:str = stub-boot
state:str = exited
state_detail:str = campaign completed
marshal_session_id:str = $marshal_session_id
global_config_path:str = $global_config
source_campaign_dsl_path:str = $campaign_dsl_path
campaign_dsl_path:str = $campaign_dir/campaign.dsl
reset_runtime_state:bool = $reset_runtime_state
stdout_path:str = $campaign_dir/stdout.log
stderr_path:str = $campaign_dir/stderr.log
started_at_ms(0,+inf):uint = $started_at
updated_at_ms(0,+inf):uint = $started_at
finished_at_ms(0,+inf):uint = $started_at
current_run_index(0,+inf):uint = 1
run_bind_id.0000:str = $binding_id
EOF
: > "$campaign_dir/stdout.log"
: > "$campaign_dir/stderr.log"
printf '{"content":[{"type":"text","text":"ok"}],"structuredContent":{"campaign_cursor":"%s","campaign":{"schema":"hero.runtime.campaign.v2","campaign_cursor":"%s","state":"exited","state_detail":"campaign completed","marshal_session_id":"%s","run_bind_ids":["%s"],"current_run_index":1},"warnings":[]},"isError":false}\n' \
  "$campaign_cursor" "$campaign_cursor" "$marshal_session_id" "$binding_id"
)RUNTIME";
  write_executable_file(runtime_stub, runtime_stub_text);

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
briefing="$(cat)"
printf 'session id: %s\n' ")CODEX") + kSmokeCodexSessionId +
                                      R"CODEX(" >&2
step=0
if [[ -f "$state_file" ]]; then
  step="$(cat "$state_file")"
fi
case "$step" in
  0)
    [[ "$resume_mode" == "0" ]] || { echo "expected fresh checkpoint" >&2; exit 3; }
    grep -F '"checkpoint_kind":"bootstrap"' <<<"$briefing" >/dev/null || { echo "missing bootstrap checkpoint" >&2; exit 4; }
    grep -F '"run_plan_progress":{' <<<"$briefing" >/dev/null || { echo "missing run plan progress" >&2; exit 4; }
    grep -F '"next_pending_bind_id":"bind_train_vicreg_primary_btcusdt"' <<<"$briefing" >/dev/null || { echo "missing first pending bind" >&2; exit 4; }
    cat > "$decision_path" <<'JSON'
{"intent":"launch_campaign","reply_text":"","launch":{"mode":"run_plan","binding_id":null,"reset_runtime_state":false,"requires_objective_mutation":false},"clarification_request":null,"governance":null,"reason":"launch the first pending RUN step","memory_note":"launch first pending bind"}
JSON
    echo 1 > "$state_file"
    exit 0
    ;;
  1)
    grep -F '"checkpoint_kind":"postcampaign"' <<<"$briefing" >/dev/null || { echo "missing postcampaign checkpoint 1" >&2; exit 4; }
    grep -F '"completed_run_bind_ids":["bind_train_vicreg_primary_btcusdt"]' <<<"$briefing" >/dev/null || { echo "missing completed bind 1" >&2; exit 4; }
    grep -F '"next_pending_bind_id":"bind_eval_vicreg_payload_btcusdt"' <<<"$briefing" >/dev/null || { echo "missing second pending bind" >&2; exit 4; }
    cat > "$decision_path" <<'JSON'
{"intent":"launch_campaign","reply_text":"","launch":{"mode":"run_plan","binding_id":null,"reset_runtime_state":false,"requires_objective_mutation":false},"clarification_request":null,"governance":null,"reason":"launch the second pending RUN step","memory_note":"launch second pending bind"}
JSON
    echo 2 > "$state_file"
    exit 0
    ;;
  2)
    grep -F '"completed_run_bind_ids":["bind_train_vicreg_primary_btcusdt","bind_eval_vicreg_payload_btcusdt"]' <<<"$briefing" >/dev/null || { echo "missing completed binds 1-2" >&2; exit 4; }
    grep -F '"next_pending_bind_id":"bind_eval_vicreg_payload_btcusdt_untouched_test"' <<<"$briefing" >/dev/null || { echo "missing third pending bind" >&2; exit 4; }
    cat > "$decision_path" <<'JSON'
{"intent":"launch_campaign","reply_text":"","launch":{"mode":"run_plan","binding_id":null,"reset_runtime_state":false,"requires_objective_mutation":false},"clarification_request":null,"governance":null,"reason":"launch the final pending RUN step","memory_note":"launch third pending bind"}
JSON
    echo 3 > "$state_file"
    exit 0
    ;;
  3)
    grep -F '"completed_run_bind_ids":["bind_train_vicreg_primary_btcusdt","bind_eval_vicreg_payload_btcusdt","bind_eval_vicreg_payload_btcusdt_untouched_test"]' <<<"$briefing" >/dev/null || { echo "missing completed binds 1-3" >&2; exit 4; }
    grep -F '"next_pending_bind_id":null' <<<"$briefing" >/dev/null || { echo "missing null next pending bind" >&2; exit 4; }
    cat > "$decision_path" <<'JSON'
{"intent":"complete","reply_text":"","launch":null,"clarification_request":null,"governance":null,"reason":"all pending RUN steps completed with replanning between binds","memory_note":"completed after per-run replanning"}
JSON
    echo 4 > "$state_file"
    exit 0
    ;;
esac
echo "unexpected codex stub invocation" >&2
exit 5
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
      std::string("runtime_hero_binary:path = ") + runtime_stub.string() +
          "\n" +
          "config_hero_binary:path = /bin/true\n"
          "lattice_hero_binary:path = /bin/true\n"
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
          "marshal_max_campaign_launches(1,+inf):int = 4\n");
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
  const command_result_t idled = wait_for_command(
      get_command,
      [](const command_result_t &result) {
        return result.exit_code == 0 &&
               contains(result.output, "\"lifecycle\":\"live\"") &&
               contains(result.output, "\"activity\":\"review\"") &&
               contains(result.output, "\"finish_reason\":\"success\"") &&
               contains(result.output, "\"checkpoint_count\":4") &&
               contains(result.output, "\"launch_count\":3");
      },
      20000, 100);
  REQUIRE(idled.exit_code == 0);
  const fs::path session_root = runtime_root / ".marshal_hero" / session_id;
  const std::string memory_text =
      read_text_file(session_root / "marshal.session.memory.md");
  REQUIRE(
      contains(memory_text, "run_plan launch decomposed to next pending bind"));

  const fs::path campaigns_root = runtime_root / ".campaigns";
  REQUIRE(fs::exists(campaigns_root));
  std::size_t campaign_count = 0;
  bool saw_train = false;
  bool saw_eval = false;
  bool saw_untouched = false;
  for (const auto &it : fs::directory_iterator(campaigns_root)) {
    if (!it.is_directory())
      continue;
    ++campaign_count;
    const std::string manifest_text =
        read_text_file(it.path() / "runtime.campaign.manifest.lls");
    REQUIRE(contains(manifest_text,
                     std::string("marshal_session_id:str = ") + session_id));
    REQUIRE(contains(manifest_text, "run_bind_id.0000:str = "));
    REQUIRE(!contains(manifest_text, "run_bind_id.0001:str = "));
    const std::string bind_id =
        extract_lls_string_value(manifest_text, "run_bind_id.0000");
    if (bind_id == "bind_train_vicreg_primary_btcusdt")
      saw_train = true;
    if (bind_id == "bind_eval_vicreg_payload_btcusdt")
      saw_eval = true;
    if (bind_id == "bind_eval_vicreg_payload_btcusdt_untouched_test")
      saw_untouched = true;
  }
  REQUIRE(campaign_count == 3);
  REQUIRE(saw_train);
  REQUIRE(saw_eval);
  REQUIRE(saw_untouched);
}
