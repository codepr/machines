#include "bytecode.h"
#include "cpu.h"
#include "lexer.h"
#include "parser.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

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

    // const char *source =
    //     "; Simple Factorial implementation\n"
    //     "; Factorial(10)\n"
    //
    //     ".data\n"
    //     "msg: db \"Insert input: \", 14\n"
    //     "buf: db 1024, 10        ; reserve 1024 bytes for input\n"
    //
    //     ".main\n"
    //     "; Initialize variables\n"
    //     "ldi ax, 10          ; Initialize starting value 10\n"
    //     "ldi bx, 9           ; Initialize second value to AX - 1\n"
    //
    //     "; Factorial loop\n"
    //     "loop:\n"
    //     "mul ax, bx       ; ax = ax * ax - 1\n"
    //     "dec bx           ; bx = bx - 1 decrement the counter\n"
    //     "cmi bx, 0        ; Check if 0 is reached\n"
    //     "jne loop         ; Jump back to loop if bx is greater than 0\n"
    //     "ldi bx, 1\n"
    //     "ldr cx, msg\n"
    //     "ldi dx, 14\n"
    //     "syscall\n"
    //     "ldi bx, 0\n"
    //     "ldr cx, buf\n"
    //     "ldi dx, 1024\n"
    //     "syscall\n"
    //     "ldi bx, 1\n"
    //     "ldr cx, buf\n"
    //     "ldi dx, 3\n"
    //     "syscall\n"
    //     "hlt              ; Halt execution\n";

    // Byte_Code *bc = bc_from_source(source);
    // bc_disassemble(bc);

    // Byte_Code *bc = bc_slurp("examples/fact.t800");
    // if (!bc)
    //     die(__LINE__, "error parsing source");
    //
    // printf("\n* disassamble \n");
    // bc_disassemble(bc);

    Byte_Code *bcr = bc_load("examples/fact.t800");
    if (!bcr)
        die(__LINE__, "error parsing source");

    printf("\n* disassamble \n");
    bc_disassemble(bcr);

    Cpu *cpu = cpu_create(bcr, 32768);
    if (!cpu)
        die(__LINE__, "Error creating CPU");

    printf("\n* Execution \n");
    cpu_run(cpu);
    printf("\n* Register status\n\n");
    cpu_print_registers(cpu);

    cpu_free(cpu);
    // bc_free(bc);
    bc_free(bcr);

    return 0;
}
