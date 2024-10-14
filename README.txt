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
  0x02 |  CLF
  0x03 |  CMP
  0x03 |  CMI
  0x04 |  LDI
  0x05 |  LDR
  0x06 |  STI
  0x07 |  STR
  0x08 |  PSR
  0x09 |  PSM
  0x0a |  PSI
  0x0b |  POP
  0x0c |  POM
  0x0d |  ADD
  0x0e |  ADI
  0x0f |  SUB
  0x10 |  SBI
  0x11 |  MUL
  0x12 |  MLI
  0x13 |  DIV
  0x14 |  DVI
  0x15 |  MOD
  0x16 |  MDI
  0x17 |  INC
  0x18 |  DEC
  0x19 |  AND
  0x1a |  BOR
  0x1b |  XOR
  0x1c |  NOT
  0x1d |  SHL
  0x1e |  SHR
  0x1f |  JMP
  0x20 |  JEQ
  0x21 |  JNE
  0x22 |  JLE
  0x23 |  JLT
  0x24 |  JGE
  0x25 |  JGT
  0x26 |  CALL
  0x27 |  RET
  0x28 |  HLT

 Semantic rules
 ====================

 NOP, RET, HLT     Takes not arguments

 CMP
 ----------------+-------------------------------------------------------
 CMP AX, BX      | Compares AX register with BX register


 LDI, LDR, STI, STR
 ----------------+-------------------------------------------------------
 LDR AX, [0x12]  | Load memory at address 0x12 into AX register
 LDI AX, 32      | Load immediate value 32 into AX register
 STR [0x12], AX  | Store AX register content into memory at address 0x12
 STI [0x1F], 32  | Store immmediate value 32 into memory at addres 0x1F

 MOV
 ----------------+-------------------------------------------------------
 MOV AX, BX      | Move the content of BX register into AX register

 ADD, ADI, SUB, SBI, MUL, MLI, DIV, DVI, MOD, MDI
 ----------------+-------------------------------------------------------
 ADD AX, BX      | Add BX register to AX register into AX register
 ADI AX, 32      | Add immediate value 32 to register AX

 PSR, PSM, PSI
 ----------------+-------------------------------------------------------
 PSR DX          | Push content of DX register into the stack
 PSM [0x1B]      | Push content of memory address 0x1B into the stack
 PSI 943         | Push immediate value into the stack

 POP, POM, INC, DEC
 ----------------+--------------------------------------------------------
 POP DX          | Pop content of the stack into DX register
 POM [0x1C]      | Pop content of the stack into memory address 0x1C
 INC BX          | Increment BX register

 JMP, JEQ, JNE, CALL
 ----------------+---------------------------------------------------------
 JMP 0x24        | Unconditional jump to PC 0x24
 JEQ 0x24        | Jump when equal to PC 0x24
 JNE 0x24	 | Jump when not equal to PC 0x24
 CLL 0x1D        | Jump to subroutine, store current PC into the stack
