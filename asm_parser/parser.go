package main

import (
	"fmt"
	"strconv"
	"strings"
)

// Parser converts tokens into instructions
type Parser struct {
	symbolTable *SymbolTable
}

func NewParser(symbolTable *SymbolTable) *Parser {
	return &Parser{
		symbolTable: symbolTable,
	}
}

// Parse converts lines of tokens into instructions
func (p *Parser) Parse(lines []Line) ([]Instruction, error) {
	instructions := make([]Instruction, 0, len(lines))

	for _, line := range lines {
		if len(line.Tokens) == 0 {
			continue
		}

		// Skip label definitions (they were already processed in Pass 1)
		if len(line.Tokens) == 1 && line.Tokens[0].Type == TokenLabel {
			continue
		}

		instr, err := p.ParseLine(line)
		if err != nil {
			return nil, fmt.Errorf("line %d: %w\n  → %s", line.Number, err, line.Original)
		}

		instructions = append(instructions, instr)
	}
	return instructions, nil
}

func (p *Parser) ParseLine(line Line) (Instruction, error) {
	tokens := line.Tokens
	if len(tokens) == 0 {
		return Instruction{}, fmt.Errorf("empty line")
	}

	// Resolve any label references to addresses
	tokens, err := p.resolveLabels(tokens, line.Number)
	if err != nil {
		return Instruction{}, err
	}

	// Last token should be opcode
	lastToken := tokens[len(tokens)-1]

	if lastToken.Type == TokenExtOpcode {
		return p.ParseExtended(line)
	}

	if lastToken.Type != TokenOpcode {
		return Instruction{}, fmt.Errorf("expected opcode, got %s", lastToken.Value)
	}

	return p.ParseRegular(line, tokens)
}

func (p *Parser) resolveLabels(tokens []Token, lineNo int) ([]Token, error) {
	resolved := make([]Token, len(tokens))
	copy(resolved, tokens)

	for i, token := range resolved {
		if token.Type == TokenLabel {
			// Symbol Table lookup
			addr, ok := p.symbolTable.Resolve(token.Value)
			if !ok {
				return nil, fmt.Errorf("undefined label: %s", token.Value)
			}
			resolved[i].Type = TokenNumber
			resolved[i].Value = fmt.Sprintf("%d", addr)
		}
		if token.Type == TokenUndefined {
			fmt.Printf("Undefined token: \"%v\"\nLine: %v:%v\n", token.Value, token.Line, token.Col)
		}
	}

	return resolved, nil
}

func (p *Parser) ParseRegular(line Line, tokens []Token) (Instruction, error) {
	opToken := tokens[len(tokens) - 1]
	op := OpcodeMap[opToken.Value]

	instr := Instruction {
		Opcode: op,
		Line: line.Number,
	}

	switch op {
	case OP_HALT, OP_NOP:
		if len(tokens) != 1 {
			return Instruction{}, fmt.Errorf("%s takes no operands", opToken.Value)
		}


	// One operand (register OR immediate for jumps)
	case OP_JMP, OP_JZ, OP_JNZ, OP_PUSH, OP_POP, OP_CALL, OP_STDOUT, OP_STDIN:
		if len(tokens) != 2 {
			return Instruction{}, fmt.Errorf("%s needs 1 operand", opToken.Value)
		}

		// For jumps, allow immediate values (resolved labels)
		if op == OP_JMP || op == OP_JZ || op == OP_JNZ || op == OP_CALL {
			if tokens[0].Type == TokenNumber {
				imm, err := parseNumber(tokens[0].Value)
				if err != nil {
					return Instruction{}, err
				}
				instr.Immediate = imm
				instr.IsImm = true
			} else if tokens[0].Type == TokenRegister {
				instr.Dst = RegisterMap[tokens[0].Value]
			} else {
				return Instruction{}, fmt.Errorf("expected register or address")
			}
		} else {
			// Other ops need register
			if tokens[0].Type != TokenRegister {
				return Instruction{}, fmt.Errorf("expected register, got %s", tokens[0].Value)
			}
			instr.Dst = RegisterMap[tokens[0].Value]
		}


	// Two registers needed
	case OP_MOV, OP_CMP:
		if len(tokens) != 3 {
			return Instruction{}, fmt.Errorf("%s needs 2 registers", opToken.Value)
		}
		if tokens[0].Type != TokenRegister || tokens[1].Type != TokenRegister {
			return Instruction{}, fmt.Errorf("expected 2 registers")
		}
		instr.Dst = RegisterMap[tokens[0].Value]
		instr.Src = RegisterMap[tokens[1].Value]


	// Register + immediate
	case OP_MOVI:
		if len(tokens) != 3 {
			return Instruction{}, fmt.Errorf("%s needs register and value", opToken.Value)
		}
		if tokens[0].Type != TokenRegister {
			return Instruction{}, fmt.Errorf("expected register")
		}
		if tokens[1].Type != TokenNumber {
			return Instruction{}, fmt.Errorf("expected number, got %s", tokens[1].Value)
		}

		instr.Dst = RegisterMap[tokens[0].Value]
		imm, err := parseNumber(tokens[1].Value)
		if err != nil {
			return Instruction{}, err
		}
		instr.Immediate = imm
		instr.IsImm = true


	default:
		return Instruction{}, fmt.Errorf("unhandled opcode: %s", opToken.Value)
	}

	return instr, nil
}

func (p *Parser) ParseExtended(line Line) (Instruction, error) {
	tokens := line.Tokens
	extToken := tokens[len(tokens)-1]
	extOp := ExtOpcodeMap[extToken.Value]

	instr := Instruction{
		Opcode:    OP_EXT,
		ExtOpcode: extOp,
		IsExt:     true,
		Line:      line.Number,
	}

	switch extOp {
	// No operands
	case EXT_RET:
		if len(tokens) != 1 {
			return Instruction{}, fmt.Errorf("%s takes no operands", extToken.Value)
		}

	// Two registers
	case EXT_LOAD, EXT_STORE, EXT_ADD, EXT_SUB, EXT_AND, EXT_OR, EXT_XOR:
		if len(tokens) != 3 {
			return Instruction{}, fmt.Errorf("%s needs 2 registers", extToken.Value)
		}
		if tokens[0].Type != TokenRegister || tokens[1].Type != TokenRegister {
			return Instruction{}, fmt.Errorf("expected 2 registers")
		}
		instr.Dst = RegisterMap[tokens[0].Value]
		instr.Src = RegisterMap[tokens[1].Value]

	default:
		return Instruction{}, fmt.Errorf("unhandled extended opcode: %s", extToken.Value)
	}

	return instr, nil
}


func parseNumber(s string) (uint16, error) {
	s = strings.ToUpper(strings.TrimSpace(s))

	if strings.HasPrefix(s, "0X") {
		val, err := strconv.ParseUint(s[2:], 16, 16)
		return uint16(val), err
	}

	val, err := strconv.ParseInt(s, 10, 16)
	return uint16(val), err
}
