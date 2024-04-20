#ifndef CPU_H
#define CPU_H

#include "bytecode.h"
#include <stdbool.h>
#include <stdlib.h>

#define STACK_SIZE 2048

typedef enum { SUCCESS, E_DIV_BY_ZERO, E_UNKNOWN_INSTRUCTION } Exec_Result;

typedef enum { AX, BX, CX, DX, NUM_REGISTERS } Register;

typedef struct cpu {
    qword *bcode;
    qword *memory;
    qword stack[STACK_SIZE];
    // Registers
    qword pc;
    qword *sp;
    qword r[NUM_REGISTERS];
    // Flags
    int64_t flags[3];
    bool run;
} Cpu;

Cpu *cpu_create(const Byte_Code *bc, size_t memory_size);

void cpu_free(Cpu *cpu);

void cpu_reset(Cpu *cpu, qword *code);

Exec_Result cpu_run(Cpu *cpu);

#endif // CPU_H
