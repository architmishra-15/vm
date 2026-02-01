package main

import (
	"fmt"
)

type Assembler struct {
	labels 	map[string]uint32		// Label -> address
	code 	[]uint16				// Generated instructions
	addr 	uint32					// Current address
}

func main() {
	fmt.Println("Not implemented yet!")
}
