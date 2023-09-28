CXX = clang++
CXXFLAGS = -Wall -target armv7a-linux-androideabi19 -static-libstdc++
SRC_FILES = injector.cpp
OUTPUT = injector

all: $(OUTPUT)

$(OUTPUT): $(SRC_FILES)
	$(CXX) -o $@ $^ $(CXXFLAGS)

clean:
	rm -f $(OUTPUT)

.PHONY: all clean
