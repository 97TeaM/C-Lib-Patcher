CXX = clang++
CXXFLAGS = -Wall -target armv7a-linux-androideabi19 -static-libstdc++
SRC_FILES = injector.cpp
BUILD_DIR = build
OUTPUT = $(BUILD_DIR)/injector

all: $(OUTPUT)

$(OUTPUT): $(SRC_FILES)
	mkdir -p $(BUILD_DIR)
	$(CXX) -o $@ $^ $(CXXFLAGS)

clean:
	rm -rf ./$(BUILD_DIR)

.PHONY: all clean
