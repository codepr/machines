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
} Byte_Code;

// Enum representing the instruction set for the virtual machine or processor.
// Each value corresponds to a specific instruction opcode.
typedef enum {
    NOP,             // No operation
    CLF,             // Clear flags
    CMP,             // Compare two registers
    CMI,             // Compare immediate value with a register
    MOV,             // Move data from one register to another
    LDI,             // Load immediate value into register
    LDR,             // Load value from memory to register
    STI,             // Store immediate value in memory
    STR,             // Store register value in memory
    PSR,             // Push register value to stack
    PSM,             // Push memory value to stack
    PSI,             // Push immediate value to stack
    POP,             // Pop value from stack into register
    POM,             // Pop value from stack into memory
    ADD,             // Add two registers
    ADI,             // Add immediate value to register
    SUB,             // Subtract two registers
    SBI,             // Subtract immediate value from register
    MUL,             // Multiply two registers
    MLI,             // Multiply register by immediate value
    DIV,             // Divide two registers
    DVI,             // Divide register by immediate value
    MOD,             // Modulo operation on two registers
    MDI,             // Modulo operation on register and immediate value
    INC,             // Increment register
    DEC,             // Decrement register
    AND,             // Bitwise AND between registers
    BOR,             // Bitwise OR between registers
    XOR,             // Bitwise XOR between registers
    NOT,             // Bitwise NOT of a register
    SHL,             // Shift left
    SHR,             // Shift right
    JMP,             // Jump to address
    JEQ,             // Jump if equal
    JNE,             // Jump if not equal
    JLE,             // Jump if less than or equal
    JLT,             // Jump if less than
    JGE,             // Jump if greater than or equal
    JGT,             // Jump if greater than
    CALL,            // Call a subroutine
    RET,             // Return from subroutine
    SYSCALL,         // System call
    HLT,             // Halt the execution
    NUM_INSTRUCTIONS // Total number of instructions
} Instruction_Set;

// Enum representing the general-purpose registers available in the virtual
// machine.
typedef enum { AX, BX, CX, DX, NUM_REGISTERS } Register;

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

void bc_free(Byte_Code *bc);

#endif // BYTECODE_H
