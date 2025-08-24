# -----------------------------------------------------------------------------
# Dependencies
# -----------------------------------------------------------------------------
ROOT_PATH := ../../../../..
include $(ROOT_PATH)/Makefile.config

# -----------------------------------------------------------------------------
# Local Paths
# -----------------------------------------------------------------------------
HERE_PATH  := $(TESTS_PATH)/bench/wikimyei/heuristics/ts2vec
REL_MODULE := $(patsubst $(TESTS_PATH)/%,%,$(HERE_PATH))

# -----------------------------------------------------------------------------
# Build + run aliases
# -----------------------------------------------------------------------------
$(eval $(call TEST_ONEFILE,test_ts2vec_datautils,   test_ts2vec_datautils.cpp))
$(eval $(call TEST_ONEFILE,test_ts2vec_losses,      test_ts2vec_losses.cpp))
$(eval $(call TEST_ONEFILE,test_ts2vec_dilated_conv,test_ts2vec_dilated_conv.cpp))
$(eval $(call TEST_ONEFILE,test_ts2vec_encoder,     test_ts2vec_encoder.cpp))
$(eval $(call TEST_ONEFILE,test_ts2vec_state_dict,  test_ts2vec_state_dict.cpp))
$(eval $(call TEST_ONEFILE,test_ts2vec_clonable,    test_ts2vec_clonable.cpp))
$(eval $(call TEST_ONEFILE,test_ts2vec,             test_ts2vec.cpp))
$(eval $(call TEST_ONEFILE,test_ts2vec_ECG200,      test_ts2vec_ECG200.cpp))

# ---- Attach Torch/SSL/cURL deps to each test --------------------------------
# Headers for compile:
$(TEST_OUT)/test_ts2vec_datautils:    INCLUDES_EXTRA += $(TORCH_INCLUDE_PATHS)
$(TEST_OUT)/test_ts2vec_losses:       INCLUDES_EXTRA += $(TORCH_INCLUDE_PATHS)
$(TEST_OUT)/test_ts2vec_dilated_conv: INCLUDES_EXTRA += $(TORCH_INCLUDE_PATHS)
$(TEST_OUT)/test_ts2vec_encoder:      INCLUDES_EXTRA += $(TORCH_INCLUDE_PATHS)
$(TEST_OUT)/test_ts2vec_state_dict:   INCLUDES_EXTRA += $(TORCH_INCLUDE_PATHS)
$(TEST_OUT)/test_ts2vec_clonable:     INCLUDES_EXTRA += $(TORCH_INCLUDE_PATHS)
$(TEST_OUT)/test_ts2vec:              INCLUDES_EXTRA += $(TORCH_INCLUDE_PATHS)
$(TEST_OUT)/test_ts2vec_ECG200:       INCLUDES_EXTRA += $(TORCH_INCLUDE_PATHS)

# Link order: torch/curl first, then common (provider)
TEST_DEFAULT_LDLIBS :=
$(TEST_OUT)/test_ts2vec_datautils:    LDLIBS_EXTRA += $(libtorch_a) $(libcommon_a) $(LDLIBS_torch)
$(TEST_OUT)/test_ts2vec_losses:       LDLIBS_EXTRA += $(libtorch_a) $(libcommon_a) $(LDLIBS_torch)
$(TEST_OUT)/test_ts2vec_dilated_conv: LDLIBS_EXTRA += $(libtorch_a) $(libcommon_a) $(LDLIBS_torch)
$(TEST_OUT)/test_ts2vec_encoder:      LDLIBS_EXTRA += $(libtorch_a) $(libcommon_a) $(LDLIBS_torch)
$(TEST_OUT)/test_ts2vec_state_dict:   LDLIBS_EXTRA += $(libtorch_a) $(libcommon_a) $(LDLIBS_torch)
$(TEST_OUT)/test_ts2vec_clonable:     LDLIBS_EXTRA += $(libtorch_a) $(libcommon_a) $(LDLIBS_torch)
$(TEST_OUT)/test_ts2vec:              LDLIBS_EXTRA += $(libtorch_a) $(libcommon_a) $(LDLIBS_torch)
$(TEST_OUT)/test_ts2vec_ECG200:       LDLIBS_EXTRA += $(libtorch_a) $(libcommon_a) $(LDLIBS_torch)

# -----------------------------------------------------------------------------
# Aggregate build
# -----------------------------------------------------------------------------
.PHONY: all
all: \
	$(TEST_OUT)/test_ts2vec_datautils \
	$(TEST_OUT)/test_ts2vec_losses \
	$(TEST_OUT)/test_ts2vec_dilated_conv \
	$(TEST_OUT)/test_ts2vec_encoder \
	$(TEST_OUT)/test_ts2vec_state_dict \
	$(TEST_OUT)/test_ts2vec_clonable \
	$(TEST_OUT)/test_ts2vec \
	$(TEST_OUT)/test_ts2vec_ECG200
	@$(LOG_SUCCESS)

# -----------------------------------------------------------------------------
# Aggregate run
# -----------------------------------------------------------------------------
.PHONY: run
run: \
     run-test_ts2vec_datautils \
     run-test_ts2vec_losses \
     run-test_ts2vec_dilated_conv \
     run-test_ts2vec_encoder \
     run-test_ts2vec_state_dict \
     run-test_ts2vec_clonable \
     run-test_ts2vec \
     run-test_ts2vec_ECG200

# -----------------------------------------------------------------------------
# Clean
# -----------------------------------------------------------------------------
.PHONY: clean
clean:
	@rm -f \
	  $(TEST_OUT)/test_ts2vec_datautils \
	  $(TEST_OUT)/test_ts2vec_losses \
	  $(TEST_OUT)/test_ts2vec_dilated_conv \
	  $(TEST_OUT)/test_ts2vec_encoder \
	  $(TEST_OUT)/test_ts2vec_state_dict \
	  $(TEST_OUT)/test_ts2vec_clonable \
	  $(TEST_OUT)/test_ts2vec \
	  $(TEST_OUT)/test_ts2vec_ECG200
