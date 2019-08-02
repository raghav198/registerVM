#include <stdbool.h>

typedef enum {
    ADD,
    SUB,
    SHL,
    ASHR,

    LSHR,
    AND,
    OR,
    NOT,

    LD,
    ST,
    STACK,
    LOCS,

    CMP,
    BR,
    TRAP,
} opcode;

typedef enum {
    reg0,
    reg1,
    reg2,
    reg3,
    reg4,
    reg5,
    reg6,
    reg7
} regstr;

typedef enum {
    rr, // register/register
    ri, // register/immediate
    rm, // register/indirect
    im, // immediate/indirect
    rb, // register/base offset
    bm, // base offset/indirect
    r, // single register
    i // single immediate (or base offset)
} style;

typedef enum {
    jmp,
    jgt,
    jeq,
    jge,
    jlt,
    jsr,
    jle,
    ret
} jump;

typedef struct {
    opcode o;
    style s;
    regstr dst;
    bool flag;
    jump j;
    union {
        regstr reg;
        short imm;
    };
} instruction;

instruction decode(short);