CC=gcc
CFLAGS=-Wall -Wextra -Werror
DEPS = cpu.h memory.h
OBJ = main.o cpu.o memory.o

build/main: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

run: build/main
	build/main	

.PHONY: clean

clean:
	rm -f *.o build/main

