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

void bc_push_instruction(Byte_Code *bc, struct instruction_line *i)
{
    da_push(bc->code_segment, bc_encode_instruction(i));
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

static const char *reg_to_str[NUM_REGISTERS]    = {"AX", "BX", "CX", "DX"};

static const char *instr_defs[NUM_INSTRUCTIONS] = {
    "NOP", "CLF", "CMP", "MOV", "PSH", "POP", "ADD",  "SUB", "MUL",     "DIV",
    "MOD", "INC", "DEC", "AND", "BOR", "XOR", "NOT",  "SHL", "SHR",     "JMP",
    "JEQ", "JNE", "JLE", "JLT", "JGE", "JGT", "CALL", "RET", "SYSCALL", "HLT",
};

static const char *instruction_line_show(const struct instruction_line *instr,
                                         char *dst)
{
    if (instr->op < 0 || instr->op >= NUM_INSTRUCTIONS) {
        fprintf(stderr, "unrecognized assembly instruction %d\n", instr->op);
        return NULL;
    }

    const char *iname = instr_defs[instr->op];

    switch (instr->sem) {
    case IS_ATOM:
        snprintf(dst, INSTR_SHOW_LEN, "%s", iname);
        break;
    case IS_SRC_IMM:
        snprintf(dst, INSTR_SHOW_LEN, "%s %lli", iname, instr->src);
        break;
    case IS_SRC_REG:
        snprintf(dst, INSTR_SHOW_LEN, "%s %s", iname, reg_to_str[instr->dst]);
        break;
    case IS_SRC_MEM:
        snprintf(dst, INSTR_SHOW_LEN, "%s [0x%lli]", iname, instr->dst);
        break;
    case IS_DST_REG:
        snprintf(dst, INSTR_SHOW_LEN, "%s %s", iname, reg_to_str[instr->dst]);
        break;
    case IS_DST_MEM:
        snprintf(dst, INSTR_SHOW_LEN, "%s [0x%lli]", iname, instr->dst);
        break;
    case IS_SEM_REG_REG:
        snprintf(dst, INSTR_SHOW_LEN, "%s %s %s", iname, reg_to_str[instr->dst],
                 reg_to_str[instr->src]);
        break;
    case IS_SEM_REG_MEM:
        snprintf(dst, INSTR_SHOW_LEN, "%s [0x%lli] %s", iname, instr->dst,
                 reg_to_str[instr->src]);
        break;
    case IS_SEM_IMM_MEM:
        snprintf(dst, INSTR_SHOW_LEN, "%s [0x%lli] %lli", iname, instr->dst,
                 instr->src);
        break;
    case IS_SEM_MEM_REG:
        snprintf(dst, INSTR_SHOW_LEN, "%s %s [0x%lli]", iname,
                 reg_to_str[instr->dst], instr->src);
        break;
    case IS_SEM_IMM_REG:
        snprintf(dst, INSTR_SHOW_LEN, "%s %s %lli", iname,
                 reg_to_str[instr->dst], instr->src);
        break;
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
               i, instruction_line_show(&ins, instr_str),
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
    int err = parser_run(&p, bc);
    if (err < 0)
        goto panic;

    return bc;

panic:
    bc_free(bc);
    lexer_token_list_free(&tl);
    return NULL;
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
    int err = parser_run(&p, bc);
    if (err < 0)
        goto panic;

    free(buffer);

    return bc;

exit:
    free(buffer);
    fclose(fp);
    return NULL;

panic:
    free(buffer);
    bc_free(bc);
    lexer_token_list_free(&tl);
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
    int err = parser_run(&p, bc);
    if (err < 0)
        goto panic;

    fclose(fp);

    return bc;

panic:
    bc_free(bc);
    fclose(fp);
    lexer_token_list_free(&tl);
    return NULL;
}

void bc_free(Byte_Code *bc)
{
    segment_free(CODE_SEGMENT, bc->code_segment);
    segment_free(DATA_SEGMENT, bc->data_segment);
    free(bc);
}
