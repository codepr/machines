#ifndef ASSEMBLER_H
#define ASSEMBLER_H

typedef struct bytecode Byte_Code;

Byte_Code *asm_compile(const char *path, int debug);
Byte_Code *asm_compile_from_stdin(int debug);

void asm_disassemble(const Byte_Code *bc);

#endif
