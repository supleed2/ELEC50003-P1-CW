#ifndef CONTROL_STATUS_H
#define CONTROL_STATUS_H

typedef enum {
    CS_ERROR = -1,
    CS_IDLE,
    CS_MOVING,
    CS_CHARGING
} ControlStatus_t;

#endif
