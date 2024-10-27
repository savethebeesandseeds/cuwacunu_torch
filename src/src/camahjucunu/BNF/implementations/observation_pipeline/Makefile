# -----------------------------------------------------------------
# Dependencies - - - - - - - - - - - - - - - - - - - - - - - - - - 
# -----------------------------------------------------------------
ROOT_PATH := ../../../../..
include $(ROOT_PATH)/Makefile.config
# -----------------------------------------------------------------
# Local Makefile Variables - - - - - - - - - - - - - - - - - - - - 
# -----------------------------------------------------------------
HERE_PATH := $(SRC_PATH)/camahjucunu/BNF/implementations/observation_pipeline
USUAL_DEPS := $(OUTPUT_PATH)/dutils.o $(OUTPUT_PATH)/dconfig.o $(OUTPUT_PATH)/BNF_AST.o $(OUTPUT_PATH)/BNF_grammar_lexer.o $(OUTPUT_PATH)/BNF_grammar_parser.o
COMPILE_CMD = $(LOG_START) && $(LOCAL_CLEAN) && $(GXX) $(GXX_FLAGS) $(USUAL_INCLUDE_PATHS) -c $< -o $@ && $(LOG_END)
# # -----------------------------------------------------------------
# # - - - Cross files Uitilies  - - - - - - - - - - - - - - - - - - -
# # -----------------------------------------------------------------
BNF_OBS_PIPE := \
	$(OUTPUT_PATH)/observation_pipeline.o
# -----------------------------------------------------------------
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
# -----------------------------------------------------------------
$(OUTPUT_PATH)/observation_pipeline.o: $(HERE_PATH)/observation_pipeline.cpp $(USUAL_DEPS)
	@$(COMPILE_CMD)
.PHONY: bnf_obs_pipe
bnf_obs_pipe: $(BNF_OBS_PIPE)
	@$(LOG_SUCCESS)
# -----------------------------------------------------------------
# - - - General Target -- - - - - - - - - - - - - - - - - - - - - -
# -----------------------------------------------------------------
.PHONY: all
all: bnf_obs_pipe
	@$(LOG_SUCCESS)
# -----------------------------------------------------------------
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
# -----------------------------------------------------------------