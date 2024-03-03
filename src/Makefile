ENV := valgrind-debug
ENV := gdb-debug
ENV := no-debug

STATIC_FLAGS := -static # enable static
STATIC_FLAGS := # disable static

VALGRIND_COMAND=--tool=helgrind
VALGRIND_COMAND=--tool=memcheck --leak-check=full --show-leak-kinds=all
VALGRIND_COMAND=--tool=memcheck --track-origins=yes --leak-check=full --show-leak-kinds=all --track-fds=yes -s
VALGRIND_COMAND=--tool=memcheck --track-origins=yes --leak-check=full --track-fds=yes -s

PROJECT_PATH := ./
OUTPUT_PATH := ./build
TARGET := cuwacunu
SOURCES := main.cpp

GXX := g++
GXX_VERSION := c++17
REMOVE_WARNINGS := -Wno-unused-function
LIBTORCH := /external/libtorch
LIBCUDA := /usr/local/cuda-12.1
INCLUDE_PATHS := -I$(PROJECT_PATH)/ -I$(PROJECT_PATH)/include -I$(LIBTORCH)/include -I$(LIBTORCH)/include/torch/csrc/api/include -I$(LIBCUDA)/include
LD_FLAGS := -L$(LIBTORCH)/lib -L$(LIBCUDA)/lib64 -Wl,-rpath,$(LIBTORCH)/lib -ltorch_cuda -lc10_cuda -ltorch_cpu -ltorch -lcudnn -lc10 -lcuda -lcudart -lpthread -ldl -lrt -lm 

ifeq ($(ENV),gdb-debug)
GXX_FLAGS := -std=$(GXX_VERSION) $(REMOVE_WARNINGS) -Wall -g -O0 $(STATIC_FLAGS) $(INCLUDE_PATHS)
else
ifeq ($(ENV),valgrind-debug)
GXX_FLAGS := -std=$(GXX_VERSION) $(REMOVE_WARNINGS) -Wall -g -O0 $(STATIC_FLAGS) $(INCLUDE_PATHS)
else
GXX_FLAGS := -std=$(GXX_VERSION) $(REMOVE_WARNINGS) -Wall -g $(STATIC_FLAGS) $(INCLUDE_PATHS)
endif
endif

main:
	$(MAKE) -j$(shell nproc) m_main
clean:
	rm -f $(OUTPUT_PATH)/*.o
m_cuwacunu:
	$(GXX) $(GXX_FLAGS) $(PROJECT_PATH)/$(SOURCES) $(LD_FLAGS) -o $(OUTPUT_PATH)/$(TARGET)
	chmod +x $(OUTPUT_PATH)/$(TARGET)
m_main:
	make clean
	echo "Building [$(TARGET)]..."
	make -j$(nproc) m_cuwacunu
ifeq ($(ENV),gdb-debug)
	gdb $(OUTPUT_PATH)/$(TARGET)
else
ifeq ($(ENV),valgrind-debug)
	valgrind $(VALGRIND_COMAND) $(OUTPUT_PATH)/$(TARGET)
else
	$(OUTPUT_PATH)/$(TARGET)
endif
endif

# g++ -std=c++17 -Wall -I/external/libtorch/include -I/external/libtorch/include/torch/csrc/api/include -L/external/libtorch/lib /src/lib/main.cpp -o /src/build/cuwacucnu.o -ltorch -lc10
