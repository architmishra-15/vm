package main

import (
	"strings"
)

type Lexer struct {
	lines []string
	internPool map[string]string // Reuse common strings
}

func NewLexer(src string) *Lexer {
	return &Lexer{
		lines: strings.Split(src, "\n"),
		internPool: make(map[string]string, 64),
	}
}

func (l *Lexer) intern(s string) string {
    if interned, ok := l.internPool[s]; ok {
        return interned
    }
    l.internPool[s] = s
    return s
}

// I honestly hate K&R style braces, why tf Go don't allow Allaman Style?!
func (l *Lexer) Tokenize() ([]Line, error) {

	result := make([]Line, 0, len(l.lines))

	for lineNum, rawLine := range l.lines {
		line := l.tokenizeLine(rawLine, lineNum+1) 
		if line != nil {
			result = append(result, *line)
		}
	}

	return result, nil
}

func (l *Lexer) tokenizeLine(rawLine string, lineNo int) *Line {
	original := rawLine
	line := strings.TrimSpace(rawLine)

	if line == "" {
		return nil
	}

	// Comments - ';' & '#'
	if strings.HasPrefix(line, ";") || strings.HasPrefix(line, "#") {
		return nil
	}

	// Remove inline comments
	if idx := strings.Index(line, ";"); idx >= 0 {
		line = strings.TrimSpace(line[:idx])
	}
	if idx := strings.Index(line, "#"); idx >= 0 {
		line = strings.TrimSpace(line[:idx])
	}

	// Skip if empty after comment removal
	if line == "" {
		return nil
	}

	line = strings.ReplaceAll(line, ",", " ")
	fields := strings.Fields(line)

	parsedLine := &Line {
		Original: original,
		Number: lineNo,
		Tokens: make([]Token, 0, len(fields)),
	}

	for col, field := range fields {
		token := l.identifyToken(field, lineNo, col)
		parsedLine.Tokens = append(parsedLine.Tokens, token)
	}

	return parsedLine
}

func (l *Lexer) identifyToken(field string, line, col int) Token {
	field = strings.ToUpper(field)
	field = l.intern(field)

	token := Token {
		Value: field,
		Line: line,
		Col: col,
	}

	// Check for label (avoid creating new string if not needed)
    if len(field) > 0 && field[len(field)-1] == ':' {
        token.Type = TokenLabel
        token.Value = l.intern(field[:len(field)-1]) // ✅ Slice instead of TrimSuffix
        return token
	}

	// Label definition (ends with :)
    if strings.HasSuffix(field, ":") {
        token.Type = TokenLabel
        token.Value = l.intern(strings.TrimSuffix(field, ":"))
	}

	// Check for register
	if _, ok := RegisterMap[field]; ok {
		token.Type = TokenRegister
		return token
	}

	// Check if it's extended opcode
	if _, ok := ExtOpcodeMap[field]; ok {
		token.Type = TokenExtOpcode
		return token
	}

	// Check if it's a regular opcode
	if _, ok := OpcodeMap[field]; ok {
		token.Type = TokenOpcode
		return token
	}

	// Check if it's a number
	if isNumber(field) {
		token.Type = TokenNumber
		return token
	}

	// Anything not matched will be Undefined tokens
	token.Type = TokenLabel
	return token
}

func isNumber(s string) bool {
	if len(s) == 0 {
		return false
	}

	s = strings.ToUpper(s)

	// Hex: 0x... or 0X...
	if len(s) >= 3 && s[0] == '0' && s[1] == 'X' {
		for _, c := range s[2:] {
			if !isHexDigit(c) {
				return false
			}
		}
		return true
	}

	// Signed number
	start := 0
	if s[0] == '-' || s[0] == '+' {
		start = 1
		if len(s) == 1 {
			return false
		}
	}

	for _, c := range s[start:] {
		if c < '0' || c > '9' {
			return false
		}
	}
	return true
}

func isHexDigit(c rune) bool {
	return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F')
}

