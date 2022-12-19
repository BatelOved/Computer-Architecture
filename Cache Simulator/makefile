# 046267 Computer Architecture - Winter 20/21 - HW #2

# Environment for C++ 
CXX = g++
CXXFLAGS = -std=c++11 -Wall -g
OBJ = cacheSimulator.o cacheSim.o


cacheSim: $(OBJ)
	$(CXX) -o $@ $(OBJ)

cacheSimulator.o: cacheSimulator.cpp
	$(CXX) -c $(CXXFLAGS) -o $@ $^ -lm



.PHONY: clean
clean:
	rm -f *.o
	rm -f cacheSim
