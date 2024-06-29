# -----------------------------------------------------------------
# Dependencies - - - - - - - - - - - - - - - - - - - - - - - - - - 
# -----------------------------------------------------------------
ROOT_PATH := ../..
include $(ROOT_PATH)/Makefile.config
# -----------------------------------------------------------------
# Local Makefile Variables - - - - - - - - - - - - - - - - - - - - 
# -----------------------------------------------------------------
HERE_PATH := $(SRC_PATH)/piaabo
COMPILE_CMD = $(LOG_START) && $(LOCAL_CLEAN) && $(GXX) $(GXX_FLAGS) $(USUAL_INCLUDE_PATHS) -c $< -o $@ && $(LOG_END)
COMPILE_ENCRYPT_CMD = $(LOG_START) && $(LOCAL_CLEAN) && $(GXX) $(GXX_FLAGS) $(USUAL_INCLUDE_PATHS) $(SSL_INCLUDE_PATHS) -c $< -o $@ && $(LOG_END)
# # -----------------------------------------------------------------
# # - - - Cross files Uitilies  - - - - - - - - - - - - - - - - - - -
# # -----------------------------------------------------------------
PIAABO := $(OUTPUT_PATH)/dutils.o $(OUTPUT_PATH)/architecture.o $(OUTPUT_PATH)/config_space.o $(OUTPUT_PATH)/statistics_space.o $(OUTPUT_PATH)/evaluation.o $(OUTPUT_PATH)/security.o
# -----------------------------------------------------------------
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
# -----------------------------------------------------------------
$(OUTPUT_PATH)/dutils.o: $(HERE_PATH)/dutils.cpp
	$(COMPILE_CMD)
$(OUTPUT_PATH)/architecture.o: $(HERE_PATH)/architecture.cpp $(OUTPUT_PATH)/dutils.o
	@$(COMPILE_CMD)
$(OUTPUT_PATH)/config_space.o: $(HERE_PATH)/config_space.cpp $(OUTPUT_PATH)/dutils.o
	@$(COMPILE_CMD)
$(OUTPUT_PATH)/statistics_space.o: $(HERE_PATH)/statistics_space.cpp $(OUTPUT_PATH)/dutils.o
	@$(COMPILE_CMD)
$(OUTPUT_PATH)/evaluation.o: $(HERE_PATH)/evaluation.cpp $(OUTPUT_PATH)/dutils.o
	@$(COMPILE_CMD)
$(OUTPUT_PATH)/encryption.o: $(HERE_PATH)/encryption.cpp $(OUTPUT_PATH)/dutils.o
	@$(COMPILE_ENCRYPT_CMD)
$(OUTPUT_PATH)/security.o: $(HERE_PATH)/security.cpp $(OUTPUT_PATH)/dutils.o $(OUTPUT_PATH)/encryption.o $(OUTPUT_PATH)/config_space.o
	@$(COMPILE_ENCRYPT_CMD)
.PHONY: piaabo
piaabo: $(PIAABO)
	@$(LOG_SUCCESS)
# -----------------------------------------------------------------
# - - - General Target -- - - - - - - - - - - - - - - - - - - - - -
# -----------------------------------------------------------------
.PHONY: all
all: $(PIAABO)
	@$(LOG_SUCCESS)
# -----------------------------------------------------------------
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
# -----------------------------------------------------------------