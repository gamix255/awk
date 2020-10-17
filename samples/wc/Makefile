# default architecture
ifndef ARCH
ARCH = x64
endif

# compiler(s)
CXX := g++
CPPFLAGS := -std=c++11 -I ../../include/ -Wall -pedantic  -O3 -DNDEBUG -c

LIBDIR := ../../lib/x64/Release/

AWKLIB = $(LIBDIR)/libawk.a


all: wc

wc: wc.o $(AWKLIB)
	$(CXX)  -o $@ $^
#$(CXX)  -o $@ wc.o $(AWKLIB)

wc.o: wc.cpp
	$(CXX) $(CPPFLAGS) -o $@ $<  
