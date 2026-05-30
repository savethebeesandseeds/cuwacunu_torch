ROOT_PATH := ../../../../../..
include $(ROOT_PATH)/Makefile.config

HERE_PATH  := $(TESTS_PATH)/bench/wikimyei/representation/encoding/mtf_jepa_mae_vicreg
REL_MODULE := $(patsubst $(TESTS_PATH)/%,%,$(HERE_PATH))

TEST_DEFAULT_LDLIBS :=

$(eval $(call TEST_ONEFILE, test_wikimyei_mtf_jepa_mae_vicreg, test_wikimyei_mtf_jepa_mae_vicreg.cpp, \
  $(LDLIBS_torch)))
$(eval $(call TEST_ONEFILE, train_synthetic_mtf_jepa_mae_vicreg, train_synthetic_mtf_jepa_mae_vicreg.cpp, \
  $(LDLIBS_torch)))

$(TEST_OUT)/test_wikimyei_mtf_jepa_mae_vicreg: INCLUDES_EXTRA += $(TORCH_INCLUDE_PATHS)
$(TEST_OUT)/train_synthetic_mtf_jepa_mae_vicreg: INCLUDES_EXTRA += $(TORCH_INCLUDE_PATHS)

.PHONY: all
all: $(TEST_OUT)/test_wikimyei_mtf_jepa_mae_vicreg $(TEST_OUT)/train_synthetic_mtf_jepa_mae_vicreg
	@$(LOG_SUCCESS)

.PHONY: run
run: run-test_wikimyei_mtf_jepa_mae_vicreg run-train_synthetic_mtf_jepa_mae_vicreg

.PHONY: clean
clean:
	@rm -f $(TEST_OUT)/test_wikimyei_mtf_jepa_mae_vicreg
	@rm -f $(TEST_OUT)/train_synthetic_mtf_jepa_mae_vicreg
