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
PIAABO := \
	$(OUTPUT_PATH)/common/dutils.o \
	$(OUTPUT_PATH)/common/darchitecture.o \
	$(OUTPUT_PATH)/common/dconfig.o \
	$(OUTPUT_PATH)/common/djson_parsing.o \
	$(OUTPUT_PATH)/common/evaluation.o \
	$(OUTPUT_PATH)/common/dfiles.o \
	$(OUTPUT_PATH)/openssl/dencryption.o \
	$(OUTPUT_PATH)/openssl/dsecurity.o
# -----------------------------------------------------------------
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
# -----------------------------------------------------------------
$(OUTPUT_PATH)/common/dutils.o: $(HERE_PATH)/dutils.cpp
	@$(COMPILE_CMD)
$(OUTPUT_PATH)/common/darchitecture.o: $(HERE_PATH)/darchitecture.cpp $(OUTPUT_PATH)/common/dutils.o
	@$(COMPILE_CMD)
$(OUTPUT_PATH)/common/djson_parsing.o: $(HERE_PATH)/djson_parsing.cpp $(OUTPUT_PATH)/common/dutils.o
	@$(COMPILE_CMD)
$(OUTPUT_PATH)/common/evaluation.o: $(HERE_PATH)/evaluation.cpp $(OUTPUT_PATH)/common/dutils.o
	@$(COMPILE_CMD)
$(OUTPUT_PATH)/common/dfiles.o: $(HERE_PATH)/dfiles.cpp $(OUTPUT_PATH)/common/dutils.o
	@$(COMPILE_CMD)
$(OUTPUT_PATH)/common/dconfig.o: $(HERE_PATH)/dconfig.cpp $(OUTPUT_PATH)/common/dutils.o $(OUTPUT_PATH)/common/dfiles.o
	@$(COMPILE_CMD)
$(OUTPUT_PATH)/openssl/dencryption.o: $(HERE_PATH)/dencryption.cpp $(OUTPUT_PATH)/common/dutils.o
	@$(COMPILE_ENCRYPT_CMD)
$(OUTPUT_PATH)/openssl/dsecurity.o: $(HERE_PATH)/dsecurity.cpp $(OUTPUT_PATH)/common/dutils.o $(OUTPUT_PATH)/openssl/dencryption.o $(OUTPUT_PATH)/common/dconfig.o
	@$(COMPILE_ENCRYPT_CMD)
# -----------------------------------------------------------------
# - - - General Target -- - - - - - - - - - - - - - - - - - - - - -
# -----------------------------------------------------------------
.PHONY: all
all: $(PIAABO)
	$(MAKE) -C $(HERE_PATH)/math_compat all
	$(MAKE) -C $(HERE_PATH)/torch_compat all
	@$(LOG_SUCCESS)
# -----------------------------------------------------------------
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
# -----------------------------------------------------------------