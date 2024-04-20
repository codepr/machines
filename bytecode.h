#ifndef BYTECODE_H
#define BYTECODE_H

#include <stddef.h>
#include <stdint.h>

#define DEST_MASK 0x000FF00000000000
#define ADDR_MASK 0x00000FFFFFFFFFFF

typedef uint8_t hword;
typedef uint64_t qword;
typedef struct code Code;
typedef struct bytecode {
    Code *code;
} Byte_Code;

typedef enum {
    NOP,
    MOV,
    LDI,
    LDR,
    STI,
    STR,
    PSR,
    PSM,
    PSI,
    POP,
    POM,
    ADD,
    ADI,
    SUB,
    SBI,
    MUL,
    MLI,
    DIV,
    DVI,
    MOD,
    MDI,
    INC,
    DEC,
    AND,
    BOR,
    XOR,
    NOT,
    SHL,
    SHR,
    JMP,
    JEQ,
    JNE,
    CALL,
    RET,
    HLT,
    INSTRUCTION_NUM
} Instruction_Set;

struct instruction {
    hword op;
    qword src, dst;
};

Byte_Code *bc_create(qword *bytecode, size_t length);

Byte_Code *bc_load(const char *path);

qword *bc_code(const Byte_Code *const bc);

qword bc_encode_instruction(qword opcode, qword dst, qword src);

struct instruction bc_decode_instruction(qword einstr);

void bc_free(Byte_Code *bc);

#endif // BYTECODE_H
