#include "cpu.h"
#include <stdio.h>
#include <string.h>

static void reset(Cpu *cpu, qword *code)
{
    memset(cpu->r, 0x00, NUM_REGISTERS * sizeof(*cpu->r));
    memset(cpu->stack, 0x00, STACK_SIZE * sizeof(*cpu->stack));
    cpu->bcode = code;
    cpu->pc = 0;
    cpu->sp = cpu->stack;
    cpu->run = false;
}

static qword fetch(Cpu *cpu) { return cpu->bcode[cpu->pc++]; }

static struct instruction decode(qword einstr)
{
    return bc_decode_instruction(einstr);
}

static void clear_flags(Cpu *cpu)
{
    cpu->flags[0] = 0;
    cpu->flags[1] = 0;
    cpu->flags[2] = 0;
}

static void set_flags(Cpu *cpu, qword a, qword b)
{
    qword result = a - b;
    cpu->flags[0] = (result == 0);
    cpu->flags[1] = (result < 0);
    cpu->flags[2] = (result > 0);
}

static Exec_Result execute(Cpu *cpu, struct instruction *instr)
{
    switch (instr->op) {
    case NOP: {
        break;
    }
    case MOV: {
        cpu->r[instr->dst] = cpu->r[instr->src];
        break;
    }
    case LDI: {
        cpu->r[instr->dst] = instr->src;
        break;
    }
    case LDR: {
        cpu->r[instr->dst] = cpu->memory[instr->src];
        break;
    }
    case STI: {
        cpu->memory[instr->dst] = instr->src;
        break;
    }
    case STR: {
        cpu->memory[instr->dst] = cpu->r[instr->src];
        break;
    }
    case PSI: {
        *cpu->sp++ = instr->src;
        break;
    }
    case PSR: {
        *cpu->sp++ = cpu->r[instr->dst];
        break;
    }
    case PSM: {
        *cpu->sp++ = cpu->memory[instr->src];
        break;
    }
    case POP: {
        cpu->r[instr->dst] = *cpu->sp--;
        break;
    }
    case POM: {
        cpu->memory[instr->src] = *cpu->sp--;
        break;
    }
    case ADD: {
        cpu->r[instr->dst] += cpu->r[instr->src];
        break;
    }
    case ADI: {
        cpu->r[instr->dst] += instr->src;
        break;
    }
    case SUB: {
        cpu->r[instr->dst] -= cpu->r[instr->src];
        break;
    }
    case SBI: {
        cpu->r[instr->dst] -= instr->src;
        break;
    }
    case MUL: {
        cpu->r[instr->dst] *= cpu->r[instr->src];
        break;
    }
    case MLI: {
        cpu->r[instr->dst] *= instr->src;
        break;
    }
    case DIV: {
        if (cpu->r[instr->src] == 0)
            return E_DIV_BY_ZERO;

        cpu->r[instr->dst] /= cpu->r[instr->src];
        break;
    }
    case DVI: {
        if (instr->src == 0)
            return E_DIV_BY_ZERO;

        cpu->r[instr->dst] /= instr->src;
        break;
    }
    case MOD: {
        cpu->r[instr->dst] %= cpu->r[instr->src];
        break;
    }
    case MDI: {
        cpu->r[instr->dst] %= instr->src;
        break;
    }
    case INC: {
        cpu->r[instr->dst]++;
        break;
    }
    case DEC: {
        cpu->r[instr->dst]--;
        break;
    }
    case CLF: {
        clear_flags(cpu);
        break;
    }
    case CMP: {
        set_flags(cpu, cpu->r[instr->dst], cpu->r[instr->src]);
        break;
    }
    case CMI: {
        set_flags(cpu, cpu->r[instr->dst], instr->src);
        break;
    }
    case JMP: {
        cpu->pc = instr->dst;
        break;
    }
    case JEQ: {
        if (cpu->flags[0])
            cpu->pc = instr->dst;
        break;
    }
    case JNE: {
        if (!cpu->flags[0])
            cpu->pc = instr->dst;
        break;
    }
    case JLE: {
        if (cpu->flags[0] || cpu->flags[1])
            cpu->pc = instr->dst;
        break;
    }
    case JLT: {
        if (!cpu->flags[0] && cpu->flags[1])
            cpu->pc = instr->dst;
        break;
    }
    case JGE: {
        if (cpu->flags[0] || cpu->flags[2])
            cpu->pc = instr->dst;
        break;
    }
    case JGT: {
        if (!cpu->flags[0] && cpu->flags[2])
            cpu->pc = instr->dst;
        break;
    }
    case AND: {
        cpu->r[instr->dst] &= cpu->r[instr->src];
        break;
    }
    case BOR: {
        cpu->r[instr->dst] |= cpu->r[instr->src];
        break;
    }
    case XOR: {
        cpu->r[instr->dst] ^= cpu->r[instr->src];
        break;
    }
    case NOT: {
        cpu->r[instr->dst] = -cpu->r[instr->src];
        break;
    }
    case SHR: {
        cpu->r[instr->dst] >>= cpu->r[instr->src];
        break;
    }
    case SHL: {
        cpu->r[instr->dst] <<= cpu->r[instr->src];
        break;
    }
    case CALL: {
        *cpu->sp++ = (cpu->pc - 1);
        cpu->pc = instr->dst;
        break;
    }
    case RET: {
        cpu->pc = *--cpu->sp;
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

void cpu_reset(Cpu *cpu, qword *code) { reset(cpu, code); }

Exec_Result cpu_run(Cpu *cpu)
{
    Exec_Result r = SUCCESS;
    cpu->run = true;
    while (cpu->run && r == SUCCESS) {
        qword encoded_instr = fetch(cpu);
        struct instruction instr = decode(encoded_instr);
        r = execute(cpu, &instr);
    }

    return r;
}

void cpu_print_registers(const Cpu *const cpu)
{
    printf("AX: %lu BX: %lu CX: %lu DX: %lu\n", cpu->r[AX], cpu->r[BX],
           cpu->r[CX], cpu->r[DX]);
}
