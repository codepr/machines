#include "vm.h"
#include "syscall.h"
#include <stdio.h>
#include <string.h>

static void reset(VM *vm, qword *code_segment, hword *data_segment,
                  size_t data_len)
{
    memset(vm->r, 0x00, NUM_REGISTERS * sizeof(*vm->r));
    memset(vm->stack, 0x00, STACK_SIZE * sizeof(*vm->stack));
    vm->bcode = code_segment;

    vm->pc    = 0;
    vm->sp    = vm->stack;
    vm->run   = false;

    if (!data_segment)
        return;

    memset(vm->memory, 0x00, 32768);
    // Addressing
    for (size_t i = DATA_OFFSET; i < DATA_OFFSET * 2; ++i)
        vm->memory[i] = i + DATA_OFFSET;
    // Storing
    for (size_t i = DATA_OFFSET * 2, j = 0; j < data_len - DATA_OFFSET;
         ++i, ++j)
        vm->memory[i] = data_segment[j];
}

static qword fetch(VM *vm) { return vm->bcode[vm->pc++]; }

static struct instruction_line decode(qword einstr)
{
    return bc_decode_instruction(einstr);
}

static void clear_flags(VM *vm)
{
    vm->flags[0] = 0;
    vm->flags[1] = 0;
    vm->flags[2] = 0;
}

static void set_flags(VM *vm, qword a, qword b)
{
    qword result = a - b;
    vm->flags[0] = (result == 0);
    vm->flags[1] = (result < 0);
    vm->flags[2] = (result > 0);
}

static qword *set_operands(VM *vm, const struct instruction_line *i, qword *src)
{
    *src = (i->sem & IS_SRC_REG)   ? vm->r[i->src]
           : (i->sem & IS_SRC_MEM) ? vm->memory[i->src]
                                   : i->src;

    return (i->sem & IS_DST_REG) ? &vm->r[i->dst] : &vm->memory[i->dst];
}

static Exec_Result execute(VM *vm, struct instruction_line *instr)
{
    switch (instr->op) {
    case NOP: {
        break;
    }
    case MOV: {
        qword src  = 0;
        qword *dst = set_operands(vm, instr, &src);

        *dst       = src;
        break;
    }
    case PSH: {
        if (instr->sem & IS_DST_REG)
            *vm->sp++ = vm->r[instr->dst];
        else if (instr->sem & IS_DST_MEM)
            *vm->sp++ = vm->memory[instr->dst];
        else if (instr->sem & IS_SRC_IMM)
            *vm->sp++ = instr->src;

        break;
    }
    case POP: {
        if (instr->sem & IS_DST_REG)
            vm->r[instr->dst] = *--vm->sp;
        else if (instr->sem & IS_DST_MEM)
            vm->memory[instr->dst] = *--vm->sp;
        break;
    }
    case ADD: {
        qword src  = 0;
        qword *dst = set_operands(vm, instr, &src);

        *dst += src;
        break;
    }
    case SUB: {
        qword src  = 0;
        qword *dst = set_operands(vm, instr, &src);

        *dst -= src;
        break;
    }
    case MUL: {
        qword src  = 0;
        qword *dst = set_operands(vm, instr, &src);

        *dst *= src;

        break;
    }
    case DIV: {
        qword src  = 0;
        qword *dst = set_operands(vm, instr, &src);

        if (src == 0)
            return E_DIV_BY_ZERO;

        *dst /= src;
        break;
    }
    case MOD: {
        qword src  = 0;
        qword *dst = set_operands(vm, instr, &src);

        *dst %= src;
        break;
    }
    case INC: {
        if (instr->sem & IS_SRC_MEM)
            vm->memory[instr->dst]++;
        else
            vm->r[instr->dst]++;
        break;
    }
    case DEC: {
        if (instr->sem & IS_SRC_MEM)
            vm->memory[instr->dst]++;
        else
            vm->r[instr->dst]--;
        break;
    }
    case CLF: {
        clear_flags(vm);
        break;
    }
    case CMP: {
        qword src  = 0;
        qword *dst = set_operands(vm, instr, &src);

        set_flags(vm, *dst, src);
        break;
    }
    case JMP: {
        vm->pc = instr->dst;
        break;
    }
    case JEQ: {
        if (vm->flags[0])
            vm->pc = instr->dst;
        break;
    }
    case JNE: {
        if (!vm->flags[0])
            vm->pc = instr->dst;
        break;
    }
    case JLE: {
        if (vm->flags[0] || vm->flags[1])
            vm->pc = instr->dst;
        break;
    }
    case JLT: {
        if (!vm->flags[0] && vm->flags[1])
            vm->pc = instr->dst;
        break;
    }
    case JGE: {
        if (vm->flags[0] || vm->flags[2])
            vm->pc = instr->dst;
        break;
    }
    case JGT: {
        if (!vm->flags[0] && vm->flags[2])
            vm->pc = instr->dst;
        break;
    }
    case AND: {
        vm->r[instr->dst] &= vm->r[instr->src];
        break;
    }
    case BOR: {
        vm->r[instr->dst] |= vm->r[instr->src];
        break;
    }
    case XOR: {
        vm->r[instr->dst] ^= vm->r[instr->src];
        break;
    }
    case NOT: {
        vm->r[instr->dst] = -vm->r[instr->src];
        break;
    }
    case SHR: {
        vm->r[instr->dst] >>= vm->r[instr->src];
        break;
    }
    case SHL: {
        vm->r[instr->dst] <<= vm->r[instr->src];
        break;
    }
    case CALL: {
        *vm->sp++ = (vm->pc - 1);
        vm->pc    = instr->dst;
        break;
    }
    case RET: {
        vm->pc = *--vm->sp;
        break;
    }
    case SYSCALL: {
        switch (vm->r[BX]) {
        // STDIN
        case 0:
            syscall_read(vm->r[BX], &vm->memory[vm->r[CX]],
                         vm->r[DX] * sizeof(qword));
            break;
        // STDOUT
        case 1:
            syscall_write(vm->r[BX], &vm->memory[vm->r[CX]],
                          vm->r[DX] * sizeof(qword));
            fflush(stdout);
            break;
        case 64:
            vm->r[AX] = syscall_atoi(&vm->memory[vm->r[CX]]);
            break;
        }
        break;
    }
    case HLT:
        vm->run = false;
        break;
    default:
        return E_UNKNOWN_INSTRUCTION;
    }

    return SUCCESS;
}

VM *vm_create(const Byte_Code *bc, size_t memory_size)
{
    VM *vm = malloc(sizeof(*vm));
    if (!vm)
        return NULL;

    vm->memory = malloc(memory_size * sizeof(qword));
    if (!vm->memory) {
        free(vm);
        return NULL;
    }

    reset(vm, bc_code(bc), bc_data(bc), bc_data_addr(bc));

    return vm;
}

void vm_free(VM *vm)
{
    free(vm->memory);
    free(vm);
}

void vm_reset(VM *vm, qword *code, hword *data, size_t data_len)
{
    reset(vm, code, data, data_len);
}

Exec_Result vm_run(VM *vm)
{
    Exec_Result r = SUCCESS;
    vm->run       = true;
    while (vm->run && r == SUCCESS) {
        qword encoded_instr           = fetch(vm);
        struct instruction_line instr = decode(encoded_instr);
        r                             = execute(vm, &instr);
    }

    return r;
}

void vm_print_registers(const VM *const vm)
{
    printf("AX: %llu BX: %llu CX: %llu DX: %llu\n", vm->r[AX], vm->r[BX],
           vm->r[CX], vm->r[DX]);
}
