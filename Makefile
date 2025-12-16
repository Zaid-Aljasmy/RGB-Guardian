# ============================================
# Makefile for RGB Guardian Project
# ============================================

CXX = g++                              # C++ Compiler
CXXFLAGS = -std=c++17 -Wall -Wextra    # Compilation flags
LDFLAGS = -lSDL2 -lSDL2_mixer -lSDL2_ttf  # Linking libraries

# Directories
SRC_DIR = src
BUILD_DIR = build
TARGET = rgb_guardian                  # Executable name

# Source files
SOURCES = $(wildcard $(SRC_DIR)/*.cpp)
OBJECTS = $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/%.o, $(SOURCES))

# ============================================
# Targets
# ============================================

# Default target: build the game
all: $(TARGET)
	@echo "✅ Build successful!"
	@echo "Run the game: make run"

# Build executable from object files
$(TARGET): $(OBJECTS)
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(OBJECTS) -o $@ $(LDFLAGS)
	@echo "🔗 Linked executable: $(TARGET)"

# Compile source files to object files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@
	@echo "⚙️  Compiling: $<"

# Run the game
run: $(TARGET)
	@echo "🎮 Starting game..."
	./$(TARGET)

# Clean generated files
clean:
	@rm -rf $(BUILD_DIR) $(TARGET)
	@echo "🧹 Project cleaned"

# Rebuild from scratch
rebuild: clean all

# Show help
help:
	@echo "=== Available Makefile Commands ==="
	@echo "  make        or  make all     : Build the project"
	@echo "  make run                     : Run the game"
	@echo "  make clean                   : Clean generated files"
	@echo "  make rebuild                 : Rebuild from scratch"
	@echo "  make help                    : Show this help"
	@echo "===================================="

# Prevent make from confusing targets with file names
.PHONY: all run clean rebuild help
