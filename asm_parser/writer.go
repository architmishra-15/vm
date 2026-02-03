package main

import "os"

// Writer handles file output
type Writer struct{}

func NewWriter() *Writer {
	return &Writer{}
}

// WriteBinary writes bytecode to file
func (w *Writer) WriteBinary(filename string, data []byte) error {
	return os.WriteFile(filename, data, 0644)
}
