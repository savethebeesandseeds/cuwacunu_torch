# =============================================================================
# tests/bench/camahjucunu/data/Makefile — memory-mapped datasets/dataloaders
# =============================================================================
# 0) Root & Config
ROOT_PATH := ../../../..
include $(ROOT_PATH)/Makefile.config

# 1) Local Paths
HERE_PATH  := $(TESTS_PATH)/bench/camahjucunu/data
REL_MODULE := $(patsubst $(TESTS_PATH)/%,%,$(HERE_PATH))

# 2) Defaults (libs/flags)
TEST_DEFAULT_LDLIBS :=

# 3) Tests (build + run aliases)
COMMON_LINK = $(libtorch_a) $(libssl_a) $(libcurl_a) $(libcommon_a) $(LDLIBS_torch) $(LDLIBS_ssl) $(LDLIBS_curl)
EDGE_LINK   = $(libtorch_a) $(libssl_a) $(libcommon_a) $(libcurl_a) $(libcommon_a) $(LDLIBS_torch) $(LDLIBS_ssl) $(LDLIBS_curl)
$(eval $(call TEST_ONEFILE, test_memory_mapped_datafile,          test_memory_mapped_datafile.cpp,          $(COMMON_LINK)))
$(eval $(call TEST_ONEFILE, test_memory_mapped_dataset,           test_memory_mapped_dataset.cpp,           $(COMMON_LINK)))
$(eval $(call TEST_ONEFILE, test_memory_mapped_concat_dataset,    test_memory_mapped_concat_dataset.cpp,    $(COMMON_LINK)))
$(eval $(call TEST_ONEFILE, test_memory_mapped_concat_dataloader, test_memory_mapped_concat_dataloader.cpp, $(COMMON_LINK)))
$(eval $(call TEST_ONEFILE, test_memory_mapped_edge_cases,        test_memory_mapped_edge_cases.cpp,        $(EDGE_LINK)))

# 4) Target-specific includes/link flags
$(TEST_OUT)/test_memory_mapped_%: INCLUDES_EXTRA += $(TORCH_INCLUDE_PATHS) $(SSL_INCLUDE_PATHS) $(LIBCURL_INCLUDE_PATHS)

# 5) Aggregate: all
.PHONY: all
all: \
	$(TEST_OUT)/test_memory_mapped_datafile \
	$(TEST_OUT)/test_memory_mapped_dataset \
	$(TEST_OUT)/test_memory_mapped_concat_dataset \
	$(TEST_OUT)/test_memory_mapped_concat_dataloader \
	$(TEST_OUT)/test_memory_mapped_edge_cases
	@$(LOG_SUCCESS)

# 6) Aggregate: run
.PHONY: run
run: \
	run-test_memory_mapped_datafile \
	run-test_memory_mapped_dataset \
	run-test_memory_mapped_concat_dataset \
	run-test_memory_mapped_concat_dataloader \
	run-test_memory_mapped_edge_cases

# 7) Clean
.PHONY: clean
clean:
	@rm -f $(TEST_OUT)/test_memory_mapped_*
