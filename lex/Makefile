.PHONY: all test

all:

BIN=lua-exporter
WARNINGS = -Wall -Wextra -g
OTHERS += -std=c++11 -I /usr/src/llvm/llvm-6.0.0.src/tools/clang/include
DEBUG =
LDFLAGS = -L . -lclang
LD = g++ -std=c++11

CXXFLAGS += $(WARNINGS) $(DEBUG) $(OTHERS) -MMD -MP

# Enumerations of all the source files and headers.
SOURCES = $(wildcard *.cpp)
HEADERS = $(wildcard *.h)

_OBJS = $(SOURCES:%.cpp=%.o)
OBJS = $(patsubst %,%,$(_OBJS))


$(BIN): $(OBJS)
	+$(LD) -o $(BIN) $(OBJS) $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CPPFLAGS) $(DEFINES) $(CXXFLAGS) -c $< -o $@

all: ${BIN}
	
astyle:
	astyle --options=../.astylerc -n $(SOURCES) $(HEADERS)

test: ${BIN}
	gdb -q --args ./${BIN} ../src/ammo.h

-include ${OBJS:.o=.d}
