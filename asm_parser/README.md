# Parser
The main parser which would parse the custom assembly and would output a binary/bytecode which would be read and executed by the VM.

## File Structure -

```
assembler/
├── main.go           # Entry point, CLI handling
├── lexer.go          # Tokenization
├── parser.go         # Instruction parsing
├── encoder.go        # Binary encoding
├── symbols.go        # Label/symbol table
├── opcodes.go        # Opcode definitions
└── writer.go         # Binary & listing output
```

## Working -
This is how the encoding works. We'll take the [first example](./tests/01_test.vm) from the [test](./tests) directory.

- Code -
```asm
R0 5 PUT
R1 15 PUT
R0 R1 SUM
R0 PRINT
DIE
```
* Binary (what parser writes)

```bash
❯ hexdump -C output.bin
00000000  63 62 69 6e 05 30 0f 32  08 d6 00 b0 00 00  |cbin.0.2......|
0000000e
```

> Note: first 4 bytes are a header (`cbin`) — strip them before decoding words.

Remove header → words (little-endian pairs):

* `05 30` → `0x3005`
* `0f 32` → `0x320F`
* `08 d6` → `0xD608`
* `00 b0` → `0xB000`
* `00 00` → `0x0000`

### Instruction storing (bit layout)

* Standard register:

```
bits 15..12 : OPCODE  (4)
bits 11..9  : DST     (3)
bits 8..6   : SRC     (3)
bits 5..0   : UNUSED  (6)
```

* Immediate:

```
bits 15..12 : OPCODE  (4)
bits 11..9  : DST     (3)
bits 8..0   : IMM9    (9)
```

* Extended:

```
bits 15..12 : OP_EXT  (4)
bits 11..9  : EXT_OP  (3)
bits 8..6   : DST     (3)
bits 5..3   : SRC     (3)
bits 2..0   : UNUSED  (3)
```
Masks:

* opcode: `& 0xF`  (4 bits)
* reg:    `& 0x7`  (3 bits)
* imm9:   `& 0x1FF` (9 bits)

### Decoding

We'll AND (&) it with 0xF, 0x7 and 0x1FF to get the 4, 3 and 9 bits respectively.

- First instruciton (`0x3005`):
```python
(0x3005 >> 12) & 0xF = 0x3
(0x3005 >> 9)  & 0x7 = 0x0
0x3005 & 0x1FF = 0x5
```
* `0x3` -> [OP_MOVI](https://github.com/architmishra-15/vm/blob/main/asm_parser/opcodes.go#L10)
* `0x0` -> [R0](https://github.com/architmishra-15/vm/blob/main/asm_parser/opcodes.go#L41)
* `0x5` -> Immediate value 5

Final - `OP_MOVI R0 5`

* Second (`0x320F`):

```text
(opcode) (0x320f >> 12) & 0xF = 0x3 -> OP_MOVI
(dst)    (0x320f >> 9)  & 0x7 = 0x1 -> R1
(imm)    0x320f & 0x1FF        = 0x0F -> 15
=> MOVI R1, 15
```

* Third (`0xD608`):

```text
(opcode) (0xD608 >> 12) & 0xF = 0xD -> OP_EXT
(extOp)  (0xD608 >> 9)  & 0x7 = 0x3 -> EXT_ADD (SUM)
(dst)    (0xD608 >> 6)  & 0x7 = 0x0 -> R0
(src)    (0xD608 >> 3)  & 0x7 = 0x1 -> R1
=> EXT ADD R0, R1   ; SUM
```

* Fourth (`0xB000`):

```text
(opcode) (0xB000 >> 12) & 0xF = 0xB -> OP_STDOUT
(dst)    (0xB000 >> 9)  & 0x7 = 0x0 -> R0
=> STDOUT R0  ; PRINT R0
```

* Fifth (`0x0000`):

```text
(opcode) 0x0 -> OP_HALT
=> HALT
```
---

# Immediate sign rules (do this when executing MOVI)

* imm9 is **9-bit two’s complement** → range `-256 .. +255`.
* Sign-extend imm9 → 16 bits before storing/operating.

**Quick sign-extend (Go):**

```go
// returns int16
func signExtend9ToInt16(imm9 uint16) int16 {
    return int16(imm9<<7) >> 7
}
```

Or explicit:

```go
if imm9 & 0x100 != 0 {
    val = int16(imm9 | 0xFE00) // negative
} else {
    val = int16(imm9)
}
```

---
### Encoder formulas (how word is built)

* register:

```
word = (uint16(op) << 12) | (uint16(dst) << 9) | (uint16(src) << 6)
```

* immediate:

```
word = (uint16(op) << 12) | (uint16(dst) << 9) | (imm9 & 0x1FF)
```

* extended:

```
word = (uint16(OP_EXT) << 12) | (uint16(extOp) << 9) |
       (uint16(dst) << 6) | (uint16(src) << 3)
```

### Quick decoder (copy/paste)

```go
word := uint16(low) | uint16(high)<<8
opcode := (word >> 12) & 0xF
switch opcode {
case OP_EXT:
    ext := (word >> 9) & 0x7
    dst := (word >> 6) & 0x7
    src := (word >> 3) & 0x7
case OP_MOVI:
    dst := (word >> 9) & 0x7
    imm9 := word & 0x1FF
    val := signExtend9ToInt16(imm9)
default:
    dst := (word >> 9) & 0x7
    src := (word >> 6) & 0x7
}
```

---
