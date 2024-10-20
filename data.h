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

#endif
