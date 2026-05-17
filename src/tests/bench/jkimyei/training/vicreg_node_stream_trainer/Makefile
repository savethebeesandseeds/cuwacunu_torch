ROOT_PATH := ../../../../..
include $(ROOT_PATH)/Makefile.config

HERE_PATH  := $(TESTS_PATH)/bench/jkimyei/training/vicreg_node_stream_trainer
REL_MODULE := $(patsubst $(TESTS_PATH)/%,%,$(HERE_PATH))

TEST_DEFAULT_LDLIBS :=

$(eval $(call TEST_ONEFILE, test_jkimyei_vicreg_node_stream_trainer, test_jkimyei_vicreg_node_stream_trainer.cpp, \
  $(libtorch_a) $(LDLIBS_torch)))

$(TEST_OUT)/test_jkimyei_vicreg_node_stream_trainer: INCLUDES_EXTRA += $(TORCH_INCLUDE_PATHS)

.PHONY: all
all: $(TEST_OUT)/test_jkimyei_vicreg_node_stream_trainer
	@$(LOG_SUCCESS)

.PHONY: run
run: run-test_jkimyei_vicreg_node_stream_trainer

.PHONY: clean
clean:
	@rm -f $(TEST_OUT)/test_jkimyei_vicreg_node_stream_trainer
