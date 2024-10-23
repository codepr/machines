 T-800-64 Spec
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

 ------+--------
 Value | Opcode
 ------+--------
  0x00 |  NOP
  0x01 |  CLF
  0x02 |  CMP
  0x03 |  MOV
  0x03 |  PSH
  0x04 |  POP
  0x05 |  ADD
  0x06 |  SUB
  0x07 |  MUL
  0x08 |  DIV
  0x09 |  MOD
  0x0a |  INC
  0x0b |  DEC
  0x0c |  AND
  0x0d |  BOR
  0x0e |  XOR
  0x0f |  NOT
  0x10 |  SHL
  0x11 |  SHR
  0x12 |  JMP
  0x13 |  JEQ
  0x14 |  JNE
  0x15 |  JLE
  0x16 |  JLT
  0x17 |  JGE
  0x18 |  JGT
  0x19 |  CALL
  0x1a |  RET
  0x1b |  SYSCALL
  0x1c |  HLT

 Semantic rules
 ====================

 NOP, RET, SYSCALL, HLT     Takes not arguments

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
 MOV [0x1A], AX  | Move AX register content into memory at address 0x1A
 MOV [0x1F], 32  | Move immmediate value 32 into memory at addres 0x1F


 ADD, SUB, MUL, DIV, MOD
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

 POP, INC, DEC
 ----------------+--------------------------------------------------------
 POP DX          | Pop content of the stack into DX register
 POP [0x1C]      | Pop content of the stack into memory address 0x1C
 INC BX          | Increment BX register
 DEC [0x1C]      | Decrement BX register

 JMP, JEQ, JNE, CALL
 ----------------+---------------------------------------------------------
 JMP 0x24        | Unconditional jump to PC 0x24
 JEQ 0x24        | Jump when equal to PC 0x24
 JNE 0x24	     | Jump when not equal to PC 0x24
 CALL 0x1D       | Jump to subroutine, store current PC into the stack
