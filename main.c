#include <errno.h>
#include <stdio.h>
#include <stdarg.h>
#include "bytecode.h"
#include "cpu.h"

// #define panic(fmt, ...) die(__LINE__, (fmt), __VA_ARGS__)

static void die(int line_nr, const char *fmt, ...) {
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
	Byte_Code *bc = bc_load("examples/fib.tasm");
	if (!bc)
		die(__LINE__, "Error parsing source");	

    Cpu *cpu = cpu_create(bc, 32768);
    if (!cpu)
        die(__LINE__, "Error creating CPU");

    cpu_run(cpu);

    cpu_free(cpu);
    bc_free(bc);

	return 0;
}
