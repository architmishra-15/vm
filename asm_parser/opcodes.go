package main

// Opcode represents a VM instruction
type Opcode uint8

const (
	OP_HALT   Opcode = 0x0
	OP_NOP    Opcode = 0x1
	OP_MOV    Opcode = 0x2
	OP_MOVI   Opcode = 0x3
	OP_CMP    Opcode = 0x4
	OP_JMP    Opcode = 0x5
	OP_JZ     Opcode = 0x6
	OP_JNZ    Opcode = 0x7
	OP_PUSH   Opcode = 0x8
	OP_POP    Opcode = 0x9
	OP_CALL   Opcode = 0xA
	OP_STDOUT Opcode = 0xB
	OP_STDIN  Opcode = 0xC
	OP_EXT    Opcode = 0xD
)

// ExtOpcode represents extended opcodes (when OP_EXT is used)
type ExtOpcode uint8

const (
	EXT_RET   ExtOpcode = 0x0
	EXT_LOAD  ExtOpcode = 0x1
	EXT_STORE ExtOpcode = 0x2
	EXT_ADD   ExtOpcode = 0x3
	EXT_SUB   ExtOpcode = 0x4
	EXT_AND   ExtOpcode = 0x5
	EXT_OR    ExtOpcode = 0x6
	EXT_XOR   ExtOpcode = 0x7
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
	"CHECK":    OP_CMP,
	"TELEPORT": OP_JMP,
	"IFZ":      OP_JZ,
	"IFNZ":     OP_JNZ,
	"DEPOSIT":  OP_PUSH,
	"WITHDRAW": OP_POP,
	"EXEC":     OP_CALL,
	"PRINT":    OP_STDOUT,
	"READ":     OP_STDIN,

	// Shorthand
	"X":  OP_HALT,
	"M":  OP_MOV,
	"I":  OP_MOVI,
	"?":  OP_CMP,
	"@":  OP_JMP,
	">":  OP_PUSH,
	"<":  OP_POP,
	"P":  OP_STDOUT,
	"IN": OP_STDIN,
}

// ExtOpcodeMap maps assembly mnemonics to extended opcodes
var ExtOpcodeMap = map[string]ExtOpcode{
	// Full names
	"RETURN": EXT_RET,
	"FETCH":  EXT_LOAD,
	"SAVE":   EXT_STORE,
	"SUM":    EXT_ADD,
	"DIF":    EXT_SUB,
	"BAND":   EXT_AND,
	"BOR":    EXT_OR,
	"BXOR":   EXT_XOR,

	// Shorthand
	"RET": EXT_RET,
	"+":   EXT_ADD,
	"-":   EXT_SUB,
	"&":   EXT_AND,
	"|":   EXT_OR,
	"^":   EXT_XOR,
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
	TypeExtended                          // Extended opcode instruction
)

// OpcodeInfo holds metadata about an opcode
type OpcodeInfo struct {
	Type InstructionType
	Name string
}

// OpcodeTable maps opcodes to their metadata
var OpcodeTable = map[Opcode]OpcodeInfo{
	OP_HALT:   {TypeNone, "HALT"},
	OP_NOP:    {TypeNone, "NOP"},
	OP_MOV:    {TypeTwoReg, "MOV"},
	OP_MOVI:   {TypeRegImm, "MOVI"},
	OP_CMP:    {TypeTwoReg, "CMP"},
	OP_JMP:    {TypeOneReg, "JMP"},
	OP_JZ:     {TypeOneReg, "JZ"},
	OP_JNZ:    {TypeOneReg, "JNZ"},
	OP_PUSH:   {TypeOneReg, "PUSH"},
	OP_POP:    {TypeOneReg, "POP"},
	OP_CALL:   {TypeOneReg, "CALL"},
	OP_STDOUT: {TypeOneReg, "STDOUT"},
	OP_STDIN:  {TypeOneReg, "STDIN"},
	OP_EXT:    {TypeExtended, "EXT"},
}

// ExtOpcodeInfo holds metadata about extended opcodes
type ExtOpcodeInfo struct {
	Type InstructionType
	Name string
}

// ExtOpcodeTable maps extended opcodes to their metadata
var ExtOpcodeTable = map[ExtOpcode]ExtOpcodeInfo{
	EXT_RET:   {TypeNone, "RET"},
	EXT_LOAD:  {TypeTwoReg, "LOAD"},
	EXT_STORE: {TypeTwoReg, "STORE"},
	EXT_ADD:   {TypeTwoReg, "ADD"},
	EXT_SUB:   {TypeTwoReg, "SUB"},
	EXT_AND:   {TypeTwoReg, "AND"},
	EXT_OR:    {TypeTwoReg, "OR"},
	EXT_XOR:   {TypeTwoReg, "XOR"},
}

// IsExtendedOpcode checks if a mnemonic is an extended opcode
func IsExtendedOpcode(mnemonic string) bool {
	_, ok := ExtOpcodeMap[mnemonic]
	return ok
}
