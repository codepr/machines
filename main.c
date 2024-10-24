#include "bytecode.h"
#include "parser.h"
#include "vm.h"
#include <stdarg.h>
#include <stdio.h>

static void die(int line_nr, const char *fmt, ...)
{
    va_list vargs;
    va_start(vargs, fmt);
    fprintf(stderr, "%d: ", line_nr);
    vfprintf(stderr, fmt, vargs);
    fprintf(stderr, "\n");
    va_end(vargs);
    exit(1);
}

int main(int argc, char **argv)
{
    const char *asm = argc > 1 ? argv[1] : "examples/mul.t800";
    Byte_Code *bc   = bc_load(asm);
    if (!bc)
        die(__LINE__, "error parsing source");

    printf("\n* disassamble \n");
    bc_disassemble(bc);

    VM *vm = vm_create(bc, 32768);
    if (!vm)
        die(__LINE__, "Error creating CPU");

    printf("\n* Execution \n");
    vm_run(vm);
    printf("\n\n* Register status\n\n");
    vm_print_registers(vm);

    vm_free(vm);
    // bc_free(bc);
    bc_free(bc);

    return 0;
}
