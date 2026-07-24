#include "3ds/srv.h"
#include "3ds/services/apt.h"
#include "3ds/services/irrst.h"
#include "cpp.h"
#include "common.h"
#include "draw.h"
#include "input.h"
#include "camera.h"
#include "input_remap.h"

#ifdef RSTICK
bool new3dsFlag;//extern variable -> see common.h
#endif

static void Free_Camera_Init(void) {
    static u8 initialized = 0;
    if (!initialized) {
        srvInit();
        #ifdef RSTICK
        APT_CheckNew3DS(&new3dsFlag);
        if (new3dsFlag) irrstInit();
        else cppInit();
        #endif
        Draw_SetupFramebuffer();
        initialized = 1;
    }
}

void before_GlobalContext_Update(GlobalContext* globalCtx) {
    Free_Camera_Init();
    Input_Update();
    InputRemap_Update(globalCtx);
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

    //refaire ici
}

void after_GlobalContext_Update(GlobalContext* globalCtx) {
    displayHUD();
}