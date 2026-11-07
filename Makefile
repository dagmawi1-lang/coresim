CXX ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -Wpedantic -Werror
SRC = src/isa.cpp src/cpu.cpp src/assembler.cpp src/view.cpp
.PHONY: all test clean demo
all: coresim
coresim: $(SRC) src/main.cpp src/*.hpp
	$(CXX) $(CXXFLAGS) -o $@ $(SRC) src/main.cpp
coresim_tests: $(SRC) tests/test_main.cpp src/*.hpp
	$(CXX) $(CXXFLAGS) -DPROGRAMS_DIR='"$(CURDIR)/programs"' -o $@ $(SRC) tests/test_main.cpp
test: coresim_tests
	./coresim_tests
demo: coresim
	./coresim run programs/factorial.asm --html factorial_trace.html
clean:
	rm -f coresim coresim_tests *_trace.html
