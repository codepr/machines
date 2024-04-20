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
  0x02 |  LDI
  0x03 |  LDR
  0x04 |  STI
  0x05 |  STR
  0x06 |  PSR
  0x07 |  PSM
  0x08 |  PSI
  0x09 |  POP
  0x0a |  POM
  0x0b |  ADD
  0x0c |  ADI
  0x0d |  SUB
  0x0e |  SBI
  0x0f |  MUL
  0x10 |  MLI
  0x11 |  DIV
  0x12 |  DVI
  0x13 |  MOD
  0x14 |  MDI
  0x15 |  INC
  0x16 |  DEC
  0x17 |  AND
  0x18 |  BOR
  0x19 |  XOR
  0x1a |  NOT
  0x1b |  SHL
  0x1c |  SHR
  0x1d |  JMP
  0x1e |  JEQ
  0x1f |  JNE
  0x20 |  CALL
  0x21 |  RET
  0x22 |  HLT

 Semantic rules
 ====================

 NOP, RET, HLT     Takes not arguments

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
 JNE 0x24	  | Jump when not equal to PC 0x24
 CLL 0x1D        | Jump to subroutine, store current PC into the stack
