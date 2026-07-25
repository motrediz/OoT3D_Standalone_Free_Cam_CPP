#include "input_remap.h"
#include "hid.h"
#include "camera.h"
#include "input.h"

// OoT3D's GameState owns a sampled controller state beginning at offset 0x14.
// The sampler copies its source block at +0x04 into the GameState destination
// at +0x00, so the destination's held/new/released masks begin at
// PlayState+0x14/+0x18/+0x1C. This differs from MM3D's later pad::State layout.
#define PLAY_PAD_BUTTONS_OFFSET     0x14
#define PLAY_PAD_PRESSED_BUTTONS_OFFSET 0x18
#define PLAY_PAD_RELEASED_BUTTONS_OFFSET 0x1C

typedef struct {
    u32 sourceButton;
    u32 targetButton;
} ButtonMap;

#ifdef RSTICK
// Injects additional button inputs into the game state.
// Source buttons remain unchanged.
// Used for injecting ZR, ZL and R (CPP only) into the game state.
static void InputRemap_InjectButtonMappings(GlobalContext* globalCtx, const ButtonMap* remaps, u32 count) {
    volatile u32* const gameHeld = (volatile u32*)((u8*)globalCtx + PLAY_PAD_BUTTONS_OFFSET);
    volatile u32* const gamePressed = (volatile u32*)((u8*)globalCtx + PLAY_PAD_PRESSED_BUTTONS_OFFSET);
    volatile u32* const gameReleased = (volatile u32*)((u8*)globalCtx + PLAY_PAD_RELEASED_BUTTONS_OFFSET);

    const u32 realHeld = *gameHeld;

    u32 held = 0, pressed = 0, released = 0;

    for (u32 i = 0; i < count; ++i) {
        const u32 src = remaps[i].sourceButton;
        const u32 tgt = remaps[i].targetButton;

        if (rInputCtx.cur.val & src) {
            held |= tgt;
            // Only signals a "new press" if the source button has *itself* just transitioned to the pressed
            //state, and the target is not already physically held by the player to avoid a double signal.
            if ((rInputCtx.pressed.val & src) && !(realHeld & tgt)) {
                pressed |= tgt;
            }
        } else if (rInputCtx.up.val & src) {
            if (!(realHeld & tgt)) {
                released |= tgt;
            }
        }
    }
    //Write to the game state in memory, not to HID, since HID is read-only.
    *gameHeld |= held;
    *gamePressed |= pressed;
    *gameReleased |= released;
}
#endif

void InputRemap_Update(GlobalContext* globalCtx) {
    #ifdef RSTICK
    static ButtonMap sButtonMaps[] = {
        { BUTTON_ZR, BUTTON_R1 },
        { BUTTON_ZL, BUTTON_L1 },
        { BUTTON_R1, BUTTON_R1},
    };

    InputRemap_InjectButtonMappings(globalCtx, sButtonMaps,sizeof(sButtonMaps) / sizeof(sButtonMaps[0]));
    #endif

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
        case CONTROL_ACTION_NONE:
            break;
    }
}