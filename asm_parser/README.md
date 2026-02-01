# Parser
The main parser which would parse the custom assembly and would output a binary/bytecode which would be read and executed by the VM.

## File Structure -

```
assembler/
├── main.go           # Entry point, CLI handling
├── lexer.go          # Tokenization
├── parser.go         # Instruction parsing
├── encoder.go        # Binary encoding
├── symbols.go        # Label/symbol table
├── opcodes.go        # Opcode definitions
└── writer.go         # Binary & listing output
```

> TODO: Write a parser and lexer
