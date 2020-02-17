CC=gcc
CFLAGS=-Wall -Wextra -Werror
DEPS = cpu.h memory.h
OBJ = main.o cpu.o memory.o

bin/linux/main: $(OBJ)
	mkdir -p bin/linux
	$(CC) -o $@ $^ $(CFLAGS)

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

run: bin/linux/main
	$< $(rom)

.PHONY: clean

clean:
	rm -f *.o bin/linux/main

