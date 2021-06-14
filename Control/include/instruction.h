#ifndef INSTRUCTION_H
#define INSTRUCTION_H

typedef enum {
    INSTR_RESET = -1,
    INSTR_STOP,
    INSTR_MOVE,
    INSTR_CHARGE
} instr_t;

typedef struct instruction
{
    int id;
    int instr;
    int heading;
    int distance;
    float speed;
    int charge;
} RoverInstruction;

#endif
