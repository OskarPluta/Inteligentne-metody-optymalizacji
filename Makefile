CXX      = g++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra

SRCS = main.cpp \
       src/instance.cpp \
       src/solution.cpp \
       src/experiment.cpp \
       src/algorithms/random_solution.cpp \
       src/algorithms/phase2.cpp \
       src/algorithms/nn.cpp \
       src/algorithms/gc.cpp \
       src/algorithms/regret.cpp

TARGET = tsp

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRCS)

clean:
	rm -f $(TARGET)
