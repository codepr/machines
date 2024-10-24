CC=gcc
CFLAGS=-Wall -Werror -pedantic -ggdb -std=c11 -fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer -pg

vm: main.c
	$(CC) $(CFLAGS) -o T800-64 main.c vm.c bytecode.c syscall.c lexer.c parser.c data.c

test: tests.c
	$(CC) $(CFLAGS) -o tests tests.c vm.c bytecode.c syscall.c lexer.c parser.c data.c

all: test vm

run: vm
	./T800-64

clean:
	@rm -rf *.o
	@rm -rf T800-64
	@rm -rf tests
