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
PIAABO := $(OUTPUT_PATH)/dutils.o $(OUTPUT_PATH)/darchitecture.o $(OUTPUT_PATH)/dconfig.o $(OUTPUT_PATH)/json_parsing.o $(OUTPUT_PATH)/dlarge_files.o $(OUTPUT_PATH)/statistics_space.o $(OUTPUT_PATH)/evaluation.o $(OUTPUT_PATH)/dsecurity.o
# -----------------------------------------------------------------
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
# -----------------------------------------------------------------
$(OUTPUT_PATH)/dutils.o: $(HERE_PATH)/dutils.cpp
	@$(COMPILE_CMD)
$(OUTPUT_PATH)/darchitecture.o: $(HERE_PATH)/darchitecture.cpp $(OUTPUT_PATH)/dutils.o
	@$(COMPILE_CMD)
$(OUTPUT_PATH)/dconfig.o: $(HERE_PATH)/dconfig.cpp $(OUTPUT_PATH)/dutils.o
	@$(COMPILE_CMD)
$(OUTPUT_PATH)/json_parsing.o: $(HERE_PATH)/json_parsing.cpp $(OUTPUT_PATH)/dutils.o
	@$(COMPILE_CMD)
$(OUTPUT_PATH)/dlarge_files.o: $(HERE_PATH)/dlarge_files.cpp $(OUTPUT_PATH)/dutils.o
	@$(COMPILE_ENCRYPT_CMD)
$(OUTPUT_PATH)/statistics_space.o: $(HERE_PATH)/statistics_space.cpp $(OUTPUT_PATH)/dutils.o
	@$(COMPILE_CMD)
$(OUTPUT_PATH)/evaluation.o: $(HERE_PATH)/evaluation.cpp $(OUTPUT_PATH)/dutils.o
	@$(COMPILE_CMD)
$(OUTPUT_PATH)/dencryption.o: $(HERE_PATH)/dencryption.cpp $(OUTPUT_PATH)/dutils.o
	@$(COMPILE_ENCRYPT_CMD)
$(OUTPUT_PATH)/dsecurity.o: $(HERE_PATH)/dsecurity.cpp $(OUTPUT_PATH)/dutils.o $(OUTPUT_PATH)/dencryption.o $(OUTPUT_PATH)/dconfig.o
	@$(COMPILE_ENCRYPT_CMD)
# -----------------------------------------------------------------
# - - - General Target -- - - - - - - - - - - - - - - - - - - - - -
# -----------------------------------------------------------------
.PHONY: all
all: $(PIAABO)
	$(MAKE) -C $(HERE_PATH)/torch_compat all
	@$(LOG_SUCCESS)
# -----------------------------------------------------------------
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
# -----------------------------------------------------------------