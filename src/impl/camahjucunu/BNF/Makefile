# -----------------------------------------------------------------
# Dependencies & Local Variables
# -----------------------------------------------------------------
ROOT_PATH := ../../..
include $(ROOT_PATH)/Makefile.config
HERE_PATH := $(IMPL_PATH)/camahjucunu/BNF
REL_MODULE  := $(patsubst $(IMPL_PATH)/%,%,$(HERE_PATH))
MODULE_NAME := $(notdir $(HERE_PATH))
# -----------------------------------------------------------------
# Objects in this module (produced under build/common)
# -----------------------------------------------------------------
BNF_OBJS := \
  $(OUTPUT_PATH)/common/BNF_types.o \
  $(OUTPUT_PATH)/common/BNF_AST.o \
  $(OUTPUT_PATH)/common/BNF_grammar_lexer.o \
  $(OUTPUT_PATH)/common/BNF_grammar_parser.o \
  $(OUTPUT_PATH)/common/BNF_instruction_lexer.o \
  $(OUTPUT_PATH)/common/BNF_instruction_parser.o

# -----------------------------------------------------------------
# Build Rules (producers; no lib*.a prerequisites)
#   BUILD_OBJ(module, source_basename, phony_target)
# -----------------------------------------------------------------
$(eval $(call BUILD_OBJ, common, BNF_types,               bnf_types))
$(eval $(call BUILD_OBJ, common, BNF_AST,                 bnf_ast))
$(eval $(call BUILD_OBJ, common, BNF_grammar_lexer,       bnf_lex))
$(eval $(call BUILD_OBJ, common, BNF_grammar_parser,      bnf_parse))
$(eval $(call BUILD_OBJ, common, BNF_instruction_lexer,   bnf_ilex))
$(eval $(call BUILD_OBJ, common, BNF_instruction_parser,  bnf_iparse))

# Aggregate target for this folder’s objects
.PHONY: bnf
bnf: $(BNF_OBJS)
	@$(LOG_SUCCESS)

# -----------------------------------------------------------------
# Aggregate Target
#   - Build local objects, then recurse into sub-implementations
# -----------------------------------------------------------------
.PHONY: all
all: bnf
	$(MAKE) -C $(HERE_PATH)/implementations all
	@$(LOG_SUCCESS)
