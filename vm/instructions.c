/*
Instruction set:
- ADD   rr/ri
- SUB   rr/ri
- SHL   r
- ASHR  r
- LSHR  r
- JMP   i
- JSR   i
- BR    i
- CMP   rr/ri
- AND   rr/ri
- OR    rr/ri
- NOT   r
- LD    rr/ri/rm
- ST    rm/im
- PUSH  [rset]
- POP   [rset]
- TRAP
- RET
*/
    
/* ADD/SUB/SHL/ASHR     LSHR/AND/OR/NOT  LD/ST/STACK/LOCS  CMP/JMP/TRAP */

/*
For add-like instructions (rr/ri):
################
XXXX0DDD11iiiiii (ri)
XXXX1DDDS0RRR111 (rr)

ADD:    SUB:    CMP:    AND:    OR:
0000    0001    1100    0101    0110


For ST: (rm/im)
################
XXXX0MMM01iiiiii (im) MMM is the register holding the memory address, iiiiii is the immediate value
XXXX1MMM01RRR110 (rm) MMM is the register holding the memory address, RRR is the register holding the value
XXXX0MMM00BBBBBB (bm) MMM is the register holding the value, BBBBBB is the base offset
1001

for LD: (rr/ri/rm)
################
XXXX0DDD11iiiiii (ri) DDD is the destination register, iiiiii is the immediate value
XXXX1DDD00RRR101 (rr)           RRR is  the register holding the source value
XXXX1DDD01MMM101 (rm)           MMM is the register holding the address of the source value
XXXX0DDD10BBBBBB (rb)           BBBBBB is the base offset
1000


for single R instructions:
################
XXXX0RRR1111iiii

SHL     ASHR    LSHR    NOT
0010    0011    1000    1011

the imm4 is the number of bits to shift by, or all 1s for NOT

for branches:
################
XXXXCCCiiiiiiiii
1110

JMP 000
JSR 101
JEQ 010
JGT 001
JLT 100
JGE 011
JLE 110
RET 111

for stack manipulation:
################
XXXX1101RRRRRRRR (push)
XXXX1100RRRRRRRR (pop)
1010

LOCS:
################
XXXX11111SBBBBBB (s=1 then allocate s=0 then deallocate)
1011

TRAP:
################
XXXX00U0HHHHHHHH
1110
*/

#include "instructions.h"
#include <stdio.h>

instruction decode(short code)
{
    instruction decoded;
    decoded.o = (code >> 12) & 0xF; // get the opcode
    switch(decoded.o) {
    case ADD:
    case SUB:
    case CMP:
    case AND:
    case OR:
        decoded.dst = (code >> 8) & 0x7;
        if ((code >> 11) & 1)
        {
            // register mode (rr)
            decoded.s = rr;
            decoded.reg = ((code >> 3) & 0x7); // source register
            decoded.flag = ((code >> 5) & 1); // set if signed, cleared if unsigned
        }
        else
        {
            // immediate mode (ri)
            decoded.s = ri;
            decoded.imm = code & 0x3F; // last 6 bits are immediate value
        }
        break;
    case ST:
        decoded.dst = (code >> 8) & 0x7;
        if ((code >> 11) & 1)
        {
            // register mode (rm)
            decoded.s = rm;
            decoded.reg = ((code >> 3) & 0x7);
        }
        else
        {
            // immediate mode (im) or base offset mode (bm)
            decoded.s = ((code >> 6) & 1) ? im : bm;
            decoded.imm = code & 0x3F;
        }
        break;
    case LD:
        decoded.dst = (code >> 8) & 0x7;
        if ((code >> 11) & 1)
        {
            decoded.s = ((code >> 6) & 1) ? rm : rr;
            decoded.reg = (code >> 3) & 0x7;
        }
        else
        {
            // immediate load (ri) or base offset load (rb)
            decoded.s = ((code >> 6) & 1) ? ri : rb;
            decoded.imm = code & 0x3F;
        }
        break;
    case SHL:
    case ASHR:
    case LSHR:
        decoded.imm = code & 0xF;
    case NOT:
        decoded.s = r;
        decoded.dst = (code >> 8) & 0x7;
        break;
    case BR:
        decoded.s = i;
        decoded.j = (code >> 9) & 0x7;
        decoded.imm = code & 0x1FF;
        break;
    case STACK:
        decoded.s = i;
        decoded.imm = code & 0xFF;
        decoded.flag = (code >> 8) & 1;
        break;
    case LOCS:
        decoded.s = i;
        decoded.imm = code & 0x3F;
        decoded.flag = (code >> 6) & 1;
        break;
    case TRAP:
        decoded.s = i;
        decoded.flag = (code >> 9) & 1;
        decoded.imm = code & 0xFF;
        break;
    }
    return decoded;
}