#ifndef INSTRUCTION_H
#define INSTRUCTION_H
#include <string>

typedef enum {
    INSTR_RESET = -1,
    INSTR_STOP,
    INSTR_MOVE,
    INSTR_CHARGE,
    INSTR_WAIT,
    INSTR_COLOUR
} instr_t;

typedef struct instruction
{
    int id;
    int instr;
    int heading;
    int distance;
    float speed;
    int charge;
    int time;
    String colour;
} RoverInstruction;

#endif
