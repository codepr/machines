#include "parser.h"
#include "data.h"
#include "lexer.h"
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

// Boolean comparison with the following token in the list, used to add some
// simple constraints on the semantic of the source code by specifying what
// tokens can be allowed after each one parsed
static inline int parser_expect(const struct parser *p, Token_Type type)
{
    return (p->current + 1)->type == type;
}

static inline struct token *parser_peek(const struct parser *p)
{
    return (p->current + 1);
}

static inline struct token *parser_current(const struct parser *p)
{
    return p->current;
}

static inline void parser_advance(struct parser *p) { p->current++; }

static void parser_append_label(struct parser *p, const char *value,
                                Section section)
{
    strncpy(p->label_list.labels[p->label_list.length].name, value,
            strlen(value));
    if (section == DATA_SECTION) {
        p->label_list.labels[p->label_list.length].offset =
            p->label_list.base_offset;
    } else {
        p->label_list.labels[p->label_list.length].offset = p->current_address;
    }
    p->label_list.length++;
}

// - 1 byte (half-word)
// - 2 bytes (word)
// - 4 bytes (double-word)
// - 8 bytes (quad-word)
static size_t directive_multiplier[] = {1, 2, 4, 8};

static void parser_append_string(struct parser *p, Byte_Code *bc,
                                 const char *data, size_t data_len)
{
    // Skip delimiter
    if (*data == '"')
        data++;

    while (bc->data_segment->length + data_len + 1 >=
           bc->data_segment->capacity)
        da_extend(bc->data_segment);

    // TODO use memcpy or strncpy for nul inclusion
    for (size_t i = 0; i < data_len; ++i) {
        bc->data_segment->data[bc->data_segment->length++] = data[i];
    }
    bc->data_segment->data[bc->data_segment->length++] = '\0';

    p->label_list.base_offset += data_len;
}

static void parser_reserve_space(struct parser *p, Byte_Code *bc, size_t bytes)
{
    bytes *= directive_multiplier[p->current_directive];
    // Reserve bytes space in the data segment
    while (bc->data_segment->length + bytes >= bc->data_segment->capacity)
        da_extend(bc->data_segment);
    bc->data_segment->length += bytes;

    p->label_list.base_offset += bytes;
}

// An hashmap would be helpful here, the list of instructions is still pretty
// small so not a big deal to sequentially scan and compare each string
// instruction
static Instruction_Set parse_instruction(const char *str)
{
    if (strncasecmp(str, "NOP", 3) == 0)
        return OP_NOP;
    if (strncasecmp(str, "HLT", 3) == 0)
        return OP_HLT;
    if (strncasecmp(str, "MOV", 3) == 0)
        return OP_MOV;
    if (strncasecmp(str, "MOD", 3) == 0)
        return OP_MOD;
    if (strncasecmp(str, "CLF", 3) == 0)
        return OP_CLF;
    if (strncasecmp(str, "CMP", 3) == 0)
        return OP_CMP;
    if (strncasecmp(str, "PSH", 3) == 0)
        return OP_PSH;
    if (strncasecmp(str, "POP", 3) == 0)
        return OP_POP;
    if (strncasecmp(str, "ADD", 3) == 0)
        return OP_ADD;
    if (strncasecmp(str, "SUB", 3) == 0)
        return OP_SUB;
    if (strncasecmp(str, "MUL", 3) == 0)
        return OP_MUL;
    if (strncasecmp(str, "DIV", 3) == 0)
        return OP_DIV;
    if (strncasecmp(str, "INC", 3) == 0)
        return OP_INC;
    if (strncasecmp(str, "DEC", 3) == 0)
        return OP_DEC;
    if (strncasecmp(str, "CALL", 4) == 0)
        return OP_CALL;
    if (strncasecmp(str, "SYSCALL", 7) == 0)
        return OP_SYSCALL;
    if (strncasecmp(str, "RET", 3) == 0)
        return OP_RET;
    if (strncasecmp(str, "JMP", 3) == 0)
        return OP_JMP;
    if (strncasecmp(str, "JNE", 3) == 0)
        return OP_JNE;
    if (strncasecmp(str, "JLE", 3) == 0)
        return OP_JLE;
    if (strncasecmp(str, "JEQ", 3) == 0)
        return OP_JEQ;
    if (strncasecmp(str, "JLT", 3) == 0)
        return OP_JLT;
    if (strncasecmp(str, "JGT", 3) == 0)
        return OP_JGT;
    if (strncasecmp(str, "JGE", 3) == 0)
        return OP_JGE;
    if (strncasecmp(str, "AND", 3) == 0)
        return OP_AND;
    if (strncasecmp(str, "BOR", 3) == 0)
        return OP_BOR;
    if (strncasecmp(str, "XOR", 3) == 0)
        return OP_XOR;
    if (strncasecmp(str, "NOT", 3) == 0)
        return OP_NOT;
    if (strncasecmp(str, "SHL", 3) == 0)
        return OP_SHL;
    if (strncasecmp(str, "SHR", 3) == 0)
        return OP_SHR;

    return -1;
}

static int64_t parse_register(const char *value)
{
    if (strncasecmp(value, "AX", 2) == 0)
        return R_AX;
    else if (strncasecmp(value, "BX", 2) == 0)
        return R_BX;
    else if (strncasecmp(value, "CX", 2) == 0)
        return R_CX;
    else if (strncasecmp(value, "DX", 2) == 0)
        return R_DX;
    return -1;
}

static Directive parse_directive(const char *value)
{
    if (strncasecmp(value, "DB", 2) == 0)
        return D_DB;
    else if (strncasecmp(value, "DW", 2) == 0)
        return D_DW;
    else if (strncasecmp(value, "DD", 2) == 0)
        return D_DD;
    else if (strncasecmp(value, "DQ", 2) == 0)
        return D_DQ;

    return -1;
}

static inline int parser_panic(struct parser *p)
{
    fprintf(stderr, "unexpected token %s after %s (%s) at line %lu\n",
            lexer_show_token(parser_peek(p)),
            lexer_show_token(parser_current(p)), parser_current(p)->value,
            p->lines);
    return -1;
}

static int64_t parser_parse_hex(const char *value)
{

    char *endptr;
    // To distinguish success/failure after call
    errno       = 0;

    int64_t val = strtol(value, &endptr, 16);

    // Check for various possible errors.
    if (errno != 0) {
        perror("strtol");
        exit(EXIT_FAILURE);
    }

    if (endptr == value) {
        fprintf(stderr, "No digits were found\n");
        exit(EXIT_FAILURE);
    }

    return val;
}

void parser_init(struct parser *p, const struct token_list *tokens)
{
    p->lines                  = 0;
    p->tokens                 = tokens;
    p->current                = &tokens->data[0];
    // Basically the line number, this will be updated each time a NEWLINE token
    // is consumed
    p->current_address        = 0;
    p->current_directive      = D_DB;
    p->label_list.length      = 0;
    p->label_list.base_offset = DATA_OFFSET;
}

int parser_parse_source(struct parser *p, Byte_Code *bc)
{
    p->lines                                 = 1;
    struct instruction_line last_instruction = {0, IS_ATOM, -1, -1};
    // TOOD consider a dispatch table, the switch is still contianed tho
    while (parser_peek(p)->type != TOKEN_EOF) {
        struct token *current = parser_current(p);

        switch (current->type) {
        case TOKEN_LABEL:
            parser_append_label(p, current->value, current->section);
            if (current->section == DATA_SECTION) {
                if (!(parser_expect(p, TOKEN_LABEL) ||
                      parser_expect(p, TOKEN_CONSTANT) ||
                      parser_expect(p, TOKEN_DIRECTIVE) ||
                      parser_expect(p, TOKEN_STRING))) {
                    goto panic;
                }
            }
            break;
        case TOKEN_INSTR:
            if (current->section == DATA_SECTION) {
                goto panic;
            }
            Instruction_Set op = parse_instruction(current->value);
            if (op < 0) {
                fprintf(stderr, "unrcognized instruction %s\n", current->value);
                return -1;
            }
            last_instruction.op = op;
            if (parser_expect(p, TOKEN_ADDRESS))
                last_instruction.sem = IS_DST_MEM;
            else if (parser_expect(p, TOKEN_REGISTER))
                last_instruction.sem = IS_DST_REG;
            else if (parser_expect(p, TOKEN_CONSTANT))
                last_instruction.sem = IS_SRC_IMM;

            if (parser_expect(p, TOKEN_COMMENT) ||
                parser_expect(p, TOKEN_NEWLINE)) {
                bc_push_instruction(bc, &last_instruction);
                p->current_address++;
                data_reset_instruction(&last_instruction);
            }

            if (!(parser_expect(p, TOKEN_CONSTANT) ||
                  parser_expect(p, TOKEN_REGISTER) ||
                  parser_expect(p, TOKEN_ADDRESS) ||
                  parser_expect(p, TOKEN_COMMENT) ||
                  parser_expect(p, TOKEN_NEWLINE))) {
                goto panic;
            }
            break;
        case TOKEN_REGISTER:
            if (current->section == DATA_SECTION) {
                goto panic;
            }
            qword reg = parse_register(current->value);
            if (reg < 0) {
                fprintf(stderr, "unrcognized instruction %s\n", current->value);
                return -1;
            }
            if (last_instruction.dst == -1) {
                last_instruction.dst = reg;
                // PSH, POP, INC, DEC
                if (parser_expect(p, TOKEN_COMMENT) ||
                    parser_expect(p, TOKEN_NEWLINE)) {
                    last_instruction.sem = IS_DST_REG;
                    bc_push_instruction(bc, &last_instruction);
                    p->current_address++;
                    data_reset_instruction(&last_instruction);
                }
                if (!(parser_expect(p, TOKEN_CONSTANT) ||
                      parser_expect(p, TOKEN_REGISTER) ||
                      parser_expect(p, TOKEN_COMMA) ||
                      parser_expect(p, TOKEN_COMMENT) ||
                      parser_expect(p, TOKEN_NEWLINE))) {
                    goto panic;
                }
            } else {
                // MOV, ADD, SUB, NUL, DIV, AND, BOR, XOR, NOT
                last_instruction.src = reg;
                last_instruction.sem |= IS_SRC_REG;
                if (!(parser_expect(p, TOKEN_COMMENT) ||
                      parser_expect(p, TOKEN_NEWLINE))) {
                    goto panic;
                }
                bc_push_instruction(bc, &last_instruction);
                p->current_address++;
                data_reset_instruction(&last_instruction);
            }
            break;
        case TOKEN_STRING:
            if (current->section != DATA_SECTION) {
                goto panic;
            }
            // COMMA
            parser_advance(p);
            (void)parser_current(p);
            // LEN
            parser_advance(p);
            struct token *next = parser_current(p);
            if (next->type != TOKEN_CONSTANT)
                goto panic;
            size_t len = atoll(next->value);
            // TODO check for nul
            parser_append_string(p, bc, current->value, len);
            if (!(parser_expect(p, TOKEN_COMMENT) ||
                  parser_expect(p, TOKEN_NEWLINE))) {
                goto panic;
            }
            break;
        case TOKEN_CONSTANT:
            if (current->section == DATA_SECTION) {
                qword constant = (strncmp(current->value, "0x", 2) == 0)
                                     ? parser_parse_hex(current->value)
                                     : atoll(current->value);
                parser_reserve_space(p, bc, constant);
                if (!(parser_expect(p, TOKEN_NEWLINE) ||
                      parser_expect(p, TOKEN_COMMA) ||
                      parser_expect(p, TOKEN_COMMENT))) {
                    goto panic;
                }
            } else {
                // PSH, MOV, ADD, SUB, NUL, DIV, AND, BOR, XOR, NOT
                if (!(parser_expect(p, TOKEN_COMMENT) ||
                      parser_expect(p, TOKEN_NEWLINE))) {
                    goto panic;
                } else {
                    last_instruction.sem |= IS_SRC_IMM;
                }
                last_instruction.src = (strncmp(current->value, "0x", 2) == 0)
                                           ? parser_parse_hex(current->value)
                                           : atoll(current->value);

                bc_push_instruction(bc, &last_instruction);
                p->current_address++;
                data_reset_instruction(&last_instruction);
            }
            break;
        case TOKEN_ADDRESS:
            if (!(parser_expect(p, TOKEN_COMMENT) ||
                  parser_expect(p, TOKEN_NEWLINE))) {
                goto panic;
            } else {
                if (last_instruction.sem != IS_DST_MEM)
                    last_instruction.sem |= IS_SRC_MEM;
            }

            // Label case e.g. a JMP, check for the presence of the
            // label in the labels array
            for (size_t i = 0; i < p->label_list.length; ++i) {
                if (strncmp(current->value, p->label_list.labels[i].name,
                            strlen(current->value)) == 0) {
                    if (last_instruction.dst == -1)
                        last_instruction.dst = p->label_list.labels[i].offset;
                    else
                        last_instruction.src = p->label_list.labels[i].offset;

                    bc_push_instruction(bc, &last_instruction);
                    p->current_address++;
                    data_reset_instruction(&last_instruction);
                    break;
                }
            }
            break;
        case TOKEN_SECTION:
            break;
        case TOKEN_DIRECTIVE:
            p->current_directive = parse_directive(current->value);
            break;
        case TOKEN_COMMA:
            if (parser_expect(p, TOKEN_REGISTER))
                last_instruction.sem |= IS_SRC_REG;
            else if (parser_expect(p, TOKEN_CONSTANT))
                last_instruction.sem |= IS_SRC_IMM;
            else if (parser_expect(p, TOKEN_ADDRESS))
                last_instruction.sem |= IS_SRC_MEM;
            break;
        case TOKEN_NEWLINE:
            p->lines++;
            break;
        case TOKEN_COMMENT:
            if (!(parser_expect(p, TOKEN_NEWLINE) ||
                  parser_expect(p, TOKEN_EOF))) {
                goto panic;
            }
            break;
        case TOKEN_UNKNOWN:
            fprintf(stderr, "unexpected token %s at line %ld\n",
                    parser_current(p)->value, p->lines);
            return -1;
            break;
        case TOKEN_EOF:
            break;
        }
        parser_advance(p);
    }

    bc->data_addr = p->label_list.base_offset;

    return 0;

panic:

    return parser_panic(p);
}
