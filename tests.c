#include "bytecode.h"
#include "cpu.h"
#include <assert.h>
#include <stdio.h>

void test_bc_encode_decode(void)
{
    qword instruction = bc_encode_instruction(ADI, AX, 3);
    const struct instruction i = bc_decode_instruction(instruction);

    assert(i.op == ADI);
    assert(i.src == 3);
    assert(i.dst == AX);

    printf("Encode/Decode: pass\n");
}

void test_add(Cpu *c)
{
    qword bytecode[3] = {bc_encode_instruction(ADI, AX, 3),
                         bc_encode_instruction(ADD, BX, AX),
                         bc_encode_instruction(HLT, 0, 0)};

    Byte_Code *bc = bc_create(bytecode, 3);
    assert(bc);

    cpu_reset(c, bc_code(bc));
    assert(c->r[BX] == 0);

    cpu_run(c);

    assert(c->r[BX] == 3);

    bc_free(bc);

    printf("Instruction ADD: pass\n");
}

void test_adi(Cpu *c)
{
    qword bytecode[3] = {bc_encode_instruction(ADI, AX, 3),
                         bc_encode_instruction(ADI, AX, 4),
                         bc_encode_instruction(HLT, 0, 0)};

    Byte_Code *bc = bc_create(bytecode, 3);
    assert(bc);

    cpu_reset(c, bc_code(bc));
    cpu_run(c);

    assert(c->r[AX] == 7);

    bc_free(bc);

    printf("Instruction ADI: pass\n");
}

void test_sub(Cpu *c)
{
    qword bytecode[4] = {
        bc_encode_instruction(ADI, AX, 3), bc_encode_instruction(ADI, BX, 2),
        bc_encode_instruction(SUB, AX, BX), bc_encode_instruction(HLT, 0, 0)};

    Byte_Code *bc = bc_create(bytecode, 4);
    assert(bc);

    cpu_reset(c, bc_code(bc));
    assert(c->r[BX] == 0);

    cpu_run(c);

    assert(c->r[AX] == 1);

    bc_free(bc);

    printf("Instruction SUB: pass\n");
}

void test_sbi(Cpu *c)
{
    qword bytecode[4] = {bc_encode_instruction(ADI, AX, 3),
                         bc_encode_instruction(SBI, AX, 1),
                         bc_encode_instruction(HLT, 0, 0)};

    Byte_Code *bc = bc_create(bytecode, 3);
    assert(bc);

    cpu_reset(c, bc_code(bc));
    cpu_run(c);

    assert(c->r[AX] == 2);

    bc_free(bc);

    printf("Instruction SBI: pass\n");
}

void test_mul(Cpu *c)
{
    qword bytecode[4] = {
        bc_encode_instruction(ADI, AX, 3), bc_encode_instruction(ADI, BX, 2),
        bc_encode_instruction(MUL, AX, BX), bc_encode_instruction(HLT, 0, 0)};

    Byte_Code *bc = bc_create(bytecode, 4);
    assert(bc);

    cpu_reset(c, bc_code(bc));
    assert(c->r[BX] == 0);

    cpu_run(c);

    assert(c->r[AX] == 6);

    bc_free(bc);

    printf("Instruction MUL: pass\n");
}

void test_mli(Cpu *c)
{
    qword bytecode[4] = {bc_encode_instruction(ADI, AX, 3),
                         bc_encode_instruction(MLI, AX, 2),
                         bc_encode_instruction(HLT, 0, 0)};

    Byte_Code *bc = bc_create(bytecode, 4);
    assert(bc);

    cpu_reset(c, bc_code(bc));
    cpu_run(c);

    assert(c->r[AX] == 6);

    bc_free(bc);

    printf("Instruction MLI: pass\n");
}

void test_div(Cpu *c)
{
    qword bytecode[4] = {
        bc_encode_instruction(ADI, AX, 6), bc_encode_instruction(ADI, BX, 2),
        bc_encode_instruction(DIV, AX, BX), bc_encode_instruction(HLT, 0, 0)};

    Byte_Code *bc = bc_create(bytecode, 4);
    assert(bc);

    cpu_reset(c, bc_code(bc));
    assert(c->r[BX] == 0);

    cpu_run(c);

    assert(c->r[AX] == 3);

    bc_free(bc);

    printf("Instruction DIV: pass\n");
}

void test_dvi(Cpu *c)
{
    qword bytecode[4] = {bc_encode_instruction(ADI, AX, 8),
                         bc_encode_instruction(DVI, AX, 2),
                         bc_encode_instruction(HLT, 0, 0)};

    Byte_Code *bc = bc_create(bytecode, 4);
    assert(bc);

    cpu_reset(c, bc_code(bc));
    cpu_run(c);

    assert(c->r[AX] == 4);

    bc_free(bc);

    printf("Instruction DVI: pass\n");
}

void test_mod(Cpu *c)
{
    qword bytecode[4] = {
        bc_encode_instruction(ADI, AX, 6), bc_encode_instruction(ADI, BX, 5),
        bc_encode_instruction(MOD, AX, BX), bc_encode_instruction(HLT, 0, 0)};

    Byte_Code *bc = bc_create(bytecode, 4);
    assert(bc);

    cpu_reset(c, bc_code(bc));
    assert(c->r[BX] == 0);

    cpu_run(c);

    assert(c->r[AX] == 1);

    bc_free(bc);

    printf("Instruction MOD: pass\n");
}

void test_mdi(Cpu *c)
{
    qword bytecode[4] = {bc_encode_instruction(ADI, AX, 8),
                         bc_encode_instruction(MDI, AX, 2),
                         bc_encode_instruction(HLT, 0, 0)};

    Byte_Code *bc = bc_create(bytecode, 4);
    assert(bc);

    cpu_reset(c, bc_code(bc));
    cpu_run(c);

    assert(c->r[AX] == 0);

    bc_free(bc);

    printf("Instruction MDI: pass\n");
}

void test_dvi_e(Cpu *c)
{
    qword bytecode[4] = {bc_encode_instruction(ADI, AX, 8),
                         bc_encode_instruction(DVI, AX, 0),
                         bc_encode_instruction(HLT, 0, 0)};

    Byte_Code *bc = bc_create(bytecode, 4);
    assert(bc);

    cpu_reset(c, bc_code(bc));
    Exec_Result r = cpu_run(c);

    assert(r == E_DIV_BY_ZERO);

    bc_free(bc);

    printf("Instruction EDV: pass\n");
}

void test_mov(Cpu *c)
{
    qword bytecode[3] = {bc_encode_instruction(ADI, AX, 3),
                         bc_encode_instruction(MOV, BX, AX),
                         bc_encode_instruction(HLT, 0, 0)};

    Byte_Code *bc = bc_create(bytecode, 3);
    assert(bc);

    cpu_reset(c, bc_code(bc));

    assert(c->r[BX] == 0);

    cpu_run(c);

    assert(c->r[BX] == 3);

    bc_free(bc);

    printf("Instruction MOV: pass\n");
}

void test_ldi(Cpu *c)
{
    qword bytecode[2] = {bc_encode_instruction(LDI, AX, 3),
                         bc_encode_instruction(HLT, 0, 0)};

    Byte_Code *bc = bc_create(bytecode, 2);
    assert(bc);

    cpu_reset(c, bc_code(bc));
    cpu_run(c);

    assert(c->r[AX] == 3);

    bc_free(bc);

    printf("Instruction LDI: pass\n");
}

void test_ldr(Cpu *c)
{
    qword bytecode[2] = {bc_encode_instruction(LDR, AX, 1),
                         bc_encode_instruction(HLT, 0, 0)};

    Byte_Code *bc = bc_create(bytecode, 2);
    assert(bc);

    cpu_reset(c, bc_code(bc));

    c->memory[1] = 7;

    cpu_run(c);

    assert(c->r[AX] == 7);

    bc_free(bc);

    printf("Instruction LDR: pass\n");
}

void test_sti(Cpu *c)
{
    qword bytecode[2] = {bc_encode_instruction(STI, 2, 1),
                         bc_encode_instruction(HLT, 0, 0)};

    Byte_Code *bc = bc_create(bytecode, 2);
    assert(bc);

    cpu_reset(c, bc_code(bc));
    cpu_run(c);

    assert(c->memory[2] == 1);

    bc_free(bc);

    printf("Instruction STI: pass\n");
}

void test_str(Cpu *c)
{
    qword bytecode[2] = {bc_encode_instruction(STR, 2, AX),
                         bc_encode_instruction(HLT, 0, 0)};

    Byte_Code *bc = bc_create(bytecode, 2);
    assert(bc);

    cpu_reset(c, bc_code(bc));

    c->r[AX] = 9;

    cpu_run(c);

    assert(c->memory[2] == 9);

    bc_free(bc);

    printf("Instruction STR: pass\n");
}

void test_inc(Cpu *c)
{
    qword bytecode[2] = {bc_encode_instruction(INC, AX, 0),
                         bc_encode_instruction(HLT, 0, 0)};

    Byte_Code *bc = bc_create(bytecode, 2);
    assert(bc);

    cpu_reset(c, bc_code(bc));
    cpu_run(c);

    assert(c->r[AX] == 1);

    bc_free(bc);

    printf("Instruction INC: pass\n");
}

void test_dec(Cpu *c)
{
    qword bytecode[3] = {bc_encode_instruction(INC, AX, 0),
                         bc_encode_instruction(DEC, AX, 0),
                         bc_encode_instruction(HLT, 0, 0)};

    Byte_Code *bc = bc_create(bytecode, 3);
    assert(bc);

    cpu_reset(c, bc_code(bc));
    cpu_run(c);

    assert(c->r[AX] == 0);

    bc_free(bc);

    printf("Instruction DEC: pass\n");
}

void test_psi(Cpu *c)
{
    qword bytecode[2] = {bc_encode_instruction(PSI, 0, 24),
                         bc_encode_instruction(HLT, 0, 0)};

    Byte_Code *bc = bc_create(bytecode, 2);
    assert(bc);

    cpu_reset(c, bc_code(bc));
    cpu_run(c);

    assert(c->stack[0] == 24);

    bc_free(bc);

    printf("Instruction PSI: pass\n");
}

void test_psr(Cpu *c)
{
    qword bytecode[3] = {bc_encode_instruction(LDI, AX, 32),
                         bc_encode_instruction(PSR, 0, AX),
                         bc_encode_instruction(HLT, 0, 0)};

    Byte_Code *bc = bc_create(bytecode, 3);
    assert(bc);

    cpu_reset(c, bc_code(bc));
    cpu_run(c);

    assert(c->stack[0] == 32);

    bc_free(bc);

    printf("Instruction PSR: pass\n");
}

void test_psm(Cpu *c)
{
    qword bytecode[3] = {bc_encode_instruction(STI, 2, 32),
                         bc_encode_instruction(PSM, 0, 2),
                         bc_encode_instruction(HLT, 0, 0)};

    Byte_Code *bc = bc_create(bytecode, 3);
    assert(bc);

    cpu_reset(c, bc_code(bc));
    cpu_run(c);

    assert(c->stack[0] == 32);

    bc_free(bc);

    printf("Instruction PSM: pass\n");
}

void test_pop(Cpu *c)
{
    qword bytecode[4] = {
        bc_encode_instruction(LDI, AX, 32), bc_encode_instruction(PSR, 0, AX),
        bc_encode_instruction(POP, AX, 0), bc_encode_instruction(HLT, 0, 0)};

    Byte_Code *bc = bc_create(bytecode, 4);
    assert(bc);

    cpu_reset(c, bc_code(bc));
    cpu_run(c);

    assert(c->stack[0] == 32);

    bc_free(bc);

    printf("Instruction POP: pass\n");
}

void test_pom(Cpu *c)
{
    qword bytecode[4] = {
        bc_encode_instruction(STI, 2, 32), bc_encode_instruction(PSM, 0, 2),
        bc_encode_instruction(POM, 0, 2), bc_encode_instruction(HLT, 0, 0)};

    Byte_Code *bc = bc_create(bytecode, 4);
    assert(bc);

    cpu_reset(c, bc_code(bc));
    cpu_run(c);

    assert(c->stack[0] == 32);

    bc_free(bc);

    printf("Instruction POM: pass\n");
}

int main(void)
{
    Cpu *cpu = cpu_create(NULL, 64);

    test_bc_encode_decode();
    test_mov(cpu);
    test_add(cpu);
    test_adi(cpu);
    test_sub(cpu);
    test_sbi(cpu);
    test_mul(cpu);
    test_mli(cpu);
    test_div(cpu);
    test_dvi(cpu);
    test_dvi_e(cpu);
    test_mod(cpu);
    test_mdi(cpu);
    test_ldi(cpu);
    test_ldr(cpu);
    test_sti(cpu);
    test_str(cpu);
    test_inc(cpu);
    test_dec(cpu);
    test_psi(cpu);
    test_psr(cpu);
    test_psm(cpu);
    test_pop(cpu);
    test_pom(cpu);

    cpu_free(cpu);

    printf("All tests passed\n");

    return 0;
}
