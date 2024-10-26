#ifndef PARSER_H
#define PARSER_H

#include "bytecode.h"
#include <stdint.h>
#include <stdlib.h>

struct token_list;
typedef struct bytecode Byte_Code;

#define LABELSIZE    64
#define LABELS_TOTAL 128

struct labels_s {
    // Name of the label (symbolic reference)
    char name[LABELSIZE];
    // Bytecode offset or memory address for the label
    size_t offset;
};

// Struct representing the parser for source code, which processes a list of
// tokens and maintains state such as current token, current address, and label
// information.
struct parser {
    // Pointer to the list of tokens to be parsed
    const struct token_list *tokens;
    // Pointer to the current token being processed
    struct token *current;
    // Current address in the bytecode or source being parsed
    size_t current_address;
    // Current directive in the bytecode when parsing DATA SECTION
    Directive current_directive;
    // Number of lines parsed (used for error reporting or tracking)
    size_t lines;

    // Struct for managing label information during parsing
    struct {
        // Array of labels defined in the source
        struct labels_s resolved[LABELS_TOTAL];
        // Number of labels defined
        size_t length;
        // Array of unresolved labels
        struct labels_s unresolved[LABELS_TOTAL];
        // Number of unresolvd labels
        size_t unresolved_length;
        // Base offset to apply when resolving label addresses
        size_t base_offset;
    } label_list;

    // Struct to keep track of the instructions parsed, represents
    // a middle stage before being encoded into the final bytecode
    struct {
        struct instruction_line *data;
        size_t length;
        size_t capacity;
    } instructions;
};

/**
 * @brief Initializes the parser with a given list of tokens.
 *
 * This function sets up the parser by assigning the token list to the parser
 * structure and resetting the parsing state such as current token and address.
 *
 * @param p Pointer to the parser structure to initialize.
 * @param tokens Pointer to the list of tokens that will be parsed.
 */
void parser_init(struct parser *p, const struct token_list *tokens);

/**
 * @brief Parses the source code tokens and generates bytecode.
 *
 * This function processes the tokens in the token list, parses them according
 * to the assembly's syntax, and generates the corresponding bytecode into the
 * provided Byte_Code structure.
 *
 * @param p Pointer to the parser structure containing the current parsing
 * state.
 * @param code Pointer to the Byte_Code structure where the generated bytecode
 * will be stored.
 * @return Returns 0 on success, non-zero on error.
 */
int parser_run(struct parser *p, Byte_Code *code);

#endif
