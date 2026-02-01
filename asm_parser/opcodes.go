package main

type Opcode uint8

const (
	OP_HALT Opcode = 0x0
	OP_NOP  Opcode = 0x1
	OP_MOV  Opcode = 0x2
	OP_MOVI Opcode = 0x3
	OP_ADD  Opcode = 0x4
	OP_SUB  Opcode = 0x5
	OP_AND  Opcode = 0x6
	OP_OR   Opcode = 0x7
	OP_XOR  Opcode = 0x8
	OP_CMP  Opcode = 0x9
	OP_JMP  Opcode = 0xA
	OP_JZ   Opcode = 0xB
	OP_JNZ  Opcode = 0xC
	OP_PUSH Opcode = 0xD
	OP_POP  Opcode = 0xE
	OP_CALL Opcode = 0xF
)

// Register represents a VM register
type Register uint8

const (
	R0 Register = 0
	R1 Register = 1
	R2 Register = 2
	R3 Register = 3
	R4 Register = 4
	R5 Register = 5
	R6 Register = 6
	R7 Register = 7
)

// OpcodeMap maps assembly mnemonics to opcodes
var OpcodeMap = map[string]Opcode{
	// Full names
	"DIE":      OP_HALT,
	"IDLE":     OP_NOP,
	"SET":      OP_MOV,
	"PUT":      OP_MOVI,
	"SUM":      OP_ADD,
	"DIF":      OP_SUB,
	"AND":      OP_AND,
	"OR":       OP_OR,
	"XOR":      OP_XOR,
	"CHECK":    OP_CMP,
	"TELEPORT": OP_JMP,
	"IFZ":      OP_JZ,
	"IFNZ":     OP_JNZ,
	"DEPOSIT":  OP_PUSH,
	"WITHDRAW": OP_POP,
	"EXEC":     OP_CALL,

	// Shorthand
	"X": OP_HALT,
	"M": OP_MOV,
	"I": OP_MOVI,
	"+": OP_ADD,
	"-": OP_SUB,
	"&": OP_AND,
	"|": OP_OR,
	"^": OP_XOR,
	"?": OP_CMP,
	"@": OP_JMP,
	">": OP_PUSH,
	"<": OP_POP,
}

// RegisterMap maps register names to register numbers
var RegisterMap = map[string]Register{
	"R0": R0,
	"R1": R1,
	"R2": R2,
	"R3": R3,
	"R4": R4,
	"R5": R5,
	"R6": R6,
	"R7": R7,
}

// InstructionType defines how many operands an instruction needs
type InstructionType int

const (
	TypeNone      InstructionType = iota // No operands (HALT, NOP)
	TypeOneReg                            // One register (PUSH, POP, JMP, etc.)
	TypeTwoReg                            // Two registers (MOV, ADD, SUB, etc.)
	TypeRegImm                            // Register + immediate (MOVI)
)

// OpcodeInfo holds metadata about an opcode
type OpcodeInfo struct {
	Type InstructionType
	Name string
}

// OpcodeTable maps opcodes to their metadata
var OpcodeTable = map[Opcode]OpcodeInfo{
	OP_HALT: {TypeNone, "HALT"},
	OP_NOP:  {TypeNone, "NOP"},
	OP_MOV:  {TypeTwoReg, "MOV"},
	OP_MOVI: {TypeRegImm, "MOVI"},
	OP_ADD:  {TypeTwoReg, "ADD"},
	OP_SUB:  {TypeTwoReg, "SUB"},
	OP_AND:  {TypeTwoReg, "AND"},
	OP_OR:   {TypeTwoReg, "OR"},
	OP_XOR:  {TypeTwoReg, "XOR"},
	OP_CMP:  {TypeTwoReg, "CMP"},
	OP_JMP:  {TypeOneReg, "JMP"},
	OP_JZ:   {TypeOneReg, "JZ"},
	OP_JNZ:  {TypeOneReg, "JNZ"},
	OP_PUSH: {TypeOneReg, "PUSH"},
	OP_POP:  {TypeOneReg, "POP"},
	OP_CALL: {TypeOneReg, "CALL"},
}

