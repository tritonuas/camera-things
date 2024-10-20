# Compiler
CXX = g++

# Compiler flags
CXXFLAGS = -Wall -Wextra -std=c++17

# Include path for libcamera
INCLUDES = -I/usr/include/libcamera/

# Linker flags for libcamera
LIBS = -lcamera

# Target executable
TARGET = camera

# Source files
SRCS = main.cpp

# Object files
OBJS = $(SRCS:.cpp=.o)

# Default target
all: $(TARGET)

# Link object files to create the executable
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(TARGET) $(LIBS)

# Compile .cpp files to .o files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Clean up build files
clean:
	rm -f $(OBJS) $(TARGET)

# Phony targets
.PHONY: all clean
