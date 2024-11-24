#include "assembler.h"
#include "bytecode.h"
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define SECTION_START    '.'
#define COMMENT_START    '#'
#define LABEL_END        ':'
#define NEWLINE          '\n'
#define TOKEN_VALUE_SIZE 512

static void parser_panic(const char *fmt, ...)
{

    assert(fmt);

    va_list ap;

    va_start(ap, fmt);
    fprintf(stderr, fmt, ap);
    va_end(ap);

    exit(EXIT_FAILURE);
}

typedef enum { DATA_SECTION, MAIN_SECTION } Section;

//
//  *******************
//  * MAPPING HELPERS *
//  ******************
//

//  These static maps are used to determine the token types during the lexical
//  analysis of the source code
const char *const instructions_table[] = {
    "LOAD",       "LOAD_CONST",  "STORE", "STORE_CONST", "CALL", "PUSH",
    "PUSH_CONST", "ADD",         "SUB",   "MUL",         "DIV",  "DUP",
    "INC",        "EQ",          "JMP",   "JEQ",         "JNE",  "MAKE_TUPLE",
    "PRINT",      "PRINT_CONST", "RET",   "HALT",        NULL};

static const char *tokens[] = {"TOKEN_LABEL",
                               "TOKEN_INSTR",
                               "TOKEN_STRING",
                               "TOKEN_CONSTANT",
                               "TOKEN_ADDRESS",
                               "TOKEN_SECTION",
                               "TOKEN_DIRECTIVE",
                               "TOKEN_COMMA",
                               "TOKEN_NEWLINE",
                               "TOKEN_COMMENT",
                               "TOKEN_UNKNOWN",
                               "TOKEN_EOF",
                               NULL};

typedef enum {
    TOKEN_LABEL,
    TOKEN_INSTR,
    TOKEN_STRING,
    TOKEN_CONSTANT,
    TOKEN_ADDRESS,
    TOKEN_SECTION,
    TOKEN_DIRECTIVE,
    TOKEN_COMMA,
    TOKEN_NEWLINE,
    TOKEN_COMMENT,
    TOKEN_UNKNOWN,
    TOKEN_EOF,
} Token_Type;

struct token {
    Token_Type type;
    Section section;
    char value[TOKEN_VALUE_SIZE];
};

struct token_list {
    struct token *data;
    size_t length;
    size_t capacity;
};

struct lexer {
    const char *buffer;
    size_t size;
    size_t pos;
};

static void lexer_init(struct lexer *l, char *buffer, size_t size)
{
    l->size   = size;
    l->buffer = buffer;
    l->pos    = 0;
}

static inline char lexer_peek(const struct lexer *l)
{
    if (l->pos >= l->size)
        return EOF;
    return l->buffer[l->pos];
}

static inline void lexer_strip_spaces(struct lexer *l)
{
    if (l->pos == l->size)
        return;

    while (lexer_peek(l) != NEWLINE && isspace(l->buffer[l->pos]) &&
           l->pos < l->size)
        l->pos++;
}

static inline char lexer_next_char(struct lexer *l)
{
    char c = lexer_peek(l);
    l->pos++;
    return c;
}

// Assume nul characteer always present
#define is_label(token)   ((token)[strlen(token) - 1] == LABEL_END)
#define is_section(token) ((token)[0] == SECTION_START)
#define is_comment(token) ((token)[0] == COMMENT_START)

static int is_instruction(const char *token)
{
    for (int i = 0; instructions_table[i] != NULL; ++i) {
        if (strncasecmp(token, instructions_table[i], strlen(token)) == 0) {
            return 1;
        }
    }
    return 0;
}

static int lexer_next(struct lexer *l, struct token *t, Token_Type prev)
{
    // Skipt whitespaces
    lexer_strip_spaces(l);

    // End of the lexing
    if (l->pos >= l->size) {
        t->type     = TOKEN_EOF;
        t->value[0] = EOF;
        return EOF;
    }

    char c = lexer_peek(l);

    if (c == NEWLINE) {
        lexer_next_char(l);
        t->type     = TOKEN_NEWLINE;
        t->value[0] = NEWLINE;
        return 1;
    }

    if (c == ',') {
        lexer_next_char(l);
        t->type     = TOKEN_COMMA;
        t->value[0] = ',';
        return 1;
    }

    size_t i = 0;
    // String
    if (c == '"' || c == '\'') {
        t->type = TOKEN_STRING;
        // Skip quotes
        lexer_next_char(l);
        do {
            t->value[i++] = l->buffer[l->pos++];
        } while (lexer_peek(l) != '\'' && lexer_peek(l) != '"' &&
                 lexer_peek(l) != NEWLINE);
        if (lexer_peek(l) == '"' || lexer_peek(l) == '\'')
            lexer_next_char(l);
        goto end;
    } else if (c == COMMENT_START) {
        t->type = TOKEN_COMMENT;
        while (lexer_peek(l) != NEWLINE) {
            t->value[i++] = lexer_next_char(l);
        }
        goto end;
    } else if (isdigit(c)) {
        // Numbers constants
        t->type = TOKEN_CONSTANT;
        while (lexer_peek(l) != ',' && lexer_peek(l) != ' ' &&
               lexer_peek(l) != NEWLINE) {
            t->value[i++] = lexer_next_char(l);
        }
        goto end;
    } else if (c == '[') {
        t->type = TOKEN_ADDRESS;
        // Skip the opening bracket
        lexer_next_char(l);
        while (lexer_peek(l) != ']') {
            t->value[i++] = lexer_next_char(l);
        }
        // Skip the closing bracket
        lexer_next_char(l);
        goto end;
    } else if (c == '@') {
        t->type = TOKEN_ADDRESS;
        // Skip the opening bracket
        lexer_next_char(l);
        while (isdigit(lexer_peek(l))) {
            t->value[i++] = lexer_next_char(l);
        }
        // Skip the closing bracket
        lexer_next_char(l);
        goto end;
    } else {
        // labels / sections / instructions
        while (lexer_peek(l) != ' ' && lexer_peek(l) != ',' &&
               lexer_peek(l) != NEWLINE) {
            t->value[i++] = lexer_next_char(l);
        }

        if (is_label(t->value)) {
            t->type = TOKEN_LABEL;
        } else if (is_section(t->value)) {
            t->type = TOKEN_SECTION;
        } else if (is_instruction(t->value)) {
            t->type = TOKEN_INSTR;
        } else if (prev == TOKEN_INSTR || prev == TOKEN_COMMA) {
            t->type = TOKEN_ADDRESS;
        } else {
            t->type = TOKEN_UNKNOWN;
        }
    }

end:
    return 1;
}

static int lexer_tokenize_stream(FILE *fp, struct token_list *tokens)
{
    char line[0xFFF];
    struct token t;
    struct lexer l;
    memset(&t, 0x00, sizeof(struct token));
    Token_Type prev = TOKEN_UNKNOWN;
    Section section = DATA_SECTION;
    while (fgets(line, 0xFFF, fp)) {
        lexer_init(&l, line, strlen(line));
        while (lexer_next(&l, &t, prev) != EOF) {
            if (section != MAIN_SECTION &&
                strncasecmp(t.value, ".main", 5) == 0)
                section = MAIN_SECTION;
            t.section = section;
            da_push(tokens, t);
            prev = t.type;
            memset(&t, 0x00, sizeof(struct token));
        }
        memset(line, 0x0, 0xFFF);
    }
    // EOF
    lexer_next(&l, &t, prev);
    t.section = section;
    da_push(tokens, t);

    return 0;
}

static void lexer_token_list_init(struct token_list *tl, size_t capacity)
{
    da_init(tl, capacity);
}

static struct token_list lexer_token_list_create(size_t capacity)
{
    struct token_list tokens;
    lexer_token_list_init(&tokens, capacity);
    return tokens;
}

static void lexer_token_list_free(struct token_list *tl) { free(tl->data); }

static const char *lexer_show_token(const struct token *t)
{
    return tokens[t->type];
}

static void lexer_print_tokens(const struct token_list *tl)
{
    for (int i = 0; i < tl->length; ++i) {
        if (tl->data[i].type == TOKEN_NEWLINE) {
            printf("%s (%d)\n", lexer_show_token(&tl->data[i]),
                   tl->data[i].type);
        } else {
            printf("%s (%d), value = %s\n", lexer_show_token(&tl->data[i]),
                   tl->data[i].type, tl->data[i].value);
        }
    }
}

/*
 *  PARSER PRIMITIVES HELPERS
 */

// Struct representing the parser for source code, which processes a list of
// tokens and maintains state such as current token, current address, and label
// information.
struct parser {
    // Pointer to the list of tokens to be parsed
    const struct token_list *tokens;
    // Pointer to the current token being processed
    struct token *current;
    // Number of lines parsed (used for error reporting or tracking)
    size_t lines;
};

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

static inline struct token *parser_next(struct parser *p)
{
    parser_advance(p);
    return parser_current(p);
}

/*
 * TOKEN HELPERS
 */

// An hashmap would be helpful here, the list of instructions is still pretty
// small so not a big deal to sequentially scan and compare each string
// instruction
static Instruction_ID parse_instruction(const char *str)
{
    if (strncasecmp(str, "LOAD_CONST", 10) == 0)
        return OP_LOAD_CONST;
    if (strncasecmp(str, "LOAD", 4) == 0)
        return OP_LOAD;
    if (strncasecmp(str, "STORE_CONST", 11) == 0)
        return OP_STORE_CONST;
    if (strncasecmp(str, "STORE", 5) == 0)
        return OP_STORE;
    if (strncasecmp(str, "CALL", 4) == 0)
        return OP_CALL;
    if (strncasecmp(str, "PUSH_CONST", 10) == 0)
        return OP_PUSH_CONST;
    if (strncasecmp(str, "PUSH", 4) == 0)
        return OP_PUSH;
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
    if (strncasecmp(str, "RET", 3) == 0)
        return OP_RET;
    if (strncasecmp(str, "JMP", 3) == 0)
        return OP_JMP;
    if (strncasecmp(str, "JNE", 3) == 0)
        return OP_JNE;
    if (strncasecmp(str, "MAKE_TUPLE", 10) == 0)
        return OP_MAKE_TUPLE;
    if (strncasecmp(str, "JEQ", 3) == 0)
        return OP_JEQ;
    if (strncasecmp(str, "RET", 3) == 0)
        return OP_RET;
    if (strncasecmp(str, "DUP", 3) == 0)
        return OP_DUP;
    if (strncasecmp(str, "EQ", 2) == 0)
        return OP_EQ;
    if (strncasecmp(str, "PRINT_CONST", 11) == 0)
        return OP_PRINT_CONST;
    if (strncasecmp(str, "PRINT", 5) == 0)
        return OP_PRINT;
    if (strncasecmp(str, "HALT", 4) == 0)
        return OP_HALT;
    return -1;
}

static uint64_t parse_constant(const char *value)
{
    char *endptr;
    // To distinguish success/failure after call
    errno = 0;
    uint64_t val;

    if (strncmp(value, "0x", 2) == 0)
        val = strtol(value, &endptr, 16);
    else
        val = strtol(value, &endptr, 10);

    // Check for various possible errors.
    if (errno != 0)
        parser_panic("error parsing constant: %s\n", strerror(errno));

    if (endptr == value)
        parser_panic("no digits were found\n", strerror(errno));

    return val;
}

static void parser_init(struct parser *p, const struct token_list *tokens)
{
    p->lines   = 0;
    p->tokens  = tokens;
    p->current = &tokens->data[0];
}

static struct parser parser_create(const struct token_list *tokens)
{
    struct parser p;
    parser_init(&p, tokens);
    return p;
}

// Rudimentary sequential scan of the token list, the absence of scope
// makes it fairly easy to keep a sort of context for each line and define
// what it's expected to be found after each token type.
//
// It makes 2 passages before encoding the instructions to bytecode, to
// handle cases where a label is referenced before having being parsed and
// thus the code position is yet to be known
// e.g.
// jmp exit
// ..
// exit:
// htl
static Byte_Code *parser_run(struct parser *p)
{
    Byte_Code *bc      = bc_create();
    struct token *curr = parser_current(p);
    while (parser_peek(p)->type != TOKEN_EOF) {
        switch (curr->type) {
        case TOKEN_ADDRESS:
            if (curr->section != DATA_SECTION)
                break;
            Data_Record record = {.address = parse_constant(curr->value)};
            curr               = parser_next(p);
            if (curr->type == TOKEN_STRING) {
                if (record.address < DATA_STRING_OFFSET) {
                    bc_free(bc);
                    parser_panic("data offset must start at %d, found %lld at "
                                 "line %ld\n",
                                 DATA_STRING_OFFSET, record.address, p->lines);
                }
                record.type = DT_STRING;
                strncpy(record.as_str, curr->value, strlen(curr->value));
            } else if (curr->type == TOKEN_CONSTANT) {
                if (record.address < DATA_OFFSET) {
                    bc_free(bc);
                    parser_panic("data offset must start at %d, found %lld at "
                                 "line %ld\n",
                                 DATA_OFFSET, record.address, p->lines);
                }
                record.type   = DT_CONSTANT;
                record.as_int = parse_constant(curr->value);
            } else {
                bc_free(bc);
                parser_panic(
                    "unexpected token in .data section %s at line %ld\n",
                    curr->value, p->lines);
            }

            da_push(bc->data_segment, record);

            break;
        case TOKEN_CONSTANT: {
            // Address
            if (parser_expect(p, TOKEN_COMMENT) ||
                parser_expect(p, TOKEN_NEWLINE))
                break;
            // OP code
            curr                   = parser_next(p);
            Instruction_ID op_code = parse_instruction(curr->value);
            da_push(bc->code_segment, op_code);
            if (parser_expect(p, TOKEN_CONSTANT) ||
                parser_expect(p, TOKEN_ADDRESS)) {
                curr = parser_next(p);
                da_push(bc->code_segment, parse_constant(curr->value));
            }
            break;
        }
        case TOKEN_UNKNOWN:
            bc_free(bc);
            parser_panic("unexpected token %s at line %ld\n",
                         parser_current(p)->value, p->lines);
        default:
            // Skip all the rest for now
            break;
        }

        p->lines++;
        curr = parser_next(p);
    }
    return bc;
}

/*
 * MAIN EXPOSED FUNCTION
 */

void asm_disassemble(const Byte_Code *bc)
{
    size_t i = 0;

    if (bc->data_segment->length > 0) {
        printf(".data\n");
        for (int i = 0; i < bc->data_segment->length; ++i) {
            if (bc->data_segment->data[i].type == DT_CONSTANT)
                printf("\t@%04llu %04llu\n", bc->data_segment->data[i].address,
                       bc->data_segment->data[i].as_int);
            else
                printf("\t@%04llu \"%s\"\n", bc->data_segment->data[i].address,
                       bc->data_segment->data[i].as_str);
        }
        printf("\n");
    }
    printf(".main\n");
    i = 0;
    while (i < bc->code_segment->length) {
        printf("\t%04li %-11s", i,
               instructions_table[bc->code_segment->data[i]]);
        if (bc_nary_instruction(bc->code_segment->data[i])) {
            switch (bc->code_segment->data[i]) {
            case OP_PUSH:
                printf(" @%04llu", bc->code_segment->data[++i]);
                break;
            case OP_CALL:
                printf(" (%04llu)", bc->code_segment->data[++i]);
                break;
            case OP_JMP:
            case OP_JNE:
            case OP_JEQ:
            case OP_LOAD_CONST:
            case OP_STORE_CONST:
                // printf(" [%04llu]", bc->code_segment->data[++i]);
                printf(" [0x%02llX]", bc->code_segment->data[++i]);
                break;
            default:
                printf(" %04llu", bc->code_segment->data[++i]);
                break;
            }
        }
        printf("\n");
        i++;
    }
}

Byte_Code *asm_compile(const char *path, int debug)
{
    if (!path)
        return NULL;

    FILE *fp = fopen(path, "r");
    if (!fp)
        return NULL;

    struct token_list tl = lexer_token_list_create(24);

    int err              = lexer_tokenize_stream(fp, &tl);
    if (err < 0)
        goto errdefer;

    fclose(fp);

    if (debug) {
        printf("\n");
        printf("=====================\n");
        printf("[*] Lexical analysis\n");
        printf("=====================\n\n");
        lexer_print_tokens(&tl);
        printf("\n");
    }

    struct parser p = parser_create(&tl);
    Byte_Code *bc   = parser_run(&p);

    lexer_token_list_free(&tl);
    return bc;

errdefer:
    lexer_token_list_free(&tl);

    return NULL;
}
