#include "bytecode.h"
#include "data.h"
#include "lexer.h"
#include "parser.h"
#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INSTR_SHOW_LEN 128

typedef enum { CODE_SEGMENT, DATA_SEGMENT } Segment_Type;

static void *segment_create(Segment_Type type)
{
    void *ptr       = NULL;
    size_t capacity = 4;
    if (type == CODE_SEGMENT) {
        struct qword_segment *c = calloc(1, sizeof(*c));
        if (!c)
            return NULL;

        da_init(c, capacity);
        ptr = c;
    } else {
        struct hword_segment *c = calloc(1, sizeof(*c));
        if (!c)
            return NULL;

        da_init(c, capacity);
        ptr = c;
    }
    return ptr;
}

static void segment_free(Segment_Type type, void *c)
{
    if (type == CODE_SEGMENT)
        free(((struct qword_segment *)c)->data);
    else
        free(((struct hword_segment *)c)->data);
    free(c);
}

qword bc_encode_instruction(struct instruction_line *i)
{
    // For instructions like HALT or SYSCALL, just to avoid shifting on
    // negatives
    i->src = i->src == -1 ? 0 : i->src;
    i->dst = i->dst == -1 ? 0 : i->dst;

    return data_encode_instruction(i);
}
struct instruction_line bc_decode_instruction(qword e_instr)
{
    return data_decode_instruction(e_instr);
}

qword *bc_code(const Byte_Code *const bc)
{
    if (!bc)
        return NULL;
    return bc->code_segment->data;
}

hword *bc_data(const Byte_Code *const bc)
{
    if (!bc)
        return NULL;
    return bc->data_segment->data;
}

qword bc_data_addr(const Byte_Code *const bc)
{
    if (!bc)
        return 0;
    return bc->data_addr;
}

Byte_Code *bc_create(void)
{
    Byte_Code *bc = calloc(1, sizeof(*bc));
    if (!bc)
        return NULL;

    bc->code_segment = segment_create(CODE_SEGMENT);
    if (!bc->code_segment)
        goto code_error;

    bc->data_segment = segment_create(DATA_SEGMENT);
    if (!bc->data_segment)
        goto data_error;

    bc->data_addr = DATA_OFFSET;

    return bc;

data_error:
    free(bc->code_segment);

code_error:
    free(bc);
    return NULL;
}

Byte_Code *bc_from_raw(const qword *bytecode, size_t length)
{
    Byte_Code *bc = bc_create();

    for (size_t i = 0; i < length; ++i)
        da_push(bc->code_segment, bytecode[i]);

    return bc;
}

typedef enum { OP_NONE, OP_REG, OP_IMM, OP_ADDR } OP_Type;

typedef struct {
    const char *name;
    OP_Type dst, src;
} Instruction_Def;

static const char *reg_to_str[NUM_REGISTERS] = {"AX", "BX", "CX", "DX"};

static const Instruction_Def instr_defs[NUM_INSTRUCTIONS] = {
    {.name = "NOP", .dst = OP_NONE, .src = OP_NONE},
    {.name = "CLF", .dst = OP_NONE, .src = OP_NONE},
    {.name = "CMP", .dst = OP_REG, .src = OP_REG},
    {.name = "CMI", .dst = OP_REG, .src = OP_IMM},
    {.name = "MOV", .dst = OP_REG, .src = OP_REG},
    {.name = "LDI", .dst = OP_REG, .src = OP_IMM},
    {.name = "LDR", .dst = OP_REG, .src = OP_ADDR},
    {.name = "STI", .dst = OP_ADDR, .src = OP_IMM},
    {.name = "STR", .dst = OP_ADDR, .src = OP_REG},
    {.name = "PSR", .dst = OP_REG, .src = OP_NONE},
    {.name = "PSM", .dst = OP_ADDR, .src = OP_NONE},
    {.name = "PSI", .dst = OP_IMM, .src = OP_NONE},
    {.name = "POP", .dst = OP_REG, .src = OP_NONE},
    {.name = "POM", .dst = OP_ADDR, .src = OP_NONE},
    {.name = "ADD", .dst = OP_REG, .src = OP_REG},
    {.name = "ADI", .dst = OP_REG, .src = OP_IMM},
    {.name = "SUB", .dst = OP_REG, .src = OP_REG},
    {.name = "SBI", .dst = OP_REG, .src = OP_IMM},
    {.name = "MUL", .dst = OP_REG, .src = OP_REG},
    {.name = "MLI", .dst = OP_REG, .src = OP_IMM},
    {.name = "DIV", .dst = OP_REG, .src = OP_REG},
    {.name = "DVI", .dst = OP_REG, .src = OP_IMM},
    {.name = "MOD", .dst = OP_REG, .src = OP_REG},
    {.name = "MDI", .dst = OP_REG, .src = OP_IMM},
    {.name = "INC", .dst = OP_REG, .src = OP_NONE},
    {.name = "DEC", .dst = OP_REG, .src = OP_NONE},
    {.name = "AND", .dst = OP_REG, .src = OP_REG},
    {.name = "BOR", .dst = OP_REG, .src = OP_REG},
    {.name = "XOR", .dst = OP_REG, .src = OP_REG},
    {.name = "NOT", .dst = OP_REG, .src = OP_NONE},
    {.name = "SHL", .dst = OP_REG, .src = OP_IMM},
    {.name = "SHR", .dst = OP_REG, .src = OP_IMM},
    {.name = "JMP", .dst = OP_ADDR, .src = OP_NONE},
    {.name = "JEQ", .dst = OP_ADDR, .src = OP_NONE},
    {.name = "JNE", .dst = OP_ADDR, .src = OP_NONE},
    {.name = "JLE", .dst = OP_ADDR, .src = OP_NONE},
    {.name = "JLT", .dst = OP_ADDR, .src = OP_NONE},
    {.name = "JGE", .dst = OP_ADDR, .src = OP_NONE},
    {.name = "JGT", .dst = OP_ADDR, .src = OP_NONE},
    {.name = "CALL", .dst = OP_ADDR, .src = OP_NONE},
    {.name = "RET", .dst = OP_NONE, .src = OP_NONE},
    {.name = "SYSCALL", .dst = OP_NONE, .src = OP_NONE},
    {.name = "HLT", .dst = OP_NONE, .src = OP_NONE}

};

static const char *instruction_to_str(const struct instruction_line *instr,
                                      char *dst)
{
    if (instr->op < 0 || instr->op >= NUM_INSTRUCTIONS) {
        fprintf(stderr, "unrecognized assembly instruction %d\n", instr->op);
        return NULL;
    }

    Instruction_Def idef = instr_defs[instr->op];
    size_t nbytes        = 0;

    switch (idef.dst) {
    case OP_IMM: {
        nbytes =
            snprintf(dst, INSTR_SHOW_LEN, "%s %lli", idef.name, instr->dst);
        break;
    }
    case OP_REG: {
        nbytes = snprintf(dst, INSTR_SHOW_LEN, "%s %s", idef.name,
                          reg_to_str[instr->dst]);
        break;
    }
    case OP_ADDR: {
        nbytes =
            snprintf(dst, INSTR_SHOW_LEN, "%s [0x%lli]", idef.name, instr->dst);
        break;
    }
    default:
        nbytes = snprintf(dst, INSTR_SHOW_LEN, "%s", idef.name);
        break;
    }

    switch (idef.src) {
    case OP_IMM: {
        nbytes = snprintf(dst + nbytes, INSTR_SHOW_LEN, " %lli", instr->src);
        break;
    }
    case OP_REG: {
        nbytes = snprintf(dst + nbytes, INSTR_SHOW_LEN, " %s",
                          reg_to_str[instr->src]);
        break;
    }
    case OP_ADDR: {
        nbytes =
            snprintf(dst + nbytes, INSTR_SHOW_LEN, " [0x%llx]", instr->src);
        break;
    }
    default:
        break;
    }

    return dst;
}

void bc_disassemble(const Byte_Code *const bc)
{

    size_t i    = 0;

    qword *code = bc_code(bc);
    char instr_str[INSTR_SHOW_LEN];

    printf("\nOffset  Instructions    Hex words\n");
    printf("-------------------------------------------------------\n\n");

    while (i < bc->code_segment->length) {
        memset(instr_str, 0x00, sizeof(instr_str));
        const struct instruction_line ins = bc_decode_instruction(code[i]);
        printf("0x%04lX\t%-16s%02X %02X %02X %02X %02X %02X %02X %02X  "
               "0x%04lX",
               i, instruction_to_str(&ins, instr_str),
               (unsigned short)(code[i] >> 56 & 0xFF),
               (unsigned short)(code[i] >> 48 & 0xFF),
               (unsigned short)(code[i] >> 40 & 0xFF),
               (unsigned short)(code[i] >> 32 & 0xFF),
               (unsigned short)(code[i] >> 24 & 0xFF),
               (unsigned short)(code[i] >> 16 & 0xFF),
               (unsigned short)(code[i] >> 8 & 0xFF),
               (unsigned short)(code[i] >> 0 & 0xFF),
               (unsigned long)i * sizeof(qword));

        printf("\n");
        i++;
    }
}

Byte_Code *bc_from_source(const char *source)
{
    Byte_Code *bc = bc_create();
    if (!bc)
        return NULL;

    // Initialise lexer and parser
    struct lexer lex;
    lexer_init(&lex, (char *)source, strlen(source));

    struct token_list tl;
    lexer_token_list_init(&tl, 4);
    lexer_tokenize(&lex, &tl);

    lexer_print_tokens(&tl);

    struct parser p;
    parser_init(&p, &tl);
    parser_parse_source(&p, bc);

    return bc;
}

Byte_Code *bc_slurp(const char *path)
{
    if (!path)
        return NULL;

    FILE *fp = fopen(path, "rb");
    if (!fp)
        return NULL;

    // Seek to the end of the file to determine its size
    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    rewind(fp); // Go back to the start of the file

    if (file_size < 0) {
        perror("Failed to determine file size");
        fclose(fp);
        return NULL;
    }

    // Allocate memory for the buffer (+1 for null terminator)
    char *buffer = malloc(file_size + 1);
    if (buffer == NULL) {
        perror("Failed to allocate memory");
        fclose(fp);
        return NULL;
    }

    // Read the entire file into the buffer
    size_t read_size = fread(buffer, 1, file_size, fp);
    if (read_size != file_size) {
        perror("Failed to read file");
        goto exit;
    }

    // Null-terminate the buffer
    buffer[file_size] = '\0';

    // Close the file and return the buffer
    fclose(fp);

    Byte_Code *bc = bc_create();
    if (!bc)
        goto exit;

    // Initialise lexer and parser
    struct lexer lex;
    lexer_init(&lex, buffer, strlen(buffer));

    struct token_list tl;
    lexer_token_list_init(&tl, 4);
    lexer_tokenize(&lex, &tl);

    lexer_print_tokens(&tl);

    struct parser p;
    parser_init(&p, &tl);
    parser_parse_source(&p, bc);

    free(buffer);

    return bc;

exit:
    free(buffer);
    fclose(fp);
    return NULL;
}

Byte_Code *bc_load(const char *path)
{
    if (!path)
        return NULL;

    FILE *fp = fopen(path, "rb");
    if (!fp)
        return NULL;

    Byte_Code *bc = bc_create();
    if (!bc)
        return NULL;

    struct lexer lex;
    struct token_list tl;
    lexer_token_list_init(&tl, 4);

    lexer_tokenize_stream(fp, &lex, &tl);

    struct parser p;
    parser_init(&p, &tl);
    parser_parse_source(&p, bc);

    return bc;
}

void bc_free(Byte_Code *bc)
{
    segment_free(CODE_SEGMENT, bc->code_segment);
    segment_free(DATA_SEGMENT, bc->data_segment);
    free(bc);
}
