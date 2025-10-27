CC=gcc
CFLAGS=-Wall -Werror -pedantic -ggdb -std=c11 -fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer -pg

SRC = src/vm.c src/bytecode.c src/assembler.c src/parser.c
OBJ = $(SRC:.c=.o)
EXEC = atom-vm

ASM_SRC = src/aasm.c src/bytecode.c src/assembler.c src/parser.c
ASM_OBJ = $(ASM_SRC:.c=.o)
ASM_EXEC = aasm

all: $(EXEC) $(ASM_EXEC)

$(EXEC): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

$(ASM_EXEC): $(ASM_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(EXEC)
	rm -f $(ASM_OBJ) ($(ASM_EXEC)

.PHONY: all clean
