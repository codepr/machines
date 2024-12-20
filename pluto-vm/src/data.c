#include "data.h"

qword data_encode_instruction(const struct instruction_line *i)
{
    qword encoded = 0;

    // Encode the 5-bit operation code (op)
    encoded |= ((qword)i->op << 59);

    // Encode the 6-bit instruction semantic (sem)
    encoded |= ((qword)i->sem << 53);

    // Encode the 27-bit source operand (src)
    encoded |= ((qword)(i->src & SRC_MASK) << 27);

    // Encode the 26-bit destination operand (dst)
    encoded |= ((qword)i->dst & DST_MASK);

    return encoded;
}

struct instruction_line data_decode_instruction(qword e_instr)
{
    struct instruction_line instr;

    // Decode the 5-bit operation code (op)
    instr.op  = (hword)((e_instr >> 59) & 0x1F);

    // Decode the 6-bit instruction semantic (sem)
    instr.sem = (Instr_Semantic)((e_instr >> 53) & 0x3F);

    // Decode the 27-bit source operand (src)
    instr.src = (e_instr >> 27) & SRC_MASK;

    // Decode the 26-bit destination operand (dst)
    instr.dst = e_instr & DST_MASK;

    return instr;
}

void data_reset_instruction(struct instruction_line *instruction)
{
    instruction->op  = 0;
    instruction->sem = IS_ATOM;
    instruction->src = -1;
    instruction->dst = -1;
}
