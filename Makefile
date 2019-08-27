CXXFLAGS = -std=c++17 -O -Wall -Wextra
routecount: routecount.o
	$(CXX) -o $@ $<
