package main

import "fmt"

type AssemblerConfig struct {
	verbose				bool
	optimizationLevel 	int

	// TODO: Remove the comment wehen different versions/arch of the VM is made
	// targetArch 			string
}

type Assembler struct {
	symbolTable *SymbolTable
	config 		AssemblerConfig
}

type AssemblerOption func(*AssemblerConfig)

func WithVerbose(v bool) AssemblerOption {
    return func(c *AssemblerConfig) {
        c.verbose = v
    }
}

func WithOptimization(level int) AssemblerOption {
    return func(c *AssemblerConfig) {
        c.optimizationLevel = level
    }
}

func NewAssembler(opts ...AssemblerOption) *Assembler {
	config := AssemblerConfig{
        verbose:         false,
        optimizationLevel: 0,
    }
    
    for _, opt := range opts {
        opt(&config)
    }
    
    return &Assembler{
        symbolTable: NewSymbolTable(),
        config:      config,
    }
}

// Assemble performs two-pass assembly
func (a *Assembler) Assemble(source string) ([]Instruction, error) {

    lexer := NewLexer(source)
    lines, err := lexer.Tokenize()
    if err != nil {
        return nil, fmt.Errorf("lexer error: %w", err)
    }

    // Pass 1: Build symbol table
    if err := a.buildSymbolTable(lines); err != nil {
        return nil, fmt.Errorf("pass 1 error: %w", err)
    }

    // Pass 2: Parse and resolve symbols
    parser := NewParser(a.symbolTable)
    instructions, err := parser.Parse(lines)
    if err != nil {
        return nil, fmt.Errorf("pass 2 error: %w", err)
    }

    // Warn about unused labels
    if unused := a.symbolTable.UnusedLabels(); len(unused) > 0 {
        fmt.Printf("Warning: unused labels: %v\n", unused)
    }

    return instructions, nil
}

// buildSymbolTable is Pass 1: collect all label definitions
func (a *Assembler) buildSymbolTable(lines []Line) error {
    address := uint32(0)

    for _, line := range lines {
        if len(line.Tokens) == 0 {
            continue
        }

        // Check if this is a label definition
        if len(line.Tokens) == 1 && line.Tokens[0].Type == TokenLabel {
            label := line.Tokens[0].Value
            if err := a.symbolTable.Define(label, address, line.Number); err != nil {
                return fmt.Errorf("line %d: %w", line.Number, err)
            }
            // Labels don't consume space
            continue
        }

        // Regular instruction - takes 2 bytes (16 bits)
        address += 2
    }

    return nil
}
