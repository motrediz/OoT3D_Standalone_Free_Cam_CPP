#include "controls.h"
#include "common.h"
#include "hid.h"

ControlAction Controls_Resolve(uint32_t held, uint32_t pressed) {
    const uint32_t configChord = BUTTON_L1 | BUTTON_R1;

    if ((held & configChord) == configChord) {
        if (pressed & BUTTON_UP) {
            return CONTROL_ACTION_CAMERA_SENSITIVITY_UP;
        }
        if (pressed & BUTTON_DOWN) {
            return CONTROL_ACTION_CAMERA_SENSITIVITY_DOWN;
        }
        if (pressed & BUTTON_LEFT) {
            return CONTROL_ACTION_CAMERA_INVERT_PREVIOUS;
        }
        if (pressed & BUTTON_RIGHT) {
            return CONTROL_ACTION_CAMERA_INVERT_NEXT;
        }
        #ifdef RSTICK
        // Allows Old 3DS users to disable the CPP, as it may cause interference when unplugged.
        if (!new3dsFlag){
            if (pressed & BUTTON_SELECT) {
                return CONTROL_ACTION_CPP_DISABLE;
            }
        }
        #endif
    }
    return CONTROL_ACTION_NONE;
}
