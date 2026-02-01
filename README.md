# Project

A 16-bit virtual machine written in C with a custom assembly language and assembler written in Go.

## Specs

- **Architecture**: 16-bit
- **Endianness**: Little Endian
- **Memory**: 1 MiB (0x00000 - 0xFFFFF)
- **Stack**: Grows downward from top of memory
- **Instruction Format**: 16-bit fixed width
  - Format 1: `OPCODE(4) | DST_REG(3) | SRC_REG(3) | UNUSED(6)`
  - Format 2: `OPCODE(4) | REG(3) | IMMEDIATE(9)`

### Registers

- **R0-R7**: General purpose (16-bit)
- **PC**: Program counter (20-bit)
- **SP**: Stack pointer (20-bit)
- **FLAGS**: Status flags (8-bit)
  - Zero (Z)
  - Sign (S)
  - Carry (C)
  - Overflow (O)

## Assembly Syntax

Instructions follow reverse Polish notation:
```
<operand1> <operand2> <opcode>
```

Example program:
```assembly
R0 5 PUT         ; Load 5 into R0
R1 10 PUT        ; Load 10 into R1
R0 R1 SUM        ; R0 = R0 + R1
R0 R1 CHECK      ; Compare R0 and R1
R2 IFZ           ; Jump to address in R2 if zero
DIE              ; Halt
```

### Opcodes

| OpCode    | Equivalent | Description                  |
|-----------|------------|------------------------------|
| DIE       | HALT       | Stop execution               |
| IDLE      | NOP        | No operation                 |
| SET       | MOV        | Copy register to register    |
| PUT       | MOVI       | Load immediate value         |
| SUM       | ADD        | Add two registers            |
| DIF       | SUB        | Subtract two registers       |
| AND       | AND        | Bitwise AND                  |
| OR        | OR         | Bitwise OR                   |
| XOR       | XOR        | Bitwise XOR                  |
| CHECK     | CMP        | Compare (sets flags)         |
| TELEPORT  | JMP        | Unconditional jump           |
| IFZ       | JZ         | Jump if zero flag set        |
| IFNZ      | JNZ        | Jump if zero flag clear      |
| DEPOSIT   | PUSH       | Push to stack                |
| WITHDRAW  | POP        | Pop from stack               |
| EXEC      | CALL       | Call subroutine              |

### Shorthand Opcodes

Single-character alternatives for compact code:

| Symbol | OpCode |
|--------|--------|
| X      | HALT   |
| M      | MOV    |
| I      | MOVI   |
| +      | ADD    |
| -      | SUB    |
| &      | AND    |
| \|     | OR     |
| ^      | XOR    |
| ?      | CMP    |
| @      | JMP    |
| >      | PUSH   |
| <      | POP    |

## Usage

Assemble a program:
```bash
go run assembler.go program.asm program.bin
```

Run the bytecode:
```bash
./vm program.bin
```

## Building

### Using Makefile -
```
make all -j8
```

### Manual -

VM:
```bash
gcc -o vm main.c -std=c23 -fPIC -O3
```

Assembler:
```bash
cd asm_parser
go build .
```
