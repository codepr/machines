プルート Pluto Spec
===================

Simple register-based virtual machine with 4 registers at 64bit. See examples/ for
small assembly programs to run.

Registers
===================
AX Accumulator
BX Base
CX Counter
DX Data
PC Program counter
SP Stack pointer
F  Flags

Instructions
====================

R: Register I: Immediate A: Address (memory/label)

------+----------+---------------------------------------+--------------
Value | Opcode   | Description                           | Operand types
------+----------+---------------------------------------+--------------
 0x00 |  NOP     | No operation                          | N/A
 0x01 |  CLF     | Clear flags                           | N/A
 0x02 |  CMP     | Compare two operands                  | R, I, A
 0x03 |  MOV     | Move data from one operand to another | R, I, A
 0x03 |  PSH     | Push value to stack                   | R, I, A
 0x04 |  POP     | Pop value from stack into a register  | R, A
 0x05 |  ADD     | Add two operands                      | R, I, A
 0x06 |  SUB     | Subtract two operands                 | R, I, A
 0x07 |  MUL     | Multiply two operands                 | R, I, A
 0x08 |  DIV     | Divide two operands                   | R, I, A
 0x09 |  MOD     | Modulo operation on two operands      | R, I, A
 0x0a |  INC     | Increment operand                     | R, A
 0x0b |  DEC     | Decrement operand                     | R, A
 0x0c |  AND     | Bitwise AND between operands          | R, I, A
 0x0d |  BOR     | Bitwise OR between operands           | R, I, A
 0x0e |  XOR     | Bitwise XOR between operands          | R, I, A
 0x0f |  NOT     | Bitwise NOT of an operand             | R, A
 0x10 |  SHL     | Shift left                            | R, I, A
 0x11 |  SHR     | Shift right                           | R, I, A
 0x12 |  JMP     | Unconditional jump to address         | A
 0x13 |  JEQ     | Jump if equal                         | A
 0x14 |  JNE     | Jump if not equal                     | A
 0x15 |  JLE     | Jump if less than or equal            | A
 0x16 |  JLT     | Jump if less than                     | A
 0x17 |  JGE     | Jump if greater than or equal         | A
 0x18 |  JGT     | Jump if greater than                  | A
 0x19 |  CALL    | Call a subroutine                     | A
 0x1a |  RET     | Return from subroutine                | N/A
 0x1b |  SYSCALL | System call                           | N/A
 0x1c |  HLT     | Halt the execution                    | N/A

Semantic rules
====================

NOP, RET, SYSCALL, HLT     Takes not arguments
----------------+-------------------------------------------------------

CMP
----------------+-------------------------------------------------------
CMP AX, BX      | Compares AX register with BX register
CMP AX, 11      | Compares AX register with immediate value 11
CMP AX, [0x1f]  | Compares AX register with memory value at address 0x1f

MOV
----------------+-------------------------------------------------------
MOV AX, BX      | Move the content of BX register into AX register
MOV AX, [0x1A]  | Move memory at address 0x1A into AX register
MOV AX, 32      | Move immediate value 32 into AX register
MOV DX, [BX]    | Move indirect register BX into DX
MOV [0x1A], AX  | Move AX register content into memory at address 0x1A
MOV [0x1F], 32  | Move immmediate value 32 into memory at addres 0x1F


ADD, SUB, MUL, DIV, MOD, SHL, SHR, BOR, XOR, AND
----------------+-------------------------------------------------------
ADD AX, BX      | Add BX register to AX register into AX register
ADD AX, 32      | Add immediate value 32 to register AX
ADD AX, [0x1A]  | Add memory at address 0x1A to register AX
ADD [0x1A], 11  | Add immediate value 11 to memory at address 0x1A
ADD [0x1A], AX  | Add register AX to memory at address 0x1A

PSH
----------------+-------------------------------------------------------
PSH DX          | Push content of DX register into the stack
PSH [0x1B]      | Push content of memory address 0x1B into the stack
PSH 943         | Push immediate value into the stack

POP, INC, DEC, NOT
----------------+--------------------------------------------------------
POP DX          | Pop content of the stack into DX register
POP [0x1C]      | Pop content of the stack into memory address 0x1C
INC BX          | Increment BX register
DEC [0x1C]      | Decrement BX register

JMP, JEQ, JNE, JGT, JLT, JGE, CALL
----------------+---------------------------------------------------------
JMP 0x24        | Unconditional jump to PC 0x24
JEQ 0x24        | Jump when equal to PC 0x24
JNE 0x24        | Jump when not equal to PC 0x24
CALL 0x1D       | Jump to subroutine, store current PC into the stack
