#include "bytecode.h"
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define MAX_LABELS 32
#define LABEL_SIZE 64

struct code {
    Word *code;
    size_t length;
    size_t capacity;
};

struct labels {
    char labels[MAX_LABELS][LABEL_SIZE];
    size_t length;
};

typedef struct {
    const char *name;
    bool has_arg;
} Instruction_Def;

static const Instruction_Def instr_defs[20] = {
    {.name = "LOAD", .has_arg = false},
    {.name = "LOADI", .has_arg = true},
    {.name = "STORE", .has_arg = false},
    {.name = "STOREI", .has_arg = true},
    {.name = "CALL", .has_arg = true},
    {.name = "PUSH", .has_arg = true},
    {.name = "PUSHI", .has_arg = true},
    {.name = "ADD", .has_arg = false},
    {.name = "SUB", .has_arg = false},
    {.name = "MUL", .has_arg = false},
    {.name = "DIV", .has_arg = false},
    {.name = "DUP", .has_arg = false},
    {.name = "INC", .has_arg = false},
    {.name = "EQ", .has_arg = false},
    {.name = "JMP", .has_arg = true},
    {.name = "JEQ", .has_arg = true},
    {.name = "JNE", .has_arg = true},
    {.name = "PRINT", .has_arg = false},
    {.name = "RET", .has_arg = false},
    {.name = "HALT", .has_arg = false}

};

static const char *instruction_to_str(Instruction instr) {
    return instr_defs[instr].name;
}

static Instruction str_to_instruction(const char *str) {
    if (strncmp(str, "PUSHI", 5) == 0)
        return OP_PUSHI;
    if (strncmp(str, "PUSH", 4) == 0)
        return OP_PUSH;
    if (strncmp(str, "LOADI", 5) == 0)
        return OP_LOADI;
    if (strncmp(str, "LOAD", 4) == 0)
        return OP_LOAD;
    if (strncmp(str, "STOREI", 6) == 0)
        return OP_STOREI;
    if (strncmp(str, "STORE", 5) == 0)
        return OP_STORE;
    if (strncmp(str, "ADD", 3) == 0)
        return OP_ADD;
    if (strncmp(str, "CALL", 4) == 0)
        return OP_CALL;
    if (strncmp(str, "PRINT", 5) == 0)
        return OP_PRINT;
    if (strncmp(str, "RET", 3) == 0)
        return OP_RET;
    if (strncmp(str, "SUB", 3) == 0)
        return OP_SUB;
    if (strncmp(str, "MUL", 3) == 0)
        return OP_MUL;
    if (strncmp(str, "DIV", 3) == 0)
        return OP_DIV;
    if (strncmp(str, "DUP", 3) == 0)
        return OP_DUP;
    if (strncmp(str, "INC", 3) == 0)
        return OP_INC;
    if (strncmp(str, "EQ", 2) == 0)
        return OP_EQ;
    if (strncmp(str, "JMP", 3) == 0)
        return OP_JMP;
    if (strncmp(str, "JNE", 3) == 0)
        return OP_JNE;
    if (strncmp(str, "JEQ", 3) == 0)
        return OP_JEQ;

    return OP_HALT;
}

static Code *code_extend(Code *c) {
    c->capacity *= 2;
    c->code = realloc(c->code, c->capacity * sizeof(*c->code));
    if (!c->code) {
        free(c);
        return NULL;
    }

    return c;
}

static Code *code_create(void) {
    Code *c = calloc(1, sizeof(*c));
    if (!c)
        return NULL;

    c->code = calloc(4, sizeof(Word));
    if (!c->code) {
        free(c);
        return NULL;
    }

    c->length = 0;
    c->capacity = 4;
    return c;
}

static void code_free(Code *c) {
    free(c->code);
    free(c);
}

static Labels *labels_create(void) {
    Labels *l = calloc(1, sizeof(*l));
    if (!l)
        return NULL;

    l->length = 0;
    return l;
}

static void labels_free(Labels *l) { free(l); }

Byte_Code *bc_create(void) {
    Byte_Code *bc = calloc(1, sizeof(*bc));
    if (!bc)
        return NULL;

    bc->code = code_create();
    if (!bc->code)
        goto error;

    bc->data = code_create();
    if (!bc->data)
        goto error;

    bc->labels = labels_create();
    if (!bc->labels)
        goto error;

    return bc;

error:
    free(bc);
    return NULL;
}

void bc_free(Byte_Code *bc) {
    code_free(bc->code);
    code_free(bc->data);
    labels_free(bc->labels);
    free(bc);
}

Word *bc_code(const Byte_Code *bc) { return bc->code->code; }

Word bc_constant(const Byte_Code *bc, uint64_t addr) {
    return bc->data->code[addr];
}

int bc_push_unary_instr(Byte_Code *bc, const char *instr) {
    if (!instr)
        return -1;

    if (bc->code->length + 1 == bc->code->capacity)
        bc->code = code_extend(bc->code);

    bc->code->code[bc->code->length++] = str_to_instruction(instr);

    return 0;
}

int bc_push_binary_instr(Byte_Code *bc, const char *instr, const char *arg) {
    if (!instr || !arg)
        return -1;

    if (bc->code->length + 2 >= bc->code->capacity)
        bc->code = code_extend(bc->code);

    bc->code->code[bc->code->length++] = str_to_instruction(instr);
    // let's assume only long integers for now
    bc->code->code[bc->code->length++] = strtoul(arg, NULL, 10);

    return 0;
}

int bc_push_constant(Byte_Code *bc, const char *addr, const char *value) {
    if (!addr || !value)
        return -1;

    if (*addr != '@')
        return -1;

    if (bc->data->length + 2 >= bc->data->capacity)
        bc->data = code_extend(bc->data);

    size_t index = strtoul(addr + 1, NULL, 10);
    bc->data->code[index] = strtoul(value, NULL, 10);
    bc->data->length++;

    return 0;
}

int bc_push_label(Byte_Code *bc, const char *label) {
    if (strlen(label) > LABEL_SIZE)
        return -1;
    if (bc->labels->length == MAX_LABELS)
        return -1;

    strncpy(bc->labels->labels[bc->labels->length++], label, strlen(label));

    return 0;
}

void bc_disassemble(const Byte_Code *bc) {
    size_t i = 0;

    if (bc->labels->length > 0) {
        printf("%s", bc->labels->labels[0]);
        while (i < bc->data->length) {
            printf("\t@%lu %llu\n", i, bc->data->code[i]);
            i++;
        }
        printf("\n");
        if (bc->labels->length > 1)
            printf("%s", bc->labels->labels[1]);
    }
    i = 0;
    while (i < bc->code->length) {
        printf("\t%04li %s", i, instruction_to_str(bc->code->code[i]));
        if (instr_defs[bc->code->code[i]].has_arg) {
            if (bc->code->code[i] == OP_PUSH)
                printf(" @");
            printf("%llu", bc->code->code[++i]);
        }
        printf("\n");
        i++;
    }
}

static inline void strip_spaces(char **str) {
    if (!*str)
        return;
    while (isspace(**str) && **str)
        ++(*str);
}

static inline void read_token(char **str, char *dest) {

    if (!str || !dest)
        return;

    while (!isspace(**str) && **str)
        *dest++ = *(*str)++;
}

typedef enum { READ_MAIN, READ_DATA } Assembly_Step;

Byte_Code *bc_load(const char *path) {
    if (!path)
        return NULL;

    FILE *fp = fopen(path, "r");
    if (!fp)
        return NULL;

    Byte_Code *bc = bc_create();
    if (!bc)
        return NULL;

    char line[0xfff], instr_offset[8], instr[16], arg[8];
    // size_t line_nr = 0;
    char *line_ptr;
    Assembly_Step state;

    while (fgets(line, 0xfff, fp)) {

        memset(arg, 0x00, 8);
        memset(instr, 0x00, 16);
        memset(instr_offset, 0x00, 8);

        // line_nr++;

        // Strip whitespaces if any before the line
        line_ptr = line;
        strip_spaces(&line_ptr);

        // Skip comments or empty lines
        if (*line_ptr == '#')
            continue;

        if (*line_ptr == '.') {
            if (bc_push_label(bc, line_ptr) < 0)
                goto errdefer;
            // We're in the constant data area
            state =
                (strncmp(line_ptr, ".data", 5) == 0) ? READ_DATA : READ_MAIN;

            continue;
        }

        if (*line_ptr == '\0')
            continue;

        // Read instruction offset
        read_token(&line_ptr, instr_offset);

        strip_spaces(&line_ptr);

        // Read instruction
        read_token(&line_ptr, instr);

        strip_spaces(&line_ptr);

        // if it's inside the data area store it in the data array
        if (state == READ_DATA) {
            if (bc_push_constant(bc, instr_offset, instr) < 0)
                goto errdefer;
            continue;
        }

        // Maybe read arg
        if (*line_ptr == '\0') {
            if (bc_push_unary_instr(bc, instr) < 0)
                goto errdefer;
            continue;
        }

        read_token(&line_ptr, arg);

        if (bc_push_binary_instr(bc, instr, arg) < 0)
            goto errdefer;
    }

    return bc;

errdefer:

    bc_free(bc);
    return NULL;
}
