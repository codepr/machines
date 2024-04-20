CC=gcc
CFLAGS=-Wall -Werror -pedantic -ggdb -std=c11 -fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer -pg

cpu: main.c
	$(CC) $(CFLAGS) -o T800-64 main.c cpu.c bytecode.c

clean:
	@rm -rf *.o
	@rm -rf T800-64
