CC=gcc
CFLAGS=-Wall -Werror -pedantic -ggdb -std=c11 -fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer -pg

SRC = src/main.c src/vm.c src/bytecode.c src/syscall.c src/lexer.c src/parser.c src/data.c
OBJ = $(SRC:.c=.o)
EXEC = pluto-vm

TEST_SRC = tests/tests.c src/vm.c src/bytecode.c src/syscall.c src/lexer.c src/parser.c src/data.c
TEST_OBJ = $(TEST_SRC:.c=.o)
TEST_EXEC = pluto-vm-tests

all: $(EXEC) $(TEST_EXEC)

$(EXEC): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

$(TEST_EXEC): $(TEST_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(EXEC)

.PHONY: all clean
