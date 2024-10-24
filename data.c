#include "data.h"

qword data_encode_instruction(const struct instruction_line *i)
{
    qword encoded = 0;

    // Encode the 5-bit operation code (op)
    encoded |= ((qword)i->op << 59);

    // Encode the 5-bit instruction semantic (sem)
    encoded |= ((qword)i->sem << 54);

    // Encode the 27-bit source operand (src)
    encoded |= ((qword)(i->src & ADDR_MASK) << 27);

    // Encode the 27-bit destination operand (dst)
    encoded |= (i->dst & DEST_MASK);

    return encoded;
}

struct instruction_line data_decode_instruction(qword e_instr)
{
    struct instruction_line instr;

    // Decode the 5-bit operation code (op)
    instr.op  = (hword)((e_instr >> 59) & 0x1F);

    // Decode the 5-bit instruction semantic (sem)
    instr.sem = (Instr_Semantic)((e_instr >> 54) & 0x1F);

    // Decode the 27-bit source operand (src)
    instr.src = (e_instr >> 27) & ADDR_MASK;

    // Decode the 27-bit destination operand (dst)
    instr.dst = e_instr & DEST_MASK;

    return instr;
}

void data_reset_instruction(struct instruction_line *instruction)
{
    instruction->op  = 0;
    instruction->sem = IS_ATOM;
    instruction->src = -1;
    instruction->dst = -1;
}
