##CFLAGS += -include tt.h
#program : 
#    g++ *.cpp $(CXXFLAGS) -o program
#

# Compiler and flags
CXX      := g++
# -MMD -MP: 让 g++ 自动为每个 .cpp 生成 .d 文件，列出其 #include 的所有头文件。
CXXFLAGS := -std=c++17 -Wall -Werror -g -MMD -MP -I.

# Source files and output
SRC      := main.cpp utils.cpp $(wildcard tests/*.cpp)
OBJ      := $(patsubst %.cpp, out/%.o, $(SRC))
# 把 .o 文件路径转换成 .d 文件路径。
DEP      := $(OBJ:.o=.d)
OUT      := out
TARGET   := $(OUT)/program

# Default target
all: $(TARGET)

# Link the final executable
$(TARGET): $(OBJ)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Compile each .cpp file into a .o file
$(OUT)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Include auto-generated dependency files
# 告诉 make 包含这些 .d 文件，这样一旦 utils.h 改了，相关的 .o 会自动重编译。
-include $(DEP)

# Clean up build files
clean:
	rm -rf $(OUT)

# Phony targets
.PHONY: all clean
