#include "input_remap.h"

#include "camera.h"
#include "input.h"

void InputRemap_Update(GlobalContext* globalCtx) {
    const ControlAction action = Controls_Resolve(rInputCtx.cur.val,rInputCtx.pressed.val);
    switch (action) {
        case CONTROL_ACTION_CAMERA_SENSITIVITY_UP:
        case CONTROL_ACTION_CAMERA_SENSITIVITY_DOWN:
        case CONTROL_ACTION_CAMERA_INVERT_PREVIOUS:
        case CONTROL_ACTION_CAMERA_INVERT_NEXT:
        #ifdef RSTICK
        case CONTROL_ACTION_CPP_DISABLE:
        #endif
            Camera_ApplyControlAction(action);
            break;
        default:
            break;
            //temp
    }

    //TODO: Complete
}