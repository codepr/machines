#ifndef VM_H
#define VM_H

#include "bytecode.h"
#include <stdbool.h>
#include <stdlib.h>

#define STACK_SIZE 2048

typedef enum { SUCCESS, E_DIV_BY_ZERO, E_UNKNOWN_INSTRUCTION } Exec_Result;

typedef struct vm_s {
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
} VM;

VM *vm_create(const Byte_Code *bc, size_t memory_size);

void vm_free(VM *vm);

void vm_reset(VM *vm, qword *code, hword *data, size_t len);

Exec_Result vm_run(VM *vm);

void vm_print_registers(const VM *const vm);

#endif // VM_H
