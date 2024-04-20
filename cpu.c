#include "cpu.h"
#include <stdio.h>
#include <string.h>

struct instruction {
    hword op;
    qword src, dst;
};

typedef enum { SUCCESS, E_DIV_BY_ZERO, E_UNKNOWN_INSTRUCTION } Exec_Result;

static void reset(Cpu *cpu, qword *code)
{
    memset(cpu->r, 0x00, NUM_REGISTERS * sizeof(*cpu->r));
    cpu->bcode = code;
    cpu->pc = cpu->bcode;
    cpu->sp = cpu->stack;
    cpu->run = false;
}

static qword fetch(Cpu *cpu) { return *cpu->pc++; }

static struct instruction decode(Cpu *cpu, qword e_instr)
{
    hword op = (uint64_t)e_instr >> 56;
    qword dst = (e_instr & DEST_MASK) >> 48;
    qword src = e_instr & ADDR_MASK;

    return (struct instruction){.op = op, .src = src, .dst = dst};
}

static Exec_Result execute(Cpu *cpu, struct instruction *instr)
{
    switch (instr->op) {
    case ADD: {
        cpu->r[instr->dst] += cpu->r[instr->src];
        printf("ADD\n");
        break;
    }
    case ADI: {
        cpu->r[instr->dst] += instr->src;
        printf("ADI %ld\n", cpu->r[instr->dst]);
        break;
    }
    case HLT:
        cpu->run = false;
        break;
    default:
        return E_UNKNOWN_INSTRUCTION;
    }

    return SUCCESS;
}

Cpu *cpu_create(const Byte_Code *bc, size_t memory_size)
{
    Cpu *cpu = malloc(sizeof(*cpu));
    if (!cpu)
        return NULL;

    cpu->memory = malloc(memory_size);
    if (!cpu->memory) {
        free(cpu);
        return NULL;
    }

    reset(cpu, bc_code(bc));

    return cpu;
}

void cpu_free(Cpu *cpu)
{
    free(cpu->memory);
    free(cpu);
}

void cpu_run(Cpu *cpu)
{
    Exec_Result r = SUCCESS;
    cpu->run = true;
    while (cpu->run && r == SUCCESS) {
        qword encoded_instr = fetch(cpu);
        struct instruction instr = decode(cpu, encoded_instr);
        r = execute(cpu, &instr);
    }
}
