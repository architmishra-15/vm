package main

import "fmt"

// SymbolTable manages labels and their addresses
type SymbolTable struct {
	symbols map[string]uint32
}

// NewSymbolTable creates a new symbol table
func NewSymbolTable() *SymbolTable {
	return &SymbolTable{
		symbols: make(map[string]uint32),
	}
}

// Define adds a label to the symbol table
func (st *SymbolTable) Define(label string, address uint32) error {
	if _, exists := st.symbols[label]; exists {
		return fmt.Errorf("label '%s' already defined", label)
	}
	st.symbols[label] = address
	return nil
}

// Resolve looks up a label's address
func (st *SymbolTable) Resolve(label string) (uint32, bool) {
	addr, ok := st.symbols[label]
	return addr, ok
}

// All returns all symbols
func (st *SymbolTable) All() map[string]uint32 {
	return st.symbols
}

// Count returns number of symbols
func (st *SymbolTable) Count() int {
	return len(st.symbols)
}
