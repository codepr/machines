#ifndef LEXER_H
#define LEXER_H

#include <stdio.h>

#define TOKEN_NAME_SIZE 64

struct lexer {
    char *buffer;
    size_t pos;
    size_t size;
};

typedef enum {
    TOKEN_LABEL,
    TOKEN_INSTR,
    TOKEN_REGISTER,
    TOKEN_STRING,
    TOKEN_CONSTANT,
    TOKEN_ADDRESS,
    TOKEN_SECTION,
    TOKEN_COMMA,
    TOKEN_NEWLINE,
    TOKEN_COMMENT,
    TOKEN_UNKNOWN,
    TOKEN_EOF,
} Token_Type;

struct token {
    Token_Type type;
    char value[TOKEN_NAME_SIZE];
};

struct token_list {
    struct token *items;
    size_t length;
    size_t capacity;
};

void lexer_token_list_init(struct token_list *tl, size_t capacity);

void lexer_init(struct lexer *l, char *buffer, size_t size);
int lexer_tokenize(struct lexer *l, struct token_list *tokens);
void lexer_print_tokens(const struct token_list *tl);

#endif
