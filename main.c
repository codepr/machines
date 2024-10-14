#include "bytecode.h"
#include "cpu.h"
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

int main(void)
{
    Byte_Code *bc = bc_load("examples/fact.t800");
    if (!bc)
        die(__LINE__, "Error parsing source");

    printf("\n* Disassamble \n");
    bc_disassemble(bc);

    Cpu *cpu = cpu_create(bc, 32768);
    if (!cpu)
        die(__LINE__, "Error creating CPU");

    printf("\n* Execution \n");
    cpu_run(cpu);
    printf("\n* Register status\n\n");
    cpu_print_registers(cpu);

    cpu_free(cpu);
    bc_free(bc);

    return 0;
}
