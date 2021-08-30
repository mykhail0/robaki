CC=g++
CFLAGS=-Wall -Wextra -g -std=c++17
DEPS = iset.h
OBJ = iset_test.o iset.o

.PHONY: clean

%.o: %.cpp $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

iset_test: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	rm *.o iset_test
