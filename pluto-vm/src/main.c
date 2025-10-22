#include "bytecode.h"
#include "parser.h"
#include "vm.h"
#include <stdarg.h>
#include <stdio.h>

#define DEFAULT_MEMORY_SIZE 32768

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
    if (argc < 2)
        die(__LINE__, "Please specify a source path");

    const char *source_path = argv[1];
    // Construct the absolute path
    Byte_Code *bc           = bc_load(source_path);
    if (!bc)
        die(__LINE__, "error parsing source");

    printf("\n* disassamble \n");
    bc_disassemble(bc);

    VM *vm = vm_create(bc, DEFAULT_MEMORY_SIZE);
    if (!vm)
        die(__LINE__, "Error creating CPU");

    printf("\n* Execution \n");
    vm_run(vm);
    printf("\n\n* Register status\n\n");
    vm_print_registers(vm);

    vm_free(vm);
    bc_free(bc);

    return 0;
}
