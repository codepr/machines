#include "lexer.h"
#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#define da_init(da, capacity)                                                  \
    do {                                                                       \
        assert((capacity) > 0);                                                \
        (da)->length   = 0;                                                    \
        (da)->capacity = (capacity);                                           \
        (da)->items    = calloc((capacity), sizeof(*(da)->items));             \
    } while (0)

#define da_extend(da)                                                          \
    do {                                                                       \
        (da)->capacity *= 2;                                                   \
        (da)->items =                                                          \
            realloc((da)->items, (da)->capacity * sizeof(*(da)->items));       \
        if (!(da)->items) {                                                    \
            free((da));                                                        \
            fprintf(stderr, "DA realloc failed");                              \
            exit(EXIT_FAILURE);                                                \
        }                                                                      \
    } while (0)

#define da_push(da, item)                                                      \
    do {                                                                       \
        assert((da));                                                          \
        if ((da)->length + 1 == (da)->capacity)                                \
            da_extend((da));                                                   \
        (da)->items[(da)->length++] = (item);                                  \
    } while (0)

static const char *instructions[] = {
    "nop", "clf", "cmp", "cmi",  "mov", "ldi",     "ldr", "sti", "str",
    "psr", "psm", "psi", "pop",  "pom", "add",     "adi", "sub", "sbi",
    "mul", "mli", "div", "dvi",  "mod", "mdi",     "inc", "dec", "and",
    "bor", "xor", "not", "shl",  "shr", "jmp",     "jeq", "jne", "jle",
    "jlt", "jge", "jgt", "call", "ret", "syscall", "hlt", NULL};

static const char *registers[] = {"ax", "bx", "cx", "dx", NULL};

static const char *tokens[]    = {
    "TOKEN_LABEL",    "TOKEN_INSTR",   "TOKEN_REGISTER", "TOKEN_STRING",
    "TOKEN_CONSTANT", "TOKEN_ADDRESS", "TOKEN_SECTION",  "TOKEN_COMMA",
    "TOKEN_NEWLINE",  "TOKEN_COMMENT", "TOKEN_UNKNOWN",  "TOKEN_EOF"};

void lexer_init(struct lexer *l, char *buffer, size_t size)
{
    l->size   = size;
    l->buffer = buffer;
    l->pos    = 0;
}

static inline void lexer_strip_spaces(struct lexer *l)
{
    if (l->pos == l->size)
        return;

    while (isspace(l->buffer[l->pos]) && l->pos < l->size)
        l->pos++;
}

static inline char lexer_peek(const struct lexer *l)
{
    if (l->pos >= l->size)
        return EOF;
    return l->buffer[l->pos];
}

static inline char lexer_next_char(struct lexer *l)
{
    char c = lexer_peek(l);
    l->pos++;
    return c;
}

// Assume nul characteer always present
#define is_label(token)   ((token)[strlen(token) - 1] == ':')
#define is_section(token) ((token)[0] == '.')
#define is_comment(token) ((token)[0] == ';')

int is_instruction(const char *token)
{
    for (int i = 0; instructions[i] != NULL; ++i) {
        if (strncasecmp(token, instructions[i], strlen(token)) == 0) {
            return 1;
        }
    }
    return 0;
}

int is_register(const char *token)
{
    for (int i = 0; registers[i] != NULL; ++i) {
        if (strncasecmp(token, registers[i], strlen(token)) == 0) {
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
    if (l->pos == l->size) {
        t->type = TOKEN_EOF;
        return EOF;
    }

    char c = lexer_peek(l);

    if (c == '\n') {
        lexer_next_char(l);
        t->type = TOKEN_NEWLINE;
        return 1;
    }

    if (c == ',') {
        lexer_next_char(l);
        t->type = TOKEN_COMMA;
        return 1;
    }

    size_t i = 0;
    // String
    if (c == '"' || c == '\'') {
        t->type = TOKEN_STRING;
        do {
            t->value[i++] = l->buffer[l->pos++];
        } while (lexer_peek(l) != '\'' && lexer_peek(l) != '"' &&
                 lexer_peek(l) != '\n');
        goto end;
    } else if (c == ';') {
        t->type = TOKEN_COMMENT;
        while (lexer_peek(l) != '\n') {
            t->value[i++] = lexer_next_char(l);
        }
        goto end;
    } else if (isdigit(c)) {
        // Numbers constants
        t->type = TOKEN_CONSTANT;
        while (isdigit(lexer_peek(l))) {
            t->value[i++] = lexer_next_char(l);
        }
        goto end;
    } else if (c == '[') {
        t->type = TOKEN_ADDRESS;
        while (lexer_peek(l) != ']') {
            t->value[i++] = lexer_next_char(l);
        }
        lexer_next_char(l);
        goto end;
    } else {
        // labels / sections / instructions
        while (lexer_peek(l) != ' ' && lexer_peek(l) != ',' &&
               lexer_peek(l) != '\n') {
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
        } else if (prev == TOKEN_REGISTER || prev == TOKEN_INSTR) {
            t->type = TOKEN_ADDRESS;
        } else {
            t->type = TOKEN_UNKNOWN;
        }
    }

end:
    return 1;
}

static void lexer_print_token(const struct token *t)
{
    printf("token type %s (%d), value = %s\n", tokens[t->type], t->type,
           t->value);
}

int lexer_tokenize(struct lexer *l, struct token_list *tokens)
{
    struct token t;
    Token_Type prev = TOKEN_UNKNOWN;
    while (lexer_next(l, &t, prev) != EOF) {
        da_push(tokens, t);
        prev = t.type;
        memset(&t, 0x00, sizeof(struct token));
    }

    return 0;
}

void lexer_token_list_init(struct token_list *tl, size_t capacity)
{
    da_init(tl, capacity);
}

void lexer_print_tokens(const struct token_list *tl)
{
    for (int i = 0; i < tl->length; ++i)
        lexer_print_token(&tl->items[i]);
}
