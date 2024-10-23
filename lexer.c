#include "lexer.h"
#include "data.h"
#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#define SECTION_START '.'
#define COMMENT_START ';'
#define LABEL_END     ':'
#define NEWLINE       '\n'

//
//  *******************
//  * MAPPING HELPERS *
//  ******************
//
//  These static maps are used to determine the token types during the lexical
//  analysis of the source code
static const char *instructions[] = {
    "nop", "clf", "cmp",  "mov", "psh",     "pop", "add", "sub",
    "mul", "div", "mod",  "inc", "dec",     "and", "bor", "xor",
    "not", "shl", "shr",  "jmp", "jeq",     "jne", "jle", "jlt",
    "jge", "jgt", "call", "ret", "syscall", "hlt", NULL};

static const char *registers[]  = {"ax", "bx", "cx", "dx", NULL};
static const char *directives[] = {"db", "dw", NULL};

static const char *tokens[]     = {"TOKEN_LABEL",    "TOKEN_INSTR",
                                   "TOKEN_REGISTER", "TOKEN_STRING",
                                   "TOKEN_CONSTANT", "TOKEN_ADDRESS",
                                   "TOKEN_SECTION",  "TOKEN_DIRECTIVE",
                                   "TOKEN_COMMA",    "TOKEN_NEWLINE",
                                   "TOKEN_COMMENT",  "TOKEN_UNKNOWN",
                                   "TOKEN_EOF",      NULL};

void lexer_init(struct lexer *l, char *buffer, size_t size)
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

int is_instruction(const char *token)
{
    for (int i = 0; instructions[i] != NULL; ++i) {
        if (strncasecmp(token, instructions[i], strlen(token)) == 0) {
            return 1;
        }
    }
    return 0;
}

static int is_directive(const char *token)
{
    for (int i = 0; directives[i] != NULL; ++i) {
        if (strncasecmp(token, directives[i], strlen(token)) == 0) {
            return 1;
        }
    }
    return 0;
}

static int is_register(const char *token)
{
    for (int i = 0; registers[i] != NULL; ++i) {
        if (strncasecmp(token, registers[i], strlen(token)) == 0) {
            return 1;
        }
    }
    return 0;
}

int lexer_next(struct lexer *l, struct token *t, Token_Type prev)
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
        } else if (is_register(t->value)) {
            t->type = TOKEN_REGISTER;
        } else if (is_directive(t->value)) {
            t->type = TOKEN_DIRECTIVE;
        } else if (prev == TOKEN_REGISTER || prev == TOKEN_INSTR ||
                   prev == TOKEN_COMMA) {
            t->type = TOKEN_ADDRESS;
        } else {
            t->type = TOKEN_UNKNOWN;
        }
    }

end:
    return 1;
}

int lexer_tokenize(struct lexer *l, struct token_list *tokens)
{
    struct token t;
    Token_Type prev = TOKEN_UNKNOWN;
    Section section = DATA_SECTION;
    while (lexer_next(l, &t, prev) != EOF) {
        if (strncasecmp(t.value, ".data", 5) == 0)
            section = DATA_SECTION;
        if (strncasecmp(t.value, ".main", 5) == 0)
            section = MAIN_SECTION;
        t.section = section;
        da_push(tokens, t);
        prev = t.type;
        memset(&t, 0x00, sizeof(struct token));
    }
    // EOF
    lexer_next(l, &t, prev);
    t.section = section;
    da_push(tokens, t);

    return 0;
}

int lexer_tokenize_stream(FILE *fp, struct lexer *l, struct token_list *tokens)
{
    char line[0xFFF];
    struct token t;
    Token_Type prev = TOKEN_UNKNOWN;
    Section section = DATA_SECTION;
    while (fgets(line, 0xFFF, fp)) {
        lexer_init(l, line, strlen(line));
        while (lexer_next(l, &t, prev) != EOF) {
            if (strncasecmp(t.value, ".data", 5) == 0)
                section = DATA_SECTION;
            if (strncasecmp(t.value, ".main", 5) == 0)
                section = MAIN_SECTION;
            t.section = section;
            da_push(tokens, t);
            prev = t.type;
            memset(&t, 0x00, sizeof(struct token));
        }
        memset(line, 0x0, 0xFFF);
    }
    // EOF
    lexer_next(l, &t, prev);
    t.section = section;
    da_push(tokens, t);

    return 0;
}

void lexer_token_list_init(struct token_list *tl, size_t capacity)
{
    da_init(tl, capacity);
}

void lexer_token_list_free(struct token_list *tl) { free(tl->data); }

const char *lexer_show_token(const struct token *t) { return tokens[t->type]; }

void lexer_print_tokens(const struct token_list *tl)
{
    for (int i = 0; i < tl->length; ++i)
        printf("token type %s (%d), value = %s\n",
               lexer_show_token(&tl->data[i]), tl->data[i].type,
               tl->data[i].value);
}
