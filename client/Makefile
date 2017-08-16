INC=-I../common/ -I/usr/include/SDL2
CPPFLAGS=-g -std=c++14 $(INC)
LDFLAGS=-std=c++14 -o
LDLIBS=-lSDL2 -lpthread

all: main.o
	g++ $(LDFLAGS) client main.o $(LDLIBS)

main.o: main.cpp
	g++ $(CPPFLAGS) -c main.cpp

clean:
	rm -f main.o
