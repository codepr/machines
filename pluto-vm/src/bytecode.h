#ifndef BYTECODE_H
#define BYTECODE_H

#include "data.h"
#include <stddef.h>
#include <stdint.h>

// Generic quadword segment for storing the code, it's meant to be compatible
// with the basic dynamic array managing macros da_init / da_push
typedef struct qword_segment {
    qword *data;
    size_t length;
    size_t capacity;
} Code_Segment;

// Data segment using halfwords, it's meant to be compatible with the
// basic dynamic array managing macros da_init / da_push
typedef struct hword_segment {
    hword *data;
    size_t length;
    size_t capacity;
} Data_Segment;

// Structure representing the bytecode program, which consists of a code segment
// and a data segment. It also keeps track of the base data address.
// The current logic is pretty dumb but simple, the `data_addr` is initialised
// to DATA_OFFSET and the the actual data storage starts at DATA_OFFSET * 2.
// The space between DATA_OFFSET and DATA_OFFSET * 2 is used to address the
// start of each piece of data stored to easily pass pointers around.
typedef struct bytecode {
    Code_Segment *code_segment;
    Data_Segment *data_segment;
    // Base address for the data segment in memory
    qword data_addr;
    // Main entry point address
    qword entrypoint;
} Byte_Code;

// Enum representing the instruction set for the virtual machine or processor.
// Each value corresponds to a specific instruction opcode.
typedef enum {
    OP_NOP,          // No operation
    OP_CLF,          // Clear flags
    OP_CMP,          // Compare two registers
    OP_MOV,          // Move data from one register to another
    OP_PSH,          // Push value to stack
    OP_POP,          // Pop value from stack into register
    OP_ADD,          // Add two registers
    OP_SUB,          // Subtract two registers
    OP_MUL,          // Multiply two registers
    OP_DIV,          // Divide two registers
    OP_MOD,          // Modulo operation on two registers
    OP_INC,          // Increment register
    OP_DEC,          // Decrement register
    OP_AND,          // Bitwise AND between registers
    OP_BOR,          // Bitwise OR between registers
    OP_XOR,          // Bitwise XOR between registers
    OP_NOT,          // Bitwise NOT of a register
    OP_SHL,          // Shift left
    OP_SHR,          // Shift right
    OP_JMP,          // Jump to address
    OP_JEQ,          // Jump if equal
    OP_JNE,          // Jump if not equal
    OP_JLE,          // Jump if less than or equal
    OP_JLT,          // Jump if less than
    OP_JGE,          // Jump if greater than or equal
    OP_JGT,          // Jump if greater than
    OP_CALL,         // Call a subroutine
    OP_RET,          // Return from subroutine
    OP_SYSCALL,      // System call
    OP_HLT,          // Halt the execution
    NUM_INSTRUCTIONS // Total number of instructions
} Instruction_Set;

// Enum representing the general-purpose registers available in the virtual
// machine.
typedef enum { R_AX, R_BX, R_CX, R_DX, NUM_REGISTERS } Register;

typedef enum { D_DB, D_DW, D_DD, D_DQ, NUM_DIRECTIVES } Directive;

Byte_Code *bc_create(void);

// Testing heleper, wrap a raw bytecode into a proper bytecode struct
Byte_Code *bc_from_raw(const qword *bytecode, size_t length);

// Generate bytecode from a literal string or an already slurped from disk one
Byte_Code *bc_from_source(const char *source);

// Generate bytecode from a file in the filesystem by slurping as a whole in
// memory first
Byte_Code *bc_slurp(const char *path);

// Generate bytecode by reading a file from the filesystem line by line and
// streaming it into the lexer
Byte_Code *bc_load(const char *path);

// Simple debug disassemble function to write in human readable characters a
// bytecode
void bc_disassemble(const Byte_Code *const bc);

qword *bc_code(const Byte_Code *const bc);

hword *bc_data(const Byte_Code *const bc);

qword bc_data_addr(const Byte_Code *const bc);

qword bc_encode_instruction(struct instruction_line *i);

struct instruction_line bc_decode_instruction(qword einstr);

void bc_push_instruction(Byte_Code *bc, struct instruction_line *i);

void bc_free(Byte_Code *bc);

#endif // BYTECODE_H
