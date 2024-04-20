#include "bytecode.h"
#include "cpu.h"
#include <assert.h>
#include <stdio.h>

void test_bc_encode_decode(void)
{
    qword instruction = bc_encode_instruction(ADI, AX, 3);
    struct instruction instr = bc_decode_instruction(instruction);

    assert(instr.op == ADI);
    assert(instr.src == 3);
    assert(instr.dst == AX);

    printf("Encode/Decode: pass\n");
}

void test_add(Cpu *c)
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

    printf("Instruction ADD: pass\n");
}

int main(void)
{
    Cpu *cpu = cpu_create(NULL, 64);

    test_bc_encode_decode();
    test_add(cpu);

    cpu_free(cpu);

    printf("All tests passed\n");

    return 0;
}
