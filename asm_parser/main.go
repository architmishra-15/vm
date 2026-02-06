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

	assembler := NewAssembler()
	instructions, err := assembler.Assemble(string(source))
	if err != nil {
		fmt.Fprintf(os.Stderr, "Error: %v\n", err)
		os.Exit(1)
	}

	fmt.Println("=== Instructions ===")
	for i, instr := range instructions {
		fmt.Printf("[%04X] %v\n", i*2, instr)
	}

	fmt.Println("\n=== Symbol Table ===")
	for label, addr := range assembler.symbolTable.All() {
		fmt.Printf("%s → 0x%04X\n", label, addr)
	}

	encoder := NewEncoder()
	bytecode := encoder.EncodeAll(instructions)

	writer := NewWriter()
	if err := writer.WriteBinary(outputFile, bytecode); err != nil {
		fmt.Fprintf(os.Stderr, "Write error: %v\n", err)
		os.Exit(1)
	}

	fmt.Printf("\n✓ Successfully assembled %d instructions (%d bytes) to %s\n",
		len(instructions), len(bytecode), outputFile)
}
