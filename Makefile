SOURCES=httpserver.cpp
INCLUDES=$(wildcard*.h)
TARGET=server
_submit_CXXFLAGS=-std=gnu++11 -Wall -Wextra -Wpedantic -Wshadow -g -O2
LDFLAGS=
OBJECTS=$(SOURCES:.cpp=.o)
DEPS=$(SOURCES:.cpp=.d)
CXX=clang++

all: $(TARGET)


clean:
	-rm $(DEPS) $(OBJECTS)

spotless: clean
	-rm $(TARGET)

format:
	clang-format -i $(SOURCES) $(INCLUDES)

$(TARGET): $(OBJECTS)
	$(CXX) $(LDFLAGS) -o $@ $(OBJECTS)
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -MD -o $@ $<

-include $(DEPS)
.PHONY: all clean format spotless

