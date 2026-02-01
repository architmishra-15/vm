# Makefile — builds vm (C) and parser (Go)
# Usage: make [all|vm|parser|help|clean|distclean]
# Override defaults like: make CC=clang CFLAGS="-O3"


# Tooling / output
CC ?= cc
BIN_DIR ?= bin

# C build flags (tweak if you want portability over max perf)
CFLAGS ?= -std=c23 -O3 -march=native -flto -pipe -DNDEBUG -Wall -Wextra
LDFLAGS ?= -flto -s

# Go build settings (can be overridden on CLI)
# - CGO_ENABLED=0 avoids cgo (smaller/static).
# - -trimpath removes file system paths.
# - -ldflags "-s -w" strips symbol + debug info to shrink binary.
GO_BUILD_CMD ?= CGO_ENABLED=0 go build -trimpath -ldflags="-s -w"

# Sources
SRCS := $(wildcard *.c)
OBJS := $(SRCS:.c=.o)
VM := $(BIN_DIR)/vm
PARSER := $(BIN_DIR)/parser

.PHONY: all help vm parser clean distclean

all: vm parser

help:
	@printf "Make targets:\n"
	@printf "  all        Build both vm and parser\n"
	@printf "  vm         Compile C sources in project root -> $(VM)\n"
	@printf "  parser     Build Go parser in asm_parser/ -> $(PARSER)\n"
	@printf "  clean      Remove object files\n"
	@printf "  distclean  Remove build artifacts (bin/ + objects)\n\n"
	@printf "Customize: make CC=clang CFLAGS='-O2 -g' or override GO_BUILD_CMD\n"

# ensure bin dir exists
$(BIN_DIR):
	mkdir -p $(BIN_DIR)

# VM target: compile & link all C files in repo root
vm: $(BIN_DIR) $(VM)

$(VM): $(OBJS) | $(BIN_DIR)
	$(CC) $(LDFLAGS) -o $@ $(BIN_DIR)/$(OBJS)

# generic rule to build .o from .c
%.o: %.c
	$(CC) $(CFLAGS) -c -o $(BIN_DIR)/$@ $<

# Parser target: build Go project under asm_parser/
# puts binary into $(PARSER)
parser: $(BIN_DIR)
	@echo "Building parser (Go) in asm_parser/ ..."
	cd asm_parser && $(GO_BUILD_CMD) -o ../$(PARSER) ./...
	@echo "Built -> $(PARSER)"

clean:
	@rm -rf $(BIN_DIR)

