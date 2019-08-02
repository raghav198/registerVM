#include "vm.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Perform 16-bit binary addition and set Overflow, Carry, Zero, and Negative flags */
uint16_t do_adder(uint32_t a, uint32_t b, uint8_t * flags)
{
    uint8_t f = 0;
    uint32_t sum = a + b;
    if (sum > 0xFFFF)
        f |= SF_C;
    if (0x8000 & 
            ((sum & ~a & ~b) |
            (~sum & (a | b))))
        f |= SF_V;
    if (sum & 0x8000)
        f |= SF_N;
    if ((sum & 0xFFFF) == 0)
        f |= SF_Z;

    *flags = f;
    return (sum & 0xFFFF);
}

/* Read a 16-bit value from a buffer at the given address */
uint16_t read_short(uint8_t * mem, uint16_t addr)
{
    uint16_t val = mem[addr];
    val |= mem[addr + 1] << 8;
    return val;
}

/* Push a 16-bit value onto the stack if possible */
void push(vm * v, uint16_t val)
{
    if (v->sp > 0xFFFD)    
    {
        fprintf(stderr, "Stack overflow!\n");
        exit(-1);
    }
    memcpy(&v->stack[v->sp], &val, 2);
    v->sp += 2;
}

/* Pop a 16-bit value off the stack if possible */
uint16_t pop(vm * v)
{
    if (v->sp < 2)
    {
        fprintf(stderr, "Stack underflow!\n");
        exit(-1);
    }
    uint16_t val = read_short(v->stack, v->sp);
    v->sp -= 2;
    return val;
}

/* Read a 16-bit value at the given offset from the base pointer */
uint16_t read_base(vm * v, uint16_t offset)
{
    if (v->bp + offset * 2 > v->sp)
    {
        fprintf(stderr, "Illegal access!\n");
        exit(-1);
    }
    return read_short(v->stack, v->bp + 2 * offset);
}

/* Execute a single decoded instruction on the VM */
void execute_single(vm * v, instruction instr)
{
    switch(instr.o)
    {
    case ADD:
    {
        uint32_t addend = (instr.s == rr) ? v->regs[instr.reg] : instr.imm;
        uint32_t orig = v->regs[instr.dst];

        v->regs[instr.dst] = do_adder(addend, orig, &v->flags);
        break;
    }
    case SUB:
    {
        uint32_t addend = (instr.s == rr) ? v->regs[instr.reg] : instr.imm;
        addend = 1 + ~addend;
        uint32_t orig = v->regs[instr.dst];

        v->regs[instr.dst] = do_adder(addend, orig, &v->flags);
        break;
    }
    case SHL:
    {
        v->flags = 0;
        uint32_t result = v->regs[instr.dst] << instr.imm;
        if ((result & 0xFFFF) == 0)
            v->flags |= SF_Z;

        if (result & 0x10000)
            v->flags |= SF_C;
        
        if (result & 0x8000)
            v->flags |= SF_N;

        v->regs[instr.dst] = (result & 0xFFFF);
        break;
    }
    case ASHR:
    {
        uint32_t val = v->regs[instr.dst];
        if (val & 0x8000)
            val |= 0xFFFF0000;
        val = val >> instr.imm;
        v->regs[instr.dst] = val & 0xFFFF;
    }   
    case LSHR:
    {
        uint32_t val = v->regs[instr.dst];
        val = val >> instr.imm;
        v->regs[instr.dst] = val & 0xFFFF;
        break;
    }
    case AND:
    {
        uint32_t mask = (instr.s == rr) ? v->regs[instr.reg] : instr.imm;
        uint32_t orig = v->regs[instr.dst];
        v->regs[instr.dst] = (orig & mask) & 0xFFFF;
        break;
    }
    case OR:
    {
        uint32_t mask = (instr.s == rr) ? v->regs[instr.reg] : instr.imm;
        uint32_t orig = v->regs[instr.dst];
        v->regs[instr.dst] = (orig | mask) & 0xFFFF;
        break;
    }
    case NOT:
    {
        uint32_t orig = v->regs[instr.dst];
        v->regs[instr.dst] = (~orig) & 0xFFFF;
        break;
    }
    case ST:
    {
        uint16_t dest = v->regs[instr.dst];
        uint16_t val = (instr.s == im) ? instr.imm : 
                        (instr.s == bm) ? read_base(v, instr.imm) : v->regs[instr.reg];

        memcpy(v->memory + dest, &val, 2);
        break;
    }
    case LD:
    {
        uint16_t val = (instr.s == rr) ? v->regs[instr.reg] :
                        (instr.s == ri) ? instr.imm :
                        (instr.s == rb) ? read_base(v, instr.imm) : 
                                            read_short(v->memory, v->regs[instr.reg]);

        v->regs[instr.dst] = val;
        break;
    }
    case STACK:
    {
        if (instr.flag)
        {
            // push registers
            for (regstr reg = reg0; reg <= reg7; reg++)
            {
                if (instr.imm & (1 << reg))
                    push(v, v->regs[reg]);
            }
        }
        else
        {
            // pop into registers
            if (instr.imm == 0)
                pop(v); // pop a single value out of the stack
            for (regstr reg = reg0; reg <= reg7; reg++)
            {
                if (instr.imm & (1 << reg))
                    v->regs[reg] = pop(v);
            }
        }
        break;
    }
    case LOCS:
    {
         if (v->sp + instr.imm > 0xFFFF)
         {
             fprintf(stderr, "Base allocation overflow!\n");
             exit(-1);
         }
         v->sp += instr.imm;
         break;
    }
    case CMP:
    {
        uint32_t orig = v->regs[instr.dst];
        uint32_t comparator = (instr.s == rr) ? v->regs[instr.reg] : instr.imm;
        comparator = 1 + ~comparator;

        do_adder(comparator, orig, &v->flags);
        break;
    }
    case BR:
    {
        int16_t offset = instr.imm;
        if (offset & 0x10000)
        {
            // interpret as two's complement
            offset = 1 + ~offset;
            offset = -offset;
        }

        bool jump = false;
        if (instr.j == jmp)
            jump = true;
        else if (instr.j == jeq)
            jump = FLAG(SF_Z);
        else if (instr.j == jgt)
            jump = ~FLAG(SF_Z) & (FLAG(SF_N) ^ ~FLAG(SF_V));
        else if (instr.j == jlt)
            jump = (FLAG(SF_N) ^ FLAG(SF_V));
        else if (instr.j == jge)
            jump = (FLAG(SF_N) ^ ~FLAG(SF_V));
        else if (instr.j == jle)
            jump = FLAG(SF_Z) | (FLAG(SF_N) ^ FLAG(SF_Z));
        
        if (jump)
        {
            v->pc += 2 * offset - 2;
            break;
        } 
        
        if (instr.j == jsr)
        {
            push(v, v->pc + 2);
            v->pc += 2 * offset - 2;
        }
        else if (instr.j == ret)
        {
            v->sp = v->bp;
            v->bp = pop(v);
            v->pc = pop(v);
        }
        break;
    }
    case TRAP:
        if (instr.flag)
            printf("Received user trap code 0x%02x\n", instr.imm);
        else
            printf("Received system trap code 0x%02x\n", instr.imm);

        if (instr.imm == 0)
        {
            printf("Halting program...\n");
            exit(0);
        }
        else if (instr.imm == 1)
        {
            for (regstr reg = reg0; reg <= reg7; reg++)
                printf("Register %d contains 0x%04x\n", reg, v->regs[reg]);
        }
        break;
    }
}

void run(vm * v, uint16_t start)
{
    v->pc = start;
    while(true)
    {
        printf("PC is %d\n", v->pc);
        uint16_t opcode = read_short(v->memory, v->pc);
        instruction instr = decode(opcode);
        execute_single(v, instr);
        v->pc += 2;
    }
}



/*
LD reg0,5
LD reg1,6
ADD reg0,reg1
LD reg2,reg0


0:  LD reg0,6
2:  PUSH reg0
4:  POP reg1
6:  LD reg0,reg1
8:  SUB reg1,1
10: CMP reg1,1
12: JEQ +8
14: LD reg2,reg0
16: LD reg3,reg1
18: SUB reg3,1
20: CMP reg3,0
22: JEQ -7
24: ADD reg0,reg2
26: JMP -4
28: TRAP 1
30: TRAP 0

*/
int main()
{
    uint16_t program[] = {
        0b1000000011000101, // LD reg0, 5
        0b1000000111000110, // LD reg1, 6
        0b0000100010001111, // ADD reg0, reg1
        0b1000101000000101, // LD reg2,reg0
        0b1110000000000001, // TRAP 1 (dump registers)
        0b1110000000000000  // TRAP 0 (halt)
    };

    uint16_t program2[] = {
        0b1000000011000110, // LD reg0, 6
        0b1010110100000001, // PUSH reg0
        0b1010110000000010, // POP reg1
        0b1000100000001101, // LD reg0, reg1
        0b0001000111000001, // SUB reg1, 1
    };

    vm * v = (vm *)malloc(sizeof(vm));
    memcpy(v->memory, program, 12);
    run(v, 0);
}