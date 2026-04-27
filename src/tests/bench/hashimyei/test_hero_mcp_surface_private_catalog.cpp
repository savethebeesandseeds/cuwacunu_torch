static void test_hashimyei_tools_list_surface() {
  temp_dir_t tmp("test_hashimyei_mcp_tools");
  const fs::path store_root = tmp.dir / "hash_store";
  const fs::path catalog = store_root / "hashimyei_catalog.idydb";
  fs::create_directories(store_root);

  const command_result_t result =
      run_command(tools_list_command(kHashimyeiMcpBin, store_root, catalog));
  REQUIRE(result.exit_code == 0);
  REQUIRE(contains(result.output, "\"hero.hashimyei.list\""));
  REQUIRE(contains(result.output,
                   "\"hero.hashimyei.evaluate_contract_compatibility\""));
  REQUIRE(
      contains(result.output, "\"hero.hashimyei.get_founding_dsl_bundle\""));
  REQUIRE(contains(result.output, "\"hero.hashimyei.update_rank\""));
  REQUIRE(contains(result.output,
                   "\"required\":[\"family\",\"component_compatibility_sha256_"
                   "hex\",\"ordered_hashimyeis\"]"));
  REQUIRE(contains(result.output, "\"hero.hashimyei.reset_catalog\""));
  REQUIRE(!contains(result.output, "\"anyOf\":"));
  REQUIRE(!contains(result.output, "\"oneOf\":"));
  REQUIRE(!contains(result.output, "\"allOf\":"));
  REQUIRE(!contains(result.output, "\"hero.hashimyei.get_history\""));
  REQUIRE(!contains(result.output, "\"hero.hashimyei.get_report_lls\""));
}

static void test_lattice_tools_list_surface() {
  temp_dir_t tmp("test_lattice_mcp_tools");
  const fs::path store_root = tmp.dir / "lat_store";
  const fs::path catalog = store_root / "lattice_catalog.idydb";
  fs::create_directories(store_root);

  const command_result_t result =
      run_command(tools_list_command(kLatticeMcpBin, store_root, catalog));
  REQUIRE(result.exit_code == 0);
  REQUIRE(contains(result.output, "\"hero.lattice.list_facts\""));
  REQUIRE(contains(result.output, "\"hero.lattice.get_fact\""));
  REQUIRE(contains(result.output, "\"hero.lattice.list_views\""));
  REQUIRE(contains(result.output, "\"hero.lattice.get_view\""));
  REQUIRE(contains(result.output, "\"required\":[\"view_kind\"]"));
  REQUIRE(contains(result.output, "\"hero.lattice.refresh\""));
  REQUIRE(!contains(result.output, "\"hero.lattice.get_runs\""));
  REQUIRE(!contains(result.output, "\"hero.lattice.list_report_fragments\""));
  REQUIRE(!contains(result.output, "\"hero.lattice.get_report_lls\""));
  REQUIRE(!contains(result.output, "\"hero.lattice.get_view_lls\""));
  REQUIRE(!contains(result.output, "\"hero.lattice.reset_catalog\""));
  REQUIRE(!contains(result.output, "\"hero.lattice.get_history\""));
  REQUIRE(!contains(result.output, "\"hero.lattice.get_performance\""));
  REQUIRE(!contains(result.output, "\"hero.lattice.debug_dump\""));
  REQUIRE(!contains(result.output, "\"hero.lattice.run_or_get\""));
  REQUIRE(!contains(result.output, "\"hero.lattice.matrix\""));
  REQUIRE(!contains(result.output, "\"hero.lattice.trials\""));
}

static void test_marshal_tools_list_surface() {
  const command_result_t result =
      run_command(list_tools_json_command(kMarshalMcpBin, kGlobalConfig));
  REQUIRE(result.exit_code == 0);
  REQUIRE(contains(result.output, "\"hero.marshal.start_session\""));
  REQUIRE(contains(result.output, "\"hero.marshal.list_sessions\""));
  REQUIRE(contains(result.output, "\"hero.marshal.get_session\""));
  REQUIRE(contains(result.output, "\"hero.marshal.reconcile_session\""));
  REQUIRE(contains(result.output, "\"hero.marshal.pause_session\""));
  REQUIRE(contains(result.output, "\"hero.marshal.resume_session\""));
  REQUIRE(contains(result.output, "\"hero.marshal.message_session\""));
  REQUIRE(contains(result.output, "\"hero.marshal.archive_session\""));
  REQUIRE(contains(result.output, "\"hero.marshal.terminate_session\""));
  REQUIRE(!contains(result.output, "\"hero.marshal.start_loop\""));
  REQUIRE(!contains(result.output, "\"hero.marshal.list_loops\""));
  REQUIRE(!contains(result.output, "\"hero.marshal.get_loop\""));
  REQUIRE(!contains(result.output, "\"hero.marshal.stop_loop\""));
  REQUIRE(!contains(result.output, "\"hero.marshal.apply_human_resolution\""));
}

static void test_human_tools_list_surface() {
  const command_result_t result =
      run_command(list_tools_json_command(kHumanMcpBin, kGlobalConfig));
  REQUIRE(result.exit_code == 0);
  REQUIRE(contains(result.output, "\"hero.human.list_requests\""));
  REQUIRE(contains(result.output, "\"hero.human.get_request\""));
  REQUIRE(contains(result.output, "\"hero.human.answer_request\""));
  REQUIRE(contains(result.output, "\"hero.human.resolve_governance\""));
  REQUIRE(contains(result.output, "\"hero.human.list_summaries\""));
  REQUIRE(contains(result.output, "\"hero.human.get_summary\""));
  REQUIRE(contains(result.output, "\"hero.human.ack_summary\""));
  REQUIRE(contains(result.output, "\"hero.human.archive_summary\""));
  REQUIRE(!contains(result.output, "\"hero.human.list_sessions\""));
  REQUIRE(!contains(result.output, "\"hero.human.get_session\""));
  REQUIRE(!contains(result.output, "\"hero.human.pause_session\""));
  REQUIRE(!contains(result.output, "\"hero.human.resume_session\""));
  REQUIRE(!contains(result.output, "\"hero.human.message_session\""));
  REQUIRE(!contains(result.output, "\"hero.human.terminate_session\""));
  REQUIRE(!contains(result.output, "\"hero.human.answer_input\""));
  REQUIRE(!contains(result.output, "\"hero.human.dismiss_summary\""));
  REQUIRE(!contains(result.output, "\"hero.human.list_escalations\""));
  REQUIRE(!contains(result.output, "\"hero.human.get_escalation\""));
  REQUIRE(!contains(result.output, "\"hero.human.resolve_escalation\""));
  REQUIRE(!contains(result.output, "\"hero.human.list_reports\""));
  REQUIRE(!contains(result.output, "\"hero.human.get_report\""));
  REQUIRE(!contains(result.output, "\"hero.human.ack_report\""));
  REQUIRE(!contains(result.output, "\"hero.human.dismiss_report\""));
}

static void test_hashimyei_reset_catalog_smoke() {
  temp_dir_t tmp("test_hashimyei_mcp_reset");
  const fs::path store_root = tmp.dir / "hash_store";
  const fs::path catalog = store_root / "hashimyei_catalog.idydb";
  fs::create_directories(store_root);

  const command_result_t result =
      run_command(reset_hashimyei_command(store_root, catalog));
  REQUIRE(result.exit_code == 0);
  REQUIRE(contains(result.output, "\"isError\":false"));
  REQUIRE(contains(result.output, "\"component_count\":"));
  REQUIRE(fs::exists(catalog));
}

static void test_hashimyei_list_direct_tool_skips_ingest_rebuild() {
  temp_dir_t tmp("test_hashimyei_mcp_list_snapshot");
  const fs::path store_root = tmp.dir / "hash_store";
  const fs::path catalog = store_root / "hashimyei_catalog.idydb";
  const fs::path ingest_lock =
      store_root / "_meta" / "catalog" / ".ingest.lock";
  fs::create_directories(store_root);

  const command_result_t result = run_command(direct_tool_command_with_paths(
      kHashimyeiMcpBin, kGlobalConfig, store_root, catalog,
      "hero.hashimyei.list", "{}"));
  REQUIRE(result.exit_code == 0);
  REQUIRE(contains(result.output, "\"isError\":false"));
  REQUIRE(contains(result.output, "\"count\":0"));
  REQUIRE(!fs::exists(ingest_lock));
}

static void test_lattice_reset_catalog_smoke() {
  temp_dir_t tmp("test_lattice_mcp_reset");
  const fs::path store_root = tmp.dir / "lat_store";
  const fs::path catalog = store_root / "lattice_catalog.idydb";
  fs::create_directories(store_root);

  const command_result_t result =
      run_command(reset_lattice_command(store_root, catalog));
  REQUIRE(result.exit_code == 0);
  REQUIRE(contains(result.output, "\"isError\":false"));
  REQUIRE(contains(result.output, "\"runtime_report_fragment_count\":"));
  REQUIRE(fs::exists(catalog));
}

static void test_lattice_removed_tool_rejected() {
  temp_dir_t tmp("test_lattice_mcp_removed_tool");
  const fs::path store_root = tmp.dir / "lat_store";
  const fs::path catalog = store_root / "lattice_catalog.idydb";
  fs::create_directories(store_root);

  const command_result_t result =
      run_command(call_removed_lattice_tool_command(store_root, catalog));
  REQUIRE(result.exit_code != 0);
}
