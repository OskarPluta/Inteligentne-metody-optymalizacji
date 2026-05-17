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
       src/algorithms/regret.cpp \
       src/algorithms/local_search.cpp \
       src/algorithms/local_search_lm.cpp \
       src/algorithms/local_search_cm.cpp \
       src/algorithms/msls.cpp \
       src/algorithms/ils.cpp \
       src/algorithms/lns.cpp \
       src/algorithms/random_walk.cpp

TARGET = tsp

Z5_SRCS = zadanie5.cpp \
          src/instance.cpp \
          src/solution.cpp \
          src/algorithms/random_solution.cpp \
          src/algorithms/local_search.cpp

Z5_TARGET = zadanie5

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRCS)

$(Z5_TARGET): $(Z5_SRCS)
	$(CXX) $(CXXFLAGS) -o $(Z5_TARGET) $(Z5_SRCS)

clean:
	rm -f $(TARGET) $(Z5_TARGET)
