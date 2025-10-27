#pragma once

#include "bytecode.h"
#include <stdio.h>
#include <stdlib.h>

typedef struct token Token;
typedef struct token_list {
    Token *data;
    size_t length;
    size_t capacity;
} Token_List;

// Struct representing the parser for source code, which processes a list of
// tokens and maintains state such as current token, current address, and label
// information.
typedef struct parser {
    // Pointer to the list of tokens to be parsed
    const Token_List *tokens;
    // Pointer to the current token being processed
    Token *current;
    // Number of lines parsed (used for error reporting or tracking)
    size_t lines;
    // When parsing, keeps track of the latest defined directive. This field
    // is used only when in a DATA_SECTION
    Directive current_directive;
    // Current address in the bytecode or source being parsed
    size_t current_address;
} Parser;

int parser_init(FILE *fp, Parser *p);
void parser_free(Parser *p);
Byte_Code *parser_run(Parser *p);
void parser_print_tokens(const Parser *p);
