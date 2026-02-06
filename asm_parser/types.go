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
	TokenLabel
	TokenUndefined
)

// Token represents a single assembly token
type Token struct {
	Value string    // 16 bytes (string header)
	Line  int       // 8 bytes
	Col   int       // 8 bytes
	Type  TokenType // 8 bytes (int on 64-bit)
}

// Line represents a parsed line of assembly
type Line struct {
	Tokens   []Token
	Original string
	Number   int
}

// Instruction represents a parsed instruction before encoding
type Instruction struct {
	Line      int       // 8 bytes (align first for best packing)
	Immediate uint16    // 2 bytes
	Opcode    Opcode    // 1 byte  
	ExtOpcode ExtOpcode // 1 byte
	Dst       Register  // 1 byte
	Src       Register  // 1 byte
	IsExt     bool      // 1 byte
	IsImm     bool      // 1 byte
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
