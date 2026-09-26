# Host unit tests for the pure code in arm9/source/core and arm9/source/gfx.
CXX      ?= g++
CXXFLAGS := -std=gnu++17 -Wall -Wextra -Werror -O1 -I../arm9/source -I.
CORE     := $(wildcard ../arm9/source/core/*.cpp ../arm9/source/gfx/*.cpp)
TESTS    := $(wildcard test_*.cpp)
BIN      := build/run_tests

.PHONY: all clean

all: $(BIN)
	./$(BIN)

$(BIN): main.cpp $(TESTS) $(CORE) $(wildcard ../arm9/source/core/*.h ../arm9/source/gfx/*.h)
	@mkdir -p build
	$(CXX) $(CXXFLAGS) -o $@ main.cpp $(TESTS) $(CORE)

clean:
	rm -rf build
