#include "bytecode.h"
#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
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

// TODO: Dispatch the correct encoding based on opcode
qword bc_encode_instruction(qword opcode, qword dst, qword src)
{
    qword quad_word = opcode << 56;
    quad_word |= (dst << 48);
    quad_word |= (src & ADDR_MASK);

    return quad_word;
}

// TODO: Dispatch the correct decoding based on opcode
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

Byte_Code *bc_create(void)
{
    Byte_Code *bc = calloc(1, sizeof(*bc));
    if (!bc)
        return NULL;

    bc->code = code_create();
    if (!bc->code)
        goto error;

    return bc;

error:
    free(bc);
    return NULL;
}

Byte_Code *bc_from_raw(qword *bytecode, size_t length)
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

static inline void strip_spaces(char **str)
{
    if (!*str)
        return;
    while ((isspace(**str) && **str) || **str == ',')
        ++(*str);
}

static inline void read_token(char **str, char *dest)
{

    if (!str || !dest)
        return;

    while (!isspace(**str) && **str && **str != ',')
        *dest++ = *(*str)++;
}

static Instruction_Set str_to_instruction(const char *str)
{
    if (strncmp(str, "NOP", 3) == 0)
        return NOP;
    if (strncmp(str, "HLT", 3) == 0)
        return HLT;
    if (strncmp(str, "MOV", 3) == 0)
        return MOV;
    if (strncmp(str, "CLF", 3) == 0)
        return CLF;
    if (strncmp(str, "CMP", 3) == 0)
        return CMP;
    if (strncmp(str, "CMI", 3) == 0)
        return CMI;
    if (strncmp(str, "LDI", 3) == 0)
        return LDI;
    if (strncmp(str, "LDR", 3) == 0)
        return LDR;
    if (strncmp(str, "STI", 3) == 0)
        return STI;
    if (strncmp(str, "STR", 3) == 0)
        return STR;
    if (strncmp(str, "PSR", 3) == 0)
        return PSR;
    if (strncmp(str, "PSM", 3) == 0)
        return PSM;
    if (strncmp(str, "PSI", 3) == 0)
        return PSI;
    if (strncmp(str, "POM", 3) == 0)
        return POM;
    if (strncmp(str, "POP", 3) == 0)
        return POP;
    if (strncmp(str, "ADD", 3) == 0)
        return ADD;
    if (strncmp(str, "ADI", 3) == 0)
        return ADI;
    if (strncmp(str, "SUB", 3) == 0)
        return SUB;
    if (strncmp(str, "SBI", 3) == 0)
        return SBI;
    if (strncmp(str, "MUL", 3) == 0)
        return MUL;
    if (strncmp(str, "MLI", 3) == 0)
        return MLI;
    if (strncmp(str, "DIV", 3) == 0)
        return DIV;
    if (strncmp(str, "DVI", 3) == 0)
        return DVI;
    if (strncmp(str, "INC", 3) == 0)
        return INC;
    if (strncmp(str, "DEC", 3) == 0)
        return DEC;
    if (strncmp(str, "CALL", 4) == 0)
        return CALL;
    if (strncmp(str, "RET", 3) == 0)
        return RET;
    if (strncmp(str, "JMP", 3) == 0)
        return JMP;
    if (strncmp(str, "JNE", 3) == 0)
        return JNE;
    if (strncmp(str, "JEQ", 3) == 0)
        return JEQ;
    if (strncmp(str, "JLT", 3) == 0)
        return JLT;
    if (strncmp(str, "JGE", 3) == 0)
        return JGE;

    return -1;
}

struct label {
    char name[64];
    uint64_t offset;
};

typedef struct {
    struct label names[64];
    size_t length;
} Labels;

static struct instruction parse_instruction(const char *op, const char *dst,
                                            const char *src, Labels *labels)
{
    struct instruction instr;

    instr.op = str_to_instruction(op);

    // Maybe parse dest operand
    if (!dst || *dst == '\0')
        goto exit;

    if (strncmp(dst, "AX", 2) == 0)
        instr.dst = AX;
    else if (strncmp(dst, "BX", 2) == 0)
        instr.dst = BX;
    else if (strncmp(dst, "CX", 2) == 0)
        instr.dst = CX;
    else if (strncmp(dst, "DX", 2) == 0)
        instr.dst = DX;
    else {
        if (*dst == '[') {
            // Hex value
            if (strncmp(dst + 1, "0x", 2) == 0) {
                dst += 3;
                char str[16];
                char *pstr = &str[0];
                while (dst && isxdigit(*dst))
                    *pstr++ = *dst++;

                if (*dst != ']') {
                    fprintf(stderr, "Malformed memory address\n");
                    exit(EXIT_FAILURE);
                }

                char *endptr;
                // To distinguish success/failure after call
                errno = 0;

                instr.dst = strtol(str, &endptr, 16);

                // Check for various possible errors.
                if (errno != 0) {
                    perror("strtol");
                    exit(EXIT_FAILURE);
                }

                if (endptr == str) {
                    fprintf(stderr, "No digits were found\n");
                    exit(EXIT_FAILURE);
                }
            }
        } else {
            bool found = false;
            // Label case e.g. a JMP, check for the presence of the label
            // in the labels array
            for (size_t i = 0; i < labels->length && !found; ++i) {
                if (strncmp(dst, labels->names[i].name, strlen(dst)) == 0) {
                    instr.dst = labels->names[i].offset;
                    found = true;
                }
            }

            if (!found) {
                fprintf(stderr, "Label not found\n");
                exit(EXIT_FAILURE);
            }
        }
    }

    if (!src || *src == '\0')
        goto exit;

    if (strncmp(src, "AX", 2) == 0)
        instr.src = AX;
    else if (strncmp(src, "BX", 2) == 0)
        instr.src = BX;
    else if (strncmp(src, "CX", 2) == 0)
        instr.src = CX;
    else if (strncmp(src, "DX", 2) == 0)
        instr.src = DX;
    else {
        if (*src == '[') {
            // Hex value
            if (strncmp(src + 1, "0x", 2) == 0) {
                src++;
                char str[16];
                char *pstr = &str[0];
                while (src && isxdigit(src))
                    *pstr++ = *src++;

                if (*src != ']') {
                    fprintf(stderr, "Malformed memory address\n");
                    exit(EXIT_FAILURE);
                }

                char *endptr;
                // To distinguish success/failure after call
                errno = 0;

                instr.src = strtol(str, &endptr, 16);

                // Check for various possible errors.
                if (errno != 0) {
                    perror("strtol");
                    exit(EXIT_FAILURE);
                }

                if (endptr == str) {
                    fprintf(stderr, "No digits were found\n");
                    exit(EXIT_FAILURE);
                }
            }
        } else {
            // Immediate value
            char str[16];
            char *endptr;
            char *pstr = &str[0];
            if (strncmp(src + 1, "0x", 2) == 0) {
                src++;
                while (src && isxdigit(src))
                    *pstr++ = *src++;

                // To distinguish success/failure after call
                errno = 0;

                instr.src = strtol(str, &endptr, 16);

                // Check for various possible errors.
                if (errno != 0) {
                    perror("strtol");
                    exit(EXIT_FAILURE);
                }

                if (endptr == str) {
                    fprintf(stderr, "No digits were found\n");
                    exit(EXIT_FAILURE);
                }
            } else {
                // To distinguish success/failure after call
                errno = 0;

                instr.src = strtol(src, &endptr, 10);

                // Check for various possible errors.
                if (errno != 0) {
                    perror("strtol");
                    exit(EXIT_FAILURE);
                }

                if (endptr == src) {
                    fprintf(stderr, "No digits were found\n");
                    exit(EXIT_FAILURE);
                }
            }
        }
    }

exit:
    return instr;
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
    {.name = "HLT", .dst = OP_NONE, .src = OP_NONE}

};

static const char *instruction_to_str(const struct instruction *instr,
                                      char *dst)
{
    Instruction_Def idef = instr_defs[instr->op];
    size_t nbytes = 0;

    switch (idef.dst) {
    case OP_IMM: {
        nbytes = snprintf(dst, 64, "%s %li", idef.name, instr->dst);
        break;
    }
    case OP_REG: {
        nbytes = snprintf(dst, 64, "%s %s", idef.name, reg_to_str[instr->dst]);
        break;
    }
    case OP_ADDR: {
        nbytes = snprintf(dst, 64, "%s [0x%li]", idef.name, instr->dst);
        break;
    }
    default:
        nbytes = snprintf(dst, 64, "%s", idef.name);
        break;
    }

    switch (idef.src) {
    case OP_IMM: {
        nbytes = snprintf(dst + nbytes, 64, " %li", instr->src);
        break;
    }
    case OP_REG: {
        nbytes = snprintf(dst + nbytes, 64, " %s", reg_to_str[instr->src]);
        break;
    }
    case OP_ADDR: {
        nbytes = snprintf(dst + nbytes, 64, " [0x%li]", instr->src);
        break;
    }
    default:
        break;
    }

    return dst;
}

void bc_disassemble(const Byte_Code *const bc)
{

    size_t i = 0;

    qword *code = bc_code(bc);
    char instr_str[64];

    printf("\n# Off Assembly Hex\n");
    printf("----------------------\n\n");

    while (i < bc->code->length) {
        memset(instr_str, 0x00, sizeof(instr_str));
        const struct instruction ins = bc_decode_instruction(code[i]);
        printf("%04lx %s\t(0x%lx)", i, instruction_to_str(&ins, instr_str),
               code[i]);

        printf("\n");
        i++;
    }
}

static Labels scan_labels(FILE *fp)
{
    if (!fp) {
        fprintf(stderr, "Invalid file handler\n");
        exit(EXIT_FAILURE);
    }

    Labels labels;

    char line[0xFFF], label[64];
    char *line_ptr;
    size_t line_nr = 0;

    while (fgets(line, 0xFFF, fp)) {
        line_ptr = line;

        strip_spaces(&line_ptr);

        if (*line_ptr == '\0' || *line_ptr == ';' || *line_ptr == '.')
            continue;

        read_token(&line_ptr, label);

        size_t token_length = strlen(label);

        if (label[token_length - 1] == ':') {
            if (token_length > 64) {
                fprintf(stderr, "Label too long: %s\n", label);
                exit(EXIT_FAILURE);
            }
            labels.names[labels.length].offset = line_nr;
            strncpy(labels.names[labels.length].name, label, token_length);
            labels.length++;
        }

        line_nr++;
    }

    return labels;
}

#define is_label(token) token[strlen(token) - 1] == ':'

enum { TOKEN_OP, TOKEN_DST, TOKEN_SRC, NUM_TOKENS };

Byte_Code *bc_load(const char *path)
{
    if (!path)
        return NULL;

    FILE *fp = fopen(path, "r");
    if (!fp)
        return NULL;

    Byte_Code *bc = bc_create();
    if (!bc)
        goto exit;

    char line[0xfff]; // instr[16], dst[8], src[8];
    char tokens[NUM_TOKENS][64];
    int ntokens = 0;
    size_t line_nr = 0;
    char *line_ptr;

    Labels labels = scan_labels(fp);

    rewind(fp);

    while (fgets(line, 0xFFF, fp)) {
        ntokens = 0;
        line_nr++;
        line_ptr = line;
        strip_spaces(&line_ptr);
        memset(tokens, 0x00, NUM_TOKENS * sizeof(tokens[0]));
        while (*line_ptr != '\0') {

            if (*line_ptr == '.' || *line_ptr == ';') {
                *line_ptr = '\0';
            } else {
                read_token(&line_ptr, tokens[ntokens++]);
                strip_spaces(&line_ptr);
            }
        }

        switch (ntokens) {
        case 1: {
            if (is_label(tokens[TOKEN_OP]))
                break;
            const struct instruction i =
                parse_instruction(tokens[TOKEN_OP], NULL, NULL, &labels);
            bc_push_instr(bc, bc_encode_instruction(i.op, i.dst, i.src));

            break;
        }
        case 2: {
            const struct instruction i = parse_instruction(
                tokens[TOKEN_OP], tokens[TOKEN_DST], NULL, &labels);
            bc_push_instr(bc, bc_encode_instruction(i.op, i.dst, i.src));

            break;
        }
        case 3: {
            const struct instruction i =
                parse_instruction(tokens[TOKEN_OP], tokens[TOKEN_DST],
                                  tokens[TOKEN_SRC], &labels);
            bc_push_instr(bc, bc_encode_instruction(i.op, i.dst, i.src));
            break;
        }
        default:
            break;
        }
    }

    fclose(fp);

    bc_disassemble(bc);

    return bc;

exit:
    fclose(fp);
    return NULL;
}

void bc_free(Byte_Code *bc)
{
    code_free(bc->code);
    free(bc);
}
