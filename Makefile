CXX = g++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra
TARGET = tsp

all: $(TARGET)

$(TARGET): main.cpp include/*.hpp
	$(CXX) $(CXXFLAGS) -o $(TARGET) main.cpp

clean:
	rm -f $(TARGET) *.svg *.txt

run: $(TARGET)
	./$(TARGET) TSPA.csv TSPB.csv 200

.PHONY: all clean run
