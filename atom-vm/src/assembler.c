#include "assembler.h"
#include "bytecode.h"
#include "parser.h"

void asm_disassemble(const Byte_Code *bc)
{
    size_t i = 0;

    if (bc->data_segment->length > 0) {
        printf(".data\n");
        for (int i = 0; i < bc->data_segment->length; ++i) {
            if (bc->data_segment->data[i].type == DT_CONSTANT)
                printf("\t@%04llX %04llu\n", bc->data_segment->data[i].address,
                       bc->data_segment->data[i].as_int);
            else if (bc->data_segment->data[i].type == DT_STRING)
                printf("\t@%04llX \"%s\"\n", bc->data_segment->data[i].address,
                       bc->data_segment->data[i].as_str);
            else if (bc->data_segment->data[i].type == DT_BUFFER)
                printf("\t@%04llX buffer(%llu bytes)\n",
                       bc->data_segment->data[i].address,
                       bc->data_segment->data[i].as_int);
        }
        printf("\n");
    }
    i = 0;
    while (i < bc->code_segment->length) {
        if (bc->entry_point == i)
            printf(".main\n");
        printf("\t%04lX %-11s", i,
               instructions_table[bc->code_segment->data[i]]);
        if (bc->code_segment->data[i] == OP_RET)
            printf("\n");
        if (bc_nary_instruction(bc->code_segment->data[i])) {
            switch (bc->code_segment->data[i]) {
            case OP_PUSH:
                printf(" @%04llX", bc->code_segment->data[++i]);
                break;
            case OP_CALL:
                printf(" (%04llX)", bc->code_segment->data[++i]);
                break;
            case OP_JMP:
            case OP_JNE:
            case OP_JEQ:
            case OP_LOAD_CONST:
            case OP_STORE_CONST:
                printf(" [%02llu]", bc->code_segment->data[++i]);
                break;
            default:
                printf(" %04llu", bc->code_segment->data[++i]);
                break;
            }
        }
        printf("\n");
        i++;
    }
}

Byte_Code *asm_compile(const char *path, int debug)
{
    if (!path)
        return NULL;

    FILE *fp = fopen(path, "r");
    if (!fp)
        return NULL;

    Parser p;
    int err = parser_init(fp, &p);
    if (err < 0)
        goto errdefer;

    fclose(fp);

    if (debug) {
        printf("\n");
        printf("=====================\n");
        printf("[*] Lexical analysis\n");
        printf("=====================\n\n");
        parser_print_tokens(&p);
        printf("\n");
    }

    Byte_Code *bc = parser_run(&p);

    parser_free(&p);
    return bc;

errdefer:
    fclose(fp);

    return NULL;
}

Byte_Code *asm_compile_from_stdin(int debug)
{

    Parser p;
    int err = parser_init(stdin, &p);
    if (err < 0)
        return NULL;

    if (debug) {
        printf("\n");
        printf("=====================\n");
        printf("[*] Lexical analysis\n");
        printf("=====================\n\n");
        parser_print_tokens(&p);
        printf("\n");
    }

    Byte_Code *bc = parser_run(&p);

    parser_free(&p);

    return bc;
}
