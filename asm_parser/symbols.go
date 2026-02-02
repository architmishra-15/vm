package main

import "fmt"

type Symbol struct {
	Name		string
	Address		uint32
	Line		int
	Used		bool		// for dead code elimination and compiler warnings.
}

// SymbolTable manages labels and their addresses
type SymbolTable struct {
	symbols map[string]*Symbol
}

// NewSymbolTable creates a new symbol table and returns the pointer to it
func NewSymbolTable() *SymbolTable {
	return &SymbolTable{
		symbols: make(map[string]*Symbol),
	}
}

// Define adds a label to the symbol table
func (st *SymbolTable) Define(label string, address uint32, line int) error {
	if _, exists := st.symbols[label]; exists {
		return fmt.Errorf("label '%s' already defined", label)
	}
	st.symbols[label] = &Symbol{
		Name:    label,
		Address: address,
		Line:    line,
		Used:    false,
	}
	return nil
}

// Resolve looks up a label's address and marks it as used
func (st *SymbolTable) Resolve(label string) (uint32, bool) {
	addr, ok := st.symbols[label]
	if ok {
		addr.Used = true
		return addr.Address, true
	}
	return 0, false
}

// Get retrieves a symbol without marking it as used
func (st *SymbolTable) Get(label string) (*Symbol, bool) {
	sym, ok := st.symbols[label]
	return sym, ok
}

// All returns all symbols as map
func (st *SymbolTable) All() map[string]uint32 {
	result := make(map[string]uint32)
	for name, sym := range st.symbols {
		result[name] = sym.Address
	}

	return result
}

// AllSymbols returns all symbol objects
func (st *SymbolTable) AllSymbols() []*Symbol {
	result := make([]*Symbol, 0, len(st.symbols))
	for _, sym := range st.symbols {
		result = append(result, sym)
	}
	return result
}

// Count returns number of symbols
func (st *SymbolTable) Count() int {
	return len(st.symbols)
}

// UnusedLabels returns labels that were defined but never referenced
func (st *SymbolTable) UnusedLabels() []string {
	var unused []string
	for _, sym := range st.symbols {
		if !sym.Used {
			unused = append(unused, sym.Name)
		}
	}
	return unused
}

// Exists checks if a label is defined
func (st *SymbolTable) Exists(label string) bool {
	_, ok := st.symbols[label]
	return ok
}

