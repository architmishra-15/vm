package main

// TokenType represents different kinds of tokens
type TokenType int

const (
	TokenEOF TokenType = iota
	TokenRegister
	TokenNumber
	TokenOpcode
	TokenExtOpcode
	TokenComment
	TokenLabel // For future when you add labels
)

// Token represents a single assembly token
type Token struct {
	Type  TokenType
	Value string
	Line  int
	Col   int
}

// Line represents a parsed line of assembly
type Line struct {
	Tokens   []Token
	Original string
	Number   int
}

// Instruction represents a parsed instruction before encoding
type Instruction struct {
	Opcode 	 	Opcode
	ExtOpcode 	ExtOpcode
	Dst			Register
	Src 		Register
	Immediate	uint16
	IsExt     bool // Is this an extended opcode?
	IsImm     bool // Does this use immediate value?
	Line      int  // Line number for error reporting
}

// TODO: Extend this with more values in the future
func (t TokenType) String() string {
	switch t {
	case TokenEOF:
		return "EOF"
	case TokenRegister:
		return "REGISTER"
	case TokenNumber:
		return "NUMBER"
	case TokenOpcode:
		return "OPCODE"
	case TokenExtOpcode:
		return "EXT_OPCODE"
	case TokenComment:
		return "COMMENT"
	case TokenLabel:
		return "LABEL"
	default:
		return "UNKNOWN"
	}
}
