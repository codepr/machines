#include "bytecode.h"
#include "data.h"
#include "vm.h"
#include <assert.h>
#include <stdio.h>

#define encode_instr(opcode, dst_op, src_op, sem_code)                         \
    bc_encode_instruction(&(struct instruction_line){                          \
        .op = (opcode), .src = (src_op), .dst = (dst_op), .sem = (sem_code)})

void test_bc_encode_decode(void)
{
    qword instruction = bc_encode_instruction(&(struct instruction_line){
        .op = ADD, .src = 3, .dst = AX, .sem = IS_SEM_IMM_REG});
    const struct instruction_line i = bc_decode_instruction(instruction);

    assert(i.op == ADD);
    assert(i.src == 3);
    assert(i.dst == AX);

    printf("Encode/Decode: pass\n");
}

void test_add(VM *c)
{
    qword bytecode[3] = {encode_instr(ADD, AX, 3, IS_SEM_IMM_REG),
                         encode_instr(ADD, BX, AX, IS_SEM_REG_REG),
                         encode_instr(HLT, 0, 0, IS_ATOM)};

    Byte_Code *bc     = bc_from_raw(bytecode, 3);
    assert(bc);

    vm_reset(c, bc_code(bc), bc_data(bc), bc->data_addr);
    assert(c->r[BX] == 0);

    vm_run(c);

    assert(c->r[BX] == 3);

    bc_free(bc);

    printf("Instruction ADD: pass\n");
}

void test_sub(VM *c)
{
    qword bytecode[4] = {encode_instr(ADD, AX, 3, IS_SEM_IMM_REG),
                         encode_instr(ADD, BX, 2, IS_SEM_IMM_REG),
                         encode_instr(SUB, AX, BX, IS_SEM_REG_REG),
                         encode_instr(HLT, 0, 0, IS_ATOM)};

    Byte_Code *bc     = bc_from_raw(bytecode, 4);
    assert(bc);

    vm_reset(c, bc_code(bc), bc_data(bc), bc->data_addr);
    assert(c->r[BX] == 0);

    vm_run(c);

    assert(c->r[AX] == 1);

    bc_free(bc);

    printf("Instruction SUB: pass\n");
}

void test_mul(VM *c)
{
    qword bytecode[4] = {encode_instr(ADD, AX, 3, IS_SEM_IMM_REG),
                         encode_instr(ADD, BX, 2, IS_SEM_IMM_REG),
                         encode_instr(MUL, AX, BX, IS_SEM_REG_REG),
                         encode_instr(HLT, 0, 0, IS_ATOM)};

    Byte_Code *bc     = bc_from_raw(bytecode, 4);
    assert(bc);

    vm_reset(c, bc_code(bc), bc_data(bc), bc->data_addr);
    assert(c->r[BX] == 0);

    vm_run(c);

    assert(c->r[AX] == 6);

    bc_free(bc);

    printf("Instruction MUL: pass\n");
}

void test_div(VM *c)
{
    qword bytecode[4] = {encode_instr(ADD, AX, 6, IS_SEM_IMM_REG),
                         encode_instr(ADD, BX, 2, IS_SEM_IMM_REG),
                         encode_instr(DIV, AX, BX, IS_SEM_REG_REG),
                         encode_instr(HLT, 0, 0, IS_ATOM)};

    Byte_Code *bc     = bc_from_raw(bytecode, 4);
    assert(bc);

    vm_reset(c, bc_code(bc), bc_data(bc), bc->data_addr);
    assert(c->r[BX] == 0);

    vm_run(c);

    assert(c->r[AX] == 3);

    bc_free(bc);

    printf("Instruction DIV: pass\n");
}

void test_mod(VM *c)
{
    qword bytecode[4] = {encode_instr(ADD, AX, 6, IS_SEM_IMM_REG),
                         encode_instr(ADD, BX, 5, IS_SEM_IMM_REG),
                         encode_instr(MOD, AX, BX, IS_SEM_REG_REG),
                         encode_instr(HLT, 0, 0, IS_ATOM)};

    Byte_Code *bc     = bc_from_raw(bytecode, 4);
    assert(bc);

    vm_reset(c, bc_code(bc), bc_data(bc), bc->data_addr);
    assert(c->r[BX] == 0);

    vm_run(c);

    assert(c->r[AX] == 1);

    bc_free(bc);

    printf("Instruction MOD: pass\n");
}

void test_div_e(VM *c)
{
    qword bytecode[4] = {encode_instr(ADD, AX, 8, IS_SEM_IMM_REG),
                         encode_instr(DIV, AX, 0, IS_SEM_IMM_REG),
                         encode_instr(HLT, 0, 0, IS_ATOM)};

    Byte_Code *bc     = bc_from_raw(bytecode, 4);
    assert(bc);

    vm_reset(c, bc_code(bc), bc_data(bc), bc->data_addr);
    Exec_Result r = vm_run(c);

    assert(r == E_DIV_BY_ZERO);

    bc_free(bc);

    printf("Instruction EDV: pass\n");
}

void test_mov(VM *c)
{
    qword bytecode[3] = {encode_instr(ADD, AX, 3, IS_SEM_IMM_REG),
                         encode_instr(MOV, BX, AX, IS_SEM_REG_REG),
                         encode_instr(HLT, 0, 0, IS_ATOM)};

    Byte_Code *bc     = bc_from_raw(bytecode, 3);
    assert(bc);

    vm_reset(c, bc_code(bc), bc_data(bc), bc->data_addr);

    assert(c->r[BX] == 0);

    vm_run(c);

    assert(c->r[BX] == 3);

    bc_free(bc);

    printf("Instruction MOV: pass\n");
}

void test_inc(VM *c)
{
    qword bytecode[2] = {encode_instr(INC, AX, 0, IS_SEM_IMM_REG),
                         encode_instr(HLT, 0, 0, IS_ATOM)};

    Byte_Code *bc     = bc_from_raw(bytecode, 2);
    assert(bc);

    vm_reset(c, bc_code(bc), bc_data(bc), bc->data_addr);
    vm_run(c);

    assert(c->r[AX] == 1);

    bc_free(bc);

    printf("Instruction INC: pass\n");
}

void test_dec(VM *c)
{
    qword bytecode[3] = {encode_instr(INC, AX, 0, IS_SEM_IMM_REG),
                         encode_instr(DEC, AX, 0, IS_SEM_IMM_REG),
                         encode_instr(HLT, 0, 0, IS_ATOM)};

    Byte_Code *bc     = bc_from_raw(bytecode, 3);
    assert(bc);

    vm_reset(c, bc_code(bc), bc_data(bc), bc->data_addr);
    vm_run(c);

    assert(c->r[AX] == 0);

    bc_free(bc);

    printf("Instruction DEC: pass\n");
}

void test_psh(VM *c)
{
    qword bytecode[2] = {encode_instr(PSH, 0, 24, IS_SRC_IMM),
                         encode_instr(HLT, 0, 0, IS_ATOM)};

    Byte_Code *bc     = bc_from_raw(bytecode, 2);
    assert(bc);

    vm_reset(c, bc_code(bc), bc_data(bc), bc->data_addr);
    vm_run(c);

    assert(c->stack[0] == 24);

    bc_free(bc);

    printf("Instruction PSH: pass\n");
}

void test_pop(VM *c)
{
    qword bytecode[4] = {encode_instr(MOV, AX, 32, IS_SEM_IMM_REG),
                         encode_instr(PSH, AX, AX, IS_DST_REG),
                         encode_instr(POP, DX, DX, IS_DST_REG),
                         encode_instr(HLT, 0, 0, IS_ATOM)};

    Byte_Code *bc     = bc_from_raw(bytecode, 4);
    assert(bc);

    vm_reset(c, bc_code(bc), bc_data(bc), bc->data_addr);
    vm_run(c);

    assert(c->stack[0] == 32);
    assert(c->r[DX] == 32);

    bc_free(bc);

    printf("Instruction POP: pass\n");
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
