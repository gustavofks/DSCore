# Host unit tests for the pure code in arm9/source/core, gfx and ui.
CXX      ?= g++
CXXFLAGS := -std=gnu++17 -Wall -Wextra -Werror -Wno-missing-field-initializers -O1 -I../arm9/source -I.
CORE     := $(wildcard ../arm9/source/core/*.cpp ../arm9/source/gfx/*.cpp ../arm9/source/ui/*.cpp)
TESTS    := $(wildcard test_*.cpp)
BIN      := build/run_tests

.PHONY: all clean

all: $(BIN)
	./$(BIN)

$(BIN): main.cpp $(TESTS) $(CORE) $(wildcard ../arm9/source/core/*.h ../arm9/source/gfx/*.h ../arm9/source/ui/*.h)
	@mkdir -p build
	$(CXX) $(CXXFLAGS) -o $@ main.cpp $(TESTS) $(CORE)

clean:
	rm -rf build

# Host preview of the UI rendered from real ROM folders (see preview.cpp); not part of the test run.
PLATFORM := ../arm9/source/platform/FileIo.cpp ../arm9/source/platform/RomFiles.cpp
LODEPNG  := ../third_party/twilight/universal/source/lodepng/lodepng.cpp

.PHONY: preview
preview: build/preview

build/preview: preview.cpp build/lodepng.o $(CORE) $(PLATFORM) $(wildcard ../arm9/source/*/*.h)
	$(CXX) $(CXXFLAGS) -I../third_party/twilight/universal/include -o $@ preview.cpp $(CORE) $(PLATFORM) build/lodepng.o

# Third-party code: built without this project's warning flags.
build/lodepng.o: $(LODEPNG)
	@mkdir -p build
	$(CXX) -std=gnu++17 -O1 -I../third_party/twilight/universal/include -c -o $@ $<
