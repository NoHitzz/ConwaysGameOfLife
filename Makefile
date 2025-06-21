# 
# Makefile
# chip8-emulator
# 
# Noah Hitz 2025
# 

TARGET := conway 

BUILD_DIR := ./build
SRC_DIRS := ./src

# Source files
SRCS = main.cpp 

# Compiler
CXX = g++

# Compiler flags
# CPPFLAGS = -std=c++20 -O0 -Wall -g
CPPFLAGS = -std=c++20 -O1 -Wall
INC_FLAGS := -I /usr/local/include
LIB_FLAGS := -L /usr/local/lib
LIBS := -lSDL3 -lSDL3_ttf 

# Object files
OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)

# Linking
$(BUILD_DIR)/$(TARGET): $(OBJS)
	$(CXX) $(OBJS) $(CPPFLAGS) $(LIB_FLAGS) $(LIBS) -o $@

# Compilation
$(BUILD_DIR)/%.cpp.o: $(SRC_DIRS)/%.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(INC_FLAGS) -c $< -o $@
	compiledb make

.PHONY: run
run: 
	./$(BUILD_DIR)/$(TARGET) 

.PHONY: clean
clean:
	rm -r $(BUILD_DIR)
