#include "input_remap.h"
#include "hid.h"
#include "camera.h"
#include "input.h"

#define PLAY_PAD_BUTTONS_OFFSET          0x14
#define PLAY_PAD_PRESSED_BUTTONS_OFFSET  0x18
#define PLAY_PAD_RELEASED_BUTTONS_OFFSET 0x1C

// IMPORTANT: this is the exact EUR address from the Luma/Gateshark cheat that
// has already been proven to work on the user's console:
//
//   098F722C 41A00000
//
// In Gateshark, the leading 0 is the 32-bit-write code type. It is NOT part of
// the address. Therefore the actual target address is 0x08F722C, not
// 0x098F722C. Our very first implementation got this wrong; later attempts used
// the USA plugin's Player+0x222C offset, which need not match the EUR Player
// layout even though Player+0x77 still produced the visible jump.
#define FAST_MOVE_EUR_ADDRESS 0x08F722Cu
#define FAST_MOVE_VALUE       0x41A00000u

typedef struct {
    u32 sourceButton;
    u32 targetButton;
} ButtonMap;

#ifdef RSTICK
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

static inline void InputRemap_ApplyFastMove(void) {
#if defined(RSTICK) && defined(Version_EUR)
    if (rInputCtx.cur.val & BUTTON_ZR) {
        *(volatile u32*)FAST_MOVE_EUR_ADDRESS = FAST_MOVE_VALUE;
    }
#endif
}

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

    // Stamp the exact known-good EUR cheat address before the game update.
    InputRemap_ApplyFastMove();

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
    // Stamp it again after the game update. This mirrors the continuously-
    // applied nature of a Luma cheat without introducing a separate thread.
    InputRemap_ApplyFastMove();
}

void InputRemap_StartFastMoveThread(void) {
    // Kept as a no-op so older call sites/build artifacts remain source-compatible.
}
