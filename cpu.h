#ifndef CPU_H
#define CPU_H

#include "bytecode.h"
#include <stdbool.h>
#include <stdlib.h>

typedef enum { AX, BX, CX, DX, NUM_REGISTERS } Register;

typedef struct cpu {
    qword *bcode;
    qword *memory;
    qword *stack;
    // Registers
    uint64_t *pc;
    uint64_t *sp;
    qword r[NUM_REGISTERS];
    // Flags
    uint64_t flags[3];
    bool run;
} Cpu;

Cpu *cpu_create(const Byte_Code *bc, size_t memory_size);

void cpu_free(Cpu *cpu);

void cpu_reset(Cpu *cpu, qword *code);

void cpu_run(Cpu *cpu);

#endif // CPU_H
