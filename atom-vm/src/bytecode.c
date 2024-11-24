#include "bytecode.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

bool bc_nary_instruction(Instruction_ID instr)
{
    return (instr > OP_LOAD && instr < OP_ADD) ||
           (instr > OP_EQ && instr < OP_PRINT);
}

static Word_Segment *word_segment_create(void)
{
    Word_Segment *c = calloc(1, sizeof(*c));
    if (!c)
        return NULL;

    size_t capacity = 4;
    da_init(c, capacity);

    return c;
}

static Data_Segment *data_segment_create(void)
{
    Data_Segment *d = calloc(1, sizeof(*d));
    if (!d)
        return NULL;

    size_t capacity = 4;
    da_init(d, capacity);

    return d;
}

static void word_segment_free(Word_Segment *c)
{
    free(c->data);
    free(c);
}

static void data_segment_free(Data_Segment *d)
{
    free(d->data);
    free(d);
}

static Labels *labels_create(void)
{
    Labels *l = calloc(1, sizeof(*l));
    if (!l)
        return NULL;

    l->length = 0;
    return l;
}

static void labels_free(Labels *l) { free(l); }

Byte_Code *bc_create(void)
{
    Byte_Code *bc = calloc(1, sizeof(*bc));
    if (!bc)
        return NULL;

    bc->entry_point  = 0;
    bc->code_segment = word_segment_create();
    if (!bc->code_segment)
        goto error;

    bc->data_segment = data_segment_create();
    if (!bc->data_segment)
        goto error;

    bc->labels = labels_create();
    if (!bc->labels)
        goto error;

    return bc;

error:
    free(bc);
    return NULL;
}

void bc_free(Byte_Code *bc)
{
    word_segment_free(bc->code_segment);
    data_segment_free(bc->data_segment);
    labels_free(bc->labels);
    free(bc);
}

Word *bc_code(const Byte_Code *bc) { return bc->code_segment->data; }

static void write_u64(uint8_t *buf, uint64_t val)
{
    *buf++ = val >> 56;
    *buf++ = val >> 48;
    *buf++ = val >> 40;
    *buf++ = val >> 32;
    *buf++ = val >> 24;
    *buf++ = val >> 16;
    *buf++ = val >> 8;
    *buf++ = val;
}

uint64_t read_u64(uint8_t *buf)
{
    return ((uint64_t)buf[0] << 56) | ((uint64_t)buf[1] << 48) |
           ((uint64_t)buf[2] << 40) | ((uint64_t)buf[3] << 32) |
           ((uint64_t)buf[4] << 24) | ((uint64_t)buf[5] << 16) |
           ((uint64_t)buf[6] << 8) | buf[7];
}

void bc_dump(const Byte_Code *bc, const char *path)
{
    if (!path)
        return;

    FILE *fp = fopen(path, "wb");
    if (!fp)
        return;

    uint8_t buffer[sizeof(uint64_t)] = {0};

    for (size_t i = 0; i < bc->code_segment->length; ++i) {
        memset(buffer, 0x00, sizeof(buffer));
        write_u64(buffer, bc->code_segment->data[i]);
        fwrite(buffer, sizeof(buffer), 1, fp);
    }

    fclose(fp);
}

Byte_Code *bc_load(const char *path)
{
    if (!path)
        return NULL;

    FILE *fp = fopen(path, "rb");
    if (!fp)
        return NULL;

    uint8_t buffer[sizeof(uint64_t)] = {0};
    Byte_Code *bc                    = bc_create();
    if (!bc) {
        fclose(fp);
        return NULL;
    }

    while (fread(buffer, sizeof(buffer), 1, fp) == 1) {
        da_push(bc->code_segment, read_u64(buffer));
    }

    fclose(fp);

    return bc;
}
