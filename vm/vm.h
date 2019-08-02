#include "instructions.h"
#include <stdint.h>

enum {
    SF_Z = 0x1,
    SF_N = 0x2,
    SF_V = 0x4,
    SF_C = 0x8
};

#define FLAG(x) (v->flags & (x))

typedef struct {
    uint8_t memory[65536];
    uint8_t stack[65536];

    uint16_t regs[8];
    uint16_t pc, sp, bp;

    uint8_t flags;
} vm;

vm * init_vm();

void execute_single(vm *, instruction);
void run(vm *, uint16_t);