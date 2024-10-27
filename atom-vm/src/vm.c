#include "bytecode.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STACK_SIZE 256
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

static void vm_init(void) {
    memset(vm.stack, 0x00, STACK_SIZE * sizeof(Word));
    memset(vm.memory, 0x00, MEMORY_SIZE * sizeof(Word));
    memset(vm.call_stack, 0x00, STACK_SIZE * sizeof(Word));
    vm.ip = NULL;
    vm.stack_top = NULL;
    vm.cstack_top = NULL;
}

static void vm_reset(Word *bytecode) {
    vm = (Vm){
        .stack_top = vm.stack, .cstack_top = vm.call_stack, .ip = bytecode};
}

#define vm_next() (*vm.ip++)
#define vm_push(value) (*vm.stack_top++ = (value))
#define vm_pop() (*--vm.stack_top)
#define vm_tos() (vm.stack_top - 1)
#define vm_peek() (*(vm.stack_top - 1))

// static void vm_print_stack(void) {
//     printf("=======");
//     for (int i = 0; i < STACK_SIZE; ++i) {
//         printf("%llu\n", vm.stack[i]);
//     }
// }

Interpret_Result vm_interpret(Byte_Code *bc) {
    Word *bytecode = bc_code(bc);
    vm_reset(bytecode);
    size_t pc = 0;

    for (;;) {
        switch (vm_next()) {
        case OP_LOAD: {
            uint64_t addr = vm_pop();
            vm_push(vm.memory[addr]);
            pc++;
            break;
        }
        case OP_LOADI: {
            uint64_t addr = vm_next();
            vm_push(vm.memory[addr]);
            pc += 2;
            break;
        }
        case OP_STORE: {
            uint64_t addr = vm_pop();
            uint64_t value = vm_pop();
            vm.memory[addr] = value;
            pc++;
            break;
        }
        case OP_STOREI: {
            uint64_t value = vm_pop();
            uint64_t addr = vm_next();
            vm.memory[addr] = value;
            pc += 2;
            break;
        }
        case OP_CALL: {
            Word addr = vm_next();
            pc += 2;
            *vm.cstack_top++ = pc;
            pc = addr;
            vm.ip = bytecode + addr;
            break;
        }
        case OP_PUSH: {
            uint64_t arg = vm_next();
            vm_push(bc_constant(bc, arg));
            pc += 2;
            break;
        }
        case OP_PUSHI: {
            uint64_t arg = vm_next();
            vm_push(arg);
            pc += 2;
            break;
        }
        case OP_ADD: {
            uint64_t right = vm_pop();
            *vm_tos() += right;
            pc++;
            break;
        }
        case OP_SUB: {
            uint64_t right = vm_pop();
            printf("%lld -  %lld \n", *vm_tos(), right);
            *vm_tos() -= right;
            pc++;
            break;
        }
        case OP_MUL: {
            uint64_t right = vm_pop();
            *vm_tos() *= right;
            pc++;
            break;
        }
        case OP_DIV: {
            uint64_t right = vm_pop();
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
            uint64_t arg = vm_pop();
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
                vm.ip = bytecode + vm_next();
                pc++;
            }
            break;
        }
        case OP_JNE:
            pc++;
            if (!vm_peek()) {
                (void)vm_pop();
                vm.ip = bytecode + vm_next();
                pc++;
            }
            break;

        case OP_PRINT: {
            printf("%lli\n", vm_pop());
            pc++;
            break;
        }
        case OP_RET: {
            Word addr = *(--vm.cstack_top);
            vm.ip = bytecode + addr;
            pc = addr;
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

int main(void) {

    vm_init();

    Byte_Code *bc = bc_load("examples/test.atom");
    if (!bc)
        abort();

    bc_disassemble(bc);

    if (vm_interpret(bc) < 0)
        abort();

    // vm_print_stack();

    bc_free(bc);

    return 0;
}
