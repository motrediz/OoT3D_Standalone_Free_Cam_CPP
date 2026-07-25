#pragma once

#include <stdint.h>

typedef enum {
    CONTROL_ACTION_NONE = 0,
    CONTROL_ACTION_CAMERA_SENSITIVITY_UP,
    CONTROL_ACTION_CAMERA_SENSITIVITY_DOWN,
    CONTROL_ACTION_CAMERA_INVERT_PREVIOUS,
    CONTROL_ACTION_CAMERA_INVERT_NEXT,
    #ifdef RSTICK
    CONTROL_ACTION_CPP_DISABLE,
    #endif
} ControlAction;

ControlAction Controls_Resolve(uint32_t held, uint32_t pressed);