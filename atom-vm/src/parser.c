#include "parser.h"
#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <string.h>

#define NEWLINE          '\n'
#define LABEL_END        ':'
#define SECTION_START    '.'
#define COMMENT_START    '#'

#define TOKEN_VALUE_SIZE 512

typedef enum { DATA_SECTION, MAIN_SECTION } Section;
typedef enum {
    TOKEN_LABEL,
    TOKEN_INSTR,
    TOKEN_STRING,
    TOKEN_CONSTANT,
    TOKEN_ADDRESS,
    TOKEN_SECTION,
    TOKEN_PROC_DEF,
    TOKEN_PROC,
    TOKEN_DIRECTIVE,
    TOKEN_COMMA,
    TOKEN_NEWLINE,
    TOKEN_COMMENT,
    TOKEN_UNKNOWN,
    TOKEN_EOF,
} Token_Type;

static const char *directives_table[] = {"db", "dw", "dd", "dq", "rb",
                                         "rw", "rd", "rq", NULL};

static const char *tokens[]           = {
    "TOKEN_LABEL",    "TOKEN_INSTR",   "TOKEN_STRING",
    "TOKEN_CONSTANT", "TOKEN_ADDRESS", "TOKEN_SECTION",
    "TOKEN_PROC_DEF", "TOKEN_PROC",    "TOKEN_DIRECTIVE",
    "TOKEN_COMMA",    "TOKEN_NEWLINE", "TOKEN_COMMENT",
    "TOKEN_UNKNOWN",  "TOKEN_EOF",     NULL};

// =============
// LEXER APIs
// =============

struct token {
    Token_Type type;
    Section section;
    char value[TOKEN_VALUE_SIZE];
    size_t value_len;
};

typedef struct lexer {
    const char *buffer;
    size_t size;
    size_t pos;
} Lexer;

static void lexer_init(Lexer *l, const char *buffer, size_t size)
{
    l->size   = size;
    l->buffer = buffer;
    l->pos    = 0;
}

static inline char lexer_peek(const Lexer *l)
{
    if (l->pos >= l->size)
        return EOF;
    return l->buffer[l->pos];
}

static inline void lexer_strip_spaces(Lexer *l)
{
    if (l->pos == l->size)
        return;

    while (lexer_peek(l) != NEWLINE && isspace(l->buffer[l->pos]) &&
           l->pos < l->size)
        l->pos++;
}

static inline char lexer_next_char(Lexer *l)
{
    char c = lexer_peek(l);
    l->pos++;
    return c;
}

// Assume nul characteer always present
#define is_label(token)    ((token)[strlen(token) - 1] == LABEL_END)
#define is_section(token)  ((token)[0] == SECTION_START)
#define is_proc_def(token) (strncasecmp((token), ".PROC", strlen(token)) == 0)
#define is_comment(token)  ((token)[0] == COMMENT_START)
#define is_hexvalue(token) (strncasecmp(token, "0x", 2) == 0)

static bool is_label_name(const char *str)
{
    if (is_hexvalue(str))
        return false;

    for (int i = 0; str[i] != '\0'; i++) {
        if (isalpha(str[i]))
            return true;
    }

    return false;
}

static int is_instruction(const char *token)
{
    for (int i = 0; instructions_table[i] != NULL; ++i) {
        if (strncasecmp(token, instructions_table[i], strlen(token)) == 0) {
            return 1;
        }
    }
    return 0;
}

static int is_directive(const char *token)
{
    for (int i = 0; directives_table[i] != NULL; ++i) {
        if (strncasecmp(token, directives_table[i], strlen(token)) == 0) {
            return 1;
        }
    }
    return 0;
}

static int lexer_next(Lexer *l, Token *t, Token_Type prev)
{
    // Skip whitespaces
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
        t->value_len = i;
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
            t->type = prev == TOKEN_PROC_DEF ? TOKEN_PROC : TOKEN_LABEL;
        } else if (is_proc_def(t->value)) {
            t->type = TOKEN_PROC_DEF;
        } else if (is_section(t->value)) {
            t->type = TOKEN_SECTION;
        } else if (is_instruction(t->value)) {
            t->type = TOKEN_INSTR;
        } else if (is_directive(t->value)) {
            t->type = TOKEN_DIRECTIVE;
        } else if (prev == TOKEN_INSTR || prev == TOKEN_COMMA) {
            t->type = TOKEN_ADDRESS;
        } else {
            t->type = TOKEN_UNKNOWN;
        }
    }

end:
    return 1;
}

static int lexer_tokenize_stream(FILE *fp, Token_List *tokens)
{
    char line[0xFFF];
    Token t;
    Lexer l;
    memset(&t, 0x00, sizeof(Token));
    Token_Type prev = TOKEN_UNKNOWN;
    Section section = DATA_SECTION;
    while (fgets(line, 0xFFF, fp)) {
        lexer_init(&l, line, strlen(line));
        while (lexer_next(&l, &t, prev) != EOF) {
            if (section != MAIN_SECTION &&
                (strncasecmp(t.value, ".main", 5) == 0 ||
                 t.type == TOKEN_PROC_DEF))
                section = MAIN_SECTION;
            t.section = section;
            da_push(tokens, t);
            prev = t.type;
            memset(&t, 0x00, sizeof(Token));
        }
        memset(line, 0x0, 0xFFF);
    }
    // EOF
    lexer_next(&l, &t, prev);
    t.section = section;
    da_push(tokens, t);

    return 0;
}

static Token_List *lexer_token_list_create(size_t capacity)
{
    Token_List *tokens = calloc(1, sizeof(*tokens));
    da_init(tokens, capacity);
    return tokens;
}

static void lexer_token_list_free(Token_List *tl)
{
    free(tl->data);
    free(tl);
}

static const char *lexer_show_token(const Token *t) { return tokens[t->type]; }

// ================
// PARSER UTILITIES
// ================

#define SYMBOL_NAME_SIZE  64
#define SYMBOL_TABLE_SIZE 128

typedef struct symbol_entry {
    char name[SYMBOL_NAME_SIZE];
    size_t offset;
    struct symbol_entry *next;
} Symbol_Entry;

struct unresolved_symbol {
    char name[SYMBOL_NAME_SIZE];
    int addr;
};

typedef struct unresolved_symbol_list {
    size_t capacity;
    size_t length;
    struct unresolved_symbol *data;
} Unresolved_List;

typedef struct symbol_table {
    Symbol_Entry *entries[SYMBOL_TABLE_SIZE];
    Unresolved_List unresolved_list;
} Symbol_Table;

static Symbol_Table symbol_table = {0};

static inline uint32_t simple_hash(const uint8_t *in)
{
    unsigned int h = 0;
    while (*in)
        h = (h * 31) + *(in++);
    return h;
}

static void symbol_put(const void *name, size_t offset)
{
    unsigned index      = simple_hash(name) % SYMBOL_TABLE_SIZE;
    Symbol_Entry *entry = calloc(1, sizeof(*entry));
    if (!entry)
        return;

    strncpy(entry->name, name, SYMBOL_NAME_SIZE);
    entry->offset = offset;

    if (symbol_table.entries[index] &&
        strncmp(symbol_table.entries[index]->name, name, SYMBOL_NAME_SIZE) ==
            0) {
        free(symbol_table.entries[index]);
        symbol_table.entries[index] = entry;
    } else {
        entry->next                 = symbol_table.entries[index];
        symbol_table.entries[index] = entry;
    }
}

ssize_t symbol_get(const void *name)
{
    unsigned index      = simple_hash(name) % SYMBOL_TABLE_SIZE;
    Symbol_Entry *entry = symbol_table.entries[index];

    while (entry) {
        if (strncmp(entry->name, name, SYMBOL_NAME_SIZE) == 0)
            return entry->offset;
        entry = entry->next;
    }

    return -1;
}

static void symbol_add_unresolved(const char *name, int addr)
{
    if (!symbol_table.unresolved_list.data)
        symbol_table.unresolved_list.data =
            calloc(4, sizeof(*symbol_table.unresolved_list.data));
    struct unresolved_symbol symbol = {.addr = addr};
    snprintf(symbol.name, 64, "%s:", name);
    da_push(&symbol_table.unresolved_list, symbol);
}

static void parser_panic(const char *fmt, ...)
{
    assert(fmt);

    va_list ap;

    va_start(ap, fmt);
    fprintf(stderr, fmt, ap);
    va_end(ap);

    exit(EXIT_FAILURE);
}

int parser_init(FILE *fp, Parser *p)
{
    Token_List *tokens = lexer_token_list_create(24);

    int err            = lexer_tokenize_stream(fp, tokens);
    if (err < 0)
        goto errdefer;

    p->lines             = 0;
    p->tokens            = tokens;
    p->current_directive = D_DB;
    p->current           = &tokens->data[0];
    p->current_address   = 0;

    return 0;

errdefer:
    lexer_token_list_free(tokens);

    return -1;
}

// Boolean comparison with the following token in the list, used to add some
// simple constraints on the semantic of the source code by specifying what
// tokens can be allowed after each one parsed
static inline int parser_expect(const Parser *p, Token_Type type)
{
    return (p->current + 1)->type == type;
}

static inline Token *parser_peek(const Parser *p) { return (p->current + 1); }

static inline Token *parser_current(const Parser *p) { return p->current; }

static inline void parser_advance(Parser *p) { p->current++; }

static inline Token *parser_next(Parser *p)
{
    parser_advance(p);
    return parser_current(p);
}

static bool assert_next_token(const Parser *p)
{
    switch (parser_current(p)->type) {
    case TOKEN_LABEL:
        return (parser_expect(p, TOKEN_INSTR) ||
                parser_expect(p, TOKEN_DIRECTIVE));
    case TOKEN_INSTR:
        return (parser_expect(p, TOKEN_CONSTANT) ||
                parser_expect(p, TOKEN_ADDRESS) ||
                parser_expect(p, TOKEN_COMMENT) ||
                parser_expect(p, TOKEN_NEWLINE));
    case TOKEN_STRING:
    case TOKEN_CONSTANT:
        return (parser_expect(p, TOKEN_COMMA) ||
                parser_expect(p, TOKEN_COMMENT) ||
                parser_expect(p, TOKEN_NEWLINE));
    case TOKEN_COMMA:
        return (parser_expect(p, TOKEN_CONSTANT) ||
                parser_expect(p, TOKEN_COMMENT) ||
                parser_expect(p, TOKEN_NEWLINE));
    case TOKEN_ADDRESS:
        return (parser_expect(p, TOKEN_COMMENT) ||
                parser_expect(p, TOKEN_NEWLINE));
    case TOKEN_COMMENT:
        return (parser_expect(p, TOKEN_NEWLINE) || parser_expect(p, TOKEN_EOF));
    case TOKEN_DIRECTIVE:
        return (parser_expect(p, TOKEN_STRING) ||
                parser_expect(p, TOKEN_CONSTANT));
    case TOKEN_SECTION:
        return (parser_expect(p, TOKEN_COMMENT) ||
                parser_expect(p, TOKEN_NEWLINE));
    case TOKEN_PROC_DEF:
        return parser_expect(p, TOKEN_PROC);
    default:
        break;
    }

    return true;
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
    errno        = 0;
    int base     = is_hexvalue(value) ? 16 : 10;
    uint64_t val = strtol(value, &endptr, base);

    // Check for various possible errors.
    if (errno != 0)
        parser_panic("error parsing constant: %s\n", strerror(errno));

    if (endptr == value)
        parser_panic("no digits were found\n", strerror(errno));

    return val;
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
    else if (strncasecmp(value, "RB", 2) == 0)
        return D_RB;
    else if (strncasecmp(value, "RW", 2) == 0)
        return D_RW;
    else if (strncasecmp(value, "RD", 2) == 0)
        return D_RD;
    else if (strncasecmp(value, "RQ", 2) == 0)
        return D_RQ;

    return -1;
}

// RD_DATA_OFFSET 1024
static void store_constant(Byte_Code *bc, uint64_t constant)
{
    Data_Record record = {.type    = DT_CONSTANT,
                          .address = bc->data_segment->rd_data_addr_offset++,
                          .as_int  = constant};
    da_push(bc->data_segment, record);
}

// RD_STRING_OFFSET 2048
static void store_string(Byte_Code *bc, const char *data, size_t len)
{
    Data_Record record = {.type    = DT_STRING,
                          .address = bc->data_segment->rd_string_addr_offset};

    size_t i           = 0;
    while (len--)
        record.as_str[i++] = *data++;

    // nul
    record.as_str[i] = 0;
    bc->data_segment->rd_string_addr_offset += i;

    da_push(bc->data_segment, record);
}

// - 1 byte (half-word)
// - 2 bytes (word)
// - 4 bytes (double-word)
// - 8 bytes (quad-word)
// Skip the first 4 bytes as the directives for space reservation
// start at R_*
static size_t directive_multiplier[] = {0, 0, 0, 0, 1, 2, 4, 8};

// RW_BUFFER_OFFSET 4096
static void reserve_space(Byte_Code *bc, size_t count, Directive directive)
{
    size_t bytes       = count * directive_multiplier[directive];

    // Create a buffer record
    Data_Record record = {
        .type    = DT_BUFFER,
        .address = bc->data_segment->rw_data_addr_offset,
        .as_int  = bytes // Store the buffer size in bytes
    };

    da_push(bc->data_segment, record);

    // Advance the address offset for the next data item
    bc->data_segment->rw_data_addr_offset += bytes;
}

static int parse_data_section_token(Parser *p, Byte_Code *bc)
{
    Token *cur = parser_current(p);

    switch (cur->type) {
    case TOKEN_LABEL: {
        const char *label_name = cur->value;
        if (!parser_expect(p, TOKEN_DIRECTIVE))
            goto parser_error;

        cur                 = parser_next(p);
        Directive directive = parse_directive(cur->value);

        if (directive < D_DB || directive > NUM_DIRECTIVES) {
            fprintf(stderr, "unknown directive %s at line %lu\n", cur->value,
                    p->lines);
            return -1;
        }

        if (directive >= D_RB) {
            if (!parser_expect(p, TOKEN_CONSTANT))
                goto parser_error;

            cur = parser_next(p);
            symbol_put(label_name, bc->data_segment->rw_data_addr_offset);
            reserve_space(bc, parse_constant(cur->value), directive);
        } else {
            if (!parser_expect(p, TOKEN_CONSTANT) &&
                !parser_expect(p, TOKEN_STRING))
                goto parser_error;

            cur = parser_next(p);

            if (cur->type == TOKEN_CONSTANT) {
                symbol_put(label_name, bc->data_segment->rd_data_addr_offset);
                store_constant(bc, parse_constant(cur->value));
            } else {
                symbol_put(label_name, bc->data_segment->rd_string_addr_offset);
                store_string(bc, cur->value, cur->value_len);
            }
        }
        break;
    }
    case TOKEN_SECTION:
    case TOKEN_COMMENT:
    case TOKEN_NEWLINE:
    case TOKEN_COMMA:
        // Valid - just skip these and exit
        break;
    default:
        goto parser_error;
    }

    return 0;

parser_error:

    fprintf(stderr, "unexpected token in .data %s after %s (%s) at line %lu\n",
            lexer_show_token(parser_peek(p)),
            lexer_show_token(parser_current(p)), parser_current(p)->value,
            p->lines);
    return -1;
}

static int parse_main_section_token(Parser *p, Byte_Code *bc)
{
    Token *cur = parser_current(p);

    switch (cur->type) {
    case TOKEN_LABEL:
        symbol_put(cur->value, p->current_address);
        break;
    case TOKEN_PROC_DEF:
        if (!parser_expect(p, TOKEN_PROC))
            goto parser_error;
        cur = parser_next(p);
        symbol_put(cur->value, p->current_address);
        break;
    case TOKEN_INSTR: {
        Instruction_ID op_code = parse_instruction(cur->value);
        da_push(bc->code_segment, op_code);
        p->current_address++;
        if (parser_expect(p, TOKEN_CONSTANT) ||
            parser_expect(p, TOKEN_ADDRESS)) {
            cur = parser_next(p);
            if (is_label_name(cur->value)) {
                symbol_add_unresolved(cur->value, p->current_address);
                da_push(bc->code_segment, -1);
                p->current_address++;
            } else {
                da_push(bc->code_segment, parse_constant(cur->value));
                p->current_address++;
            }
        }
        break;
    }
    case TOKEN_SECTION:
    case TOKEN_COMMENT:
    case TOKEN_NEWLINE:
    case TOKEN_COMMA:
        // Valid - just skip these and exit
        break;

    default:
        goto parser_error;
    }

    return 0;

parser_error:

    fprintf(stderr, "unexpected token in .main %s after %s (%s) at line %lu\n",
            lexer_show_token(parser_peek(p)),
            lexer_show_token(parser_current(p)), parser_current(p)->value,
            p->lines);
    return -1;
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
Byte_Code *parser_run(Parser *p)
{
    Byte_Code *bc   = bc_create();
    Token *curr     = parser_current(p);
    int entry_point = -1;
    while (parser_peek(p)->type != TOKEN_EOF) {
        if (!assert_next_token(p))
            goto parser_error;

        if (curr->section == DATA_SECTION) {
            if (parse_data_section_token(p, bc) < 0)
                return NULL;
        } else {
            if (curr->type == TOKEN_SECTION &&
                strncasecmp(curr->value, ".main", 5) == 0)
                entry_point = p->current_address;
            if (parse_main_section_token(p, bc) < 0)
                return NULL;
        }

        p->lines++;
        curr = parser_next(p);
        if (curr->type == TOKEN_EOF)
            break;
    }

    bc->entry_point = entry_point;

    // 2nd pass to resolve symbols
    for (size_t i = 0; i < symbol_table.unresolved_list.length; ++i) {
        int64_t addr = symbol_get(symbol_table.unresolved_list.data[i].name);
        if (addr < 0) {
            fprintf(stderr, "label %s not found\n",
                    symbol_table.unresolved_list.data[i].name);
            return NULL;
        }

        bc->code_segment->data[symbol_table.unresolved_list.data[i].addr] =
            addr;
    }
    return bc;

parser_error:

    fprintf(stderr, "unexpected token %s after %s (%s) at line %lu\n",
            lexer_show_token(parser_peek(p)),
            lexer_show_token(parser_current(p)), parser_current(p)->value,
            p->lines);
    return NULL;
}

void parser_free(Parser *p) { lexer_token_list_free((Token_List *)p->tokens); }

void parser_print_tokens(const Parser *p)
{
    const Token_List *tl = p->tokens;
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
