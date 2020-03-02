CC=gcc
CFLAGS=-Wall -Wextra -Werror -Iinclude/
DEPS=cpu.h memory.h graphics.h
OBJ=main.o cpu.o memory.o graphics.o ops.o

bin/linux/main: $(OBJ)
	mkdir -p bin/linux
	$(CC) -o $@ $^ $(CFLAGS) `sdl2-config --cflags --libs`

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

run: bin/linux/main
	$< $(rom)

.PHONY: clean

clean:
	rm -f *.o *.obj; \
    rm -rf bin

