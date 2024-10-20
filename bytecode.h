#ifndef BYTECODE_H
#define BYTECODE_H

#include "data.h"
#include <stddef.h>
#include <stdint.h>

// Generic quadword segment for storing the code
typedef struct qword_segment {
    qword *data;
    size_t length;
    size_t capacity;
} Code_Segment;

// Data segment using halfwords
typedef struct hword_segment {
    hword *data;
    size_t length;
    size_t capacity;
} Data_Segment;

typedef struct bytecode {
    Code_Segment *code_segment;
    Data_Segment *data_segment;
    qword data_addr;
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
    SYSCALL,
    HLT,
    NUM_INSTRUCTIONS
} Instruction_Set;

typedef enum { AX, BX, CX, DX, NUM_REGISTERS } Register;

Byte_Code *bc_create(void);

Byte_Code *bc_from_raw(const qword *bytecode, size_t length);

Byte_Code *bc_from_source(const char *source);

Byte_Code *bc_load(const char *path);

void bc_disassemble(const Byte_Code *const bc);

qword *bc_code(const Byte_Code *const bc);

hword *bc_data(const Byte_Code *const bc);

qword bc_data_addr(const Byte_Code *const bc);

qword bc_encode_instruction(struct instruction_line *i);

struct instruction_line bc_decode_instruction(qword einstr);

void bc_free(Byte_Code *bc);

#endif // BYTECODE_H
