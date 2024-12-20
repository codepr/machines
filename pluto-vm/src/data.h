#ifndef DATA_H
#define DATA_H

#include <stddef.h>
#include <stdint.h>

#define SRC_MASK    0x3FFFFFF
#define DST_MASK    0x3FFFFFF
#define DATA_OFFSET (2 << 12) // 8K

typedef uint8_t hword;
typedef uint64_t qword;
typedef enum {
    IS_ATOM        = 0x00,
    IS_SRC_REG     = 0x1,  // Source is a register
    IS_SRC_MEM     = 0x2,  // Source is memory
    IS_SRC_IMM     = 0x4,  // Source is an immediate value
    IS_SRC_IREG    = 0x8,  // Source is an indirect register (memory address as
                           // register value)
    IS_DST_REG     = 0x10, // Destination is a register
    IS_DST_MEM     = 0x20, // Destination is memory
    // Combination semantics
    IS_SEM_REG_REG = IS_SRC_REG | IS_DST_REG, // Register to Register
    IS_SEM_REG_MEM = IS_SRC_REG | IS_DST_MEM, // Register to Memory
    IS_SEM_MEM_REG = IS_SRC_MEM | IS_DST_REG, // Memory to Register
    IS_SEM_IMM_REG = IS_SRC_IMM | IS_DST_REG, // Immediate to Register
    IS_SEM_IMM_MEM = IS_SRC_IMM | IS_DST_MEM, // Immediate to Memory
} Instr_Semantic;

// Represents an instruction such e.g.
//   MOV ax, [cx] ; Copy inderct regitry cx into ax
//
//   - op:  OP code of the instruction
//   - sem: Semanthic of the instruction
//   - src: Source operand (register / memory / immediate value)
//   - dst: Destination operand (register / memory)
struct instruction_line {
    hword op;
    Instr_Semantic sem;
    int64_t src, dst;
};

// TODO: Dispatch the correct encoding based on opcode
qword data_encode_instruction(const struct instruction_line *instruction);
struct instruction_line data_decode_instruction(qword e_instr);
void data_reset_instruction(struct instruction_line *instruction);

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

#endif
