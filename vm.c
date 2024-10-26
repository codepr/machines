#include "vm.h"
#include "syscall.h"
#include <stdio.h>
#include <string.h>

static void clear_flags(VM *vm);

static void reset(VM *vm, qword *code_segment, hword *data_segment,
                  size_t data_len, size_t memory_size)
{
    memset(vm->r, 0x00, NUM_REGISTERS * sizeof(*vm->r));
    memset(vm->stack, 0x00, STACK_SIZE * sizeof(*vm->stack));
    vm->bcode = code_segment;

    vm->pc    = 0;
    vm->sp    = vm->stack;
    vm->run   = false;

    if (!data_segment)
        return;

    memset(vm->memory, 0x00, memory_size * sizeof(qword));
    // Addressing
    for (size_t i = DATA_OFFSET; i < DATA_OFFSET * 2; ++i)
        vm->memory[i] = i + DATA_OFFSET;
    // Storing
    for (size_t i = DATA_OFFSET * 2, j = 0; j < data_len - DATA_OFFSET;
         ++i, ++j)
        vm->memory[i] = data_segment[j];

    clear_flags(vm);
}

static qword fetch(VM *vm) { return vm->bcode[vm->pc++]; }

static struct instruction_line decode(qword einstr)
{
    return bc_decode_instruction(einstr);
}

static void clear_flags(VM *vm)
{
    vm->flags[FL_ZRO] = 0;
    vm->flags[FL_NEG] = 0;
    vm->flags[FL_POS] = 0;
}

// TODO consider using a single register with multiple FL_* enum values
static void set_flags(VM *vm, qword v, int type)
{
    clear_flags(vm);
    if (type == 0) {
        if (vm->r[v] == 0) {
            vm->flags[FL_ZRO] = 1;
        } else if (vm->r[v] >> 63) {
            vm->flags[FL_NEG] = 1;
        } else {
            vm->flags[FL_POS] = 1;
        }
    } else {
        if (vm->memory[v] == 0) {
            vm->flags[FL_ZRO] = 1;
        } else if (vm->memory[v] >> 63) {
            vm->flags[FL_NEG] = 1;
        } else {
            vm->flags[FL_POS] = 1;
        }
    }
}

static uint64_t sign_extend(qword x, int bit_count)
{
    if ((x >> (bit_count - 1)) & 1) {
        // If the sign bit is set, extend the sign by setting the upper bits to
        // 1
        x |= (0xFFFFFFFFFFFFFFFF << bit_count);
    }
    return x;
}

static int set_operands(VM *vm, const struct instruction_line *i, qword *src,
                        qword **dst)
{
    *src = (i->sem & IS_SRC_REG)    ? vm->r[i->src]
           : (i->sem & IS_SRC_MEM)  ? vm->memory[i->src]
           : (i->sem & IS_SRC_IREG) ? vm->memory[vm->r[i->src]]
                                    : sign_extend(i->src, 27);

    if (i->sem & IS_DST_REG) {
        *dst = &vm->r[i->dst];
        return 0;
    }
    *dst = &vm->memory[i->dst];
    return 1;
}

static Exec_Result execute(VM *vm, struct instruction_line *instr)
{
    switch (instr->op) {
    case OP_NOP: {
        break;
    }
    case OP_MOV: {
        qword src  = 0;
        qword *dst = NULL;
        int type   = set_operands(vm, instr, &src, &dst);

        *dst       = src;
        set_flags(vm, instr->dst, type);
        break;
    }
    case OP_PSH: {
        if (instr->sem & IS_DST_REG)
            *vm->sp++ = vm->r[instr->dst];
        else if (instr->sem & IS_DST_MEM)
            *vm->sp++ = vm->memory[instr->dst];
        else if (instr->sem & IS_SRC_IMM)
            *vm->sp++ = instr->src;

        break;
    }
    case OP_POP: {
        if (instr->sem & IS_DST_REG)
            vm->r[instr->dst] = *--vm->sp;
        else if (instr->sem & IS_DST_MEM)
            vm->memory[instr->dst] = *--vm->sp;
        break;
    }
    case OP_ADD: {
        qword src  = 0;
        qword *dst = NULL;
        int type   = set_operands(vm, instr, &src, &dst);

        *dst += src;
        set_flags(vm, instr->dst, type);
        break;
    }
    case OP_SUB: {
        qword src  = 0;
        qword *dst = NULL;
        int type   = set_operands(vm, instr, &src, &dst);

        *dst -= src;
        set_flags(vm, instr->dst, type);
        break;
    }
    case OP_MUL: {
        qword src  = 0;
        qword *dst = NULL;
        int type   = set_operands(vm, instr, &src, &dst);

        *dst *= src;
        set_flags(vm, instr->dst, type);
        break;
    }
    case OP_DIV: {
        qword src  = 0;
        qword *dst = NULL;
        int type   = set_operands(vm, instr, &src, &dst);

        if (src == 0)
            return E_DIV_BY_ZERO;

        *dst /= src;
        set_flags(vm, instr->dst, type);
        break;
    }
    case OP_MOD: {
        qword src  = 0;
        qword *dst = NULL;
        int type   = set_operands(vm, instr, &src, &dst);

        *dst %= src;
        set_flags(vm, instr->dst, type);
        break;
    }
    case OP_INC: {
        int is_src_mem = (instr->sem & IS_SRC_MEM);
        if (is_src_mem)
            vm->memory[instr->dst]++;
        else
            vm->r[instr->dst]++;

        set_flags(vm, instr->dst, is_src_mem);
        break;
    }
    case OP_DEC: {
        int is_src_mem = (instr->sem & IS_SRC_MEM);
        if (is_src_mem)
            vm->memory[instr->dst]--;
        else
            vm->r[instr->dst]--;

        set_flags(vm, instr->dst, is_src_mem);
        break;
    }
    case OP_CLF: {
        clear_flags(vm);
        break;
    }
    case OP_CMP: {
        qword src  = 0;
        qword *dst = NULL;
        int type   = set_operands(vm, instr, &src, &dst);

        set_flags(vm, instr->dst, type);
        break;
    }
    case OP_JMP: {
        vm->pc = instr->dst;
        break;
    }
    case OP_JEQ: {
        if (vm->flags[FL_ZRO])
            vm->pc = instr->dst;
        break;
    }
    case OP_JNE: {
        if (!vm->flags[FL_ZRO])
            vm->pc = instr->dst;
        break;
    }
    case OP_JLE: {
        if (vm->flags[FL_ZRO] || vm->flags[FL_NEG])
            vm->pc = instr->dst;
        break;
    }
    case OP_JLT: {
        if (!vm->flags[FL_ZRO] && vm->flags[FL_NEG])
            vm->pc = instr->dst;
        break;
    }
    case OP_JGE: {
        if (vm->flags[FL_ZRO] || vm->flags[FL_POS])
            vm->pc = instr->dst;
        break;
    }
    case OP_JGT: {
        if (!vm->flags[FL_ZRO] && vm->flags[FL_POS])
            vm->pc = instr->dst;
        break;
    }
    case OP_AND: {
        vm->r[instr->dst] &= vm->r[instr->src];
        break;
    }
    case OP_BOR: {
        vm->r[instr->dst] |= vm->r[instr->src];
        break;
    }
    case OP_XOR: {
        vm->r[instr->dst] ^= vm->r[instr->src];
        break;
    }
    case OP_NOT: {
        vm->r[instr->dst] = -vm->r[instr->src];
        break;
    }
    case OP_SHR: {
        vm->r[instr->dst] >>= vm->r[instr->src];
        break;
    }
    case OP_SHL: {
        vm->r[instr->dst] <<= vm->r[instr->src];
        break;
    }
    case OP_CALL: {
        *vm->sp++ = (vm->pc - 1);
        vm->pc    = instr->dst;
        break;
    }
    case OP_RET: {
        vm->pc = *--vm->sp;
        break;
    }
    case OP_SYSCALL: {
        switch (vm->r[R_BX]) {
        // STDIN
        case 0:
            syscall_read(vm->r[R_BX], &vm->memory[vm->r[R_CX]],
                         vm->r[R_DX] * sizeof(qword));
            break;
        // STDOUT
        case 1:
            syscall_write(vm->r[R_BX], &vm->memory[vm->r[R_CX]],
                          vm->r[R_DX] * sizeof(qword));
            fflush(stdout);
            break;
        case 64:
            vm->r[R_AX] = syscall_atoi(&vm->memory[vm->r[R_CX]]);
            break;
        }
        break;
    }
    case OP_HLT:
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

    reset(vm, bc_code(bc), bc_data(bc), bc_data_addr(bc), memory_size);

    return vm;
}

void vm_free(VM *vm)
{
    free(vm->memory);
    free(vm);
}

void vm_reset(VM *vm, qword *code, hword *data, size_t data_len,
              size_t memory_size)
{
    reset(vm, code, data, data_len, memory_size);
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
    printf("AX: %lli BX: %lli CX: %lli DX: %lli FL_ZRO: %i FL_NEG: %i FL_POS: "
           "%i\n",
           vm->r[R_AX], vm->r[R_BX], vm->r[R_CX], vm->r[R_DX],
           vm->flags[FL_ZRO], vm->flags[FL_NEG], vm->flags[FL_POS]);
}
