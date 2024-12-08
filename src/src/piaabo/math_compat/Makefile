# -----------------------------------------------------------------
# Dependencies - - - - - - - - - - - - - - - - - - - - - - - - - - 
# -----------------------------------------------------------------
ROOT_PATH := ../../..
include $(ROOT_PATH)/Makefile.config
# -----------------------------------------------------------------
# Local Makefile Variables - - - - - - - - - - - - - - - - - - - - 
# -----------------------------------------------------------------
HERE_PATH := $(SRC_PATH)/piaabo/math_compat
COMPILE_CMD = $(LOG_START) && $(GXX) $(GXX_FLAGS) $(USUAL_INCLUDE_PATHS) -c $< -o $@ && $(LOG_END)
# -----------------------------------------------------------------
# - - - Mathematics Utilities  - - - - - - - - - - - - - - - - - -  
# -----------------------------------------------------------------
STATISTICS_TARGET := $(OUTPUT_PATH)/common/statistics_space.o 
$(STATISTICS_TARGET): $(HERE_PATH)/statistics_space.cpp $(OUTPUT_PATH)/common/dutils.o
	@$(COMPILE_CMD)
.PHONY: statistics
statistics: $(STATISTICS_TARGET)
	@$(LOG_SUCCESS)
# -----------------------------------------------------------------
# - - - General Target -- - - - - - - - - - - - - - - - - - - - - -
# -----------------------------------------------------------------
.PHONY: all
all: statistics
	@$(LOG_SUCCESS)
# -----------------------------------------------------------------
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
# -----------------------------------------------------------------