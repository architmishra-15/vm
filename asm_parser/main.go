package main

import (
	"fmt"
	"os"
)

func main() {
	if len(os.Args) != 3 {
		fmt.Fprintf(os.Stderr, "Usage: %s <input.vm> <output.bin>\n", os.Args[0])
		os.Exit(1)
	}

	inputFile 	:= os.Args[1]
	outputFile 	:= os.Args[2]

	source, err := os.ReadFile(inputFile)
	if err != nil {
		fmt.Fprintf(os.Stderr, "Error reading file: %v\n", err)
		os.Exit(1)
	}

	byteCode := []byte("cbin")
	temp, err := MainAssembly(string(source))
	
	for _, b := range temp {
		byteCode = append(byteCode, b)
	}

	if err != nil {
		fmt.Fprintf(os.Stderr, "Error: %v\n", err)
		os.Exit(1)
	}

	writer := NewWriter()
	if err := writer.WriteBinary(outputFile, byteCode); err != nil {
		fmt.Fprintf(os.Stderr, "Error writing: %v\n", err)
		os.Exit(1)
	}

	fmt.Printf("Success: %d bytes written to %s\n", len(byteCode), outputFile)
}

func MainAssembly(source string) ([]byte, error) {
	// Tokenizing
	lexer := NewLexer(source)
	lines, err := lexer.Tokenize()
	if err != nil {
		return nil, err
	}

	// Parsing
	parser := NewParser()
	instructions, err := parser.Parse(lines)
	if err != nil {
		return nil, err
	}

	// Encode
	encoder := NewEncoder()
	bytecode := encoder.EncodeAll(instructions)

	return bytecode, nil
}
