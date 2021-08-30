BINARIES = screen-worms-server
CC = gcc
CPP = g++
FLAGS = -Wall -Wextra -O2 -g
OBJ = screen-worms-server.o err.o config.o crc32.o generator.o
ODIR = obj

.PHONY: all clean

all: $(BINARIES)

err.o: err.c err.h
	$(CC) $(FLAGS) -std=c11 -c $< -o $@

config.o: config.cpp config.h consts.h err.h
	$(CPP) $(FLAGS) -std=c++17 -c $< -o $@

crc32.o: crc32.cpp crc32.h
	$(CPP) $(FLAGS) -std=c++17 -c $< -o $@

generator.o: generator.cpp generator.h
	$(CPP) $(FLAGS) -std=c++17 -c $< -o $@

screen-worms-server.o: server.cpp server.h config.h consts.h crc32.h err.h
	$(CPP) $(FLAGS) -std=c++17 -c $< -o $@

screen-worms-server: $(OBJ)
	$(CPP) $^ -o $@

clean:
	rm -f $(BINARIES) *.o
