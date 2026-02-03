package main

type Encoder struct {}

func NewEncoder() *Encoder {
	return &Encoder{}
}

// Encode converts a single instruction to 16-bit binary
func (e *Encoder) Encode(instr Instruction) uint16 {
	if instr.IsExt {
		return e.encodeExtended(instr)
	}

	if instr.IsImm {
		return e.encodeImmediate(instr)
	}

	return e.encodeRegister(instr)
}

// Regular instruction: OPCODE(4) | DST(3) | SRC(3) | UNUSED(6)
func (e *Encoder) encodeRegister(instr Instruction) uint16{
	return (uint16(instr.Opcode) << 12) |
			(uint16(instr.Dst) << 9) |
			(uint16(instr.Src) << 6)

}

// Immediate instruction: OPCODE(4) | DST(3) | IMM(9)
func (e *Encoder) encodeImmediate(instr Instruction) uint16{
	return (uint16(instr.Opcode) << 12) |
		(uint16(instr.Dst) << 9) |
		(instr.Immediate & 0x1FF)
}

// Extended instruction: OP_EXT(4) | EXT_OP(3) | DST(3) | SRC(3) | UNUSED(3)
func (e *Encoder) encodeExtended(instr Instruction) uint16{
	return (uint16(OP_EXT) << 12) |
		(uint16(instr.ExtOpcode) << 9) |
		(uint16(instr.Dst) << 6) |
		(uint16(instr.Src) << 3)
}
func (e *Encoder) EncodeAll(instructions []Instruction) []byte{
	binary := make([]byte, len(instructions)*2)

	for i, instr := range instructions {
		encoded := e.Encode(instr)
		// Little endian
		binary[i*2] = byte(encoded & 0xFF)
		binary[i*2+1] = byte((encoded >> 8) & 0xFF)
	}

	return binary
}
