/*
  Instruction format (16-bit instructions)
  Format 1: OPCODE(4) | DST_REG(3) | SRC_REG(3) | UNUSED(6)
  Format 2: OPCODE(4) | REG(3) | IMMEDIATE(9)
*/

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define MEMORY_SIZE (1 << 20)   // 1 MiB
#define ADDR_MASK 0xFFFFF       // 20-bit mask

#define TYPE_STRING 0
#define TYPE_NUMBER 1

// CPU Flags
#define FLAG_ZERO     (1 << 0)
#define FLAG_SIGN     (1 << 1)
#define FLAG_CARRY    (1 << 2)
#define FLAG_OVERFLOW (1 << 3)

typedef enum {
    R0 = 0,
    R1,
    R2,
    R3,
    R4,
    R5,
    R6,
    R7,
    NUMS_R,
} Register;

typedef enum {
    OP_HALT  = 0x0,
    OP_NOP   = 0x1,
    OP_MOV   = 0x2,   // MOV dst, src
    OP_MOVI  = 0x3,   // MOV dst, immediate
    OP_CMP   = 0x4,
    OP_JMP   = 0x5,
    OP_JZ    = 0x6,   // Jump if zero
    OP_JNZ   = 0x7,
    OP_PUSH  = 0x8,
    OP_POP   = 0x9,
    OP_CALL  = 0xA,
    OP_STDOUT = 0xB,
    OP_STDIN  = 0xC,
    OP_EXT   = 0xD,
} Opcode;

typedef enum {
    EXT_RET   = 0x0,
    EXT_LOAD  = 0x1,
    EXT_STORE = 0x2,
    EXT_ADD   = 0x3,   // ADD dst, src
    EXT_SUB   = 0x4,
    EXT_AND   = 0x5,
    EXT_OR    = 0x6,
    EXT_XOR   = 0x7,
} ExtOpcode;

typedef struct CPU {
    uint16_t regs[NUMS_R];
    uint32_t pc;
    uint32_t sp;
    uint16_t SR[4];     // optional segment regs: CS, DS, SS, ES (16-bit)
    uint8_t flags;
    uint8_t *mem;       // Dynamic memory
    bool halted;
} CPU;

/*
 * =====================================
 *           HELPER FUNCTIONS
 * =====================================
 */

static inline uint8_t mem_r8(CPU *cpu, uint32_t addr)
{
    return cpu->mem[addr & ADDR_MASK];
}

static inline void mem_w8(CPU *cpu, uint32_t addr, uint8_t value)
{
    cpu->mem[addr & ADDR_MASK] = value;
}

static inline uint16_t mem_r16(CPU *cpu, uint32_t addr)
{
    uint8_t low = mem_r8(cpu, addr);
    uint8_t high = mem_r8(cpu, addr + 1);
    return (uint16_t)(low | (high << 8));
}

static inline void mem_w16(CPU *cpu, uint32_t addr, uint16_t value)
{
    mem_w8(cpu, addr, value & 0xFF);
    mem_w8(cpu, addr + 1, (value >> 8) & 0xFF);
}

static inline uint32_t mem_r20(CPU *cpu, uint32_t addr)
{
    uint8_t b0 = mem_r8(cpu, addr);
    uint8_t b1 = mem_r8(cpu, addr + 1);
    uint8_t b2 = mem_r8(cpu, addr + 2);
    return (uint32_t)(b0 | (b1 << 8) | ((b2 & 0x0F) << 16));
}
/*
 * ========================
 */

CPU *cpu_create(void)
{
    CPU *cpu = calloc(1, sizeof(CPU));
    if (!cpu) return NULL;

    cpu->mem = malloc(MEMORY_SIZE);
    if (!cpu->mem) {
        free(cpu);
        return NULL;
    }

    memset(cpu->mem, 0, MEMORY_SIZE);
    cpu->pc = 0x0000;
    cpu->sp = MEMORY_SIZE - 2;  // Top of memory, aligned for 16-bit
    cpu->flags = 0;
    cpu->halted = false;

    return cpu;
}

void cpu_destroy(CPU *cpu)
{
    if (cpu) {
        free(cpu->mem);
        free(cpu);
    }
}

void stack_push16(CPU *cpu, uint16_t value)
{
    cpu->sp -= 2;
    mem_w16(cpu, cpu->sp, value);
}

uint16_t stack_pop16(CPU *cpu)
{
    uint16_t value = mem_r16(cpu, cpu->sp);
    cpu->sp += 2;
    return value;
}

void set_flag(CPU *cpu, uint8_t flag, bool value)
{
    if (value) {
        cpu->flags |= flag;
    } else {
        cpu->flags &= ~flag;
    }
}

bool get_flag(CPU *cpu, uint8_t flag)
{
    return (cpu->flags & flag) != 0;
}

void update_flags(CPU *cpu, uint16_t result)
{
    set_flag(cpu, FLAG_ZERO, result == 0);
    set_flag(cpu, FLAG_SIGN, (result & 0x8000) != 0);
}

uint16_t fetch_instruction(CPU *cpu)
{
    uint16_t instr = mem_r16(cpu, cpu->pc);
    cpu->pc += 2;
    return instr;
}


void cpu_step(CPU *cpu)
{
    if (cpu->halted) return;

    uint16_t instr = fetch_instruction(cpu);
    uint8_t opcode = (instr >> 12) & 0xF;
    uint8_t dst = (instr >> 9) & 0x7;
    uint8_t src = (instr >> 6) & 0x7;
    uint16_t imm9 = instr & 0x1FF;

    switch (opcode)
    {
        case OP_HALT:
            cpu->halted = true;
            printf("CPU Stopped at PC: 0x%05X\n", cpu->pc - 2);
            break;

        case OP_NOP:
            break;

        case OP_MOV:
            cpu->regs[dst] = cpu->regs[src];
            update_flags(cpu, cpu->regs[dst]);
            break;

        case OP_MOVI:
            // Sign-extend 9-bit immediate
            cpu->regs[dst] = (imm9 & 0x100) ? (imm9 | 0xFE00) : imm9;
            update_flags(cpu, cpu->regs[dst]);
            break;

        case OP_CMP: {
            // Compare: SUB but don't store result
            uint32_t result = (uint32_t)cpu->regs[dst] - (uint32_t)cpu->regs[src];
            set_flag(cpu, FLAG_CARRY, cpu->regs[dst] < cpu->regs[src]);
            update_flags(cpu, result & 0xFFFF);
            break;
        }

        case OP_JMP:
            // Jump to address in register
            cpu->pc = cpu->regs[dst] & ADDR_MASK;
            break;

        case OP_JZ:
            // Jump if zero flag is set
            if (get_flag(cpu, FLAG_ZERO)) {
                cpu->pc = cpu->regs[dst] & ADDR_MASK;
            }
            break;

        case OP_JNZ:
            // Jump if NOT zero
            if (!get_flag(cpu, FLAG_ZERO)) {
                cpu->pc = cpu->regs[dst] & ADDR_MASK;
            }
            break;

        case OP_PUSH:
            stack_push16(cpu, cpu->regs[dst]);
            break;

        case OP_POP:
            cpu->regs[dst] = stack_pop16(cpu);
            update_flags(cpu, cpu->regs[dst]);
            break;

        case OP_CALL:
            // Push return address (next instruction)
            stack_push16(cpu, cpu->pc & 0xFFFF);               // Low 16 bits
            stack_push16(cpu, (cpu->pc >> 16) & 0xF);          // High 16 bits
            cpu->pc = cpu->regs[dst] & ADDR_MASK;
            break;

        case OP_STDOUT:
            {
                if (dst == 0) {
                    // String follows immediately after this instruction
                    char* str = (char*)(cpu->mem + cpu->pc);
                    printf("%s", str);
                    cpu->pc += strlen(str) + 1;  // Skip string + null terminator
                    // Align to 2-byte boundary
                    if (cpu->pc & 1) cpu->pc++;
                }
                else if (dst == 1) {
                    // Print register value as number
                    printf("%d", (int16_t)cpu->regs[src]);
                }
                break;
            }

        case OP_STDIN:
            printf("STDIN not implemented yet!\n");
            break;

        case OP_EXT:
            {
                uint8_t ext_op = dst;
                switch (ext_op)
                {
                    case EXT_ADD: {
                        uint32_t result = (uint32_t)cpu->regs[dst] + (uint32_t)cpu->regs[src];

                        // Set carry flag if result > 16 bits
                        set_flag(cpu, FLAG_CARRY, result > 0xFFFF);

                        // Check for signed overflow
                        bool dst_sign = (cpu->regs[dst] & 0x8000) != 0;
                        bool src_sign = (cpu->regs[src] & 0x8000) != 0;
                        bool res_sign = (result & 0x8000) != 0;
                        bool overflow = (dst_sign == src_sign) && (dst_sign != res_sign);
                        set_flag(cpu, FLAG_OVERFLOW, overflow);

                        cpu->regs[dst] = result & 0xFFFF;
                        update_flags(cpu, cpu->regs[dst]);
                        break;
                    }

                    case EXT_SUB: {
                        uint32_t result = (uint32_t)cpu->regs[dst] - (uint32_t)cpu->regs[src];

                        // Carry flag set if borrow occurred
                        set_flag(cpu, FLAG_CARRY, cpu->regs[dst] < cpu->regs[src]);

                        // Overflow check for subtraction
                        bool dst_sign = (cpu->regs[dst] & 0x8000) != 0;
                        bool src_sign = (cpu->regs[src] & 0x8000) != 0;
                        bool res_sign = (result & 0x8000) != 0;
                        bool overflow = (dst_sign != src_sign) && (dst_sign != res_sign);
                        set_flag(cpu, FLAG_OVERFLOW, overflow);

                        cpu->regs[dst] = result & 0xFFFF;
                        update_flags(cpu, cpu->regs[dst]);
                        break;
                    }

                    case EXT_AND:
                        cpu->regs[dst] &= cpu->regs[src];
                        update_flags(cpu, cpu->regs[dst]);
                        break;

                    case EXT_OR:
                        cpu->regs[dst] |= cpu->regs[src];
                        update_flags(cpu, cpu->regs[dst]);
                        break;

                    case EXT_XOR:
                        cpu->regs[dst] ^= cpu->regs[src];
                        update_flags(cpu, cpu->regs[dst]);
                        break;


                    case EXT_RET:
                        {
                            uint32_t high = stack_pop16(cpu) & 0xF;
                            uint32_t low  = stack_pop16(cpu);
                            cpu->pc = (high << 16) | low;
                            break;
                        }

                    case EXT_LOAD:
                        cpu->regs[src] = mem_r16(cpu, cpu->regs[src]);
                        update_flags(cpu, cpu->regs[src]);
                        break;

                    case EXT_STORE:
                        // Use unused bits for addressing
                        mem_w16(cpu, cpu->regs[src], cpu->regs[(instr >> 3) & 0x7]);
                        break;
                }
            }
        default:
            printf("Unknown opcode: 0x%X at PC=0x%05X\n", opcode, cpu->pc - 2);
            cpu->halted = true;
            break;
    }
}

void cpu_run(CPU *cpu)
{
    while (!cpu->halted) cpu_step(cpu);
}

static inline uint16_t make_instr(uint8_t op, uint8_t dst, uint8_t src)
{
    return (op << 12) | (dst << 9) | (src << 6);
}

static inline uint16_t make_instr_imm(uint8_t op, uint8_t dst, uint16_t imm)
{
    return (op << 12) | (dst << 9) | (imm & 0x1FF);
}

static inline uint16_t make_ext_instr(uint8_t ext_op, uint8_t reg1, uint8_t reg2) {
    return (OP_EXT << 12) | (ext_op << 9) | (reg1 << 6) | (reg2 << 3);
}

void cpu_dump(CPU *cpu) 
{
    printf("\n=== CPU State ===\n");
    printf("PC: 0x%05X  SP: 0x%05X\n", cpu->pc, cpu->sp);
    printf("Flags: Z=%d S=%d C=%d O=%d\n",
           get_flag(cpu, FLAG_ZERO),
           get_flag(cpu, FLAG_SIGN),
           get_flag(cpu, FLAG_CARRY),
           get_flag(cpu, FLAG_OVERFLOW));

    printf("Registers:\n");
    for (int i = 0; i < NUMS_R; i++) {
        printf("  R%d: 0x%04X (%d)\n", i, cpu->regs[i], (int16_t)cpu->regs[i]);
    }
    printf("=================\n\n");
}


int main(void)
{
    printf("16-bit VM Demo\n");
    printf("==============\n\n");

    CPU *cpu = cpu_create();
    if (!cpu) {
        fprintf(stderr, "Failed to create CPU\n");
        return 1;
    }

    // Load a simple program into memory
    uint32_t addr = 0;

    // Test 1: Add 5 + 10
    mem_w16(cpu, addr, make_instr_imm(OP_MOVI, R0, 5));   // R0 = 5
    addr += 2;

    mem_w16(cpu, addr, make_instr_imm(OP_MOVI, R1, 10));  // R1 = 10
    addr += 2;

    mem_w16(cpu, addr, make_instr(EXT_ADD, R0, R1));       // R0 = R0 + R1
    addr += 2;

    printf("Test 1: After R0 = 5 + 10:\n");
    cpu_step(cpu);  // MOVI R0, 5
    cpu_step(cpu);  // MOVI R1, 10
    cpu_step(cpu);  // ADD R0, R1
    cpu_dump(cpu);

    // Test 2: Compare R0 with 15
    mem_w16(cpu, addr, make_instr_imm(OP_MOVI, R2, 15));  // R2 = 15
    addr += 2;

    mem_w16(cpu, addr, make_instr(OP_CMP, R0, R2));       // Compare R0, R2
    addr += 2;

    printf("Test 2: After comparing R0 (15) with R2 (15):\n");
    cpu_step(cpu);  // MOVI R2, 15
    cpu_step(cpu);  // CMP R0, R2
    cpu_dump(cpu);

    // Test 3: Overflow (32767 + 1)
    // Build 32767 using multiple instructions
    mem_w16(cpu, addr, make_instr_imm(OP_MOVI, R3, 0x1FF)); // R3 = 511
    addr += 2;

    mem_w16(cpu, addr, make_instr_imm(OP_MOVI, R5, 0x1FF)); // R5 = 511
    addr += 2;

    mem_w16(cpu, addr, make_instr(EXT_ADD, R3, R5));         // R3 = 511 + 511 = 1022
    addr += 2;

    mem_w16(cpu, addr, make_instr(EXT_ADD, R3, R5));         // R3 = 1022 + 511 = 1533
    addr += 2;

    mem_w16(cpu, addr, make_instr(EXT_ADD, R3, R5));         // R3 = 1533 + 511 = 2044
    addr += 2;

    mem_w16(cpu, addr, make_instr(EXT_ADD, R3, R5));         // R3 = 2044 + 511 = 2555
    addr += 2;

    mem_w16(cpu, addr, make_instr(EXT_ADD, R3, R5));         // R3 = 2555 + 511 = 3066
    addr += 2;

    mem_w16(cpu, addr, make_instr(EXT_ADD, R3, R5));         // R3 = 3066 + 511 = 3577
    addr += 2;

    mem_w16(cpu, addr, make_instr(EXT_ADD, R3, R5));         // R3 = 3577 + 511 = 4088
    addr += 2;

    // R3 = 0x7F, then shift left 8 bits, then add 0xFF
    mem_w16(cpu, addr, make_instr_imm(OP_MOVI, R3, 0x7F));  // R3 = 127
    addr += 2;

    uint32_t data_addr = 0x1000;  // Store data at 0x1000
    mem_w16(cpu, data_addr, 0x7FFF);  // Store 32767 as data

    // Clear previous instructions and start fresh for overflow test
    addr = 0x10;  // Start at a new address
    cpu->pc = 0x10;  // Jump PC here
    mem_w16(cpu, addr, make_instr_imm(OP_MOVI, R4, 1));     // R4 = 1
    addr += 2;

    // Manually set R3 for demo (acknowledge this is cheating)
    cpu->regs[R3] = 0x7FFF;  // Set R3 = 32767 directly

    mem_w16(cpu, addr, make_instr(EXT_ADD, R3, R4));         // R3 = 32767 + 1
    addr += 2;

    mem_w16(cpu, addr, make_instr(OP_STDOUT, R5, 0xF));
    addr += 2;

    printf("Test 3: Overflow (32767 + 1) - R3 manually set to 0x7FFF:\n");
    cpu_step(cpu);  // MOVI R4, 1
    cpu_step(cpu);  // ADD R3, R4
    cpu_dump(cpu);

    cpu_dump(cpu);

    // cpu_step(cpu);  // HALT

    mem_w16(cpu, addr, make_instr(OP_STDOUT, 0, 0));  // dst=0 means string follows
addr += 2;

    // Write string data
    const char* msg = "Hello!";
    strcpy((char*)(cpu->mem + addr), msg);
    addr += strlen(msg) + 1;
    if (addr & 1) addr++;  // Align to even address

    // Print number from register
    mem_w16(cpu, addr, make_instr_imm(OP_MOVI, R2, 99));
    addr += 2;
    mem_w16(cpu, addr, make_instr(OP_STDOUT, 1, R2));  // dst=1, src=register
    addr += 2;

    printf("After String printing -\n");
    cpu_step(cpu);
    cpu_step(cpu);
    cpu_step(cpu);
    cpu_step(cpu);

    mem_w16(cpu, addr, make_instr(OP_HALT, 0, 0));          // HALT
    addr += 2;

    cpu_step(cpu);

    cpu_dump(cpu);

    cpu_destroy(cpu);
    return 0;
}
