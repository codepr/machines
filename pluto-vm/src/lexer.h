#ifndef LEXER_H
#define LEXER_H

#include <stdio.h>

#define TOKEN_VALUE_SIZE 512

struct lexer {
    char *buffer;
    size_t pos;
    size_t size;
};

typedef enum { DATA_SECTION, MAIN_SECTION } Section;

typedef enum {
    TOKEN_LABEL,
    TOKEN_INSTR,
    TOKEN_REGISTER,
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

void lexer_token_list_init(struct token_list *tl, size_t capacity);

void lexer_token_list_free(struct token_list *tl);

void lexer_init(struct lexer *l, char *buffer, size_t size);

int lexer_next(struct lexer *l, struct token *t, Token_Type prev);

int lexer_tokenize(struct lexer *l, struct token_list *tokens);

int lexer_tokenize_stream(FILE *fp, struct lexer *l, struct token_list *tokens);

const char *lexer_show_token(const struct token *t);

void lexer_print_tokens(const struct token_list *tl);

#endif
