 T-800-64 Spec
 ===================

 Simple CPU with 4 registers at 64bit.

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
  0x01 |  MOV
  0x02 |  MOVF
  0x03 |  PSH
  0x04 |  PSHF
  0x05 |  POP
  0x06 |  POPF
  0x07 |  ADD
  0x08 |  SUB
  0x09 |  MUL
  0x0a |  DIV
  0x0b |  MOD
  0x0c |  INC
  0x0d |  DEC
  0x0e |  FADD
  0x0f |  FSUB
  0x10 |  FMUL
  0x11 |  FDIV
  0x12 |  FMOD
  0x13 |  FINC
  0x14 |  FDEC
  0x15 |  AND
  0x16 |  BOR
  0x17 |  XOR
  0x18 |  NOT
  0x19 |  SHL
  0x1a |  SHR
  0x1b |  JMP
  0x1c |  JEQ
  0x1d |  JNE
  0x1e |  CALL
  0x1f |  RET
  0x20 |  HLT

 Semantic rules
 ====================

 NOP, RET, HLT     Takes not arguments

 MOV, ADD, SUB, MUL, DIV, MOD, MOVF, FADD, FSUB, FMUL, FDIV, FMOD
 ----------------+-------------------------------------------------------
 MOV AX, BX      | Move the content of BX register into AX register
 MOV [0x12], AX  | Move the content of AX into memory address 0x12
 MOV CX, [0x12]  | Move the content memory address 0x12 into register CX
 MOV AX, 32      | Move immediate value into AX register
 MOV [0x08], 57  | Move immediate value into memory address 0x08

 PSH, PSHF
 ----------------+-------------------------------------------------------
 PSH DX          | Push content of DX register into the stack
 PSH [0x1B]      | Push content of memory address 0x1B into the stack
 PSH 943         | Push immediate value into the stack

 POP, POPF, INC, INCF, DEC, DECF
 ----------------+--------------------------------------------------------
 POP DX          | Pop content of the stack into DX register
 POP [0x1C]      | Pop content of the stack into memory address 0x1C

 JMP, JEQ, JNE, CALL
 ----------------+---------------------------------------------------------
 JMP 0x24        | Unconditional jump to PC 0x24
 JEQ 0x24        | Jump when equal to PC 0x24
 JNE 0x24	  | Jump when not equal to PC 0x24
 CLL 0x1D        | Jump to subroutine, store current PC into the stack
