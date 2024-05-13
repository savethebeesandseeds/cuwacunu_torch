# General settings
GXX := g++
GXX_VERSION := c++17
GXX_FLAGS := -std=$(GXX_VERSION) -Wno-unused-function -Wall -g -MMD -MP -O0

# Path settings
PROJECT_PATH := .
OUTPUT_PATH := $(PROJECT_PATH)/build
SRC_PATH := $(PROJECT_PATH)/src
LIBTORCH_PATH := /external/libtorch
LIBCUDA_PATH := /usr/local/cuda-12.1
USUAL_INCLUDE_PATHS := -I$(PROJECT_PATH)/ -I$(PROJECT_PATH)/include -I$(PROJECT_PATH)/include/torch_compat
TORCH_INCLUDE_PATHS := -I$(LIBTORCH_PATH)/include -I$(LIBTORCH_PATH)/include/torch/csrc/api/include -I$(LIBCUDA_PATH)/include

# Linkage settings
USUAL_LD_FLAGS := -lpthread -ldl -lrt -lm -lstdc++
TORCH_LD_FLAGS := -L$(LIBTORCH_PATH)/lib -L$(LIBCUDA_PATH)/lib64 -Wl,-rpath,$(LIBTORCH_PATH)/lib -ltorch_cuda -lc10_cuda -ltorch_cpu -ltorch -lcudnn -lc10 -lcuda -lcudart 

# Color settings for echo commands
COLOR_RED 	:= \033[31m
COLOR_GREEN := \033[32m
COLOR_RESET := \033[0m

# Dependency inclusion
-include $(wildcard $(OUTPUT_PATH)/*.d)

# -----------------------------------------------------
# COMPILATION RULES FOR INDIVUAL FILES

# -----------------------------------------------------
# General utility functions
$(OUTPUT_PATH)/dutils.o: $(SRC_PATH)/dutils.cpp
	@echo "$(COLOR_GREEN) Building $(OUTPUT_PATH)/dutils.o $(COLOR_RESET)"
	$(GXX) $(GXX_FLAGS) $(SRC_PATH)/dutils.cpp $(USUAL_INCLUDE_PATHS) -c -o $(OUTPUT_PATH)/dutils.o
# -----------------------------------------------------
# Libtorch utils
$(OUTPUT_PATH)/torch_utils.o: $(OUTPUT_PATH)/dutils.o $(SRC_PATH)/torch_compat/torch_utils.cpp
	@echo "$(COLOR_GREEN) Building $(OUTPUT_PATH)/torch_utils.o $(COLOR_RESET)"
	$(GXX) $(GXX_FLAGS) $(SRC_PATH)/torch_compat/torch_utils.cpp $(USUAL_INCLUDE_PATHS) $(TORCH_INCLUDE_PATHS) -c -o $(OUTPUT_PATH)/torch_utils.o
# -----------------------------------------------------
# Pytorch's Probability Distribitions implemented in Libtorch
$(OUTPUT_PATH)/distributions.o: $(OUTPUT_PATH)/dutils.o $(OUTPUT_PATH)/torch_utils.o $(SRC_PATH)/torch_compat/distributions.cpp
	@echo "$(COLOR_GREEN) Building $(OUTPUT_PATH)/distributions.o $(COLOR_RESET)"
	$(GXX) $(GXX_FLAGS) $(SRC_PATH)/torch_compat/distributions.cpp $(USUAL_INCLUDE_PATHS) $(TORCH_INCLUDE_PATHS) -c -o $(OUTPUT_PATH)/distributions.o
# -----------------------------------------------------
# Pytorch's Lambda learning rate scheduler
$(OUTPUT_PATH)/lambda_lr_scheduler.o: $(OUTPUT_PATH)/dutils.o $(OUTPUT_PATH)/torch_utils.o $(SRC_PATH)/torch_compat/lambda_lr_scheduler.cpp
	@echo "$(COLOR_GREEN) Building $(OUTPUT_PATH)/lambda_lr_scheduler.o $(COLOR_RESET)"
	$(GXX) $(GXX_FLAGS) $(SRC_PATH)/torch_compat/lambda_lr_scheduler.cpp $(USUAL_INCLUDE_PATHS) $(TORCH_INCLUDE_PATHS) -c -o $(OUTPUT_PATH)/lambda_lr_scheduler.o
# -----------------------------------------------------
# Object's Types definitions
$(OUTPUT_PATH)/dtypes.o: $(OUTPUT_PATH)/dutils.o $(OUTPUT_PATH)/distributions.o $(SRC_PATH)/dtypes.cpp
	@echo "$(COLOR_GREEN) Building $(OUTPUT_PATH)/dtypes.o $(COLOR_RESET)"
	$(GXX) $(GXX_FLAGS) $(SRC_PATH)/dtypes.cpp $(USUAL_INCLUDE_PATHS) $(TORCH_INCLUDE_PATHS) -c -o $(OUTPUT_PATH)/dtypes.o
# -----------------------------------------------------
# Simulation of broker interactions
$(OUTPUT_PATH)/simulated_broker.o: $(OUTPUT_PATH)/dutils.o $(OUTPUT_PATH)/dtypes.o $(SRC_PATH)/simulated_broker.cpp
	@echo "$(COLOR_GREEN) Building $(OUTPUT_PATH)/simulated_broker.o $(COLOR_RESET)"
	$(GXX) $(GXX_FLAGS) $(SRC_PATH)/simulated_broker.cpp $(USUAL_INCLUDE_PATHS) $(TORCH_INCLUDE_PATHS) -c -o $(OUTPUT_PATH)/simulated_broker.o
# -----------------------------------------------------
# Reinfocement learning kind of enviroment for the market
$(OUTPUT_PATH)/simulated_market_enviroment.o: $(OUTPUT_PATH)/dutils.o $(OUTPUT_PATH)/simulated_broker.o $(OUTPUT_PATH)/dtypes.o $(SRC_PATH)/simulated_market_enviroment.cpp
	@echo "$(COLOR_GREEN) Building $(OUTPUT_PATH)/simulated_market_enviroment.o $(COLOR_RESET)"
	$(GXX) $(GXX_FLAGS) $(SRC_PATH)/simulated_market_enviroment.cpp $(USUAL_INCLUDE_PATHS) $(TORCH_INCLUDE_PATHS) -c -o $(OUTPUT_PATH)/simulated_market_enviroment.o
# -----------------------------------------------------
# Compile the Actor network
$(OUTPUT_PATH)/actor.o: $(OUTPUT_PATH)/dutils.o $(OUTPUT_PATH)/torch_utils.o $(OUTPUT_PATH)/distributions.o $(OUTPUT_PATH)/dtypes.o $(SRC_PATH)/actor.cpp
	@echo "$(COLOR_GREEN) Building $(OUTPUT_PATH)/actor.o $(COLOR_RESET)"
	$(GXX) $(GXX_FLAGS) $(USUAL_INCLUDE_PATHS) $(TORCH_INCLUDE_PATHS) $(SRC_PATH)/actor.cpp -c -o $(OUTPUT_PATH)/actor.o
# -----------------------------------------------------
# Compile the Critic network
$(OUTPUT_PATH)/critic.o: $(OUTPUT_PATH)/dutils.o $(OUTPUT_PATH)/torch_utils.o $(OUTPUT_PATH)/distributions.o $(OUTPUT_PATH)/dtypes.o $(SRC_PATH)/critic.cpp
	@echo "$(COLOR_GREEN) Building $(OUTPUT_PATH)/critic.o $(COLOR_RESET)"
	$(GXX) $(GXX_FLAGS) $(USUAL_INCLUDE_PATHS) $(TORCH_INCLUDE_PATHS) $(SRC_PATH)/critic.cpp -c -o $(OUTPUT_PATH)/critic.o
# -----------------------------------------------------
# Compile the Actor-Critic Schema
$(OUTPUT_PATH)/actor_critic.o: $(OUTPUT_PATH)/dutils.o $(OUTPUT_PATH)/lambda_lr_scheduler.o $(OUTPUT_PATH)/actor.o $(OUTPUT_PATH)/critic.o $(SRC_PATH)/actor_critic.cpp
	@echo "$(COLOR_GREEN) Building $(OUTPUT_PATH)/actor_critic.o $(COLOR_RESET)"
	$(GXX) $(GXX_FLAGS) $(USUAL_INCLUDE_PATHS) $(TORCH_INCLUDE_PATHS) $(SRC_PATH)/actor_critic.cpp -c -o $(OUTPUT_PATH)/actor_critic.o
# -----------------------------------------------------
# Compile the main object
$(OUTPUT_PATH)/main.o: $(OUTPUT_PATH)/dutils.o $(OUTPUT_PATH)/simulated_market_enviroment.o $(OUTPUT_PATH)/actor_critic.o $(PROJECT_PATH)/main.cpp
	@echo "$(COLOR_GREEN) Building $(OUTPUT_PATH)/main.o $(COLOR_RESET)"
	$(GXX) $(GXX_FLAGS) $(USUAL_INCLUDE_PATHS) $(TORCH_INCLUDE_PATHS) $(PROJECT_PATH)/main.cpp -c -o $(OUTPUT_PATH)/main.o
# -----------------------------------------------------
# Compile the final executable
.PHONY: cuwacunu
cuwacunu: $(OUTPUT_PATH)/cuwacunu

$(OUTPUT_PATH)/cuwacunu: $(OUTPUT_PATH)/main.o
	@echo "$(COLOR_GREEN) Building $(OUTPUT_PATH)/cuwacunu $(COLOR_RESET)"
	$(GXX) $(GXX_FLAGS) $(USUAL_INCLUDE_PATHS) $(TORCH_INCLUDE_PATHS) $(OUTPUT_PATH)/*.o $(TORCH_LD_FLAGS) $(USUAL_LD_FLAGS) -o $(OUTPUT_PATH)/cuwacunu
# -----------------------------------------------------
# Clean the build
.PHONY: clean
clean:
	@echo "$(COLOR_RED) Cleaning cuwacunu build $(COLOR_RESET)"
	rm -f $(OUTPUT_PATH)/*.d $(OUTPUT_PATH)/*.o $(OUTPUT_PATH)/cuwacunu