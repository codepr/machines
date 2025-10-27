#ifndef BYTECODE_H
#define BYTECODE_H

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

// Dynamic array helpers

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

#define LABEL_SIZE         64
#define LABELS_TOTAL       128
#define DATA_OFFSET        1024
#define DATA_STRING_OFFSET 2048

typedef uint64_t Word;
typedef enum {
    OP_LOAD,
    OP_LOAD_CONST,
    OP_STORE,
    OP_STORE_CONST,
    OP_CALL,
    OP_PUSH,
    OP_PUSH_CONST,
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_DUP,
    OP_INC,
    OP_EQ,
    OP_JMP,
    OP_JEQ,
    OP_JNE,
    OP_MAKE_TUPLE,
    OP_PRINT,
    OP_PRINT_CONST,
    OP_RET,
    OP_HALT
} Instruction_ID;

//  These static maps are used to determine the token types during the lexical
//  analysis of the source code
extern const char *const instructions_table[];

typedef struct word_segment {
    Word *data;
    size_t length;
    size_t capacity;
} Word_Segment;

typedef struct labels {
    char labels[LABELS_TOTAL][LABEL_SIZE];
    size_t length;
} Labels;

#define DATA_STRING_SIZE 512
typedef enum { DT_CONSTANT, DT_STRING, DT_BUFFER } Data_Type;
typedef enum {
    D_DB,
    D_DW,
    D_DD,
    D_DQ,
    D_RB,
    D_RW,
    D_RD,
    D_RQ,
    NUM_DIRECTIVES
} Directive;

// Tagged-union for different kind of constant data
typedef struct data_record {
    Data_Type type;
    Word address;
    union {
        char as_str[DATA_STRING_SIZE];
        Word as_int;
    };
} Data_Record;

typedef struct data_segment {
    Data_Record *data;
    size_t length;
    size_t capacity;
    size_t rd_data_addr_offset;
    size_t rw_data_addr_offset;
    size_t rd_string_addr_offset;
} Data_Segment;

typedef struct bytecode {
    size_t entry_point;
    Word_Segment *code_segment;
    Data_Segment *data_segment;
    Labels *labels;
} Byte_Code;

Byte_Code *bc_create(void);

void bc_free(Byte_Code *bc);

Word *bc_code(const Byte_Code *bc);

bool bc_nary_instruction(Instruction_ID instr);

void bc_dump(const Byte_Code *bc, const char *path);

Byte_Code *bc_load(const char *path);

#endif // BYECODE_H
