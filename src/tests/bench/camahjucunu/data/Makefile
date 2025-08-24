# -----------------------------------------------------------------------------
# Dependencies
# -----------------------------------------------------------------------------
ROOT_PATH := ../../../..
include $(ROOT_PATH)/Makefile.config

# -----------------------------------------------------------------------------
# Local Paths
# -----------------------------------------------------------------------------
HERE_PATH  := $(TESTS_PATH)/bench/camahjucunu/data
REL_MODULE := $(patsubst $(TESTS_PATH)/%,%,$(HERE_PATH))

# -----------------------------------------------------------------------------
# Build + run aliases
# -----------------------------------------------------------------------------
$(eval $(call TEST_ONEFILE,test_memory_mapped_datafile,         test_memory_mapped_datafile.cpp))
$(eval $(call TEST_ONEFILE,test_memory_mapped_dataset,          test_memory_mapped_dataset.cpp))
$(eval $(call TEST_ONEFILE,test_memory_mapped_concat_dataset,   test_memory_mapped_concat_dataset.cpp))
$(eval $(call TEST_ONEFILE,test_memory_mapped_concat_dataloader,test_memory_mapped_concat_dataloader.cpp))

# ---- Attach heavy deps (Torch/SSL/cURL) to each test ------------------------
# Headers for compile:
$(TEST_OUT)/test_memory_mapped_datafile:          INCLUDES_EXTRA += $(TORCH_INCLUDE_PATHS) $(SSL_INCLUDE_PATHS) $(LIBCURL_INCLUDE_PATHS)
$(TEST_OUT)/test_memory_mapped_dataset:           INCLUDES_EXTRA += $(TORCH_INCLUDE_PATHS) $(SSL_INCLUDE_PATHS) $(LIBCURL_INCLUDE_PATHS)
$(TEST_OUT)/test_memory_mapped_concat_dataset:    INCLUDES_EXTRA += $(TORCH_INCLUDE_PATHS) $(SSL_INCLUDE_PATHS) $(LIBCURL_INCLUDE_PATHS)
$(TEST_OUT)/test_memory_mapped_concat_dataloader: INCLUDES_EXTRA += $(TORCH_INCLUDE_PATHS) $(SSL_INCLUDE_PATHS) $(LIBCURL_INCLUDE_PATHS)

# Archives + system libs for link:
# (TEST_DEFAULT_LDLIBS already adds $(libcommon_a) automatically)
$(TEST_OUT)/test_memory_mapped_datafile:          LDLIBS_EXTRA   += $(libtorch_a) $(libssl_a) $(libcurl_a) $(LDLIBS_torch) $(LDLIBS_ssl) $(LDLIBS_curl)
$(TEST_OUT)/test_memory_mapped_dataset:           LDLIBS_EXTRA   += $(libtorch_a) $(libssl_a) $(libcurl_a) $(LDLIBS_torch) $(LDLIBS_ssl) $(LDLIBS_curl)
$(TEST_OUT)/test_memory_mapped_concat_dataset:    LDLIBS_EXTRA   += $(libtorch_a) $(libssl_a) $(libcurl_a) $(LDLIBS_torch) $(LDLIBS_ssl) $(LDLIBS_curl)
$(TEST_OUT)/test_memory_mapped_concat_dataloader: LDLIBS_EXTRA   += $(libtorch_a) $(libssl_a) $(libcurl_a) $(LDLIBS_torch) $(LDLIBS_ssl) $(LDLIBS_curl)

# -----------------------------------------------------------------------------
# Aggregate build
# -----------------------------------------------------------------------------
.PHONY: all
all: \
	$(OUTPUT_PATH)/test_memory_mapped_datafile \
	$(OUTPUT_PATH)/test_memory_mapped_dataset \
	$(OUTPUT_PATH)/test_memory_mapped_concat_dataset \
	$(OUTPUT_PATH)/test_memory_mapped_concat_dataloader
	@$(LOG_SUCCESS)

# -----------------------------------------------------------------------------
# Aggregate run
# -----------------------------------------------------------------------------
.PHONY: run
run: \
     run-test_memory_mapped_datafile \
     run-test_memory_mapped_dataset \
     run-test_memory_mapped_concat_dataset \
     run-test_memory_mapped_concat_dataloader

# -----------------------------------------------------------------------------
# Clean
# -----------------------------------------------------------------------------
.PHONY: clean
clean:
	@rm -f $(OUTPUT_PATH)/test_memory_mapped_*
