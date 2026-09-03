ROOT_PATH := ../../../../../..
include $(ROOT_PATH)/Makefile.config

HERE_PATH  := $(TESTS_PATH)/bench/wikimyei/representation/encoding/mtf_jepa_mae_vicreg
REL_MODULE := $(patsubst $(TESTS_PATH)/%,%,$(HERE_PATH))

TEST_DEFAULT_LDLIBS :=

CHANNEL_GRAPH_FIRST_LAUNCHER_OBJS := \
  $(OUTPUT_PATH)/common/core/utils.o \
  $(OUTPUT_PATH)/common/io/files.o \
  $(OUTPUT_PATH)/common/parse/json/json_parsing.o \
  $(OUTPUT_PATH)/common/statistics_space.o \
  $(OUTPUT_PATH)/common/registry_data.o \
  $(OUTPUT_PATH)/common/registry_utils.o \
  $(OUTPUT_PATH)/common/source_contract.o \
  $(OUTPUT_PATH)/common/source_contract_decode.o \
  $(OUTPUT_PATH)/common/source_registry_decoder.o \
  $(OUTPUT_PATH)/common/retrieval_channel_decoder.o \
  $(OUTPUT_PATH)/common/graph_topology_decoder.o \
  $(OUTPUT_PATH)/common/parser_types.o \
  $(OUTPUT_PATH)/common/ast.o \
  $(OUTPUT_PATH)/common/grammar_lexer.o \
  $(OUTPUT_PATH)/common/grammar_parser.o \
  $(OUTPUT_PATH)/common/instruction_lexer.o \
  $(OUTPUT_PATH)/common/instruction_parser.o \
  $(OUTPUT_PATH)/common/memory_mapped_datafile.o \
  $(OUTPUT_PATH)/libtorch/memory_mapped_dataset.o \
  $(OUTPUT_PATH)/libtorch/memory_mapped_dataloader.o

GPV1_BINARY := $(TEST_OUT)/quality_wikimyei_mtf_jepa_mae_vicreg_global_pool_projector_variance_causal_decomposition
GPV1_LDLIBS_EXTRA := $(libtorch_a) $(CHANNEL_GRAPH_FIRST_LAUNCHER_OBJS) $(LDLIBS_torch)
GPV1_BUILD_RECEIPT := $(TEST_OUT)/gpv1_build_receipt_v1.txt
GPV1_FULL_DEPFILE := $(TEST_OUT)/gpv1_full_dependencies_v1.d
GPV1_BUILD_RUN_LOCK := $(TEST_OUT)/gpv1_build_and_run.lock

VVA1B_BINARY := $(TEST_OUT)/quality_wikimyei_mtf_jepa_mae_vicreg_view_loss_boundary_triad
VVA1B_SOURCE := $(HERE_PATH)/quality_wikimyei_mtf_jepa_mae_vicreg_view_loss_boundary_triad.cpp
VVA1B_OBJECT := $(OBJTMP)/quality_wikimyei_mtf_jepa_mae_vicreg_view_loss_boundary_triad.o
VVA1B_LDLIBS_EXTRA := $(libtorch_a) $(CHANNEL_GRAPH_FIRST_LAUNCHER_OBJS) $(LDLIBS_torch)
VVA1B_BUILD_MANIFEST := $(TEST_OUT)/vva1b_build_manifest_v3.txt
VVA1B_FULL_DEPFILE := $(TEST_OUT)/vva1b_full_dependencies_v3.d
VVA1B_BUILD_MANIFEST_HELPER := $(HERE_PATH)/write_vva1b_build_manifest.sh
VVA1B_BUILD_RUN_LOCK := $(TEST_OUT)/vva1b_build_and_run.lock
VVA1B_SELF_TEST_LOG := $(TEST_OUT)/representation_vva1b_v3_self_test.log
VVA1B_PREFLIGHT_LOG := $(TEST_OUT)/representation_vva1b_v3_preflight.log
VVA1B_AUTHORITATIVE_LOG := $(TEST_OUT)/representation_vva1b_v3_authoritative.log
VVA1B_PROTOCOL := $(HERE_PATH)/VICREG_VIEW_LOSS_BOUNDARY_TRIAD_PROTOCOL.md
VVA1B_AMENDMENT_A1 := $(HERE_PATH)/VICREG_VIEW_LOSS_BOUNDARY_TRIAD_PROTOCOL_AMENDMENT_A1.md
VVA1B_AMENDMENT_A2 := $(HERE_PATH)/VICREG_VIEW_LOSS_BOUNDARY_TRIAD_PROTOCOL_AMENDMENT_A2.md
VVA1B_AMENDMENT_A3 := $(HERE_PATH)/VICREG_VIEW_LOSS_BOUNDARY_TRIAD_PROTOCOL_AMENDMENT_A3.md
VVA1B_SEAM_AUDIT := $(HERE_PATH)/VICREG_VIEW_LOSS_BOUNDARY_TRIAD_SEAM_AUDIT.md
VVA1B_FAILED_ATTEMPT_LOG := $(TEST_OUT)/representation_vva1b_v1_authoritative_failed.40698.0.log
VVA1B_FAILED_ATTEMPT_EXECUTABLE := $(TEST_OUT)/quality_wikimyei_mtf_jepa_mae_vicreg_view_loss_boundary_triad_failed_attempt_v1
VVA1B_CUSTODY_SIDECARS := \
  $(HERE_PATH)/VICREG_VIEW_LOSS_BOUNDARY_TRIAD_PROTOCOL.sha256 \
  $(HERE_PATH)/VICREG_VIEW_LOSS_BOUNDARY_TRIAD_PROTOCOL_AMENDMENT_A1.sha256 \
  $(HERE_PATH)/VICREG_VIEW_LOSS_BOUNDARY_TRIAD_PROTOCOL_AMENDMENT_A2.sha256 \
  $(HERE_PATH)/VICREG_VIEW_LOSS_BOUNDARY_TRIAD_PROTOCOL_AMENDMENT_A3.sha256 \
  $(HERE_PATH)/VICREG_VIEW_LOSS_BOUNDARY_TRIAD_SEAM_AUDIT.sha256
VVA1B_CUSTODY_FILES := $(VVA1B_PROTOCOL) $(VVA1B_AMENDMENT_A1) \
  $(VVA1B_AMENDMENT_A2) $(VVA1B_AMENDMENT_A3) $(VVA1B_SEAM_AUDIT) \
  $(VVA1B_CUSTODY_SIDECARS) $(VVA1B_FAILED_ATTEMPT_LOG) \
  $(VVA1B_FAILED_ATTEMPT_LOG).sha256 $(VVA1B_FAILED_ATTEMPT_EXECUTABLE) \
  $(VVA1B_FAILED_ATTEMPT_EXECUTABLE).sha256
VVA1B_BUILD_EXTRA_INPUTS := $(abspath $(HERE_PATH)/Makefile) \
  $(abspath $(ROOT_PATH)/Makefile.config) \
  $(VVA1B_BUILD_MANIFEST_HELPER) $(VVA1B_CUSTODY_FILES) \
  $(filter %.a %.o,$(TEST_DEFAULT_LDLIBS) $(VVA1B_LDLIBS_EXTRA)) \
  $(VVA1B_OBJECT)

.PHONY: channel_graph_first_launcher_objects
channel_graph_first_launcher_objects:
	$(MAKE) -C $(IMPL_PATH)/piaabo utils files json_parsing
	$(MAKE) -C $(IMPL_PATH)/piaabo/parse/bnf all
	$(MAKE) -C $(IMPL_PATH)/piaabo/math all
	$(MAKE) -C $(IMPL_PATH)/ujcamei/source/registry/types all
	$(MAKE) -C $(IMPL_PATH)/ujcamei/source/contract all
	$(MAKE) -C $(IMPL_PATH)/ujcamei/source/retrieval/storage/memory_mapped all
	$(MAKE) -C $(IMPL_PATH)/wikimyei/inference/expected_value/mdn all

$(eval $(call TEST_ONEFILE, test_wikimyei_mtf_jepa_mae_vicreg, test_wikimyei_mtf_jepa_mae_vicreg.cpp, \
  $(LDLIBS_torch)))
$(eval $(call TEST_ONEFILE, train_synthetic_mtf_jepa_mae_vicreg, train_synthetic_mtf_jepa_mae_vicreg.cpp, \
  $(LDLIBS_torch)))
$(eval $(call TEST_ONEFILE, test_wikimyei_mtf_jepa_mae_vicreg_contracts, test_wikimyei_mtf_jepa_mae_vicreg_contracts.cpp, \
  $(LDLIBS_torch)))
$(eval $(call TEST_ONEFILE, test_representation_objective_repair_gate, test_representation_objective_repair_gate.cpp,))
$(eval $(call TEST_ONEFILE, test_representation_jepa_mae_core_decomposition_gate, test_representation_jepa_mae_core_decomposition_gate.cpp,))
$(eval $(call TEST_ONEFILE, test_representation_vicreg_variance_necessity_gate, test_representation_vicreg_variance_necessity_gate.cpp,))
$(eval $(call TEST_ONEFILE, test_representation_outer_augmentation_training_gate, test_representation_outer_augmentation_training_gate.cpp,))
$(eval $(call TEST_ONEFILE, test_outer_augmentation_causal_attribution_gate, test_outer_augmentation_causal_attribution_gate.cpp,))
$(eval $(call TEST_ONEFILE, test_representation_module_certification_gate, test_representation_module_certification_gate.cpp,))
$(eval $(call TEST_ONEFILE, test_representation_surface_sufficiency_map_gate, test_representation_surface_sufficiency_map_gate.cpp,))
$(eval $(call TEST_ONEFILE, test_representation_surface_sufficiency_map_reference_auditor, test_representation_surface_sufficiency_map_reference_auditor.cpp,))
$(eval $(call TEST_ONEFILE, test_pooling_structure_mechanism_map_gate, test_pooling_structure_mechanism_map_gate.cpp,))
$(eval $(call TEST_ONEFILE, test_pooling_structure_mechanism_map_log_auditor, test_pooling_structure_mechanism_map_log_auditor.cpp,))
$(eval $(call TEST_ONEFILE, test_structured_readout_shadow, test_structured_readout_shadow.cpp, \
  $(LDLIBS_torch)))
$(eval $(call TEST_ONEFILE, test_production_structured_readout, test_production_structured_readout.cpp, \
  $(LDLIBS_torch)))
$(eval $(call TEST_ONEFILE, test_structured_cdsb_sparse_v1, test_structured_cdsb_sparse_v1.cpp, \
  $(LDLIBS_torch)))
$(eval $(call TEST_ONEFILE, test_production_structured_readout_parity_gate, test_production_structured_readout_parity_gate.cpp,))
$(eval $(call TEST_ONEFILE, test_production_structured_readout_parity_log_auditor, test_production_structured_readout_parity_log_auditor.cpp,))
$(eval $(call TEST_ONEFILE, test_structured_readout_repair_gate, test_structured_readout_repair_gate.cpp,))
$(eval $(call TEST_ONEFILE, test_structured_readout_repair_log_auditor, test_structured_readout_repair_log_auditor.cpp,))
$(eval $(call TEST_ONEFILE, qualify_wikimyei_mtf_jepa_mae_vicreg, qualify_wikimyei_mtf_jepa_mae_vicreg.cpp, \
  $(LDLIBS_torch)))
$(eval $(call TEST_ONEFILE, quality_wikimyei_mtf_jepa_mae_vicreg_representation, quality_wikimyei_mtf_jepa_mae_vicreg_representation.cpp, \
  $(libtorch_a) $(CHANNEL_GRAPH_FIRST_LAUNCHER_OBJS) $(LDLIBS_torch)))
$(eval $(call TEST_ONEFILE, quality_wikimyei_mtf_jepa_mae_vicreg_structured_readout_repair, quality_wikimyei_mtf_jepa_mae_vicreg_structured_readout_repair.cpp, \
  $(libtorch_a) $(CHANNEL_GRAPH_FIRST_LAUNCHER_OBJS) $(LDLIBS_torch)))
$(eval $(call TEST_ONEFILE, quality_wikimyei_mtf_jepa_mae_vicreg_production_structured_readout_parity, quality_wikimyei_mtf_jepa_mae_vicreg_production_structured_readout_parity.cpp, \
  $(libtorch_a) $(CHANNEL_GRAPH_FIRST_LAUNCHER_OBJS) $(LDLIBS_torch)))
$(eval $(call TEST_ONEFILE, quality_wikimyei_mtf_jepa_mae_vicreg_representation_module_certification, quality_wikimyei_mtf_jepa_mae_vicreg_representation_module_certification.cpp, \
  $(libtorch_a) $(CHANNEL_GRAPH_FIRST_LAUNCHER_OBJS) $(LDLIBS_torch)))
$(eval $(call TEST_ONEFILE, quality_wikimyei_mtf_jepa_mae_vicreg_frozen_sequence_projection_alignment, quality_wikimyei_mtf_jepa_mae_vicreg_frozen_sequence_projection_alignment.cpp, \
  $(libtorch_a) $(CHANNEL_GRAPH_FIRST_LAUNCHER_OBJS) $(LDLIBS_torch)))
$(eval $(call TEST_ONEFILE, quality_wikimyei_mtf_jepa_mae_vicreg_geometry_preserving_whitening_distillation, quality_wikimyei_mtf_jepa_mae_vicreg_geometry_preserving_whitening_distillation.cpp, \
  $(libtorch_a) $(CHANNEL_GRAPH_FIRST_LAUNCHER_OBJS) $(LDLIBS_torch)))
$(eval $(call TEST_ONEFILE, quality_wikimyei_mtf_jepa_mae_vicreg_minimal_participation_spectral_repair, quality_wikimyei_mtf_jepa_mae_vicreg_minimal_participation_spectral_repair.cpp, \
  $(libtorch_a) $(CHANNEL_GRAPH_FIRST_LAUNCHER_OBJS) $(LDLIBS_torch)))
$(eval $(call TEST_ONEFILE, quality_wikimyei_mtf_jepa_mae_vicreg_four_objective_causal_attribution, quality_wikimyei_mtf_jepa_mae_vicreg_four_objective_causal_attribution.cpp, \
  $(libtorch_a) $(CHANNEL_GRAPH_FIRST_LAUNCHER_OBJS) $(LDLIBS_torch)))
$(eval $(call TEST_ONEFILE, quality_wikimyei_mtf_jepa_mae_vicreg_outer_augmentation_causal_attribution, quality_wikimyei_mtf_jepa_mae_vicreg_outer_augmentation_causal_attribution.cpp, \
  $(libtorch_a) $(CHANNEL_GRAPH_FIRST_LAUNCHER_OBJS) $(LDLIBS_torch)))
$(eval $(call TEST_ONEFILE, quality_wikimyei_mtf_jepa_mae_vicreg_view_augmentation_causal_attribution, quality_wikimyei_mtf_jepa_mae_vicreg_view_augmentation_causal_attribution.cpp, \
  $(libtorch_a) $(CHANNEL_GRAPH_FIRST_LAUNCHER_OBJS) $(LDLIBS_torch)))
$(eval $(call TEST_ONEFILE, quality_wikimyei_mtf_jepa_mae_vicreg_view_loss_boundary_triad, quality_wikimyei_mtf_jepa_mae_vicreg_view_loss_boundary_triad.cpp, \
  $(VVA1B_LDLIBS_EXTRA)))
$(eval $(call TEST_ONEFILE, quality_wikimyei_mtf_jepa_mae_vicreg_global_pool_projector_variance_causal_decomposition, quality_wikimyei_mtf_jepa_mae_vicreg_global_pool_projector_variance_causal_decomposition.cpp, \
  $(GPV1_LDLIBS_EXTRA)))
$(eval $(call TEST_ONEFILE, quality_wikimyei_mtf_jepa_mae_vicreg_dose_matched_mask_repair, quality_wikimyei_mtf_jepa_mae_vicreg_dose_matched_mask_repair.cpp, \
  $(libtorch_a) $(CHANNEL_GRAPH_FIRST_LAUNCHER_OBJS) $(LDLIBS_torch)))
$(eval $(call TEST_ONEFILE, quality_wikimyei_mtf_jepa_mae_vicreg_target_predictability_ceiling, quality_wikimyei_mtf_jepa_mae_vicreg_target_predictability_ceiling.cpp, \
  $(libtorch_a) $(CHANNEL_GRAPH_FIRST_LAUNCHER_OBJS) $(LDLIBS_torch)))
$(eval $(call TEST_ONEFILE, quality_wikimyei_mtf_jepa_mae_vicreg_target_relative_predictor_sufficiency, quality_wikimyei_mtf_jepa_mae_vicreg_target_relative_predictor_sufficiency.cpp, \
  $(libtorch_a) $(CHANNEL_GRAPH_FIRST_LAUNCHER_OBJS) $(LDLIBS_torch)))
$(eval $(call TEST_ONEFILE, quality_wikimyei_mtf_jepa_mae_vicreg_canonical_slot_interaction_sufficiency, quality_wikimyei_mtf_jepa_mae_vicreg_canonical_slot_interaction_sufficiency.cpp, \
  $(libtorch_a) $(CHANNEL_GRAPH_FIRST_LAUNCHER_OBJS) $(LDLIBS_torch)))
$(eval $(call TEST_ONEFILE, quality_wikimyei_mtf_jepa_mae_vicreg_support_permitted_teacher_target_alignment, quality_wikimyei_mtf_jepa_mae_vicreg_support_permitted_teacher_target_alignment.cpp, \
  $(libtorch_a) $(CHANNEL_GRAPH_FIRST_LAUNCHER_OBJS) $(LDLIBS_torch)))
$(eval $(call TEST_ONEFILE, test_wikimyei_mtf_jepa_mae_vicreg_tokenizer_information, test_wikimyei_mtf_jepa_mae_vicreg_tokenizer_information.cpp, \
  $(LDLIBS_torch)))
$(eval $(call TEST_ONEFILE, test_wikimyei_mtf_jepa_mae_vicreg_ima1_stage0, test_wikimyei_mtf_jepa_mae_vicreg_ima1_stage0.cpp, \
  $(LDLIBS_torch)))
$(eval $(call TEST_ONEFILE, test_wikimyei_mtf_jepa_mae_vicreg_ima2_mask_contract, test_wikimyei_mtf_jepa_mae_vicreg_ima2_mask_contract.cpp, \
  $(LDLIBS_torch)))
$(eval $(call TEST_ONEFILE, bench_wikimyei_mtf_jepa_mae_vicreg_performance, bench_wikimyei_mtf_jepa_mae_vicreg_performance.cpp, \
  $(LDLIBS_torch)))

$(TEST_OUT)/test_wikimyei_mtf_jepa_mae_vicreg: INCLUDES_EXTRA += $(TORCH_INCLUDE_PATHS)
$(TEST_OUT)/train_synthetic_mtf_jepa_mae_vicreg: INCLUDES_EXTRA += $(TORCH_INCLUDE_PATHS)
$(TEST_OUT)/test_wikimyei_mtf_jepa_mae_vicreg_contracts: INCLUDES_EXTRA += $(TORCH_INCLUDE_PATHS)
$(TEST_OUT)/test_structured_readout_shadow: INCLUDES_EXTRA += $(TORCH_INCLUDE_PATHS)
$(TEST_OUT)/test_production_structured_readout: INCLUDES_EXTRA += $(TORCH_INCLUDE_PATHS)
$(TEST_OUT)/test_structured_cdsb_sparse_v1: INCLUDES_EXTRA += $(TORCH_INCLUDE_PATHS)
$(TEST_OUT)/qualify_wikimyei_mtf_jepa_mae_vicreg: INCLUDES_EXTRA += $(TORCH_INCLUDE_PATHS)
$(TEST_OUT)/quality_wikimyei_mtf_jepa_mae_vicreg_representation: INCLUDES_EXTRA += $(TORCH_INCLUDE_PATHS)
$(TEST_OUT)/quality_wikimyei_mtf_jepa_mae_vicreg_representation: channel_graph_first_launcher_objects
$(TEST_OUT)/quality_wikimyei_mtf_jepa_mae_vicreg_structured_readout_repair: INCLUDES_EXTRA += $(TORCH_INCLUDE_PATHS)
$(TEST_OUT)/quality_wikimyei_mtf_jepa_mae_vicreg_structured_readout_repair: channel_graph_first_launcher_objects
$(TEST_OUT)/quality_wikimyei_mtf_jepa_mae_vicreg_production_structured_readout_parity: INCLUDES_EXTRA += $(TORCH_INCLUDE_PATHS)
$(TEST_OUT)/quality_wikimyei_mtf_jepa_mae_vicreg_production_structured_readout_parity: channel_graph_first_launcher_objects
$(TEST_OUT)/quality_wikimyei_mtf_jepa_mae_vicreg_representation_module_certification: INCLUDES_EXTRA += $(TORCH_INCLUDE_PATHS)
$(TEST_OUT)/quality_wikimyei_mtf_jepa_mae_vicreg_representation_module_certification: channel_graph_first_launcher_objects
$(TEST_OUT)/quality_wikimyei_mtf_jepa_mae_vicreg_frozen_sequence_projection_alignment: INCLUDES_EXTRA += $(TORCH_INCLUDE_PATHS)
$(TEST_OUT)/quality_wikimyei_mtf_jepa_mae_vicreg_frozen_sequence_projection_alignment: channel_graph_first_launcher_objects
$(TEST_OUT)/quality_wikimyei_mtf_jepa_mae_vicreg_geometry_preserving_whitening_distillation: INCLUDES_EXTRA += $(TORCH_INCLUDE_PATHS)
$(TEST_OUT)/quality_wikimyei_mtf_jepa_mae_vicreg_geometry_preserving_whitening_distillation: channel_graph_first_launcher_objects
$(TEST_OUT)/quality_wikimyei_mtf_jepa_mae_vicreg_minimal_participation_spectral_repair: INCLUDES_EXTRA += $(TORCH_INCLUDE_PATHS)
$(TEST_OUT)/quality_wikimyei_mtf_jepa_mae_vicreg_minimal_participation_spectral_repair: channel_graph_first_launcher_objects
$(TEST_OUT)/quality_wikimyei_mtf_jepa_mae_vicreg_four_objective_causal_attribution: INCLUDES_EXTRA += $(TORCH_INCLUDE_PATHS)
$(TEST_OUT)/quality_wikimyei_mtf_jepa_mae_vicreg_four_objective_causal_attribution: channel_graph_first_launcher_objects
$(TEST_OUT)/quality_wikimyei_mtf_jepa_mae_vicreg_outer_augmentation_causal_attribution: INCLUDES_EXTRA += $(TORCH_INCLUDE_PATHS)
$(TEST_OUT)/quality_wikimyei_mtf_jepa_mae_vicreg_outer_augmentation_causal_attribution: channel_graph_first_launcher_objects
$(TEST_OUT)/quality_wikimyei_mtf_jepa_mae_vicreg_view_augmentation_causal_attribution: INCLUDES_EXTRA += $(TORCH_INCLUDE_PATHS)
$(TEST_OUT)/quality_wikimyei_mtf_jepa_mae_vicreg_view_augmentation_causal_attribution: channel_graph_first_launcher_objects
$(TEST_OUT)/quality_wikimyei_mtf_jepa_mae_vicreg_view_loss_boundary_triad: INCLUDES_EXTRA += $(TORCH_INCLUDE_PATHS)
$(TEST_OUT)/quality_wikimyei_mtf_jepa_mae_vicreg_view_loss_boundary_triad: channel_graph_first_launcher_objects
$(TEST_OUT)/quality_wikimyei_mtf_jepa_mae_vicreg_global_pool_projector_variance_causal_decomposition: INCLUDES_EXTRA += $(TORCH_INCLUDE_PATHS)
$(TEST_OUT)/quality_wikimyei_mtf_jepa_mae_vicreg_global_pool_projector_variance_causal_decomposition: channel_graph_first_launcher_objects
$(TEST_OUT)/quality_wikimyei_mtf_jepa_mae_vicreg_dose_matched_mask_repair: INCLUDES_EXTRA += $(TORCH_INCLUDE_PATHS)
$(TEST_OUT)/quality_wikimyei_mtf_jepa_mae_vicreg_dose_matched_mask_repair: channel_graph_first_launcher_objects
$(TEST_OUT)/quality_wikimyei_mtf_jepa_mae_vicreg_target_predictability_ceiling: INCLUDES_EXTRA += $(TORCH_INCLUDE_PATHS)
$(TEST_OUT)/quality_wikimyei_mtf_jepa_mae_vicreg_target_predictability_ceiling: channel_graph_first_launcher_objects
$(TEST_OUT)/quality_wikimyei_mtf_jepa_mae_vicreg_target_relative_predictor_sufficiency: INCLUDES_EXTRA += $(TORCH_INCLUDE_PATHS)
$(TEST_OUT)/quality_wikimyei_mtf_jepa_mae_vicreg_target_relative_predictor_sufficiency: channel_graph_first_launcher_objects
$(TEST_OUT)/quality_wikimyei_mtf_jepa_mae_vicreg_canonical_slot_interaction_sufficiency: INCLUDES_EXTRA += $(TORCH_INCLUDE_PATHS)
$(TEST_OUT)/quality_wikimyei_mtf_jepa_mae_vicreg_canonical_slot_interaction_sufficiency: channel_graph_first_launcher_objects
$(TEST_OUT)/quality_wikimyei_mtf_jepa_mae_vicreg_support_permitted_teacher_target_alignment: INCLUDES_EXTRA += $(TORCH_INCLUDE_PATHS)
$(TEST_OUT)/quality_wikimyei_mtf_jepa_mae_vicreg_support_permitted_teacher_target_alignment: channel_graph_first_launcher_objects
$(TEST_OUT)/test_wikimyei_mtf_jepa_mae_vicreg_tokenizer_information: INCLUDES_EXTRA += $(TORCH_INCLUDE_PATHS)
$(TEST_OUT)/test_wikimyei_mtf_jepa_mae_vicreg_ima1_stage0: INCLUDES_EXTRA += $(TORCH_INCLUDE_PATHS)
$(TEST_OUT)/test_wikimyei_mtf_jepa_mae_vicreg_ima2_mask_contract: INCLUDES_EXTRA += $(TORCH_INCLUDE_PATHS)
$(TEST_OUT)/bench_wikimyei_mtf_jepa_mae_vicreg_performance: INCLUDES_EXTRA += $(TORCH_INCLUDE_PATHS)
$(TEST_OUT)/bench_wikimyei_mtf_jepa_mae_vicreg_performance: CXXFLAGS += -O3 -DNDEBUG

.PHONY: all isolated training-smoke
all: isolated

# Fast gate: in-memory tensors -> representation core -> losses/embeddings.
# This deliberately excludes NodeLift, MDN, Runtime, data loading, and artifacts.
isolated: $(TEST_OUT)/test_wikimyei_mtf_jepa_mae_vicreg \
  $(TEST_OUT)/test_production_structured_readout \
  $(TEST_OUT)/test_production_structured_readout_parity_gate
	@$(LOG_SUCCESS)

# Slower opt-in gate: a bounded optimizer loop over synthetic inputs.
training-smoke: $(TEST_OUT)/train_synthetic_mtf_jepa_mae_vicreg
	@$(LOG_SUCCESS)

.PHONY: run run-isolated run-training-smoke
run: run-isolated

run-isolated: run-test_wikimyei_mtf_jepa_mae_vicreg \
  run-test_production_structured_readout \
  run-test_production_structured_readout_parity_gate

run-training-smoke: run-train_synthetic_mtf_jepa_mae_vicreg

.PHONY: qualification run-qualification run-contracts run-scientific-qualification
.NOTPARALLEL: qualification run-qualification
qualification: $(TEST_OUT)/test_wikimyei_mtf_jepa_mae_vicreg_contracts \
  $(TEST_OUT)/qualify_wikimyei_mtf_jepa_mae_vicreg

run-qualification: run-contracts run-scientific-qualification

run-contracts: run-test_wikimyei_mtf_jepa_mae_vicreg_contracts

run-scientific-qualification: run-qualify_wikimyei_mtf_jepa_mae_vicreg

.PHONY: representation-quality run-tokenizer-information \
  run-representation-quality-fast \
  run-representation-quality-active \
  objective-mask-attribution run-objective-mask-attribution \
  repair-gate-test run-repair-gate-test repair-screen run-repair-screen \
  variance-necessity-gate-test run-variance-necessity-gate-test \
  variance-component-necessity-screen \
  run-variance-component-necessity-screen \
  outer-augmentation-gate-test run-outer-augmentation-gate-test \
  rmc-gate-test run-rmc-gate-test rmc-certification fspa-repair gpwd-repair \
  mpsr-repair oca1-audit oaa1-audit ima3-mask-repair-preflight \
  ima3-mask-repair-quality-trial \
  outer-augmentation-training-screen jmcd-gate-test run-jmcd-gate-test \
  jmcd-screen rssm-gate-test run-rssm-gate-test rssm-screen \
  rssm-reference-auditor-test run-rssm-reference-auditor-test \
  rssm-protocol-pin run-rssm-preflight psm-gate-test \
  run-psm-gate-test psm-protocol-pin psm-parent-pin psm-screen \
  run-psm-preflight
representation-quality: \
  $(TEST_OUT)/test_wikimyei_mtf_jepa_mae_vicreg_tokenizer_information \
  $(TEST_OUT)/quality_wikimyei_mtf_jepa_mae_vicreg_representation

run-tokenizer-information: run-test_wikimyei_mtf_jepa_mae_vicreg_tokenizer_information

run-representation-quality-fast: representation-quality
	$(Q)$(RUN_MODE) $(TEST_OUT)/quality_wikimyei_mtf_jepa_mae_vicreg_representation --tier fast --device cpu

run-representation-quality-active: representation-quality
	$(Q)CUBLAS_WORKSPACE_CONFIG=:4096:8 $(RUN_MODE) $(TEST_OUT)/quality_wikimyei_mtf_jepa_mae_vicreg_representation --tier active --device cuda

# Paired objective/context attribution plus the triggered TF/VICReg split.
objective-mask-attribution: \
  $(TEST_OUT)/quality_wikimyei_mtf_jepa_mae_vicreg_representation

run-objective-mask-attribution: objective-mask-attribution
	$(Q)CUBLAS_WORKSPACE_CONFIG=:4096:8 $(RUN_MODE) $(TEST_OUT)/quality_wikimyei_mtf_jepa_mae_vicreg_representation --experiment objective-mask-attribution --device cuda

repair-gate-test: $(TEST_OUT)/test_representation_objective_repair_gate
	@$(LOG_SUCCESS)

run-repair-gate-test: run-test_representation_objective_repair_gate

variance-necessity-gate-test: \
  $(TEST_OUT)/test_representation_vicreg_variance_necessity_gate
	@$(LOG_SUCCESS)

run-variance-necessity-gate-test: \
  run-test_representation_vicreg_variance_necessity_gate

outer-augmentation-gate-test: \
  $(TEST_OUT)/test_representation_outer_augmentation_training_gate
	@$(LOG_SUCCESS)

run-outer-augmentation-gate-test: \
  run-test_representation_outer_augmentation_training_gate

rmc-gate-test: $(TEST_OUT)/test_representation_module_certification_gate
	@$(LOG_SUCCESS)

run-rmc-gate-test: run-test_representation_module_certification_gate

rmc-certification: \
  $(TEST_OUT)/test_wikimyei_mtf_jepa_mae_vicreg_contracts \
  $(TEST_OUT)/test_structured_cdsb_sparse_v1 \
  $(TEST_OUT)/test_representation_module_certification_gate \
  $(TEST_OUT)/quality_wikimyei_mtf_jepa_mae_vicreg_representation_module_certification
	@$(LOG_SUCCESS)

fspa-repair: \
  $(TEST_OUT)/test_wikimyei_mtf_jepa_mae_vicreg_contracts \
  $(TEST_OUT)/test_structured_cdsb_sparse_v1 \
  $(TEST_OUT)/test_representation_module_certification_gate \
  $(TEST_OUT)/quality_wikimyei_mtf_jepa_mae_vicreg_frozen_sequence_projection_alignment
	@$(LOG_SUCCESS)

gpwd-repair: \
  $(TEST_OUT)/test_wikimyei_mtf_jepa_mae_vicreg_contracts \
  $(TEST_OUT)/test_structured_cdsb_sparse_v1 \
  $(TEST_OUT)/test_representation_module_certification_gate \
  $(TEST_OUT)/quality_wikimyei_mtf_jepa_mae_vicreg_geometry_preserving_whitening_distillation
	@$(LOG_SUCCESS)

mpsr-repair: \
  $(TEST_OUT)/test_wikimyei_mtf_jepa_mae_vicreg_contracts \
  $(TEST_OUT)/test_structured_cdsb_sparse_v1 \
  $(TEST_OUT)/test_representation_module_certification_gate \
  $(TEST_OUT)/quality_wikimyei_mtf_jepa_mae_vicreg_minimal_participation_spectral_repair
	@$(LOG_SUCCESS)

oca1-audit: \
  $(TEST_OUT)/test_wikimyei_mtf_jepa_mae_vicreg_contracts \
  $(TEST_OUT)/test_structured_cdsb_sparse_v1 \
  $(TEST_OUT)/test_representation_module_certification_gate \
  $(TEST_OUT)/quality_wikimyei_mtf_jepa_mae_vicreg_four_objective_causal_attribution
	@$(LOG_SUCCESS)

# Build-only OAA-1 surface. The long attribution remains explicitly opt-in.
oaa1-audit: \
  $(TEST_OUT)/test_wikimyei_mtf_jepa_mae_vicreg_contracts \
  $(TEST_OUT)/test_structured_cdsb_sparse_v1 \
  $(TEST_OUT)/test_representation_module_certification_gate \
  $(TEST_OUT)/test_outer_augmentation_causal_attribution_gate \
  $(TEST_OUT)/quality_wikimyei_mtf_jepa_mae_vicreg_outer_augmentation_causal_attribution
	@$(LOG_SUCCESS)

.PHONY: vva1-audit vva1-preflight vva1-attribution
vva1-audit: \
  $(TEST_OUT)/quality_wikimyei_mtf_jepa_mae_vicreg_view_augmentation_causal_attribution
	@$(LOG_SUCCESS)

vva1-preflight: vva1-audit
	$(Q)CUBLAS_WORKSPACE_CONFIG=:4096:8 $(RUN_MODE) $(TEST_OUT)/quality_wikimyei_mtf_jepa_mae_vicreg_view_augmentation_causal_attribution --experiment vicreg-view-augmentation-causal-attribution-preflight --device cuda --steps 512 --seeds 3

vva1-attribution: vva1-audit
	$(Q)CUBLAS_WORKSPACE_CONFIG=:4096:8 $(RUN_MODE) $(TEST_OUT)/quality_wikimyei_mtf_jepa_mae_vicreg_view_augmentation_causal_attribution --experiment vicreg-view-augmentation-causal-attribution --device cuda --steps 512 --seeds 3

# VVA-1B is deliberately excluded from the default surface. Every public entry
# acquires the same non-blocking lock, and the internal dependency chain fixes
# the only permitted order: custody/build -> CPU self-test -> CUDA preflight ->
# authoritative attribution.
.PHONY: vva1b-custody-locked vva1b-build-locked \
  vva1b-self-test-locked vva1b-preflight-locked \
  vva1b-attribution-locked vva1b-build vva1b-self-test \
  vva1b-preflight vva1b-attribution
.NOTPARALLEL: vva1b-custody-locked vva1b-build-locked \
  vva1b-self-test-locked vva1b-preflight-locked vva1b-attribution-locked

vva1b-custody-locked:
	$(Q)set -eu; \
	  test "$(VVA1B_LOCK_HELD)" = 1; \
	  cd "$(HERE_PATH)"; \
	  for sidecar in $(notdir $(VVA1B_CUSTODY_SIDECARS)); do \
	    test -f "$$sidecar"; \
	    sha256sum -c "$$sidecar"; \
	  done; \
	  cd "$(TEST_OUT)"; \
	  sha256sum -c "$(notdir $(VVA1B_FAILED_ATTEMPT_LOG)).sha256"; \
	  sha256sum -c "$(notdir $(VVA1B_FAILED_ATTEMPT_EXECUTABLE)).sha256"

vva1b-build-locked: vva1b-custody-locked
	$(Q)set -eu; \
	  test "$(VVA1B_LOCK_HELD)" = 1; \
	  $(MAKE) --no-print-directory "$(VVA1B_BINARY)"; \
	  nonce=$$$$.$${RANDOM:-0}; \
	  temporary_depfile="$(VVA1B_FULL_DEPFILE).tmp.$$nonce"; \
	  trap 'rm -f "$$temporary_depfile"' 0 HUP INT TERM; \
	  $(CXX_COMPILE) $(CXXFLAGS) -M -MF "$$temporary_depfile" \
	    -MT "$(VVA1B_BINARY)" $(USUAL_INCLUDE_PATHS) $(TORCH_INCLUDE_PATHS) \
	    "$(abspath $(VVA1B_SOURCE))"; \
	  test -s "$$temporary_depfile"; \
	  mv -f "$$temporary_depfile" "$(VVA1B_FULL_DEPFILE)"; \
	  bash "$(VVA1B_BUILD_MANIFEST_HELPER)" \
	    --project-root "$(abspath $(PROJECT_ROOT))" \
	    --binary "$(abspath $(VVA1B_BINARY))" \
	    --manifest "$(abspath $(VVA1B_BUILD_MANIFEST))" \
	    --depfile "$(abspath $(VVA1B_FULL_DEPFILE))" \
	    --compiler "$(firstword $(CXX_COMPILE))" \
	    --linker "$(firstword $(CXX_LINK))" \
	    --compile-command '$(CXX_COMPILE) $(CXXFLAGS) -MMD -MP -MF $(DEPDIR)/quality_wikimyei_mtf_jepa_mae_vicreg_view_loss_boundary_triad.d -MT $(VVA1B_BINARY) $(USUAL_INCLUDE_PATHS) $(TORCH_INCLUDE_PATHS) -c $(abspath $(VVA1B_SOURCE)) -o $(VVA1B_OBJECT)' \
	    --dependency-command '$(CXX_COMPILE) $(CXXFLAGS) -M -MF $(VVA1B_FULL_DEPFILE) -MT $(VVA1B_BINARY) $(USUAL_INCLUDE_PATHS) $(TORCH_INCLUDE_PATHS) $(abspath $(VVA1B_SOURCE))' \
	    --link-command '$(CXX_LINK) $(LDFLAGS_E) $(VVA1B_OBJECT) $(LDLIBS_common) -Wl,--start-group $(filter %.a,$(TEST_DEFAULT_LDLIBS) $(VVA1B_LDLIBS_EXTRA)) -Wl,--end-group $(filter-out %.a,$(TEST_DEFAULT_LDLIBS) $(VVA1B_LDLIBS_EXTRA)) $(LDLIBS_EXTRA) -o $(VVA1B_BINARY)' \
	    --extra-inputs '$(VVA1B_BUILD_EXTRA_INPUTS)'; \
	  trap - 0 HUP INT TERM

vva1b-self-test-locked: vva1b-build-locked
	$(Q)set -eu; \
	  test "$(VVA1B_LOCK_HELD)" = 1; \
	  manifest_digest=$$(awk 'NR == 1 { print $$1 }' "$(VVA1B_BUILD_MANIFEST).sha256"); \
	  test -n "$$manifest_digest"; \
	  if [ -e "$(VVA1B_SELF_TEST_LOG).sha256" ] && [ ! -f "$(VVA1B_SELF_TEST_LOG)" ]; then \
	    echo 'VVA-1B self-test has a sidecar without its immutable log' >&2; exit 75; \
	  fi; \
	  if [ -e "$(VVA1B_SELF_TEST_LOG)" ]; then \
	    test -f "$(VVA1B_SELF_TEST_LOG)"; \
	    grep -Fqx 'execution_status=vva1b_self_test_complete' "$(VVA1B_SELF_TEST_LOG)"; \
	    grep -Fqx 'vva1b.self_test.cache_integer_archive_roundtrip=true' "$(VVA1B_SELF_TEST_LOG)"; \
	    grep -Fqx 'vva1b.self_test.cache_resume_accounting=true' "$(VVA1B_SELF_TEST_LOG)"; \
	    grep -Fqx 'vva1b.self_test.post_resume_failure_accounting=true' "$(VVA1B_SELF_TEST_LOG)"; \
	    grep -Fqx 'vva1b.self_test.cache_codec_optimizer_updates=0' "$(VVA1B_SELF_TEST_LOG)"; \
	    grep -Fqx 'vva1b.self_test.cache_codec_ema_updates=0' "$(VVA1B_SELF_TEST_LOG)"; \
	    grep -Fqx "vva1b.wrapper.build_manifest_sha256=$$manifest_digest" "$(VVA1B_SELF_TEST_LOG)"; \
	    if [ ! -e "$(VVA1B_SELF_TEST_LOG).sha256" ]; then \
	      recovery_checksum="$(VVA1B_SELF_TEST_LOG).sha256.tmp.recover.$$$$"; \
	      trap 'rm -f "$$recovery_checksum"' 0 HUP INT TERM; \
	      checksum=$$(sha256sum "$(VVA1B_SELF_TEST_LOG)" | awk '{ print $$1 }'); \
	      printf '%s  %s\n' "$$checksum" "$(notdir $(VVA1B_SELF_TEST_LOG))" > "$$recovery_checksum"; \
	      ln "$$recovery_checksum" "$(VVA1B_SELF_TEST_LOG).sha256"; \
	      rm -f "$$recovery_checksum"; trap - 0 HUP INT TERM; \
	    else test -f "$(VVA1B_SELF_TEST_LOG).sha256"; fi; \
	    (cd "$(TEST_OUT)" && sha256sum -c "$(notdir $(VVA1B_SELF_TEST_LOG)).sha256"); \
	    grep -Fqx 'execution_status=vva1b_self_test_complete' "$(VVA1B_SELF_TEST_LOG)"; \
	    grep -Fqx "vva1b.wrapper.build_manifest_sha256=$$manifest_digest" "$(VVA1B_SELF_TEST_LOG)"; \
	    exit 0; \
	  fi; \
	  nonce=$$$$.$${RANDOM:-0}; \
	  temporary_log="$(VVA1B_SELF_TEST_LOG).tmp.$$nonce"; \
	  temporary_checksum="$(VVA1B_SELF_TEST_LOG).sha256.tmp.$$nonce"; \
	  trap 'rm -f "$$temporary_log" "$$temporary_checksum"' 0 HUP INT TERM; \
	  status=0; \
	  cd "$(PROJECT_ROOT)"; \
	  $(RUN_MODE) "$(VVA1B_BINARY)" \
	    --experiment vicreg-view-loss-boundary-triad-self-test \
	    --device cpu > "$$temporary_log" 2>&1 || status=$$?; \
	  if [ "$$status" -eq 0 ] && \
	     grep -Fqx 'execution_status=vva1b_self_test_complete' "$$temporary_log" && \
	     grep -Fqx 'vva1b.self_test.cache_integer_archive_roundtrip=true' "$$temporary_log" && \
	     grep -Fqx 'vva1b.self_test.cache_resume_accounting=true' "$$temporary_log" && \
	     grep -Fqx 'vva1b.self_test.post_resume_failure_accounting=true' "$$temporary_log" && \
	     grep -Fqx 'vva1b.self_test.cache_codec_optimizer_updates=0' "$$temporary_log" && \
	     grep -Fqx 'vva1b.self_test.cache_codec_ema_updates=0' "$$temporary_log"; then \
	    printf 'vva1b.wrapper.build_manifest_sha256=%s\n' "$$manifest_digest" >> "$$temporary_log"; \
	    checksum=$$(sha256sum "$$temporary_log" | awk '{ print $$1 }'); \
	    printf '%s  %s\n' "$$checksum" "$(notdir $(VVA1B_SELF_TEST_LOG))" > "$$temporary_checksum"; \
	    ln "$$temporary_log" "$(VVA1B_SELF_TEST_LOG)"; rm -f "$$temporary_log"; \
	    ln "$$temporary_checksum" "$(VVA1B_SELF_TEST_LOG).sha256"; rm -f "$$temporary_checksum"; \
	    (cd "$(TEST_OUT)" && sha256sum -c "$(notdir $(VVA1B_SELF_TEST_LOG)).sha256"); \
	    grep -Fqx 'execution_status=vva1b_self_test_complete' "$(VVA1B_SELF_TEST_LOG)"; \
	    grep -Fqx "vva1b.wrapper.build_manifest_sha256=$$manifest_digest" "$(VVA1B_SELF_TEST_LOG)"; \
	  else \
	    if [ "$$status" -eq 0 ]; then status=76; fi; \
	    failed_log="$(TEST_OUT)/representation_vva1b_v3_self_test_failed.$$nonce.log"; \
	    mv "$$temporary_log" "$$failed_log"; \
	    checksum=$$(sha256sum "$$failed_log" | awk '{ print $$1 }'); \
	    printf '%s  %s\n' "$$checksum" "$${failed_log##*/}" > "$$failed_log.sha256.tmp"; \
	    mv "$$failed_log.sha256.tmp" "$$failed_log.sha256"; \
	    echo "VVA-1B failed self-test preserved at $$failed_log" >&2; \
	    trap - 0 HUP INT TERM; exit "$$status"; \
	  fi; \
	  trap - 0 HUP INT TERM

vva1b-preflight-locked: vva1b-self-test-locked
	$(Q)set -eu; \
	  test "$(VVA1B_LOCK_HELD)" = 1; \
	  manifest_digest=$$(awk 'NR == 1 { print $$1 }' "$(VVA1B_BUILD_MANIFEST).sha256"); \
	  self_test_digest=$$(awk 'NR == 1 { print $$1 }' "$(VVA1B_SELF_TEST_LOG).sha256"); \
	  test -n "$$manifest_digest" && test -n "$$self_test_digest"; \
	  if [ -e "$(VVA1B_PREFLIGHT_LOG).sha256" ] && [ ! -f "$(VVA1B_PREFLIGHT_LOG)" ]; then \
	    echo 'VVA-1B preflight has a sidecar without its immutable log' >&2; exit 75; \
	  fi; \
	  if [ -e "$(VVA1B_PREFLIGHT_LOG)" ]; then \
	    test -f "$(VVA1B_PREFLIGHT_LOG)"; \
	    grep -Fqx 'execution_status=vva1b_preflight_complete' "$(VVA1B_PREFLIGHT_LOG)"; \
	    grep -Fqx 'bootstrap_replicates=512' "$(VVA1B_PREFLIGHT_LOG)"; \
	    grep -Fqx 'vva1b.recovery.seed_cache_inventory_empty=true' "$(VVA1B_PREFLIGHT_LOG)"; \
	    grep -Fqx 'vva1b.confirmation.sealed=true' "$(VVA1B_PREFLIGHT_LOG)"; \
	    grep -Fqx 'vva1b.training.opened=false' "$(VVA1B_PREFLIGHT_LOG)"; \
	    grep -Fqx "vva1b.wrapper.build_manifest_sha256=$$manifest_digest" "$(VVA1B_PREFLIGHT_LOG)"; \
	    grep -Fqx "vva1b.wrapper.self_test_sha256=$$self_test_digest" "$(VVA1B_PREFLIGHT_LOG)"; \
	    if [ ! -e "$(VVA1B_PREFLIGHT_LOG).sha256" ]; then \
	      recovery_checksum="$(VVA1B_PREFLIGHT_LOG).sha256.tmp.recover.$$$$"; \
	      trap 'rm -f "$$recovery_checksum"' 0 HUP INT TERM; \
	      checksum=$$(sha256sum "$(VVA1B_PREFLIGHT_LOG)" | awk '{ print $$1 }'); \
	      printf '%s  %s\n' "$$checksum" "$(notdir $(VVA1B_PREFLIGHT_LOG))" > "$$recovery_checksum"; \
	      ln "$$recovery_checksum" "$(VVA1B_PREFLIGHT_LOG).sha256"; \
	      rm -f "$$recovery_checksum"; trap - 0 HUP INT TERM; \
	    else test -f "$(VVA1B_PREFLIGHT_LOG).sha256"; fi; \
	    (cd "$(TEST_OUT)" && sha256sum -c "$(notdir $(VVA1B_PREFLIGHT_LOG)).sha256"); \
	    grep -Fqx 'execution_status=vva1b_preflight_complete' "$(VVA1B_PREFLIGHT_LOG)"; \
	    grep -Fqx "vva1b.wrapper.build_manifest_sha256=$$manifest_digest" "$(VVA1B_PREFLIGHT_LOG)"; \
	    grep -Fqx "vva1b.wrapper.self_test_sha256=$$self_test_digest" "$(VVA1B_PREFLIGHT_LOG)"; \
	    exit 0; \
	  fi; \
	  nonce=$$$$.$${RANDOM:-0}; \
	  temporary_log="$(VVA1B_PREFLIGHT_LOG).tmp.$$nonce"; \
	  temporary_checksum="$(VVA1B_PREFLIGHT_LOG).sha256.tmp.$$nonce"; \
	  trap 'rm -f "$$temporary_log" "$$temporary_checksum"' 0 HUP INT TERM; \
	  status=0; \
	  cd "$(PROJECT_ROOT)"; \
	  CUBLAS_WORKSPACE_CONFIG=:4096:8 $(RUN_MODE) "$(VVA1B_BINARY)" \
	    --experiment vicreg-view-loss-boundary-triad-preflight \
	    --device cuda --steps 512 --seeds 3 > "$$temporary_log" 2>&1 || status=$$?; \
	  if [ "$$status" -eq 0 ] && \
	     grep -Fqx 'execution_status=vva1b_preflight_complete' "$$temporary_log" && \
	     grep -Fqx 'bootstrap_replicates=512' "$$temporary_log" && \
	     grep -Fqx 'vva1b.recovery.seed_cache_inventory_empty=true' "$$temporary_log" && \
	     grep -Fqx 'vva1b.confirmation.sealed=true' "$$temporary_log" && \
	     grep -Fqx 'vva1b.training.opened=false' "$$temporary_log"; then \
	    printf 'vva1b.wrapper.build_manifest_sha256=%s\n' "$$manifest_digest" >> "$$temporary_log"; \
	    printf 'vva1b.wrapper.self_test_sha256=%s\n' "$$self_test_digest" >> "$$temporary_log"; \
	    checksum=$$(sha256sum "$$temporary_log" | awk '{ print $$1 }'); \
	    printf '%s  %s\n' "$$checksum" "$(notdir $(VVA1B_PREFLIGHT_LOG))" > "$$temporary_checksum"; \
	    ln "$$temporary_log" "$(VVA1B_PREFLIGHT_LOG)"; rm -f "$$temporary_log"; \
	    ln "$$temporary_checksum" "$(VVA1B_PREFLIGHT_LOG).sha256"; rm -f "$$temporary_checksum"; \
	    (cd "$(TEST_OUT)" && sha256sum -c "$(notdir $(VVA1B_PREFLIGHT_LOG)).sha256"); \
	    grep -Fqx 'execution_status=vva1b_preflight_complete' "$(VVA1B_PREFLIGHT_LOG)"; \
	    grep -Fqx "vva1b.wrapper.build_manifest_sha256=$$manifest_digest" "$(VVA1B_PREFLIGHT_LOG)"; \
	    grep -Fqx "vva1b.wrapper.self_test_sha256=$$self_test_digest" "$(VVA1B_PREFLIGHT_LOG)"; \
	  else \
	    if [ "$$status" -eq 0 ]; then status=76; fi; \
	    failed_log="$(TEST_OUT)/representation_vva1b_v3_preflight_failed.$$nonce.log"; \
	    mv "$$temporary_log" "$$failed_log"; \
	    checksum=$$(sha256sum "$$failed_log" | awk '{ print $$1 }'); \
	    printf '%s  %s\n' "$$checksum" "$${failed_log##*/}" > "$$failed_log.sha256.tmp"; \
	    mv "$$failed_log.sha256.tmp" "$$failed_log.sha256"; \
	    echo "VVA-1B failed CUDA preflight preserved at $$failed_log" >&2; \
	    trap - 0 HUP INT TERM; exit "$$status"; \
	  fi; \
	  trap - 0 HUP INT TERM

vva1b-attribution-locked: vva1b-preflight-locked
	$(Q)set -eu; \
	  test "$(VVA1B_LOCK_HELD)" = 1; \
	  (cd "$(TEST_OUT)" && sha256sum -c "$(notdir $(VVA1B_SELF_TEST_LOG)).sha256"); \
	  (cd "$(TEST_OUT)" && sha256sum -c "$(notdir $(VVA1B_PREFLIGHT_LOG)).sha256"); \
	  manifest_digest=$$(awk 'NR == 1 { print $$1 }' "$(VVA1B_BUILD_MANIFEST).sha256"); \
	  self_test_digest=$$(awk 'NR == 1 { print $$1 }' "$(VVA1B_SELF_TEST_LOG).sha256"); \
	  preflight_digest=$$(awk 'NR == 1 { print $$1 }' "$(VVA1B_PREFLIGHT_LOG).sha256"); \
	  test -n "$$manifest_digest" && test -n "$$self_test_digest" && test -n "$$preflight_digest"; \
	  if [ -e "$(VVA1B_AUTHORITATIVE_LOG).sha256" ] && [ ! -f "$(VVA1B_AUTHORITATIVE_LOG)" ]; then \
	    echo 'VVA-1B authoritative sidecar exists without its immutable log' >&2; exit 75; \
	  fi; \
	  if [ -e "$(VVA1B_AUTHORITATIVE_LOG)" ]; then \
	    test -f "$(VVA1B_AUTHORITATIVE_LOG)"; \
	    grep -Fqx 'execution_status=vva1b_measurements_complete' "$(VVA1B_AUTHORITATIVE_LOG)"; \
	    grep -Fqx 'vva1b.recovery.physical_attempted_adam_updates=6144' "$(VVA1B_AUTHORITATIVE_LOG)"; \
	    grep -Fqx 'vva1b.recovery.physical_attempted_ema_updates=6144' "$(VVA1B_AUTHORITATIVE_LOG)"; \
	    grep -Fqx 'vva1b.recovery.accepted_authoritative_adam_updates=4608' "$(VVA1B_AUTHORITATIVE_LOG)"; \
	    grep -Fqx 'vva1b.recovery.accepted_authoritative_ema_updates=4608' "$(VVA1B_AUTHORITATIVE_LOG)"; \
	    grep -Fqx "vva1b.wrapper.build_manifest_sha256=$$manifest_digest" "$(VVA1B_AUTHORITATIVE_LOG)"; \
	    grep -Fqx "vva1b.wrapper.self_test_sha256=$$self_test_digest" "$(VVA1B_AUTHORITATIVE_LOG)"; \
	    grep -Fqx "vva1b.wrapper.preflight_sha256=$$preflight_digest" "$(VVA1B_AUTHORITATIVE_LOG)"; \
	    if [ ! -e "$(VVA1B_AUTHORITATIVE_LOG).sha256" ]; then \
	      recovery_checksum="$(VVA1B_AUTHORITATIVE_LOG).sha256.tmp.recover.$$$$"; \
	      trap 'rm -f "$$recovery_checksum"' 0 HUP INT TERM; \
	      checksum=$$(sha256sum "$(VVA1B_AUTHORITATIVE_LOG)" | awk '{ print $$1 }'); \
	      printf '%s  %s\n' "$$checksum" "$(notdir $(VVA1B_AUTHORITATIVE_LOG))" > "$$recovery_checksum"; \
	      ln "$$recovery_checksum" "$(VVA1B_AUTHORITATIVE_LOG).sha256"; \
	      rm -f "$$recovery_checksum"; trap - 0 HUP INT TERM; \
	    else test -f "$(VVA1B_AUTHORITATIVE_LOG).sha256"; fi; \
	    (cd "$(TEST_OUT)" && sha256sum -c "$(notdir $(VVA1B_AUTHORITATIVE_LOG)).sha256"); \
	    exit 0; \
	  fi; \
	  nonce=$$$$.$${RANDOM:-0}; \
	  temporary_log="$(VVA1B_AUTHORITATIVE_LOG).tmp.$$nonce"; \
	  temporary_checksum="$(VVA1B_AUTHORITATIVE_LOG).sha256.tmp.$$nonce"; \
	  trap 'rm -f "$$temporary_log" "$$temporary_checksum"' 0 HUP INT TERM; \
	  status=0; \
	  cd "$(PROJECT_ROOT)"; \
	  CUBLAS_WORKSPACE_CONFIG=:4096:8 $(RUN_MODE) "$(VVA1B_BINARY)" \
	    --experiment vicreg-view-loss-boundary-triad \
	    --device cuda --steps 512 --seeds 3 > "$$temporary_log" 2>&1 || status=$$?; \
	  if [ "$$status" -eq 0 ] && \
	     grep -Fqx 'execution_status=vva1b_measurements_complete' "$$temporary_log" && \
	     grep -Fqx 'vva1b.recovery.physical_attempted_adam_updates=6144' "$$temporary_log" && \
	     grep -Fqx 'vva1b.recovery.physical_attempted_ema_updates=6144' "$$temporary_log" && \
	     grep -Fqx 'vva1b.recovery.accepted_authoritative_adam_updates=4608' "$$temporary_log" && \
	     grep -Fqx 'vva1b.recovery.accepted_authoritative_ema_updates=4608' "$$temporary_log"; then \
	    printf 'vva1b.wrapper.build_manifest_sha256=%s\n' "$$manifest_digest" >> "$$temporary_log"; \
	    printf 'vva1b.wrapper.self_test_sha256=%s\n' "$$self_test_digest" >> "$$temporary_log"; \
	    printf 'vva1b.wrapper.preflight_sha256=%s\n' "$$preflight_digest" >> "$$temporary_log"; \
	    checksum=$$(sha256sum "$$temporary_log" | awk '{ print $$1 }'); \
	    printf '%s  %s\n' "$$checksum" "$(notdir $(VVA1B_AUTHORITATIVE_LOG))" > "$$temporary_checksum"; \
	    ln "$$temporary_log" "$(VVA1B_AUTHORITATIVE_LOG)"; rm -f "$$temporary_log"; \
	    ln "$$temporary_checksum" "$(VVA1B_AUTHORITATIVE_LOG).sha256"; rm -f "$$temporary_checksum"; \
	  else \
	    if [ "$$status" -eq 0 ]; then status=76; fi; \
	    failed_log="$(TEST_OUT)/representation_vva1b_v3_authoritative_failed.$$nonce.log"; \
	    mv "$$temporary_log" "$$failed_log"; \
	    checksum=$$(sha256sum "$$failed_log" | awk '{ print $$1 }'); \
	    printf '%s  %s\n' "$$checksum" "$${failed_log##*/}" > "$$failed_log.sha256.tmp"; \
	    mv "$$failed_log.sha256.tmp" "$$failed_log.sha256"; \
	    echo "VVA-1B failed authoritative diagnostic preserved at $$failed_log" >&2; \
	    trap - 0 HUP INT TERM; exit "$$status"; \
	  fi; \
	  trap - 0 HUP INT TERM

vva1b-build:
	$(Q)set -eu; mkdir -p "$(TEST_OUT)"; exec 8>"$(VVA1B_BUILD_RUN_LOCK)"; \
	  if ! flock -n 8; then echo 'VVA-1B build or run is already active' >&2; exit 73; fi; \
	  $(MAKE) --no-print-directory VVA1B_LOCK_HELD=1 vva1b-build-locked

vva1b-self-test:
	$(Q)set -eu; mkdir -p "$(TEST_OUT)"; exec 8>"$(VVA1B_BUILD_RUN_LOCK)"; \
	  if ! flock -n 8; then echo 'VVA-1B build or run is already active' >&2; exit 73; fi; \
	  $(MAKE) --no-print-directory VVA1B_LOCK_HELD=1 vva1b-self-test-locked

vva1b-preflight:
	$(Q)set -eu; mkdir -p "$(TEST_OUT)"; exec 8>"$(VVA1B_BUILD_RUN_LOCK)"; \
	  if ! flock -n 8; then echo 'VVA-1B build or run is already active' >&2; exit 73; fi; \
	  $(MAKE) --no-print-directory VVA1B_LOCK_HELD=1 vva1b-preflight-locked

vva1b-attribution:
	$(Q)set -eu; mkdir -p "$(TEST_OUT)"; exec 8>"$(VVA1B_BUILD_RUN_LOCK)"; \
	  if ! flock -n 8; then echo 'VVA-1B build or run is already active' >&2; exit 73; fi; \
	  $(MAKE) --no-print-directory VVA1B_LOCK_HELD=1 vva1b-attribution-locked

.PHONY: gpv1-audit-locked gpv1-audit gpv1-self-test gpv1-preflight \
  gpv1-attribution gpv1-confirmation
gpv1-audit-locked: $(GPV1_BINARY)
	$(Q)set -eu; \
	  test "$(GPV1_LOCK_HELD)" = 1; \
	  nonce=$$$$; \
	  temporary_receipt="$(GPV1_BUILD_RECEIPT).tmp.$$nonce"; \
	  temporary_raw_receipt="$(GPV1_BUILD_RECEIPT).raw.tmp.$$nonce"; \
	  temporary_depfile="$(GPV1_FULL_DEPFILE).tmp.$$nonce"; \
	  temporary_ldd="$(GPV1_BUILD_RECEIPT).ldd.tmp.$$nonce"; \
	  temporary_dso_paths="$(GPV1_BUILD_RECEIPT).dso.tmp.$$nonce"; \
	  temporary_dso_records="$(GPV1_BUILD_RECEIPT).dso_records.tmp.$$nonce"; \
	  trap 'rm -f "$$temporary_receipt" "$$temporary_raw_receipt" "$$temporary_depfile" "$$temporary_ldd" "$$temporary_dso_paths" "$$temporary_dso_records"' 0 HUP INT TERM; \
	  compiler_path=$$(readlink -f "$$(command -v "$(firstword $(CXX_COMPILE))")"); \
	  linker_path=$$(readlink -f "$$(command -v "$(firstword $(CXX_LINK))")"); \
	  cc1plus_path=$$(readlink -f "$$($(CXX_COMPILE) -print-prog-name=cc1plus)"); \
	  collect2_path=$$(readlink -f "$$($(CXX_LINK) -print-prog-name=collect2)"); \
	  ld_program=$$($(CXX_LINK) -print-prog-name=ld); \
	  ld_path=$$(readlink -f "$$(command -v "$$ld_program")"); \
	  as_program=$$($(CXX_COMPILE) -print-prog-name=as); \
	  as_path=$$(readlink -f "$$(command -v "$$as_program")"); \
	  for tool in "$$compiler_path" "$$linker_path" "$$cc1plus_path" "$$collect2_path" "$$ld_path" "$$as_path"; do test -x "$$tool"; done; \
	  test -x "$(GPV1_BINARY)"; \
	  $(CXX_COMPILE) $(CXXFLAGS) -M -MF "$$temporary_depfile" -MT "$(GPV1_BINARY)" \
	    $(USUAL_INCLUDE_PATHS) $(TORCH_INCLUDE_PATHS) \
	    $(abspath quality_wikimyei_mtf_jepa_mae_vicreg_global_pool_projector_variance_causal_decomposition.cpp); \
	  test -s "$$temporary_depfile"; \
	  mv -f "$$temporary_depfile" "$(GPV1_FULL_DEPFILE)"; \
	  test -x "$(GPV1_BINARY)"; \
	  ldd "$(GPV1_BINARY)" > "$$temporary_ldd"; \
	  awk '$$2 == "=>" && $$3 ~ /^\// { print $$3 } $$1 ~ /^\// { print $$1 }' "$$temporary_ldd" > "$$temporary_dso_paths"; \
	  sort -u -o "$$temporary_dso_paths" "$$temporary_dso_paths"; \
	  : > "$$temporary_dso_records"; \
	  while IFS= read -r dso; do \
	    resolved=$$(readlink -f "$$dso"); test -f "$$resolved"; \
	    case "$$resolved" in \
	      $(abspath $(PROJECT_ROOT))/*) logical="repo/$${resolved#$(abspath $(PROJECT_ROOT))/}" ;; \
	      /usr/local/cuda-12.4/*) logical="cuda/$${resolved#/usr/local/cuda-12.4/}" ;; \
	      *) logical="host$$resolved" ;; \
	    esac; \
	    digest_line=$$(sha256sum "$$resolved"); digest=$${digest_line%% *}; \
	    printf 'dso:%s=%s\n' "$$logical" "$$digest" >> "$$temporary_dso_records"; \
	  done < "$$temporary_dso_paths"; \
	  LC_ALL=C sort -u -o "$$temporary_dso_records" "$$temporary_dso_records"; \
	  dso_count=$$(wc -l < "$$temporary_dso_records"); test "$$dso_count" -gt 0; \
	  logical_dso_count=$$(cut -d= -f1 "$$temporary_dso_records" | uniq | wc -l); \
	  test "$$logical_dso_count" -eq "$$dso_count"; \
	  { \
	    printf '%s\n' 'schema=gpv1.effective_build_receipt.v2'; \
	    printf '%s\n' 'workspace.logical_root=repo'; \
	    printf '%s\n' 'compile.command=$(CXX_COMPILE) $(CXXFLAGS) -MMD -MP -MF $(DEPDIR)/quality_wikimyei_mtf_jepa_mae_vicreg_global_pool_projector_variance_causal_decomposition.d -MT $(GPV1_BINARY) $(USUAL_INCLUDE_PATHS) $(TORCH_INCLUDE_PATHS) -c $(abspath quality_wikimyei_mtf_jepa_mae_vicreg_global_pool_projector_variance_causal_decomposition.cpp) -o $(OBJTMP)/quality_wikimyei_mtf_jepa_mae_vicreg_global_pool_projector_variance_causal_decomposition.o'; \
	    printf '%s\n' 'dependency_scan.command=$(CXX_COMPILE) $(CXXFLAGS) -M -MF $(GPV1_FULL_DEPFILE) -MT $(GPV1_BINARY) $(USUAL_INCLUDE_PATHS) $(TORCH_INCLUDE_PATHS) $(abspath quality_wikimyei_mtf_jepa_mae_vicreg_global_pool_projector_variance_causal_decomposition.cpp)'; \
	    printf '%s\n' 'link.command=$(CXX_LINK) $(LDFLAGS_E) $(OBJTMP)/quality_wikimyei_mtf_jepa_mae_vicreg_global_pool_projector_variance_causal_decomposition.o $(LDLIBS_common) -Wl,--start-group $(filter %.a,$(TEST_DEFAULT_LDLIBS) $(GPV1_LDLIBS_EXTRA)) -Wl,--end-group $(filter-out %.a,$(TEST_DEFAULT_LDLIBS) $(GPV1_LDLIBS_EXTRA)) $(LDLIBS_EXTRA) -o $(GPV1_BINARY)'; \
	    for specification in compiler:"$$compiler_path" linker_driver:"$$linker_path" cc1plus:"$$cc1plus_path" collect2:"$$collect2_path" ld:"$$ld_path" assembler:"$$as_path"; do \
	      name=$${specification%%:*}; path=$${specification#*:}; \
	      digest_line=$$(sha256sum "$$path"); digest=$${digest_line%% *}; \
	      printf 'tool:%s=%s\n' "$$name" "$$digest"; \
	    done; \
	    cat "$$temporary_dso_records"; \
	    printf 'dso.count=%s\n' "$$dso_count"; \
	  } > "$$temporary_raw_receipt"; \
	  sed 's#$(abspath $(PROJECT_ROOT))/#$${REPO}/#g' "$$temporary_raw_receipt" > "$$temporary_receipt"; \
	  test -s "$$temporary_receipt"; \
	  mv -f "$$temporary_receipt" "$(GPV1_BUILD_RECEIPT)"; \
	  rm -f "$$temporary_raw_receipt" "$$temporary_ldd" "$$temporary_dso_paths" "$$temporary_dso_records"; \
	  trap - 0 HUP INT TERM
	@$(LOG_SUCCESS)

gpv1-audit:
	$(Q)set -eu; mkdir -p "$(TEST_OUT)"; exec 8>"$(GPV1_BUILD_RUN_LOCK)"; \
	  if ! flock -n 8; then echo 'GPV-1 build or run is already active' >&2; exit 73; fi; \
	  $(MAKE) --no-print-directory GPV1_LOCK_HELD=1 gpv1-audit-locked

gpv1-self-test:
	$(Q)set -eu; mkdir -p "$(TEST_OUT)"; exec 8>"$(GPV1_BUILD_RUN_LOCK)"; \
	  if ! flock -n 8; then echo 'GPV-1 build or run is already active' >&2; exit 73; fi; \
	  $(MAKE) --no-print-directory GPV1_LOCK_HELD=1 gpv1-audit-locked; \
	  cd $(PROJECT_ROOT); \
	  $(RUN_MODE) $(GPV1_BINARY) --experiment global-pool-projector-variance-causal-decomposition-self-test --device cpu

gpv1-preflight:
	$(Q)set -eu; mkdir -p "$(TEST_OUT)"; exec 8>"$(GPV1_BUILD_RUN_LOCK)"; \
	  if ! flock -n 8; then echo 'GPV-1 build or run is already active' >&2; exit 73; fi; \
	  $(MAKE) --no-print-directory GPV1_LOCK_HELD=1 gpv1-audit-locked; \
	  cd $(PROJECT_ROOT); \
	  CUBLAS_WORKSPACE_CONFIG=:4096:8 $(RUN_MODE) $(GPV1_BINARY) --experiment global-pool-projector-variance-causal-decomposition-preflight --device cuda --steps 512 --seeds 3

gpv1-attribution:
	$(Q)set -eu; mkdir -p "$(TEST_OUT)"; exec 8>"$(GPV1_BUILD_RUN_LOCK)"; \
	  if ! flock -n 8; then echo 'GPV-1 build or run is already active' >&2; exit 73; fi; \
	  $(MAKE) --no-print-directory GPV1_LOCK_HELD=1 gpv1-audit-locked; \
	  cd $(PROJECT_ROOT); \
	  authoritative_log="$(TEST_OUT)/representation_gpv1_v2_authoritative.log"; \
	  authoritative_checksum="$$authoritative_log.sha256"; \
	  failure_log="$(TEST_OUT)/representation_gpv1_v2_failed.$$$$.log"; \
	  commit_lock="$(TEST_OUT)/gpv1_authoritative_log_commit.lock"; \
	  temporary_log="$$authoritative_log.tmp.$$$$"; \
	  temporary_checksum="$$authoritative_checksum.tmp.$$$$"; \
	  trap 'rm -f "$$temporary_log" "$$temporary_checksum"' 0 HUP INT TERM; \
	  exec 9>"$$commit_lock"; \
	  if ! flock -n 9; then echo 'GPV-1 authoritative log is already being produced' >&2; exit 73; fi; \
	  if [ -e "$$authoritative_log" ] || [ -e "$$authoritative_checksum" ]; then echo 'GPV-1 immutable authoritative evidence already exists' >&2; exit 74; fi; \
	  if CUBLAS_WORKSPACE_CONFIG=:4096:8 $(RUN_MODE) $(GPV1_BINARY) --experiment global-pool-projector-variance-causal-decomposition-attribution --device cuda --steps 512 --seeds 3 > "$$temporary_log" 2>&1; then \
	    checksum_line=$$(sha256sum "$$temporary_log"); checksum=$${checksum_line%% *}; \
	    printf '%s  %s\n' "$$checksum" 'representation_gpv1_v2_authoritative.log' > "$$temporary_checksum"; \
	    mv -f "$$temporary_log" "$$authoritative_log"; \
	    mv -f "$$temporary_checksum" "$$authoritative_checksum"; \
	  else status=$$?; mv -f "$$temporary_log" "$$failure_log"; \
	    echo "GPV-1 failed diagnostic preserved at $$failure_log" >&2; \
	    trap - 0 HUP INT TERM; exit $$status; fi; \
	  trap - 0 HUP INT TERM

gpv1-confirmation:
	$(Q)set -eu; mkdir -p "$(TEST_OUT)"; exec 8>"$(GPV1_BUILD_RUN_LOCK)"; \
	  if ! flock -n 8; then echo 'GPV-1 build or run is already active' >&2; exit 73; fi; \
	  $(MAKE) --no-print-directory GPV1_LOCK_HELD=1 gpv1-audit-locked; \
	  cd $(PROJECT_ROOT); \
	  CUBLAS_WORKSPACE_CONFIG=:4096:8 $(RUN_MODE) $(GPV1_BINARY) --experiment global-pool-projector-variance-causal-decomposition-confirmation --device cuda --steps 512 --seeds 3

.PHONY: ima1-stage0 run-ima1-stage0
ima1-stage0: $(TEST_OUT)/test_wikimyei_mtf_jepa_mae_vicreg_ima1_stage0
	@$(LOG_SUCCESS)

run-ima1-stage0: ima1-stage0
	$(Q)$(RUN_MODE) $(TEST_OUT)/test_wikimyei_mtf_jepa_mae_vicreg_ima1_stage0

.PHONY: ima2-mask-contract run-ima2-mask-contract
ima2-mask-contract: $(TEST_OUT)/test_wikimyei_mtf_jepa_mae_vicreg_ima2_mask_contract
	@$(LOG_SUCCESS)

run-ima2-mask-contract: ima2-mask-contract
	$(Q)$(RUN_MODE) $(TEST_OUT)/test_wikimyei_mtf_jepa_mae_vicreg_ima2_mask_contract

ima3-mask-repair-preflight: \
  $(TEST_OUT)/quality_wikimyei_mtf_jepa_mae_vicreg_dose_matched_mask_repair
	$(Q)CUBLAS_WORKSPACE_CONFIG=:4096:8 $(RUN_MODE) $(TEST_OUT)/quality_wikimyei_mtf_jepa_mae_vicreg_dose_matched_mask_repair --experiment dose-matched-mask-repair-preflight --device cuda

ima3-mask-repair-quality-trial: \
  $(TEST_OUT)/quality_wikimyei_mtf_jepa_mae_vicreg_dose_matched_mask_repair
	$(Q)CUBLAS_WORKSPACE_CONFIG=:4096:8 $(RUN_MODE) $(TEST_OUT)/quality_wikimyei_mtf_jepa_mae_vicreg_dose_matched_mask_repair --experiment dose-matched-mask-repair-quality-trial --device cuda

.PHONY: ima4a-target-predictability \
  run-ima4a-target-predictability-self-test \
  run-ima4a-target-predictability-audit
ima4a-target-predictability: \
  $(TEST_OUT)/quality_wikimyei_mtf_jepa_mae_vicreg_target_predictability_ceiling
	@$(LOG_SUCCESS)

run-ima4a-target-predictability-self-test: ima4a-target-predictability
	$(Q)cd $(PROJECT_ROOT) && $(RUN_MODE) $(TEST_OUT)/quality_wikimyei_mtf_jepa_mae_vicreg_target_predictability_ceiling --experiment target-predictability-ceiling-self-test --device cpu

run-ima4a-target-predictability-audit: ima4a-target-predictability
	$(Q)cd $(PROJECT_ROOT) && CUBLAS_WORKSPACE_CONFIG=:4096:8 $(RUN_MODE) $(TEST_OUT)/quality_wikimyei_mtf_jepa_mae_vicreg_target_predictability_ceiling --experiment target-predictability-ceiling-audit --device cuda > $(TEST_OUT)/representation_ima4a_v1_authoritative.log 2>&1
	$(Q)sha256sum $(TEST_OUT)/representation_ima4a_v1_authoritative.log > $(TEST_OUT)/representation_ima4a_v1_authoritative.log.sha256

.PHONY: ima4b-target-relative-predictor-sufficiency \
  run-ima4b-target-relative-predictor-sufficiency-self-test \
  run-ima4b-target-relative-predictor-sufficiency-audit
ima4b-target-relative-predictor-sufficiency: \
  $(TEST_OUT)/quality_wikimyei_mtf_jepa_mae_vicreg_target_relative_predictor_sufficiency
	@$(LOG_SUCCESS)

run-ima4b-target-relative-predictor-sufficiency-self-test: \
  ima4b-target-relative-predictor-sufficiency
	$(Q)cd $(PROJECT_ROOT) && $(RUN_MODE) $(TEST_OUT)/quality_wikimyei_mtf_jepa_mae_vicreg_target_relative_predictor_sufficiency --experiment target-relative-predictor-sufficiency-self-test --device cpu

run-ima4b-target-relative-predictor-sufficiency-audit: \
  ima4b-target-relative-predictor-sufficiency
	$(Q)cd $(PROJECT_ROOT) && { \
	  temporary_log="$(TEST_OUT)/representation_ima4b_v1a2_authoritative.log.tmp"; \
	  authoritative_log="$(TEST_OUT)/representation_ima4b_v1a2_authoritative.log"; \
	  temporary_checksum="$$authoritative_log.sha256.tmp"; \
	  authoritative_checksum="$$authoritative_log.sha256"; \
	  rm -f "$$temporary_log" "$$temporary_checksum"; \
	  if CUBLAS_WORKSPACE_CONFIG=:4096:8 $(RUN_MODE) $(TEST_OUT)/quality_wikimyei_mtf_jepa_mae_vicreg_target_relative_predictor_sufficiency --experiment target-relative-predictor-sufficiency-audit --device cuda > "$$temporary_log" 2>&1; then \
	    checksum_line=$$(sha256sum "$$temporary_log") && \
	    checksum=$${checksum_line%% *} && \
	    printf '%s  %s\n' "$$checksum" "$$authoritative_log" > "$$temporary_checksum" && \
	    mv -f "$$temporary_log" "$$authoritative_log" && \
	    mv -f "$$temporary_checksum" "$$authoritative_checksum"; \
	  else \
	    status=$$?; \
	    exit $$status; \
	  fi; \
	}

.PHONY: ima4c-canonical-slot-interaction-sufficiency \
  run-ima4c-canonical-slot-interaction-sufficiency-self-test \
  run-ima4c-canonical-slot-interaction-sufficiency-audit
ima4c-canonical-slot-interaction-sufficiency: \
  $(TEST_OUT)/quality_wikimyei_mtf_jepa_mae_vicreg_canonical_slot_interaction_sufficiency
	@$(LOG_SUCCESS)

run-ima4c-canonical-slot-interaction-sufficiency-self-test: \
  ima4c-canonical-slot-interaction-sufficiency
	$(Q)cd $(PROJECT_ROOT) && $(RUN_MODE) $(TEST_OUT)/quality_wikimyei_mtf_jepa_mae_vicreg_canonical_slot_interaction_sufficiency --experiment canonical-slot-interaction-sufficiency-self-test --device cpu

run-ima4c-canonical-slot-interaction-sufficiency-audit: \
  ima4c-canonical-slot-interaction-sufficiency
	$(Q)cd $(PROJECT_ROOT) && { \
	  temporary_log="$(TEST_OUT)/representation_ima4c_v1_authoritative.log.tmp"; \
	  authoritative_log="$(TEST_OUT)/representation_ima4c_v1_authoritative.log"; \
	  temporary_checksum="$$authoritative_log.sha256.tmp"; \
	  authoritative_checksum="$$authoritative_log.sha256"; \
	  rm -f "$$temporary_log" "$$temporary_checksum"; \
	  if CUBLAS_WORKSPACE_CONFIG=:4096:8 $(RUN_MODE) $(TEST_OUT)/quality_wikimyei_mtf_jepa_mae_vicreg_canonical_slot_interaction_sufficiency --experiment canonical-slot-interaction-sufficiency-audit --device cuda > "$$temporary_log" 2>&1; then \
	    checksum_line=$$(sha256sum "$$temporary_log") && \
	    checksum=$${checksum_line%% *} && \
	    printf '%s  %s\n' "$$checksum" "$$authoritative_log" > "$$temporary_checksum" && \
	    mv -f "$$temporary_log" "$$authoritative_log" && \
	    mv -f "$$temporary_checksum" "$$authoritative_checksum"; \
	  else \
	    status=$$?; \
	    exit $$status; \
	  fi; \
	}

.PHONY: ima5a-support-permitted-teacher-target-alignment \
  run-ima5a-support-permitted-teacher-target-alignment-self-test \
  run-ima5a-support-permitted-teacher-target-alignment-audit
ima5a-support-permitted-teacher-target-alignment: \
  $(TEST_OUT)/quality_wikimyei_mtf_jepa_mae_vicreg_support_permitted_teacher_target_alignment
	@$(LOG_SUCCESS)

run-ima5a-support-permitted-teacher-target-alignment-self-test: \
  ima5a-support-permitted-teacher-target-alignment
	$(Q)cd $(PROJECT_ROOT) && $(RUN_MODE) $(TEST_OUT)/quality_wikimyei_mtf_jepa_mae_vicreg_support_permitted_teacher_target_alignment --experiment support-permitted-teacher-target-alignment-self-test --device cpu

run-ima5a-support-permitted-teacher-target-alignment-audit: \
  ima5a-support-permitted-teacher-target-alignment
	$(Q)cd $(PROJECT_ROOT) && { \
	  temporary_log="$(TEST_OUT)/representation_ima5a_v1_authoritative.log.tmp"; \
	  authoritative_log="$(TEST_OUT)/representation_ima5a_v1_authoritative.log"; \
	  temporary_checksum="$$authoritative_log.sha256.tmp"; \
	  authoritative_checksum="$$authoritative_log.sha256"; \
	  rm -f "$$temporary_log" "$$temporary_checksum"; \
	  if CUBLAS_WORKSPACE_CONFIG=:4096:8 $(RUN_MODE) $(TEST_OUT)/quality_wikimyei_mtf_jepa_mae_vicreg_support_permitted_teacher_target_alignment --experiment support-permitted-teacher-target-alignment-audit --device cuda > "$$temporary_log" 2>&1; then \
	    checksum_line=$$(sha256sum "$$temporary_log") && \
	    checksum=$${checksum_line%% *} && \
	    printf '%s  %s\n' "$$checksum" "$$authoritative_log" > "$$temporary_checksum" && \
	    mv -f "$$temporary_log" "$$authoritative_log" && \
	    mv -f "$$temporary_checksum" "$$authoritative_checksum"; \
	  else \
	    status=$$?; \
	    exit $$status; \
	  fi; \
	}

outer-augmentation-training-screen: \
  $(TEST_OUT)/test_wikimyei_mtf_jepa_mae_vicreg_contracts \
  $(TEST_OUT)/test_representation_outer_augmentation_training_gate \
  $(TEST_OUT)/quality_wikimyei_mtf_jepa_mae_vicreg_representation
	@$(LOG_SUCCESS)

jmcd-gate-test: \
  $(TEST_OUT)/test_representation_jepa_mae_core_decomposition_gate
	@$(LOG_SUCCESS)

run-jmcd-gate-test: \
  run-test_representation_jepa_mae_core_decomposition_gate

# Build-only JMCD-1 surface. Scientific execution is intentionally absent.
jmcd-screen: \
  $(TEST_OUT)/test_wikimyei_mtf_jepa_mae_vicreg_contracts \
  $(TEST_OUT)/test_representation_jepa_mae_core_decomposition_gate \
  $(TEST_OUT)/quality_wikimyei_mtf_jepa_mae_vicreg_representation
	@$(LOG_SUCCESS)

rssm-gate-test: \
  $(TEST_OUT)/test_representation_surface_sufficiency_map_gate
	@$(LOG_SUCCESS)

run-rssm-gate-test: \
  run-test_representation_surface_sufficiency_map_gate

rssm-reference-auditor-test: \
  $(TEST_OUT)/test_representation_surface_sufficiency_map_reference_auditor
	@$(LOG_SUCCESS)

run-rssm-reference-auditor-test: \
  run-test_representation_surface_sufficiency_map_reference_auditor

rssm-protocol-pin:
	$(Q)sha256sum -c REPRESENTATION_SURFACE_SUFFICIENCY_MAP_PROTOCOL.sha256

# Build-only RSSM-1 surface. No target launches the authoritative map.
.NOTPARALLEL: rssm-screen run-rssm-preflight
rssm-screen: \
  $(TEST_OUT)/test_wikimyei_mtf_jepa_mae_vicreg_contracts \
  $(TEST_OUT)/test_wikimyei_mtf_jepa_mae_vicreg_tokenizer_information \
  $(TEST_OUT)/test_representation_surface_sufficiency_map_gate \
  $(TEST_OUT)/test_representation_surface_sufficiency_map_reference_auditor \
  $(TEST_OUT)/quality_wikimyei_mtf_jepa_mae_vicreg_representation
	@$(LOG_SUCCESS)

run-rssm-preflight: rssm-screen rssm-protocol-pin
	$(Q)$(RUN_MODE) $(TEST_OUT)/test_wikimyei_mtf_jepa_mae_vicreg_contracts
	$(Q)$(RUN_MODE) $(TEST_OUT)/test_representation_surface_sufficiency_map_gate
	$(Q)$(RUN_MODE) $(TEST_OUT)/test_representation_surface_sufficiency_map_reference_auditor --self-test
	$(Q)$(RUN_MODE) $(TEST_OUT)/test_wikimyei_mtf_jepa_mae_vicreg_tokenizer_information
	$(Q)CUBLAS_WORKSPACE_CONFIG=:4096:8 $(RUN_MODE) \
	  $(TEST_OUT)/quality_wikimyei_mtf_jepa_mae_vicreg_representation \
	  --experiment representation-surface-sufficiency-map-preflight \
	  --device cuda

psm-gate-test: $(TEST_OUT)/test_pooling_structure_mechanism_map_gate
	@$(LOG_SUCCESS)

run-psm-gate-test: run-test_pooling_structure_mechanism_map_gate

psm-log-auditor-test: $(TEST_OUT)/test_pooling_structure_mechanism_map_log_auditor
	@$(LOG_SUCCESS)

run-psm-log-auditor-test: run-test_pooling_structure_mechanism_map_log_auditor

psm-protocol-pin:
	$(Q)sha256sum -c POOLING_STRUCTURE_MECHANISM_MAP_PROTOCOL.sha256

psm-parent-pin:
	$(Q)echo "c8fc41261bf3c901fb36213371a08f4e1b601aa288be15ad1d6af3d26e0ee648  POOLING_STRUCTURE_MECHANISM_MAP_PLAN.md" | sha256sum -c -
	$(Q)echo "b1554abf3ebfab5de2f263649e34688b07c8d84f5050722f60099ee65f4abc0e  REPRESENTATION_SURFACE_SUFFICIENCY_MAP_PROTOCOL.md" | sha256sum -c -
	$(Q)echo "16be35d27836fb72bbd777e1795f10433850c5108c34670fcc27f4d287059932  ../../../../../../../.build/tests/representation_rssm_v1_authoritative.log" | sha256sum -c -
	$(Q)echo "78c34ca547be965dc213367ad037b07163beb9f801f486b12e02773d0838e44f  ../../../../../../../.build/tests/representation_rssm_v1_receipt.sha256" | sha256sum -c -

# Build-only PSM-1 surface. No target launches the authoritative map.
.NOTPARALLEL: psm-screen run-psm-preflight
psm-screen: \
  $(TEST_OUT)/test_wikimyei_mtf_jepa_mae_vicreg_contracts \
  $(TEST_OUT)/test_wikimyei_mtf_jepa_mae_vicreg_tokenizer_information \
  $(TEST_OUT)/test_pooling_structure_mechanism_map_gate \
  $(TEST_OUT)/test_pooling_structure_mechanism_map_log_auditor \
  $(TEST_OUT)/quality_wikimyei_mtf_jepa_mae_vicreg_representation
	@$(LOG_SUCCESS)

run-psm-preflight: psm-screen psm-protocol-pin psm-parent-pin
	$(Q)$(RUN_MODE) $(TEST_OUT)/test_pooling_structure_mechanism_map_gate
	$(Q)$(RUN_MODE) $(TEST_OUT)/test_pooling_structure_mechanism_map_log_auditor --self-test
	$(Q)CUBLAS_WORKSPACE_CONFIG=:4096:8 $(RUN_MODE) \
	  $(TEST_OUT)/quality_wikimyei_mtf_jepa_mae_vicreg_representation \
	  --experiment pooling-structure-mechanism-map-preflight \
	  --device cuda

.PHONY: srr-shadow-test run-srr-shadow-test srr-gate-test \
  run-srr-gate-test srr-log-auditor-test run-srr-log-auditor-test \
  srr-protocol-pin srr-parent-pin srr-screen run-srr-preflight

srr-shadow-test: $(TEST_OUT)/test_structured_readout_shadow
	@$(LOG_SUCCESS)

run-srr-shadow-test: run-test_structured_readout_shadow

srr-gate-test: $(TEST_OUT)/test_structured_readout_repair_gate
	@$(LOG_SUCCESS)

run-srr-gate-test: run-test_structured_readout_repair_gate

srr-log-auditor-test: $(TEST_OUT)/test_structured_readout_repair_log_auditor
	@$(LOG_SUCCESS)

run-srr-log-auditor-test: run-test_structured_readout_repair_log_auditor

srr-protocol-pin:
	$(Q)sha256sum -c STRUCTURED_READOUT_REPAIR_PROTOCOL.sha256

srr-parent-pin:
	$(Q)echo "c8fc41261bf3c901fb36213371a08f4e1b601aa288be15ad1d6af3d26e0ee648  POOLING_STRUCTURE_MECHANISM_MAP_PLAN.md" | sha256sum -c -
	$(Q)echo "2f574310d79d581dbc39d4040d16431f8e067ae5d3583d6d4a5597b5a8ad72d3  POOLING_STRUCTURE_MECHANISM_MAP_PROTOCOL.md" | sha256sum -c -
	$(Q)echo "9d4c5830cbea9fa6a8a3ccf5c832201cd0e5eb890069f8e2c7024bb14ae3fbc6  POOLING_STRUCTURE_MECHANISM_MAP_PROTOCOL.sha256" | sha256sum -c -
	$(Q)echo "8a6a46da7625e3f25e3402d8037336bc0fe0a2de7a377827bc6e6e7f5894196f  POOLING_STRUCTURE_MECHANISM_MAP_FINDINGS.md" | sha256sum -c -
	$(Q)echo "22ea52b1c31916e0da57c436917076805d5482e49e38ae1bdea62cbce31418f2  ../../../../../../../.build/tests/representation_psm_v1_prerun.sha256" | sha256sum -c -
	$(Q)echo "8243798d5af03d66257cbd1fd9da49a16ff7d6ba3f9e6bc54b5568dae41aa8b9  ../../../../../../../.build/tests/representation_psm_v1_authoritative.log" | sha256sum -c -
	$(Q)echo "f2aab5858536455e1bbd6b5ec0f6aa20e49b22d46abf0b94c6e9a7346a95f85a  ../../../../../../../.build/tests/representation_psm_v1_audit.log" | sha256sum -c -
	$(Q)echo "31917fd5763f0f7831a212cd8eb0a685435e20ddb1071c5d6c189c75ee72cf53  ../../../../../../../.build/tests/representation_psm_v1_receipt.sha256" | sha256sum -c -

# Build-only SRR-1 surface.  No target launches authoritative mode.
.NOTPARALLEL: srr-screen run-srr-preflight
srr-screen: \
  $(TEST_OUT)/test_structured_readout_shadow \
  $(TEST_OUT)/test_structured_readout_repair_gate \
  $(TEST_OUT)/test_structured_readout_repair_log_auditor \
  $(TEST_OUT)/quality_wikimyei_mtf_jepa_mae_vicreg_structured_readout_repair
	@$(LOG_SUCCESS)

run-srr-preflight: srr-screen srr-protocol-pin srr-parent-pin
	$(Q)$(RUN_MODE) $(TEST_OUT)/test_structured_readout_shadow
	$(Q)$(RUN_MODE) $(TEST_OUT)/test_structured_readout_repair_gate
	$(Q)$(RUN_MODE) $(TEST_OUT)/test_structured_readout_repair_log_auditor --self-test
	$(Q)CUBLAS_WORKSPACE_CONFIG=:4096:8 $(RUN_MODE) \
	  $(TEST_OUT)/quality_wikimyei_mtf_jepa_mae_vicreg_structured_readout_repair \
	  --experiment structured-readout-repair-preflight --device cuda

.PHONY: srr2-production-test run-srr2-production-test \
  srr2-gate-test run-srr2-gate-test \
  srr2-auditor-test run-srr2-auditor-test \
  srr2-parity-binary srr2-protocol-pin srr2-parent-pin srr2-screen \
  srr2-compatibility-binaries run-srr2-mechanics run-srr2-preflight

srr2-production-test: $(TEST_OUT)/test_production_structured_readout
	@$(LOG_SUCCESS)

run-srr2-production-test: run-test_production_structured_readout

srr2-gate-test: $(TEST_OUT)/test_production_structured_readout_parity_gate
	@$(LOG_SUCCESS)

run-srr2-gate-test: run-test_production_structured_readout_parity_gate

srr2-auditor-test: $(TEST_OUT)/test_production_structured_readout_parity_log_auditor
	@$(LOG_SUCCESS)

run-srr2-auditor-test: run-test_production_structured_readout_parity_log_auditor

srr2-parity-binary: $(TEST_OUT)/quality_wikimyei_mtf_jepa_mae_vicreg_production_structured_readout_parity
	@$(LOG_SUCCESS)

srr2-protocol-pin:
	$(Q)sha256sum -c PRODUCTION_STRUCTURED_READOUT_PARITY_PROTOCOL.sha256
	$(Q)sha256sum -c PRODUCTION_STRUCTURED_READOUT_PARITY_PROTOCOL_AMENDMENT_A1.sha256
	$(Q)sha256sum -c PRODUCTION_STRUCTURED_READOUT_PARITY_PROTOCOL_AMENDMENT_A2.sha256
	$(Q)sha256sum -c PRODUCTION_STRUCTURED_READOUT_PARITY_PROTOCOL_AMENDMENT_A3.sha256

srr2-parent-pin:
	$(Q)echo "1f976d5da5a79323a8fce011b0b33e53b277517bd785b2fdea68aa1888338127  STRUCTURED_READOUT_REPAIR_PLAN.md" | sha256sum -c -
	$(Q)echo "ad7c9381d58a23e8f3cec27b59b44e6532aa561227ad22d57578cc6ba0a04946  STRUCTURED_READOUT_REPAIR_PROTOCOL.md" | sha256sum -c -
	$(Q)echo "3b97b7431e34c3b875365bad27d0b9de67a5b7fd7007760eb34ab97b125140c5  STRUCTURED_READOUT_REPAIR_PROTOCOL.sha256" | sha256sum -c -
	$(Q)echo "b5b3458953f2f967a0229ea910c80810303d2e709bd6d9f237966c0e6b456c6a  STRUCTURED_READOUT_REPAIR_FINDINGS.md" | sha256sum -c -
	$(Q)echo "515c9c8a851b3aceb03c160e5c9c19fff5265774d51eb396c6d56123cf0d3acb  ../../../../../../../.build/tests/representation_srr_v1_prerun.sha256" | sha256sum -c -
	$(Q)echo "f38c99ef1294dab5f40f57fff79a958cd214c593eedd10284531976cda20ae6a  ../../../../../../../.build/tests/representation_srr_v1_authoritative.log" | sha256sum -c -
	$(Q)echo "fe943fb2aa8ad26f53953364181f7c2b452692fde17643c5a8d94ca45c9bb841  ../../../../../../../.build/tests/representation_srr_v1_audit.log" | sha256sum -c -
	$(Q)echo "994be46cab5c4bbabf3b72ed30e5fa1a8ece9247722e16ae504b428dcd0fc207  ../../../../../../../.build/tests/representation_srr_v1_receipt.sha256" | sha256sum -c -
	$(Q)echo "22f53f452836583f5402d1a28d67c9b3a9ac865094444a06b9726fd0f1c7b6dd  ../../../../../../../.build/tests/representation_srr2_v1_production_baseline.tar" | sha256sum -c -

# Build-only SRR-2 surface. The authoritative parity mode is intentionally
# absent from Make; it can be consumed only through the sealed direct command.
.NOTPARALLEL: srr2-screen run-srr2-mechanics run-srr2-preflight
srr2-compatibility-binaries:
	$(MAKE) -C $(TESTS_PATH)/bench/wikimyei/config/graph_first_specs \
	  $(TEST_OUT)/test_wikimyei_graph_first_specs
	$(MAKE) -C $(TESTS_PATH)/bench/jkimyei/training/channel_graph_first_launchers \
	  $(TEST_OUT)/test_jkimyei_channel_graph_first_launchers

srr2-screen: \
  srr2-compatibility-binaries \
  $(TEST_OUT)/test_wikimyei_mtf_jepa_mae_vicreg \
  $(TEST_OUT)/test_wikimyei_mtf_jepa_mae_vicreg_contracts \
  $(TEST_OUT)/test_structured_readout_shadow \
  $(TEST_OUT)/test_production_structured_readout \
  $(TEST_OUT)/test_production_structured_readout_parity_gate \
  $(TEST_OUT)/test_production_structured_readout_parity_log_auditor \
  $(TEST_OUT)/quality_wikimyei_mtf_jepa_mae_vicreg_production_structured_readout_parity
	$(Q)bash write_production_structured_readout_parity_build_receipt.sh \
	  > $(TEST_OUT)/representation_srr2_v1_build_receipt.txt
	@$(LOG_SUCCESS)

run-srr2-mechanics: srr2-screen srr2-protocol-pin srr2-parent-pin
	$(Q)bash run_production_structured_readout_parity_mechanics.sh \
	  > $(TEST_OUT)/representation_srr2_v1_mechanics.log

run-srr2-preflight: run-srr2-mechanics
	$(Q)CUBLAS_WORKSPACE_CONFIG=:4096:8 $(RUN_MODE) \
	  $(TEST_OUT)/quality_wikimyei_mtf_jepa_mae_vicreg_production_structured_readout_parity \
	  --experiment production-structured-readout-parity-preflight --device cuda \
	  > $(TEST_OUT)/representation_srr2_v1_preflight.log 2>&1

.PHONY: srr4-sparse-mechanics-test run-srr4-sparse-mechanics-test
srr4-sparse-mechanics-test: $(TEST_OUT)/test_structured_cdsb_sparse_v1
	@$(LOG_SUCCESS)

run-srr4-sparse-mechanics-test: run-test_structured_cdsb_sparse_v1

# Opt-in, ordered repair workflow: mechanical contracts, pure gate semantics,
# then the deterministic paired CUDA representation screen.
.NOTPARALLEL: repair-screen run-repair-screen
repair-screen: \
  $(TEST_OUT)/test_wikimyei_mtf_jepa_mae_vicreg_contracts \
  $(TEST_OUT)/test_representation_objective_repair_gate \
  $(TEST_OUT)/quality_wikimyei_mtf_jepa_mae_vicreg_representation
	@$(LOG_SUCCESS)

run-repair-screen: repair-screen
	$(Q)$(RUN_MODE) $(TEST_OUT)/test_wikimyei_mtf_jepa_mae_vicreg_contracts
	$(Q)$(RUN_MODE) $(TEST_OUT)/test_representation_objective_repair_gate
	$(Q)CUBLAS_WORKSPACE_CONFIG=:4096:8 $(RUN_MODE) $(TEST_OUT)/quality_wikimyei_mtf_jepa_mae_vicreg_representation --experiment objective-mask-attribution --device cuda

# Opt-in, three-arm component-necessity workflow. The historical repair target
# remains unchanged.
.NOTPARALLEL: variance-component-necessity-screen \
  run-variance-component-necessity-screen
variance-component-necessity-screen: \
  $(TEST_OUT)/test_wikimyei_mtf_jepa_mae_vicreg_contracts \
  $(TEST_OUT)/test_representation_vicreg_variance_necessity_gate \
  $(TEST_OUT)/quality_wikimyei_mtf_jepa_mae_vicreg_representation
	@$(LOG_SUCCESS)

run-variance-component-necessity-screen: \
  variance-component-necessity-screen
	$(Q)$(RUN_MODE) $(TEST_OUT)/test_wikimyei_mtf_jepa_mae_vicreg_contracts
	$(Q)$(RUN_MODE) $(TEST_OUT)/test_representation_vicreg_variance_necessity_gate
	$(Q)CUBLAS_WORKSPACE_CONFIG=:4096:8 $(RUN_MODE) $(TEST_OUT)/quality_wikimyei_mtf_jepa_mae_vicreg_representation --experiment vicreg-variance-component-necessity --device cuda

.PHONY: performance perf-cpu perf-cpu-batch64 perf-cuda-encode \
  perf-cuda-encode-batch64 perf-cuda-train
performance: $(TEST_OUT)/bench_wikimyei_mtf_jepa_mae_vicreg_performance

perf-cpu: $(TEST_OUT)/bench_wikimyei_mtf_jepa_mae_vicreg_performance
	$(Q)$(RUN_MODE) $< --device cpu --mode encode --anchors 1 --threads 1

perf-cpu-batch64: $(TEST_OUT)/bench_wikimyei_mtf_jepa_mae_vicreg_performance
	$(Q)$(RUN_MODE) $< --device cpu --mode encode --anchors 64 --threads 1

perf-cuda-encode: $(TEST_OUT)/bench_wikimyei_mtf_jepa_mae_vicreg_performance
	$(Q)$(RUN_MODE) $< --device cuda --mode encode --anchors 1

perf-cuda-encode-batch64: $(TEST_OUT)/bench_wikimyei_mtf_jepa_mae_vicreg_performance
	$(Q)$(RUN_MODE) $< --device cuda --mode encode --anchors 64

perf-cuda-train: $(TEST_OUT)/bench_wikimyei_mtf_jepa_mae_vicreg_performance
	$(Q)$(RUN_MODE) $< --device cuda --mode train --anchors 32

.PHONY: clean
clean:
	@rm -f $(TEST_OUT)/test_wikimyei_mtf_jepa_mae_vicreg
	@rm -f $(TEST_OUT)/train_synthetic_mtf_jepa_mae_vicreg
	@rm -f $(TEST_OUT)/test_wikimyei_mtf_jepa_mae_vicreg_contracts
	@rm -f $(TEST_OUT)/test_representation_objective_repair_gate
	@rm -f $(TEST_OUT)/test_representation_jepa_mae_core_decomposition_gate
	@rm -f $(TEST_OUT)/test_representation_vicreg_variance_necessity_gate
	@rm -f $(TEST_OUT)/test_representation_outer_augmentation_training_gate
	@rm -f $(TEST_OUT)/test_representation_surface_sufficiency_map_gate
	@rm -f $(TEST_OUT)/test_representation_surface_sufficiency_map_reference_auditor
	@rm -f $(TEST_OUT)/test_pooling_structure_mechanism_map_gate
	@rm -f $(TEST_OUT)/test_pooling_structure_mechanism_map_log_auditor
	@rm -f $(TEST_OUT)/test_structured_readout_shadow
	@rm -f $(TEST_OUT)/test_production_structured_readout
	@rm -f $(TEST_OUT)/test_structured_cdsb_sparse_v1
	@rm -f $(TEST_OUT)/test_production_structured_readout_parity_gate
	@rm -f $(TEST_OUT)/test_production_structured_readout_parity_log_auditor
	@rm -f $(TEST_OUT)/test_structured_readout_repair_gate
	@rm -f $(TEST_OUT)/test_structured_readout_repair_log_auditor
	@rm -f $(TEST_OUT)/qualify_wikimyei_mtf_jepa_mae_vicreg
	@rm -f $(TEST_OUT)/quality_wikimyei_mtf_jepa_mae_vicreg_representation
	@rm -f $(TEST_OUT)/quality_wikimyei_mtf_jepa_mae_vicreg_structured_readout_repair
	@rm -f $(TEST_OUT)/quality_wikimyei_mtf_jepa_mae_vicreg_production_structured_readout_parity
	@rm -f $(TEST_OUT)/quality_wikimyei_mtf_jepa_mae_vicreg_dose_matched_mask_repair
	@rm -f $(TEST_OUT)/quality_wikimyei_mtf_jepa_mae_vicreg_target_predictability_ceiling
	@rm -f $(TEST_OUT)/quality_wikimyei_mtf_jepa_mae_vicreg_target_relative_predictor_sufficiency
	@rm -f $(TEST_OUT)/quality_wikimyei_mtf_jepa_mae_vicreg_canonical_slot_interaction_sufficiency
	@rm -f $(TEST_OUT)/quality_wikimyei_mtf_jepa_mae_vicreg_view_augmentation_causal_attribution
	@rm -f $(TEST_OUT)/quality_wikimyei_mtf_jepa_mae_vicreg_view_loss_boundary_triad
	@rm -f $(TEST_OUT)/quality_wikimyei_mtf_jepa_mae_vicreg_global_pool_projector_variance_causal_decomposition
	@rm -f $(GPV1_BUILD_RECEIPT)
	@rm -f $(GPV1_BUILD_RECEIPT).tmp.*
	@rm -f $(GPV1_BUILD_RECEIPT).raw.tmp.*
	@rm -f $(GPV1_BUILD_RECEIPT).ldd.tmp.*
	@rm -f $(GPV1_BUILD_RECEIPT).dso.tmp.*
	@rm -f $(GPV1_BUILD_RECEIPT).dso_records.tmp.*
	@rm -f $(GPV1_FULL_DEPFILE)
	@rm -f $(GPV1_FULL_DEPFILE).tmp.*
	@rm -f $(TEST_OUT)/representation_gpv1_v1_authoritative.log.tmp.*
	@rm -f $(TEST_OUT)/representation_gpv1_v1_authoritative.log.sha256.tmp.*
	@rm -f $(TEST_OUT)/representation_gpv1_v2_authoritative.log.tmp.*
	@rm -f $(TEST_OUT)/representation_gpv1_v2_authoritative.log.sha256.tmp.*
	@rm -f $(TEST_OUT)/representation_ima4b_v1_authoritative.log.tmp
	@rm -f $(TEST_OUT)/representation_ima4b_v1_authoritative.log.sha256.tmp
	@rm -f $(TEST_OUT)/representation_ima4b_v1a1_authoritative.log.tmp
	@rm -f $(TEST_OUT)/representation_ima4b_v1a1_authoritative.log.sha256.tmp
	@rm -f $(TEST_OUT)/representation_ima4b_v1a2_authoritative.log.tmp
	@rm -f $(TEST_OUT)/representation_ima4b_v1a2_authoritative.log.sha256.tmp
	@rm -f $(TEST_OUT)/representation_ima4c_v1_authoritative.log.tmp
	@rm -f $(TEST_OUT)/representation_ima4c_v1_authoritative.log.sha256.tmp
	@rm -f $(TEST_OUT)/test_wikimyei_mtf_jepa_mae_vicreg_tokenizer_information
	@rm -f $(TEST_OUT)/test_wikimyei_mtf_jepa_mae_vicreg_ima1_stage0
	@rm -f $(TEST_OUT)/test_wikimyei_mtf_jepa_mae_vicreg_ima2_mask_contract
	@rm -f $(TEST_OUT)/bench_wikimyei_mtf_jepa_mae_vicreg_performance
