# -----------------------------------------------------------------
# Dependencies - - - - - - - - - - - - - - - - - - - - - - - - - - 
# -----------------------------------------------------------------
ROOT_PATH := .
include $(ROOT_PATH)/Makefile.config
# -----------------------------------------------------------------
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
# -----------------------------------------------------------------
.PHONY: piaabo
piaabo:
	$(MAKE) -C $(SRC_PATH)/ piaabo
# -----------------------------------------------------------------
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
# -----------------------------------------------------------------
.PHONY: torch_compat
torch_compat:
	$(MAKE) -C $(SRC_PATH)/ torch_compat
# -----------------------------------------------------------------
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
# -----------------------------------------------------------------
.PHONY: camahjucunu
camahjucunu:
	$(MAKE) -C $(SRC_PATH)/ camahjucunu
# -----------------------------------------------------------------
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
# -----------------------------------------------------------------
.PHONY: iinuji
iinuji:
	$(MAKE) -C $(SRC_PATH)/ iinuji
# -----------------------------------------------------------------
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
# -----------------------------------------------------------------
.PHONY: tests
tests:
	$(MAKE) -C $(TESTS_PATH)/ all
# -----------------------------------------------------------------
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
# -----------------------------------------------------------------
# Clean the build
.PHONY: clean
clean:
	@echo "$(COLOR_RED) Cleaning cuwacunu build $(COLOR_RESET)"
	shred -u $(OUTPUT_PATH)/*.d $(OUTPUT_PATH)/*.o
# -----------------------------------------------------------------
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
# -----------------------------------------------------------------





# # -----------------------------------------------------------------
# # - - - Cuwacunu Object's Types definitions - - - - - - - - - - - -
# # -----------------------------------------------------------------
# # instrument_space_t
# $(OUTPUT_PATH)/instrument_space.o: $(OUTPUT_PATH)/dutils.o $(SRC_PATH)/cuwacunu_types/instrument_space.cpp
# 	@echo "$(COLOR_GREEN) Building $(OUTPUT_PATH)/instrument_space.o $(COLOR_RESET)"
# 	$(GXX) $(GXX_FLAGS) $(SRC_PATH)/cuwacunu_types/instrument_space.cpp $(USUAL_INCLUDE_PATHS) -c -o $(OUTPUT_PATH)/instrument_space.o
# # -----------------------------------------------------------------
# # statistics_space_t
# $(OUTPUT_PATH)/statistics_space.o: $(OUTPUT_PATH)/dutils.o $(SRC_PATH)/cuwacunu_types/statistics_space.cpp
# 	@echo "$(COLOR_GREEN) Building $(OUTPUT_PATH)/statistics_space.o $(COLOR_RESET)"
# 	$(GXX) $(GXX_FLAGS) $(SRC_PATH)/cuwacunu_types/statistics_space.cpp $(USUAL_INCLUDE_PATHS) -c -o $(OUTPUT_PATH)/statistics_space.o
# # -----------------------------------------------------------------
# # wallet_space_t
# $(OUTPUT_PATH)/wallet_space.o: $(OUTPUT_PATH)/dutils.o $(SRC_PATH)/cuwacunu_types/wallet_space.cpp
# 	@echo "$(COLOR_GREEN) Building $(OUTPUT_PATH)/wallet_space.o $(COLOR_RESET)"
# 	$(GXX) $(GXX_FLAGS) $(SRC_PATH)/cuwacunu_types/wallet_space.cpp $(USUAL_INCLUDE_PATHS) -c -o $(OUTPUT_PATH)/wallet_space.o
# # -----------------------------------------------------------------
# # action_logits_t + action_space_t
# $(OUTPUT_PATH)/action_space.o: $(OUTPUT_PATH)/dutils.o $(OUTPUT_PATH)/instrument_space.o $(OUTPUT_PATH)/beta.o $(OUTPUT_PATH)/categorical.o $(SRC_PATH)/cuwacunu_types/action_space.cpp
# 	@echo "$(COLOR_GREEN) Building $(OUTPUT_PATH)/action_space.o $(COLOR_RESET)"
# 	$(GXX) $(GXX_FLAGS) $(SRC_PATH)/cuwacunu_types/action_space.cpp $(USUAL_INCLUDE_PATHS) $(TORCH_INCLUDE_PATHS) -c -o $(OUTPUT_PATH)/action_space.o







# # -----------------------------------------------------------------
# # Simulation of broker interactions
# $(OUTPUT_PATH)/simulated_broker.o: $(OUTPUT_PATH)/dutils.o $(OUTPUT_PATH)/dtypes.o $(SRC_PATH)/simulated_broker.cpp
# 	@echo "$(COLOR_GREEN) Building $(OUTPUT_PATH)/simulated_broker.o $(COLOR_RESET)"
# 	$(GXX) $(GXX_FLAGS) $(SRC_PATH)/simulated_broker.cpp $(USUAL_INCLUDE_PATHS) $(TORCH_INCLUDE_PATHS) -c -o $(OUTPUT_PATH)/simulated_broker.o
# # -----------------------------------------------------------------
# # Reinfocement learning kind of enviroment for the market
# $(OUTPUT_PATH)/simulated_market_enviroment.o: $(OUTPUT_PATH)/dutils.o $(OUTPUT_PATH)/simulated_broker.o $(OUTPUT_PATH)/dtypes.o $(SRC_PATH)/simulated_market_enviroment.cpp
# 	@echo "$(COLOR_GREEN) Building $(OUTPUT_PATH)/simulated_market_enviroment.o $(COLOR_RESET)"
# 	$(GXX) $(GXX_FLAGS) $(SRC_PATH)/simulated_market_enviroment.cpp $(USUAL_INCLUDE_PATHS) $(TORCH_INCLUDE_PATHS) -c -o $(OUTPUT_PATH)/simulated_market_enviroment.o
# # -----------------------------------------------------------------
# # Compile the Actor network
# $(OUTPUT_PATH)/actor.o: $(OUTPUT_PATH)/dutils.o $(OUTPUT_PATH)/torch_utils.o $(OUTPUT_PATH)/distributions.o $(OUTPUT_PATH)/dtypes.o $(SRC_PATH)/actor.cpp
# 	@echo "$(COLOR_GREEN) Building $(OUTPUT_PATH)/actor.o $(COLOR_RESET)"
# 	$(GXX) $(GXX_FLAGS) $(USUAL_INCLUDE_PATHS) $(TORCH_INCLUDE_PATHS) $(SRC_PATH)/actor.cpp -c -o $(OUTPUT_PATH)/actor.o
# # -----------------------------------------------------------------
# # Compile the Critic network
# $(OUTPUT_PATH)/critic.o: $(OUTPUT_PATH)/dutils.o $(OUTPUT_PATH)/torch_utils.o $(OUTPUT_PATH)/distributions.o $(OUTPUT_PATH)/dtypes.o $(SRC_PATH)/critic.cpp
# 	@echo "$(COLOR_GREEN) Building $(OUTPUT_PATH)/critic.o $(COLOR_RESET)"
# 	$(GXX) $(GXX_FLAGS) $(USUAL_INCLUDE_PATHS) $(TORCH_INCLUDE_PATHS) $(SRC_PATH)/critic.cpp -c -o $(OUTPUT_PATH)/critic.o
# # -----------------------------------------------------------------
# # Compile the Actor-Critic Schema
# $(OUTPUT_PATH)/actor_critic.o: $(OUTPUT_PATH)/dutils.o $(OUTPUT_PATH)/lambda_lr_scheduler.o $(OUTPUT_PATH)/actor.o $(OUTPUT_PATH)/critic.o $(SRC_PATH)/actor_critic.cpp
# 	@echo "$(COLOR_GREEN) Building $(OUTPUT_PATH)/actor_critic.o $(COLOR_RESET)"
# 	$(GXX) $(GXX_FLAGS) $(USUAL_INCLUDE_PATHS) $(TORCH_INCLUDE_PATHS) $(SRC_PATH)/actor_critic.cpp -c -o $(OUTPUT_PATH)/actor_critic.o
# # -----------------------------------------------------------------
# # Compile the main object
# $(OUTPUT_PATH)/main.o: $(OUTPUT_PATH)/dutils.o $(OUTPUT_PATH)/simulated_market_enviroment.o $(OUTPUT_PATH)/actor_critic.o $(PROJECT_PATH)/main.cpp
# 	@echo "$(COLOR_GREEN) Building $(OUTPUT_PATH)/main.o $(COLOR_RESET)"
# 	$(GXX) $(GXX_FLAGS) $(USUAL_INCLUDE_PATHS) $(TORCH_INCLUDE_PATHS) $(PROJECT_PATH)/main.cpp -o $(OUTPUT_PATH)/main.o
# # -----------------------------------------------------------------
# # Compile the final executable
# .PHONY: cuwacunu
# cuwacunu: $(OUTPUT_PATH)/cuwacunu

# $(OUTPUT_PATH)/cuwacunu: $(OUTPUT_PATH)/main.o
# 	@echo "$(COLOR_GREEN) Building $(OUTPUT_PATH)/cuwacunu $(COLOR_RESET)"
# 	$(GXX) $(GXX_FLAGS) $(USUAL_INCLUDE_PATHS) $(TORCH_INCLUDE_PATHS) $(OUTPUT_PATH)/*.o $(TORCH_LD_FLAGS) $(USUAL_LD_FLAGS) -o $(OUTPUT_PATH)/cuwacunu
# # -----------------------------------------------------------------
# # Clean the build
# .PHONY: clean
# clean:
# 	@echo "$(COLOR_RED) Cleaning cuwacunu build $(COLOR_RESET)"
# 	rm -f $(OUTPUT_PATH)/*.d $(OUTPUT_PATH)/*.o $(OUTPUT_PATH)/cuwacunu