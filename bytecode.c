#include <stdlib.h>
#include <string.h>
#include "bytecode.h"

struct code {
    qword *code;
    size_t length;
    size_t capacity;
};

static struct code *code_extend(struct code *c) 
{
    c->capacity *= 2;
    c->code = realloc(c->code, c->capacity * sizeof(*c->code));
    if (!c->code) {
        free(c);
        return NULL;
    }

    return c;
}

static struct code *code_create(void) 
{
    struct code *c = calloc(1, sizeof(*c));
    if (!c)
        return NULL;

    c->code = calloc(4, sizeof(qword));
    if (!c->code) {
        free(c);
        return NULL;
    }

    c->length = 0;
    c->capacity = 4;
    return c;
}

static void code_free(struct code *c) 
{
    free(c->code);
    free(c);
}

int bc_push_instr(Byte_Code *bc, qword instr) 
{
    if (bc->code->length + 1 == bc->code->capacity)
        bc->code = code_extend(bc->code);

    bc->code->code[bc->code->length++] = instr;

    return 0;
}

static qword make_instruction(qword opcode, qword dst, qword src) 
{
    qword quad_word = opcode << 56; 
    quad_word = quad_word | (dst << 48);
    quad_word = quad_word | (src & ADDR_MASK);

    return quad_word;
}

qword *bc_code(const Byte_Code * const bc) 
{
    return bc->code->code;
}

Byte_Code *bc_load(const char *path)
{
    (void)path;
    Byte_Code *bc = malloc(sizeof(*bc));
    bc->code = code_create();
    bc_push_instr(bc, make_instruction(ADDI, 0, 7));
    bc_push_instr(bc, make_instruction(ADDI, 0, 7));
    bc_push_instr(bc, make_instruction(HLT, 0, 0));
    return bc;
}

void bc_free(Byte_Code *bc) 
{ 
    code_free(bc->code);
    free(bc);
}
