PROGS=main

OBJS=$(PROGS:.cpp=.o)

CXX=g++
CXXFLAGS=-Wall -Wunused -Wextra # -Wshadow -Weffc++

.cpp.o: Makefile
	$(CC) -o $@ $<

all: $(OBJS)

clean:
	rm $(PROGS)
