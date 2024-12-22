# -----------------------------------------------------------------
# Dependencies - - - - - - - - - - - - - - - - - - - - - - - - - - 
# -----------------------------------------------------------------
ROOT_PATH := ../../../../..
include $(ROOT_PATH)/Makefile.config
# -----------------------------------------------------------------
# Local Makefile Variables - - - - - - - - - - - - - - - - - - - - 
# -----------------------------------------------------------------
HERE_PATH := $(SRC_PATH)/camahjucunu/BNF/implementations/training_pipeline
USUAL_DEPS := $(OUTPUT_PATH)/common/dutils.o $(OUTPUT_PATH)/common/dconfig.o $(OUTPUT_PATH)/common/BNF_AST.o $(OUTPUT_PATH)/common/BNF_grammar_lexer.o $(OUTPUT_PATH)/common/BNF_grammar_parser.o
COMPILE_CMD = $(LOG_START) && $(LOCAL_CLEAN) && $(GXX) $(GXX_FLAGS) $(USUAL_INCLUDE_PATHS) -c $< -o $@ && $(LOG_END)
# # -----------------------------------------------------------------
# # - - - Cross files Uitilies  - - - - - - - - - - - - - - - - - - -
# # -----------------------------------------------------------------
BNF_OBS_PIPE := \
	$(OUTPUT_PATH)/common/training_pipeline.o
# -----------------------------------------------------------------
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
# -----------------------------------------------------------------
$(OUTPUT_PATH)/common/training_pipeline.o: $(HERE_PATH)/training_pipeline.cpp $(USUAL_DEPS)
	@$(COMPILE_CMD)
.PHONY: bnf_train_pipe
bnf_train_pipe: $(BNF_OBS_PIPE)
	@$(LOG_SUCCESS)
# -----------------------------------------------------------------
# - - - General Target -- - - - - - - - - - - - - - - - - - - - - -
# -----------------------------------------------------------------
.PHONY: all
all: bnf_train_pipe
	@$(LOG_SUCCESS)
# -----------------------------------------------------------------
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
# -----------------------------------------------------------------