Machines - Educational Virtual Machine Project
==============================================

Playground for the development of simple compilers, an educational exploration
of virtual machine architectures and language implementation. This project
demonstrates two distinct VM paradigms with complete compilation toolchains
from high-level languages down to bytecode execution.

NOTE - it's a work in progress, still squashing bugs here and there

* Overview

This repository contains two fully functional virtual machines built from
scratch in C, along with compilers that target them from higher-level
languages. The project showcases the entire compilation pipeline: lexing,
parsing, AST generation, code generation, bytecode serialization, and
execution.

* Virtual Machines

アトム (Atom) - Stack-Based VM

A minimalist stack-based virtual machine inspired by stack machine
architectures and functional programming principles.

Architecture:
- Word Size: 64-bit unsigned integers
- Stack: 256 words
- Memory: 65,535 words
- Execution Model: Push/pop operations on implicit stack
- Instruction Set: 22 operations

Key Features:
- Stack-based arithmetic and control flow
- Function calls with return stack
- Tuple data structures
- Memory-mapped data segment

Instruction Categories:
- Memory Operations: LOAD, STORE, PUSH
- Arithmetic: ADD, SUB, MUL, DIV, INC
- Control Flow: JMP, JEQ, JNE, CALL, RET
- Data Structures: MAKE_TUPLE
- I/O: PRINT

プルート (Pluto) - Register-Based VM

A register-based virtual machine modeled after x86-64 architecture with
simplified instruction encoding and 4 general-purpose registers.

Architecture:
- Word Size: 64-bit signed integers
- Registers: 4 GP registers (AX, BX, CX, DX) + PC, SP, Flags
- Stack: 2,048 words
- Memory: 32KB addressable
- Execution Model: Explicit register manipulation
- Instruction Set: 29 operations

Key Features:
- Multiple addressing modes (register, immediate, memory, indirect)
- Flag register (Zero, Negative, Positive)
- System call interface (read, write, atoi, exit)
- Rich conditional branching

Instruction Categories:
- Data Movement: MOV, PSH, POP
- Arithmetic: ADD, SUB, MUL, DIV, MOD, INC, DEC
- Bitwise: AND, BOR, XOR, NOT, SHL, SHR
- Comparison: CMP
- Control Flow: JMP, JEQ, JNE, JLE, JLT, JGE, JGT, CALL, RET
- System: SYSCALL

* Compilation Pipeline

  High-Level Language (LISP/StackLang)
      ↓ [Ruby Lexer]
  Tokens
      ↓ [Ruby Parser]
  Abstract Syntax Tree
      ↓ [Ruby Compiler]
  Assembly (.atom/.pluto)
      ↓ [C Assembler]
  Bytecode (binary format)
      ↓ [C VM Interpreter]
  Execution & Output

* Building

Each VM can be built independently:

make -C atom-vm
make -C pluto-vm

* Running Examples

Pluto VM

./pluto-vm/pluto-vm examples/fact.pluto  # Factorial calculation
./pluto-vm/pluto-vm examples/mul.pluto   # Interactive multiplication

Key Implementation Details
==========================

  Atom VM:
  - Bytecode serialization with 8-byte instruction encoding
  - PC-addressed assembly with label resolution
  - Call stack for function returns
  - Separate data and code segments

  Pluto VM:
  - Flexible operand encoding (register/immediate/address/indirect)
  - Flag register for conditional execution
  - System call interface for I/O operations
  - Data directives: db, dw, dd, dq

Educational Goals
=================

This project demonstrates:
1. Two VM Paradigms: Stack vs. register-based architectures
2. Toolchain: From source code to execution
3. Language Implementation: Lexing, parsing, compilation, and code generation
4. Low-Level Programming: Memory management, bytecode design, instruction encoding
5. Systems Programming: C implementations with strict correctness guarantees
