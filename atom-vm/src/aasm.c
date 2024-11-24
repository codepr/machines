#include "assembler.h"
#include "bytecode.h"

int main(void)
{
    Byte_Code *bc = asm_compile("examples/func.atom", 1);
    bc_dump(bc, "test.S");
    bc_free(bc);
    Byte_Code *read_code = bc_load("test.S");
    if (!read_code)
        exit(EXIT_FAILURE);
    asm_disassemble(read_code);
    bc_free(read_code);
    return 0;
}
