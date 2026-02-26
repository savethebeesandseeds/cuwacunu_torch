# -----------------------------------------------------------------
# Dependencies & Local Variables
# -----------------------------------------------------------------
ROOT_PATH := ../../..
include $(ROOT_PATH)/Makefile.config
HERE_PATH := $(IMPL_PATH)/piaabo/bnf_compat
REL_MODULE  := $(patsubst $(IMPL_PATH)/%,%,$(HERE_PATH))
MODULE_NAME := $(notdir $(HERE_PATH))
# -----------------------------------------------------------------
# Objects in this module (produced under build/common)
# -----------------------------------------------------------------
BNF_CORE_OBJS := \
  $(OUTPUT_PATH)/common/parser_types.o \
  $(OUTPUT_PATH)/common/ast.o \
  $(OUTPUT_PATH)/common/grammar_lexer.o \
  $(OUTPUT_PATH)/common/grammar_parser.o \
  $(OUTPUT_PATH)/common/instruction_lexer.o \
  $(OUTPUT_PATH)/common/instruction_parser.o

# -----------------------------------------------------------------
# Build Rules (producers; no lib*.a prerequisites)
#   BUILD_OBJ(module, source_basename, phony_target)
# -----------------------------------------------------------------
$(eval $(call BUILD_OBJ, common, parser_types,            bnf_types))
$(eval $(call BUILD_OBJ, common, ast,                     bnf_ast))
$(eval $(call BUILD_OBJ, common, grammar_lexer,           bnf_lex))
$(eval $(call BUILD_OBJ, common, grammar_parser,          bnf_parse))
$(eval $(call BUILD_OBJ, common, instruction_lexer,       bnf_instruction_lex))
$(eval $(call BUILD_OBJ, common, instruction_parser,      bnf_instruction_parse))

# Aggregate target for this folder’s objects
.PHONY: bnf
bnf: $(BNF_CORE_OBJS)
	@$(LOG_SUCCESS)

# -----------------------------------------------------------------
# Aggregate Target
# -----------------------------------------------------------------
.PHONY: all
all: bnf
	@$(LOG_SUCCESS)
