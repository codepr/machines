#include "assembler.h"
#include "bytecode.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define STACK_SIZE  256
#define MEMORY_SIZE 65535

typedef enum { SUCCESS, E_DIV_BY_ZERO, E_UNKNOWN_INSTRUCTION } Interpret_Result;

typedef struct {
    // Instruction stack
    Word stack[STACK_SIZE];
    Word *stack_top;
    // Memory
    Word memory[MEMORY_SIZE];
    // Instruction pointer
    Word *ip;
    // Call stack for functions
    Word call_stack[STACK_SIZE];
    Word *cstack_top;
    // Result register
    Word result;
} Vm;

static Vm vm = {.ip = NULL, .stack_top = NULL, .cstack_top = NULL};

static void vm_init(void)
{
    memset(vm.stack, 0x00, STACK_SIZE * sizeof(Word));
    memset(vm.memory, 0x00, MEMORY_SIZE * sizeof(Word));
    memset(vm.call_stack, 0x00, STACK_SIZE * sizeof(Word));
    vm.ip         = NULL;
    vm.stack_top  = NULL;
    vm.cstack_top = NULL;
}

static void vm_reset(Byte_Code *bc)
{
    Word *bytecode = bc_code(bc);
    vm             = (Vm){.stack_top  = vm.stack,
                          .cstack_top = vm.call_stack,
                          .ip         = bytecode + bc->entry_point};

    for (size_t i = 0; i < bc->data_segment->length; ++i) {
        if (bc->data_segment->data[i].type == DT_CONSTANT) {
            vm.memory[bc->data_segment->data[i].address] =
                bc->data_segment->data[i].as_int;
        } else {
            for (size_t j = 0; j < strlen(bc->data_segment->data[i].as_str);
                 ++j)
                vm.memory[bc->data_segment->data[i].address + j] =
                    bc->data_segment->data[i].as_str[j];
        }
    }
}

#define vm_next()      (*vm.ip++)
#define vm_push(value) (*vm.stack_top++ = (value))
#define vm_pop()       (*--vm.stack_top)
#define vm_tos()       (vm.stack_top - 1)
#define vm_peek()      (*(vm.stack_top - 1))

// static void vm_print_stack(void)
// {
//     printf("[");
//     Word *sp = vm.stack;
//     while (sp != vm.stack_top) {
//         printf("%llu,", *sp);
//         ++sp;
//     }
//     printf("]\n");
// }

static bool string_pointer(Word value) { return value >= DATA_STRING_OFFSET; }

static void print_string_from_memory(Word address)
{
    // Start at the memory address
    Word current_cell = vm.memory[address];

    // Traverse the memory until a null terminator (0) is found
    while (current_cell != 0) {
        char ch =
            (char)(current_cell & 0xFF);   // Extract the least significant byte
        putchar(ch);                       // Print the character
        address++;                         // Move to the next memory cell
        current_cell = vm.memory[address]; // Read the next cell
    }
}

Interpret_Result vm_interpret(Byte_Code *bc)
{
    Word *bytecode = bc_code(bc);
    vm_reset(bc);
    size_t pc = 0;

    for (;;) {
        switch (vm_next()) {
        case OP_LOAD: {
            Word addr = vm_pop();
            vm_push(vm.memory[addr]);
            pc++;
            break;
        }
        case OP_LOAD_CONST: {
            Word addr = vm_next();
            vm_push(vm.memory[addr]);
            pc += 2;
            break;
        }
        case OP_STORE: {
            Word addr       = vm_pop();
            Word value      = vm_pop();
            vm.memory[addr] = value;
            pc++;
            break;
        }
        case OP_STORE_CONST: {
            Word value      = vm_pop();
            Word addr       = vm_next();
            vm.memory[addr] = value;
            pc += 2;
            break;
        }
        case OP_CALL: {
            Word addr = vm_next();
            pc += 2;
            *vm.cstack_top++ = pc;
            pc               = addr;
            vm.ip            = bytecode + addr;
            break;
        }
        case OP_PUSH: {
            Word arg = vm_next();
            // Assume PUSH can be used only for data pointers for now
            if (string_pointer(arg))
                vm_push(arg);
            else
                vm_push(vm.memory[arg]);
            pc += 2;
            break;
        }
        case OP_PUSH_CONST: {
            Word arg = vm_next();
            vm_push(arg);
            pc += 2;
            break;
        }
        case OP_ADD: {
            Word right = vm_pop();
            *vm_tos() += right;
            pc++;
            break;
        }
        case OP_SUB: {
            Word right = vm_pop();
            *vm_tos() -= right;
            pc++;
            break;
        }
        case OP_MUL: {
            Word right = vm_pop();
            *vm_tos() *= right;
            pc++;
            break;
        }
        case OP_DIV: {
            Word right = vm_pop();
            if (right == 0)
                return E_DIV_BY_ZERO;
            *vm_tos() /= right;
            pc++;
            break;
        }
        case OP_DUP: {
            vm_push(vm_peek());
            pc++;
            break;
        }
        case OP_INC: {
            *vm_tos() += 1;
            pc++;
            break;
        }
        case OP_EQ: {
            Word arg  = vm_pop();
            *vm_tos() = vm_peek() == arg;
            pc++;
            break;
        }
        case OP_JMP: {
            vm.ip = bytecode + vm_next();
            pc += 2;
            break;
        }
        case OP_JEQ: {
            pc++;
            if (vm_peek()) {
                (void)vm_pop();
                pc    = vm_next();
                vm.ip = bytecode + pc;
            }
            break;
        }
        case OP_JNE:
            pc++;
            if (!vm_peek()) {
                (void)vm_pop();
                pc    = vm_next();
                vm.ip = bytecode + pc;
            }
            break;
        case OP_MAKE_TUPLE: {
            Word address    = vm_next();
            Word tuple_size = vm_pop();
            while (tuple_size-- > 0) {
                vm.memory[address++] = vm_pop();
            }
            pc += 2;
            break;
        }
        case OP_PRINT: {
            Word address = vm_pop();
            if (string_pointer(address)) {
                print_string_from_memory(address);
            } else {
                printf("%lli", address);
            }
            fflush(stdout);
            pc++;
            break;
        }
        case OP_PRINT_CONST: {
            Word address = vm_pop();
            printf("%lli", address);
            fflush(stdout);
            pc++;
            break;
        }
        case OP_RET: {
            Word addr = *(--vm.cstack_top);
            vm.ip     = bytecode + addr;
            pc        = addr;
            break;
        }
        case OP_HALT:
            pc++;
            goto exit;
        default:
            return E_UNKNOWN_INSTRUCTION;
        }
    }

exit:

    vm.result = vm_pop();

    return SUCCESS;
}

int main(void)
{

    vm_init();

    Byte_Code *bc = asm_compile("examples/tuple.atom", 1);
    if (!bc)
        abort();

    asm_disassemble(bc);

    if (vm_interpret(bc) < 0)
        abort();

    bc_free(bc);

    return 0;
}
