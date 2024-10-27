#ifndef BYTECODE_H
#define BYTECODE_H

#include <stdint.h>
#include <stdlib.h>

typedef uint64_t Word;
typedef enum {
    OP_LOAD,
    OP_LOADI,
    OP_STORE,
    OP_STOREI,
    OP_CALL,
    OP_PUSH,
    OP_PUSHI,
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_DUP,
    OP_INC,
    OP_EQ,
    OP_JMP,
    OP_JEQ,
    OP_JNE,
    OP_PRINT,
    OP_RET,
    OP_HALT
} Instruction;

typedef struct code Code;
typedef struct labels Labels;

typedef struct bytecode {
    Code *code;
    Code *data;
    Labels *labels;
} Byte_Code;

Byte_Code *bc_create(void);

void bc_free(Byte_Code *bc);

Word *bc_code(const Byte_Code *bc);

Word bc_constant(const Byte_Code *bc, uint64_t addr);

int bc_push_unary_instr(Byte_Code *bc, const char *instr);

int bc_push_binary_instr(Byte_Code *bc, const char *instr, const char *arg);

int bc_push_constant(Byte_Code *bc, const char *addr, const char *value);

int bc_push_label(Byte_Code *bc, const char *label);

void bc_disassemble(const Byte_Code *bc);

Byte_Code *bc_load(const char *path);

#endif // BYECODE_H
