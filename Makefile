CXX=g++
CXXFLAGS=-std=c++14 -Wall -pedantic -pthread -lboost_system -DBOOST_BIND_GLOBAL_PLACEHOLDERS
CXX_INCLUDE_DIRS=/usr/local/include
CXX_INCLUDE_PARAMS=$(addprefix -I , $(CXX_INCLUDE_DIRS))
CXX_LIB_DIRS=/usr/local/lib
CXX_LIB_PARAMS=$(addprefix -L , $(CXX_LIB_DIRS))

all: 
	$(CXX) socks_server2.cpp -o socks_server $(CXX_INCLUDE_PARAMS) $(CXX_LIB_PARAMS) $(CXXFLAGS)
	$(CXX) console2.cpp -o hw4.cgi $(CXX_INCLUDE_PARAMS) $(CXX_LIB_PARAMS) $(CXXFLAGS)