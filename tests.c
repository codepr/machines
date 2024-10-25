#include "bytecode.h"
#include "data.h"
#include "vm.h"
#include <assert.h>
#include <stdio.h>

#define MEMORY_SIZE DATA_OFFSET * 2

#define encode_instr(opcode, dst_op, src_op, sem_code)                         \
    bc_encode_instruction(&(struct instruction_line){                          \
        .op = (opcode), .src = (src_op), .dst = (dst_op), .sem = (sem_code)})

void test_bc_encode_decode(void)
{
    qword instruction = bc_encode_instruction(&(struct instruction_line){
        .op = OP_ADD, .src = 3, .dst = R_AX, .sem = IS_SEM_IMM_REG});
    const struct instruction_line i = bc_decode_instruction(instruction);

    assert(i.op == OP_ADD);
    assert(i.src == 3);
    assert(i.dst == R_AX);

    printf("Encode/Decode: pass\n");
}

void test_add(VM *c)
{
    qword bytecode[3] = {encode_instr(OP_ADD, R_AX, 3, IS_SEM_IMM_REG),
                         encode_instr(OP_ADD, R_BX, R_AX, IS_SEM_REG_REG),
                         encode_instr(OP_HLT, 0, 0, IS_ATOM)};

    Byte_Code *bc     = bc_from_raw(bytecode, 3);
    assert(bc);

    vm_reset(c, bc_code(bc), bc_data(bc), bc->data_addr, MEMORY_SIZE);
    assert(c->r[R_BX] == 0);

    vm_run(c);

    assert(c->r[R_BX] == 3);

    bc_free(bc);

    printf("Instruction OP_ADD: pass\n");
}

void test_sub(VM *c)
{
    qword bytecode[4] = {encode_instr(OP_ADD, R_AX, 3, IS_SEM_IMM_REG),
                         encode_instr(OP_ADD, R_BX, 2, IS_SEM_IMM_REG),
                         encode_instr(OP_SUB, R_AX, R_BX, IS_SEM_REG_REG),
                         encode_instr(OP_HLT, 0, 0, IS_ATOM)};

    Byte_Code *bc     = bc_from_raw(bytecode, 4);
    assert(bc);

    vm_reset(c, bc_code(bc), bc_data(bc), bc->data_addr, MEMORY_SIZE);
    assert(c->r[R_BX] == 0);

    vm_run(c);

    assert(c->r[R_AX] == 1);

    bc_free(bc);

    printf("Instruction OP_SUB: pass\n");
}

void test_mul(VM *c)
{
    qword bytecode[4] = {encode_instr(OP_ADD, R_AX, 3, IS_SEM_IMM_REG),
                         encode_instr(OP_ADD, R_BX, 2, IS_SEM_IMM_REG),
                         encode_instr(OP_MUL, R_AX, R_BX, IS_SEM_REG_REG),
                         encode_instr(OP_HLT, 0, 0, IS_ATOM)};

    Byte_Code *bc     = bc_from_raw(bytecode, 4);
    assert(bc);

    vm_reset(c, bc_code(bc), bc_data(bc), bc->data_addr, MEMORY_SIZE);
    assert(c->r[R_BX] == 0);

    vm_run(c);

    assert(c->r[R_AX] == 6);

    bc_free(bc);

    printf("Instruction OP_MUL: pass\n");
}

void test_div(VM *c)
{
    qword bytecode[4] = {encode_instr(OP_ADD, R_AX, 6, IS_SEM_IMM_REG),
                         encode_instr(OP_ADD, R_BX, 2, IS_SEM_IMM_REG),
                         encode_instr(OP_DIV, R_AX, R_BX, IS_SEM_REG_REG),
                         encode_instr(OP_HLT, 0, 0, IS_ATOM)};

    Byte_Code *bc     = bc_from_raw(bytecode, 4);
    assert(bc);

    vm_reset(c, bc_code(bc), bc_data(bc), bc->data_addr, MEMORY_SIZE);
    assert(c->r[R_BX] == 0);

    vm_run(c);

    assert(c->r[R_AX] == 3);

    bc_free(bc);

    printf("Instruction OP_DIV: pass\n");
}

void test_mod(VM *c)
{
    qword bytecode[4] = {encode_instr(OP_ADD, R_AX, 6, IS_SEM_IMM_REG),
                         encode_instr(OP_ADD, R_BX, 5, IS_SEM_IMM_REG),
                         encode_instr(OP_MOD, R_AX, R_BX, IS_SEM_REG_REG),
                         encode_instr(OP_HLT, 0, 0, IS_ATOM)};

    Byte_Code *bc     = bc_from_raw(bytecode, 4);
    assert(bc);

    vm_reset(c, bc_code(bc), bc_data(bc), bc->data_addr, MEMORY_SIZE);
    assert(c->r[R_BX] == 0);

    vm_run(c);

    assert(c->r[R_AX] == 1);

    bc_free(bc);

    printf("Instruction OP_MOD: pass\n");
}

void test_div_e(VM *c)
{
    qword bytecode[4] = {encode_instr(OP_ADD, R_AX, 8, IS_SEM_IMM_REG),
                         encode_instr(OP_DIV, R_AX, 0, IS_SEM_IMM_REG),
                         encode_instr(OP_HLT, 0, 0, IS_ATOM)};

    Byte_Code *bc     = bc_from_raw(bytecode, 4);
    assert(bc);

    vm_reset(c, bc_code(bc), bc_data(bc), bc->data_addr, MEMORY_SIZE);
    Exec_Result r = vm_run(c);

    assert(r == E_DIV_BY_ZERO);

    bc_free(bc);

    printf("Instruction OP_DIV Z: pass\n");
}

void test_mov(VM *c)
{
    qword bytecode[3] = {encode_instr(OP_ADD, R_AX, 3, IS_SEM_IMM_REG),
                         encode_instr(OP_MOV, R_BX, R_AX, IS_SEM_REG_REG),
                         encode_instr(OP_HLT, 0, 0, IS_ATOM)};

    Byte_Code *bc     = bc_from_raw(bytecode, 3);
    assert(bc);

    vm_reset(c, bc_code(bc), bc_data(bc), bc->data_addr, MEMORY_SIZE);

    assert(c->r[R_BX] == 0);

    vm_run(c);

    assert(c->r[R_BX] == 3);

    bc_free(bc);

    printf("Instruction OP_MOV: pass\n");
}

void test_inc(VM *c)
{
    qword bytecode[2] = {encode_instr(OP_INC, R_AX, 0, IS_SEM_IMM_REG),
                         encode_instr(OP_HLT, 0, 0, IS_ATOM)};

    Byte_Code *bc     = bc_from_raw(bytecode, 2);
    assert(bc);

    vm_reset(c, bc_code(bc), bc_data(bc), bc->data_addr, MEMORY_SIZE);
    vm_run(c);

    assert(c->r[R_AX] == 1);

    bc_free(bc);

    printf("Instruction OP_INC: pass\n");
}

void test_dec(VM *c)
{
    qword bytecode[3] = {encode_instr(OP_INC, R_AX, 0, IS_SEM_IMM_REG),
                         encode_instr(OP_DEC, R_AX, 0, IS_SEM_IMM_REG),
                         encode_instr(OP_HLT, 0, 0, IS_ATOM)};

    Byte_Code *bc     = bc_from_raw(bytecode, 3);
    assert(bc);

    vm_reset(c, bc_code(bc), bc_data(bc), bc->data_addr, MEMORY_SIZE);
    vm_run(c);

    assert(c->r[R_AX] == 0);

    bc_free(bc);

    printf("Instruction OP_DEC: pass\n");
}

void test_psh(VM *c)
{
    qword bytecode[2] = {encode_instr(OP_PSH, 0, 24, IS_SRC_IMM),
                         encode_instr(OP_HLT, 0, 0, IS_ATOM)};

    Byte_Code *bc     = bc_from_raw(bytecode, 2);
    assert(bc);

    vm_reset(c, bc_code(bc), bc_data(bc), bc->data_addr, MEMORY_SIZE);
    vm_run(c);

    assert(c->stack[0] == 24);

    bc_free(bc);

    printf("Instruction OP_PSH: pass\n");
}

void test_pop(VM *c)
{
    qword bytecode[4] = {encode_instr(OP_MOV, R_AX, 32, IS_SEM_IMM_REG),
                         encode_instr(OP_PSH, R_AX, R_AX, IS_DST_REG),
                         encode_instr(OP_POP, R_DX, R_DX, IS_DST_REG),
                         encode_instr(OP_HLT, 0, 0, IS_ATOM)};

    Byte_Code *bc     = bc_from_raw(bytecode, 4);
    assert(bc);

    vm_reset(c, bc_code(bc), bc_data(bc), bc->data_addr, MEMORY_SIZE);
    vm_run(c);

    assert(c->stack[0] == 32);
    assert(c->r[R_DX] == 32);

    bc_free(bc);

    printf("Instruction OP_POP: pass\n");
}

int main(void)
{
    VM *vm = vm_create(NULL, DATA_OFFSET * 2);

    test_bc_encode_decode();
    test_mov(vm);
    test_add(vm);
    test_sub(vm);
    test_mul(vm);
    test_div(vm);
    test_div_e(vm);
    test_mod(vm);
    test_inc(vm);
    test_dec(vm);
    test_psh(vm);
    test_pop(vm);

    vm_free(vm);

    printf("All tests passed\n");

    return 0;
}
