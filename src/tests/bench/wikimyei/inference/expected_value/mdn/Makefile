ROOT_PATH := ../../../../../..
include $(ROOT_PATH)/Makefile.config

HERE_PATH  := $(TESTS_PATH)/bench/wikimyei/inference/expected_value/mdn
REL_MODULE := $(patsubst $(TESTS_PATH)/%,%,$(HERE_PATH))

TEST_DEFAULT_LDLIBS :=

$(eval $(call TEST_ONEFILE, test_wikimyei_mdn_affine_sidecar, test_wikimyei_mdn_affine_sidecar.cpp, \
  $(LDLIBS_torch)))

$(eval $(call TEST_ONEFILE, test_wikimyei_mdn_conditioned_affine_sidecar, test_wikimyei_mdn_conditioned_affine_sidecar.cpp, \
  $(LDLIBS_torch)))

$(eval $(call TEST_ONEFILE, test_wikimyei_mdn_conditioned_affine_shadow, test_wikimyei_mdn_conditioned_affine_shadow.cpp, \
  $(LDLIBS_torch)))

$(eval $(call TEST_ONEFILE, test_wikimyei_mdn_direct_edge_cached_readout, test_wikimyei_mdn_direct_edge_cached_readout.cpp, \
  $(LDLIBS_torch)))

$(TEST_OUT)/test_wikimyei_mdn_affine_sidecar: INCLUDES_EXTRA += $(TORCH_INCLUDE_PATHS)
$(TEST_OUT)/test_wikimyei_mdn_conditioned_affine_sidecar: INCLUDES_EXTRA += $(TORCH_INCLUDE_PATHS)
$(TEST_OUT)/test_wikimyei_mdn_conditioned_affine_shadow: INCLUDES_EXTRA += $(TORCH_INCLUDE_PATHS)
$(TEST_OUT)/test_wikimyei_mdn_direct_edge_cached_readout: INCLUDES_EXTRA += $(TORCH_INCLUDE_PATHS)

.PHONY: all
all: $(TEST_OUT)/test_wikimyei_mdn_affine_sidecar \
	$(TEST_OUT)/test_wikimyei_mdn_conditioned_affine_sidecar \
	$(TEST_OUT)/test_wikimyei_mdn_conditioned_affine_shadow \
	$(TEST_OUT)/test_wikimyei_mdn_direct_edge_cached_readout
	@$(LOG_SUCCESS)

.PHONY: run
run: run-test_wikimyei_mdn_affine_sidecar \
	run-test_wikimyei_mdn_conditioned_affine_sidecar \
	run-test_wikimyei_mdn_conditioned_affine_shadow \
	run-test_wikimyei_mdn_direct_edge_cached_readout

.PHONY: clean
clean:
	@rm -f $(TEST_OUT)/test_wikimyei_mdn_affine_sidecar \
		$(TEST_OUT)/test_wikimyei_mdn_conditioned_affine_sidecar \
		$(TEST_OUT)/test_wikimyei_mdn_conditioned_affine_shadow \
		$(TEST_OUT)/test_wikimyei_mdn_direct_edge_cached_readout
