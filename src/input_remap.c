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

// EUR OoT3D address confirmed working with the existing Luma/Gateshark fast-move code.
#ifdef Version_EUR
#define FAST_MOVE_EUR_ADDRESS 0x098F722Cu
#define FAST_MOVE_VALUE       0x41A00000u
#endif

typedef struct {
    u32 sourceButton;
    u32 targetButton;
} ButtonMap;

#ifdef RSTICK
// Injects additional button inputs into the game state.
// Source buttons remain unchanged.
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
            if ((rInputCtx.pressed.val & src) && !(realHeld & tgt)) {
                pressed |= tgt;
            }
        } else if (rInputCtx.up.val & src) {
            if (!(realHeld & tgt)) {
                released |= tgt;
            }
        }
    }

    *gameHeld |= held;
    *gamePressed |= pressed;
    *gameReleased |= released;
}
#endif

void InputRemap_Update(GlobalContext* globalCtx) {
    #ifdef RSTICK
    static ButtonMap sButtonMaps[] = {
        #ifndef Version_EUR
        { BUTTON_ZR, BUTTON_R1 },
        #endif
        { BUTTON_ZL, BUTTON_L1 },
        { BUTTON_R1, BUTTON_R1 },
    };

    InputRemap_InjectButtonMappings(globalCtx, sButtonMaps, sizeof(sButtonMaps) / sizeof(sButtonMaps[0]));
    #endif

    const ControlAction action = Controls_Resolve(rInputCtx.cur.val, rInputCtx.pressed.val);
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

void InputRemap_AfterUpdate(GlobalContext* globalCtx) {
    (void)globalCtx;

    #if defined(RSTICK) && defined(Version_EUR)
    // OoT3D rewrites Link's movement value during its normal update, so apply
    // the known-good fast-move value after the game's update for the next frame.
    if (rInputCtx.cur.val & BUTTON_ZR) {
        *(volatile u32*)FAST_MOVE_EUR_ADDRESS = FAST_MOVE_VALUE;
    }
    #endif
}
