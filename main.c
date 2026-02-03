/*
  Instruction format (16-bit instructions)
  Format 1: OPCODE(4) | DST_REG(3) | SRC_REG(3) | UNUSED(6)
  Format 2: OPCODE(4) | REG(3) | IMMEDIATE(9)
*/

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MEMORY_SIZE (1 << 20) // 1 MiB
#define ADDR_MASK 0xFFFFF     // 20-bit mask

#define TYPE_STRING 0
#define TYPE_NUMBER 1

// CPU Flags
#define FLAG_ZERO (1 << 0)
#define FLAG_SIGN (1 << 1)
#define FLAG_CARRY (1 << 2)
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
    OP_HALT = 0x0,
    OP_NOP = 0x1,
    OP_MOV = 0x2,  // MOV dst, src
    OP_MOVI = 0x3, // MOV dst, immediate
    OP_CMP = 0x4,
    OP_JMP = 0x5,
    OP_JZ = 0x6, // Jump if zero
    OP_JNZ = 0x7,
    OP_PUSH = 0x8,
    OP_POP = 0x9,
    OP_CALL = 0xA,
    OP_STDOUT = 0xB,
    OP_STDIN = 0xC,
    OP_EXT = 0xD,
} Opcode;

typedef enum {
    EXT_RET = 0x0,
    EXT_LOAD = 0x1,
    EXT_STORE = 0x2,
    EXT_ADD = 0x3, // ADD dst, src
    EXT_SUB = 0x4,
    EXT_AND = 0x5,
    EXT_OR = 0x6,
    EXT_XOR = 0x7,
} ExtOpcode;

typedef struct CPU {
    uint16_t regs[NUMS_R];
    uint32_t pc;
    uint32_t sp;
    uint16_t SR[4]; // optional segment regs: CS, DS, SS, ES (16-bit)
    uint8_t flags;
    uint8_t *mem; // Dynamic memory
    bool halted;
} CPU;

typedef struct {
    CPU *cpu;
    uint32_t addr;
} ProgramBuilder;

/*
 * =====================================
 *           HELPER FUNCTIONS
 * =====================================
 */

ProgramBuilder *pb_init(CPU *cpu)
{
    ProgramBuilder *pb = malloc(sizeof(ProgramBuilder));
    pb->cpu = cpu;
    pb->addr = 0;

    return pb;
}

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

static inline uint16_t make_instr(uint8_t op, uint8_t dst, uint8_t src)
{
    return (op << 12) | (dst << 9) | (src << 6);
}

static inline uint16_t make_instr_imm(uint8_t op, uint8_t dst, uint16_t imm)
{
    return (op << 12) | (dst << 9) | (imm & 0x1FF);
}

static inline uint16_t make_ext_instr(uint8_t ext_op, uint8_t reg1, uint8_t reg2)
{
    return (OP_EXT << 12) | (ext_op << 9) | (reg1 << 6) | (reg2 << 3);
}

static inline void pb_movi(ProgramBuilder *pb, Register reg, uint16_t imm)
{
    mem_w16(pb->cpu, pb->addr, make_instr_imm(OP_MOVI, reg, imm));
    pb->addr += 2;
}

static inline void pb_stdout_str(ProgramBuilder *pb, const char *str)
{
    mem_w16(pb->cpu, pb->addr, make_instr(OP_STDOUT, 0, 0));
    pb->addr += 2;
    strcpy((char *)(pb->cpu->mem + pb->addr), str);
    pb->addr += strlen(str) + 1;
    if (pb->addr & 1)
        pb->addr++;
}

static inline void pb_stdout_reg(ProgramBuilder *pb, Register reg)
{
    mem_w16(pb->cpu, pb->addr, make_instr(OP_STDOUT, 1, reg));
    pb->addr += 2;
}

static inline void pb_stdin_num(ProgramBuilder *pb, Register reg)
{
    mem_w16(pb->cpu, pb->addr, make_instr(OP_STDIN, 1, reg));
    pb->addr += 2;
}

static inline void pb_stdin_str(ProgramBuilder *pb, Register reg, uint32_t buffer_addr)
{
    pb->cpu->regs[reg] = buffer_addr;
    mem_w16(pb->cpu, pb->addr, make_instr(OP_STDIN, 0, reg));
    pb->addr += 2;
}

static inline void pb_halt(ProgramBuilder *pb)
{
    mem_w16(pb->cpu, pb->addr, make_instr(OP_HALT, 0, 0));
    pb->addr += 2;
}

static inline void pb_add(ProgramBuilder *pb, Register dst, Register src)
{
    mem_w16(pb->cpu, pb->addr, make_ext_instr(EXT_ADD, dst, src));
    pb->addr += 2;
}

/*
    load a full 16-bit value into a register
    Strategy: Load High Byte -> Shift Left 8 times -> OR Low Byte
*/
// TODO: Add a proper loading command
static void pb_load16(ProgramBuilder *pb, Register reg, uint16_t value)
{

    pb->cpu->regs[reg] = value;
    // uint8_t high = (value >> 8) & 0xFF;
    // uint8_t low = value & 0xFF;
    //
    // // 1. Load High Byte (0-255 fits in MOVI)
    // pb_movi(pb, reg, high);
    //
    // // 2. Shift Left 8 times (using ADD reg, reg)
    // // We need to do this only if there is a high byte,
    // // but for simplicity, we can do it always or optimize.
    // if (high > 0) {
    //     for (int i = 0; i < 8; i++) {
    //         // EXT_ADD dst, src (here dst=reg, src=reg)
    //         mem_w16(pb->cpu, pb->addr, make_ext_instr(EXT_ADD, reg, reg));
    //         pb->addr += 2;
    //     }
    // }
    //
    // // 3. Load Low Byte into a temporary register (using R7 as temp scratch)
    // // Then OR it into the destination.
    // // If high byte was 0, we can just MOVI the low byte directly.
    // if (high > 0) {
    //     Register temp = R7;
    //     pb_movi(pb, temp, low);
    //     // OR reg, temp
    //     mem_w16(pb->cpu, pb->addr, make_ext_instr(EXT_OR, reg, temp));
    //     pb->addr += 2;
    // } else {
    //     pb_movi(pb, reg, low);
    // }
}
/*
 * ========================
 */

CPU *cpu_create(void)
{
    CPU *cpu = calloc(1, sizeof(CPU));
    if (!cpu)
        return NULL;

    cpu->mem = malloc(MEMORY_SIZE);
    if (!cpu->mem) {
        free(cpu);
        return NULL;
    }

    memset(cpu->mem, 0, MEMORY_SIZE);
    cpu->pc = 0x0000;
    cpu->sp = MEMORY_SIZE - 2; // Top of memory, aligned for 16-bit
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
    if (cpu->halted)
        return;

    uint16_t instr = fetch_instruction(cpu);
    uint8_t opcode = (instr >> 12) & 0xF;
    uint8_t dst = (instr >> 9) & 0x7;
    uint8_t src = (instr >> 6) & 0x7;
    uint16_t imm9 = instr & 0x1FF;

    switch (opcode) {
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
            stack_push16(cpu, cpu->pc & 0xFFFF);      // Low 16 bits
            stack_push16(cpu, (cpu->pc >> 16) & 0xF); // High 16 bits
            cpu->pc = cpu->regs[dst] & ADDR_MASK;
            break;

        case OP_STDOUT: {
            if (dst == 0) {
                // String follows immediately after this instruction
                char *str = (char *)(cpu->mem + cpu->pc);
                printf("%s", str);
                cpu->pc += strlen(str) + 1; // Skip string + null terminator
                // Align to 2-byte boundary
                if (cpu->pc & 1)
                    cpu->pc++;
            } else if (dst == 1) {
                // Print register value as number
                printf("%d", (int16_t)cpu->regs[src]);
            } else if (dst == 2) {
                // Print string from memory address stored in SRC register
                uint32_t addr = cpu->regs[src] & ADDR_MASK;
                char *str = (char *)(cpu->mem + addr);
                printf("%s", str);
            } else if (dst == 3) {
                printf("%c", (char)(cpu->regs[src] & 0xFF));
            }

            break;
        }

        case OP_STDIN: {
            // dst field: 0 = read string into memory address in src register
            //            1 = read number into src register
            // src field: register to store in

            if (dst == 0) {
                uint32_t addr = cpu->regs[src] & ADDR_MASK;
                char buf[256];

                if (fgets(buf, sizeof(buf), stdin)) {
                    // trim the newline
                    size_t len = strlen(buf);
                    if (len > 0 && buf[len - 1] == '\n') {
                        buf[len - 1] = '\0';
                    }

                    // Copying to memory
                    strcpy((char *)(cpu->mem + addr), buf);
                }
            } else {
                int value;
                if (scanf("%d", &value) == 1) {
                    cpu->regs[src] = (uint16_t)value;
                    update_flags(cpu, cpu->regs[src]);
                }

                int c;
                while ((c = getchar()) != '\n' && c != EOF)
                    ;
            }
            break;
        }

        case OP_EXT: {
            uint8_t ext_op = dst;              // Bits 9-11 = extended opcode
            uint8_t reg1 = src;                // Bits 6-8 = first register
            uint8_t reg2 = (instr >> 3) & 0x7; // Bits 3-5 = second register

            switch (ext_op) {
                case EXT_ADD: {
                    uint32_t result = (uint32_t)cpu->regs[reg1] + (uint32_t)cpu->regs[reg2];
                    set_flag(cpu, FLAG_CARRY, result > 0xFFFF);

                    bool r1_sign = (cpu->regs[reg1] & 0x8000) != 0;
                    bool r2_sign = (cpu->regs[reg2] & 0x8000) != 0;
                    bool res_sign = (result & 0x8000) != 0;
                    bool overflow = (r1_sign == r2_sign) && (r1_sign != res_sign);
                    set_flag(cpu, FLAG_OVERFLOW, overflow);

                    cpu->regs[reg1] = result & 0xFFFF;
                    update_flags(cpu, cpu->regs[reg1]);
                    break;
                }

                case EXT_SUB: {
                    uint32_t result = (uint32_t)cpu->regs[reg1] - (uint32_t)cpu->regs[reg2];
                    set_flag(cpu, FLAG_CARRY, cpu->regs[reg1] < cpu->regs[reg2]);

                    bool r1_sign = (cpu->regs[reg1] & 0x8000) != 0;
                    bool r2_sign = (cpu->regs[reg2] & 0x8000) != 0;
                    bool res_sign = (result & 0x8000) != 0;
                    bool overflow = (r1_sign != r2_sign) && (r1_sign != res_sign);
                    set_flag(cpu, FLAG_OVERFLOW, overflow);

                    cpu->regs[reg1] = result & 0xFFFF;
                    update_flags(cpu, cpu->regs[reg1]);
                    break;
                }

                case EXT_AND:
                    cpu->regs[reg1] &= cpu->regs[reg2];
                    update_flags(cpu, cpu->regs[reg1]);
                    break;

                case EXT_OR:
                    cpu->regs[reg1] |= cpu->regs[reg2];
                    update_flags(cpu, cpu->regs[reg1]);
                    break;

                case EXT_XOR:
                    cpu->regs[reg1] ^= cpu->regs[reg2];
                    update_flags(cpu, cpu->regs[reg1]);
                    break;

                case EXT_RET: {
                    uint32_t high = stack_pop16(cpu) & 0xF;
                    uint32_t low = stack_pop16(cpu);
                    cpu->pc = (high << 16) | low;
                    break;
                }

                case EXT_LOAD:
                    cpu->regs[reg1] = mem_r16(cpu, cpu->regs[reg2]);
                    update_flags(cpu, cpu->regs[reg1]);
                    break;

                case EXT_STORE:
                    mem_w16(cpu, cpu->regs[reg1], cpu->regs[reg2]);
                    break;

                default:
                    printf("Unknown extended opcode: 0x%X\n", ext_op);
                    cpu->halted = true;
                    break;
            }
            break;
        }

        default:
            printf("Unknown opcode: 0x%X at PC=0x%05X\n", opcode, cpu->pc - 2);
            cpu->halted = true;
            break;
    }
}

void cpu_run(CPU *cpu)
{
    while (!cpu->halted)
        cpu_step(cpu);
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

/*
 * =====================================
 *         SOME EXAMPLE PROGRAMS
 * =====================================
 */

void program_fibonacci(ProgramBuilder *pb)
{
    printf("--- Generating Fibonacci Sequence ---\n");

    pb_movi(pb, R0, 0);
    pb_movi(pb, R1, 1);
    pb_movi(pb, R2, 23); // Safe limit for signed 16-bit (max 28657)

    // === LOOP START ===
    uint32_t loop_addr = pb->addr;

    // Print current number
    pb_stdout_reg(pb, R0);
    pb_stdout_str(pb, " ");

    // Calculate Next: R3 = R0 + R1
    mem_w16(pb->cpu, pb->addr, make_instr(OP_MOV, R3, R0));
    pb->addr += 2;
    pb_add(pb, R3, R1);

    // Shift registers
    mem_w16(pb->cpu, pb->addr, make_instr(OP_MOV, R0, R1));
    pb->addr += 2;
    mem_w16(pb->cpu, pb->addr, make_instr(OP_MOV, R1, R3));
    pb->addr += 2;

    // === CRITICAL FIX STARTS HERE ===

    // 1. PRE-LOAD the Jump Address into R4
    //    (Do this before touching flags!)
    pb_load16(pb, R4, (uint16_t)loop_addr);

    // 2. Decrement Counter (R2 = R2 - 1)
    //    This sets the Zero Flag correctly based on R2.
    pb_movi(pb, R5, 1);
    mem_w16(pb->cpu, pb->addr, make_ext_instr(EXT_SUB, R2, R5));
    pb->addr += 2;

    // 3. Jump (Checks the flag set by step 2 immediately)
    mem_w16(pb->cpu, pb->addr, make_instr(OP_JNZ, R4, 0));
    pb->addr += 2;

    pb_stdout_str(pb, "\nDone!\n");
    pb_halt(pb);
}

void program_multiplication(ProgramBuilder *pb)
{
    printf("--- Software Multiplication (30 * 5) ---\n");

    pb_movi(pb, R0, 30);
    pb_movi(pb, R1, 5);
    pb_movi(pb, R2, 0); // Accumulator

    // === SIMPLE LOOP - No function call ===
    uint32_t loop_addr = pb->addr;

    // Add R0 to accumulator
    pb_add(pb, R2, R0);

    // Decrement counter
    pb_movi(pb, R3, 1);
    mem_w16(pb->cpu, pb->addr, make_ext_instr(EXT_SUB, R1, R3));
    pb->addr += 2;

    // Jump back if not zero
    pb->cpu->regs[R4] = loop_addr; // Direct set
    mem_w16(pb->cpu, pb->addr, make_instr(OP_JNZ, R4, 0));
    pb->addr += 2;

    // Print result
    pb_stdout_str(pb, "Result: ");
    pb_stdout_reg(pb, R2); // R2 has the result!
    pb_stdout_str(pb, "\n");
    pb_halt(pb);
}

// =====================================

int main(void)
{
    CPU *cpu = cpu_create();
    if (!cpu) {
        fprintf(stderr, "Failed to create CPU\n");
        return 1;
    }
    ProgramBuilder *pb = pb_init(cpu);

    // program_fibonacci(pb);
    program_multiplication(pb);

    cpu_run(cpu);
    free(pb);
    cpu_destroy(cpu);
    return 0;
}
