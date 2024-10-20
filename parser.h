#ifndef PARSER_H
#define PARSER_H

#include <stdint.h>
#include <stdlib.h>

struct token_list;
typedef struct bytecode Byte_Code;

#define LABELSIZE    64
#define LABELS_TOTAL 128
// #define DATA_OFFSET  8192 // 8K

struct parser {
    const struct token_list *tokens;
    struct token *current;
    size_t current_address;
    struct {
        struct {
            char name[LABELSIZE];
            uint64_t offset;
        } labels[LABELS_TOTAL];
        size_t length;
        size_t base_offset;
    } label_list;
};

void parser_init(struct parser *p, const struct token_list *tokens);
int parser_parse_source(struct parser *p, Byte_Code *code);

#define da_init(da, capacity)                                                  \
    do {                                                                       \
        assert((capacity) > 0);                                                \
        (da)->length   = 0;                                                    \
        (da)->capacity = (capacity);                                           \
        (da)->data     = calloc((capacity), sizeof(*(da)->data));              \
    } while (0)

#define da_extend(da)                                                          \
    do {                                                                       \
        (da)->capacity *= 2;                                                   \
        (da)->data =                                                           \
            realloc((da)->data, (da)->capacity * sizeof(*(da)->data));         \
        if (!(da)->data) {                                                     \
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
        (da)->data[(da)->length++] = (item);                                   \
    } while (0)

#endif
