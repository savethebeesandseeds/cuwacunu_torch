# -----------------------------------------------------------------
# Dependencies - - - - - - - - - - - - - - - - - - - - - - - - - - 
# -----------------------------------------------------------------
ROOT_PATH := ../../..
include $(ROOT_PATH)/Makefile.config
# -----------------------------------------------------------------
# Local Makefile Variables - - - - - - - - - - - - - - - - - - - - 
# -----------------------------------------------------------------
HERE_PATH := $(SRC_PATH)/camahjucunu/BNF/
USUAL_DEPS := $(OUTPUT_PATH)/dutils.o $(OUTPUT_PATH)/dconfig.o 
COMPILE_CMD = $(LOG_START) && $(LOCAL_CLEAN) && $(GXX) $(GXX_FLAGS) $(USUAL_INCLUDE_PATHS) -c $< -o $@ && $(LOG_END)
# # -----------------------------------------------------------------
# # - - - Cross files Uitilies  - - - - - - - - - - - - - - - - - - -
# # -----------------------------------------------------------------
BNF := \
	$(OUTPUT_PATH)/BNF_types.o \
	$(OUTPUT_PATH)/BNF_AST.o \
	$(OUTPUT_PATH)/BNF_grammar_lexer.o \
	$(OUTPUT_PATH)/BNF_grammar_parser.o \
	$(OUTPUT_PATH)/BNF_instruction_lexer.o \
	$(OUTPUT_PATH)/BNF_instruction_parser.o
# -----------------------------------------------------------------
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
# -----------------------------------------------------------------
$(OUTPUT_PATH)/BNF_types.o: $(HERE_PATH)/BNF_types.cpp $(USUAL_DEPS)
	@$(COMPILE_CMD)
$(OUTPUT_PATH)/BNF_AST.o: $(HERE_PATH)/BNF_AST.cpp $(USUAL_DEPS) $(OUTPUT_PATH)/BNF_types.o
	@$(COMPILE_CMD)
$(OUTPUT_PATH)/BNF_grammar_lexer.o: $(HERE_PATH)/BNF_grammar_lexer.cpp $(USUAL_DEPS) $(OUTPUT_PATH)/BNF_types.o
	@$(COMPILE_CMD)
$(OUTPUT_PATH)/BNF_instruction_lexer.o: $(HERE_PATH)/BNF_instruction_lexer.cpp $(USUAL_DEPS) $(OUTPUT_PATH)/BNF_types.o
	@$(COMPILE_CMD)
$(OUTPUT_PATH)/BNF_grammar_parser.o: $(HERE_PATH)/BNF_grammar_parser.cpp $(USUAL_DEPS) $(OUTPUT_PATH)/BNF_types.o $(OUTPUT_PATH)/BNF_grammar_lexer.o 
	@$(COMPILE_CMD)
$(OUTPUT_PATH)/BNF_instruction_parser.o: $(HERE_PATH)/BNF_instruction_parser.cpp $(USUAL_DEPS) $(OUTPUT_PATH)/BNF_types.o $(OUTPUT_PATH)/BNF_grammar_lexer.o $(OUTPUT_PATH)/BNF_instruction_lexer.o $(OUTPUT_PATH)/BNF_AST.o $(OUTPUT_PATH)/BNF_grammar_parser.o
	@$(COMPILE_CMD)
.PHONY: bnf
bnf: $(BNF)
	@$(LOG_SUCCESS)
# -----------------------------------------------------------------
# - - - General Target -- - - - - - - - - - - - - - - - - - - - - -
# -----------------------------------------------------------------
.PHONY: all
all: bnf
	$(MAKE) -C $(HERE_PATH)/implementations all
	@$(LOG_SUCCESS)
# -----------------------------------------------------------------
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
# -----------------------------------------------------------------