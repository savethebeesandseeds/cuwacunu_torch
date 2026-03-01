# -----------------------------------------------------------------
# Dependencies & Local Variables
# -----------------------------------------------------------------
ROOT_PATH := ../../../../
include $(ROOT_PATH)/Makefile.config
HERE_PATH := $(IMPL_PATH)/camahjucunu/dsl/tsiemene_circuit
REL_MODULE  := $(patsubst $(IMPL_PATH)/%,%,$(HERE_PATH))
MODULE_NAME := $(notdir $(HERE_PATH))

# -----------------------------------------------------------------
# Build Rules
# -----------------------------------------------------------------
$(eval $(call BUILD_OBJ, common, tsiemene_circuit_runtime_invoke, dsl_tsiemene_circuit_runtime_invoke))
$(eval $(call BUILD_OBJ, common, tsiemene_circuit_runtime_resolve, dsl_tsiemene_circuit_runtime_resolve))
$(eval $(call BUILD_OBJ, common, tsiemene_circuit_runtime_validate, dsl_tsiemene_circuit_runtime_validate))
$(eval $(call BUILD_OBJ, common, tsiemene_circuit_decode_text, dsl_tsiemene_circuit_decode_text))
$(eval $(call BUILD_OBJ, common, tsiemene_circuit_decode_ast, dsl_tsiemene_circuit_decode_ast))

.PHONY: dsl_tsiemene_circuit
dsl_tsiemene_circuit: \
	dsl_tsiemene_circuit_runtime_invoke \
	dsl_tsiemene_circuit_runtime_resolve \
	dsl_tsiemene_circuit_runtime_validate \
	dsl_tsiemene_circuit_decode_text \
	dsl_tsiemene_circuit_decode_ast

# -----------------------------------------------------------------
# Aggregate Target
# -----------------------------------------------------------------
.PHONY: all
all: dsl_tsiemene_circuit
	@$(LOG_SUCCESS)
