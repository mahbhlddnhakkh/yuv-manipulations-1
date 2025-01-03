CXX = g++

CXXFLAGS = -Wall -g -O3 -std=c++17 -fopenmp

LDFLAGS = -lSDL3 -lpthread

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
