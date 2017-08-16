INC=-I../common/ -I/usr/include/SDL2
CPPFLAGS=-g -std=c++11 $(INC)
LDFLAGS=-std=c++11 -o
LDLIBS=-lSDL2

all: main.o
		g++ $(LDFLAGS) client main.o $(LDLIBS)

main.o: main.cpp
		g++ $(CPPFLAGS) -c main.cpp
