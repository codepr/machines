#include "data.h"

qword data_encode_instruction(const struct instruction_line *i)
{
    qword quad_word = ((qword)(i->op & 0xFF) << 56);
    quad_word |= (i->dst << 48);
    quad_word |= (i->src & ADDR_MASK);

    return quad_word;
}

struct instruction_line data_decode_instruction(qword e_instr)
{
    hword op  = e_instr >> 56;
    qword dst = (e_instr & DEST_MASK) >> 48;
    qword src = e_instr & ADDR_MASK;

    return (struct instruction_line){.op = op, .src = src, .dst = dst};
}

void data_reset_instruction(struct instruction_line *instruction)
{
    instruction->op  = 0;
    instruction->src = -1;
    instruction->dst = -1;
}
