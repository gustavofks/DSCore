# Host unit tests for the pure code in arm9/source/core.
CXX      ?= g++
CXXFLAGS := -std=gnu++17 -Wall -Wextra -Werror -O1 -I../arm9/source -I.
CORE     := $(wildcard ../arm9/source/core/*.cpp)
TESTS    := $(wildcard test_*.cpp)
BIN      := build/run_tests

.PHONY: all clean

all: $(BIN)
	./$(BIN)

$(BIN): main.cpp $(TESTS) $(CORE) $(wildcard ../arm9/source/core/*.h)
	@mkdir -p build
	$(CXX) $(CXXFLAGS) -o $@ main.cpp $(TESTS) $(CORE)

clean:
	rm -rf build
