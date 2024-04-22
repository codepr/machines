#ifndef BYTECODE_H
#define BYTECODE_H

#include <stddef.h>
#include <stdint.h>

#define DEST_MASK 0x000FF00000000000
#define ADDR_MASK 0x00000FFFFFFFFFFF

typedef uint8_t hword;
typedef int64_t qword;
typedef struct code Code;
typedef struct bytecode {
    Code *code;
} Byte_Code;

typedef enum {
    NOP,
    CLF,
    CMP,
    CMI,
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
    JLE,
    JLT,
    JGE,
    JGT,
    CALL,
    RET,
    HLT,
    NUM_INSTRUCTIONS
} Instruction_Set;

typedef enum { AX, BX, CX, DX, NUM_REGISTERS } Register;

struct instruction {
    hword op;
    qword src, dst;
};

Byte_Code *bc_create(void);

Byte_Code *bc_from_raw(qword *bytecode, size_t length);

Byte_Code *bc_load(const char *path);

void bc_disassemble(const Byte_Code *const bc);

qword *bc_code(const Byte_Code *const bc);

qword bc_encode_instruction(qword opcode, qword dst, qword src);

struct instruction bc_decode_instruction(qword einstr);

void bc_free(Byte_Code *bc);

#endif // BYTECODE_H
