package main

import "fmt"

type AssemblyStage string

const (
	StageLexer    AssemblyStage = "lexer"
    StageParser   AssemblyStage = "parser" 
    StageSymbols  AssemblyStage = "symbols"
    StageEncoder  AssemblyStage = "encoder"
)

type AssemblerError struct {
	Stage 	AssemblyStage
	Line	int
	Column  int
	Token 	string
	Message	string
	Cause 	error
}

func (e *AssemblerError) Error() string {
	if e.Column > 0 {
        return fmt.Sprintf("[%s:%d:%d] %s: %s", e.Stage, e.Line, e.Column, e.Token, e.Message)
    }
    return fmt.Sprintf("[%s:%d] %s", e.Stage, e.Line, e.Message)
}

func (e *AssemblerError) Unwrap() error {
    return e.Cause
}

func NewLexerError(line, col int, token, msg string) *AssemblerError {
    return &AssemblerError{
        Stage:   StageLexer,
        Line:    line,
        Column:  col,
        Token:   token,
        Message: msg,
    }
}

func NewParserError(line int, msg string) *AssemblerError {
    return &AssemblerError{
        Stage:   StageParser,
        Line:    line,
        Message: msg,
    }
}
