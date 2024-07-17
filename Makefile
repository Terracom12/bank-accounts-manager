# Loosely based on the final program in https://makefiletutorial.com/

# Run with:
#     debug=true
# to enable sanitizers and assertion macros

################################ VARIABLES ################################

# Compiler flags:
#   Warnings
#     -Wall  :  All warnings deemed sensible by gcc / clang
#     -Wextra : A few more warnings (unused parameters, fundamental type limits, missing field initializers, etc.)
#     -Wshadow  : Warn when an identifer is shadowed
#     -Wconversion  : Implicit conversion warnings
#     -Wpedantic : Warn on code that is non-conformant to ISO C++ (VLA's, reserved identifer names, etc.)
CXXFLAGS := -Wall -Wextra -Wshadow -Wconversion -Wpedantic
# Don't interpret warnings as errors for now
### CXXFLAGS += -Werror

#  Sanitizers (supported by gcc and clang)
#     address : More primitive version of valgrind (nullptr dereference, new/malloc with no delete/free, etc.)
#     undefined : Catch UB (signed integer overflow, non-void function has no return, etc.)
ifeq ($(strip $(debug)), true)
CXXFLAGS += -fsanitize=address,undefined
endif

# Debug symbols
#     -g  : Enable debug symbols
#     -O0  :  Disable optimizations as to not ommit important symbols
#     -DNDEBUG  :  Define the `NDEBUG` macro, disabling `assert` statements
ifeq ($(strip $(debug)), true)
CXXFLAGS += -g -O0
else
CXXFLAGS += -DNDEBUG
endif

# Unknown system compatibility, so setting to very old standard for now
CXXFLAGS += -std=c++20

# Unused for now:
#
# Compiler
CXX := g++
# Preprocessor flags
### CPPFLAGS :=
# Linker flags
### LDFLAGS :=

# Executable name
EXE := out
# Test Names
TESTS := test_example
# List of source files
SRCS := src/main.cpp
# Object files to be created in `./build`. Correspond 1-to-1 with sources, in that `myFile.cpp` => `build/myFile.o`
OBJS := $(SRCS:src/%.cpp=build/%.o)

################################ PHONY RULES ################################


.PHONY: all
all: build/$(EXE)

.PHONY: run
run: build/$(EXE)
	@./$<

.PHONY: test
test: $(TESTS:%=build/%)
	@for exe in $^; do ./$$exe; done

.PHONY: clean
clean:
	rm -rf build compile_commands.json

.PHONY: tar
tar: $(EXE).scr
	tar cf $(EXE).tar src/** Makefile

.PHONY: compile_commands
compile_commands:
	make clean
	bear -- make all


############################ GENERIC BUILDING RULES ############################


build/$(EXE): $(OBJS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $^ -o $@

build/%.o: src/%.cpp
	@mkdir -p build
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $< -o $@


# TODO: Warn if gtest is not installed
build/%: test/%.cpp
	@mkdir -p build
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(LDFLAGS) -lgtest -lgtest_main $< -o $@