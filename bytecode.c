#include "bytecode.h"
#include <stdlib.h>
#include <string.h>

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

qword bc_encode_instruction(qword opcode, qword dst, qword src)
{
    qword quad_word = opcode << 56;
    quad_word |= (dst << 48);
    quad_word |= (src & ADDR_MASK);

    return quad_word;
}

struct instruction bc_decode_instruction(qword e_instr)
{
    hword op = (uint64_t)e_instr >> 56;
    qword dst = (e_instr & DEST_MASK) >> 48;
    qword src = e_instr & ADDR_MASK;

    return (struct instruction){.op = op, .src = src, .dst = dst};
}

qword *bc_code(const Byte_Code *const bc)
{
    if (!bc)
        return NULL;
    return bc->code->code;
}

Byte_Code *bc_create(qword *bytecode, size_t length)
{
    Byte_Code *bc = malloc(sizeof(*bc));
    if (!bc)
        return NULL;

    bc->code = code_create();
    if (!bc->code) {
        free(bc);
        return NULL;
    }

    for (size_t i = 0; i < length; ++i)
        bc_push_instr(bc, bytecode[i]);

    return bc;
}

Byte_Code *bc_load(const char *path)
{
    (void)path;
    Byte_Code *bc = malloc(sizeof(*bc));
    bc->code = code_create();
    bc_push_instr(bc, bc_encode_instruction(ADI, 0, 7));
    bc_push_instr(bc, bc_encode_instruction(ADI, 0, 7));
    bc_push_instr(bc, bc_encode_instruction(HLT, 0, 0));
    return bc;
}

void bc_free(Byte_Code *bc)
{
    code_free(bc->code);
    free(bc);
}
