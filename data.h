#ifndef DATA_H
#define DATA_H

#include <stddef.h>
#include <stdint.h>

#define DEST_MASK   0x000FF00000000000
#define ADDR_MASK   0x00000FFFFFFFFFFF
#define DATA_OFFSET (2 << 12) // 8K

typedef uint8_t hword;
typedef int64_t qword;

struct instruction_line {
    hword op;
    qword src, dst;
};

// TODO: Dispatch the correct encoding based on opcode
qword data_encode_instruction(const struct instruction_line *instruction);
struct instruction_line data_decode_instruction(qword e_instr);
void data_reset_instruction(struct instruction_line *instruction);

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
