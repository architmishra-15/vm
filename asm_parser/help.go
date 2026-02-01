package main

import "fmt"

func help() {
	fmt.Printf("asld: Assembles the assembly into bytecode/binary understandable by the VM.\n")
}

func usageHelp() {
	fmt.Printf("Usage: asld <assembly filename>\n")
}
