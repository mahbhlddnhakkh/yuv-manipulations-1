CXX = g++

CXXFLAGS = -Wall -g -O3 -std=c++17

LDFLAGS = -lSDL3

TARGET = YUVExp

SRCS = main.cpp MyYUV.cpp YUVCompressor.cpp YUVConverter.cpp

OBJS = $(SRCS:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

#run: $(TARGET)
#	./$(TARGET)

clean:
	rm -f $(TARGET) $(OBJS)
