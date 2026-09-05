#include "input_remap.h"
#include "hid.h"
#include "camera.h"
#include "input.h"

// OoT3D's GameState owns a sampled controller state beginning at offset 0x14.
#define PLAY_PAD_BUTTONS_OFFSET          0x14
#define PLAY_PAD_PRESSED_BUTTONS_OFFSET  0x18
#define PLAY_PAD_RELEASED_BUTTONS_OFFSET 0x1C

// Fast Move offsets copied from Nanquitas' original OoT3D CTRPF plugin and
// OcarinaCTRComposer. Both operate on the live Player actor.
#define FAST_MOVE_JUMP_OFFSET  0x77
#define FAST_MOVE_SPEED_OFFSET 0x222C
#define FAST_MOVE_SPEED_VALUE  0x41A00000u

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

static void InputRemap_ApplyFastMove(GlobalContext* globalCtx) {
#if defined(RSTICK) && defined(Version_EUR)
    static u32 heldFrames = 0;

    const int zrHeld = (rInputCtx.cur.val & BUTTON_ZR) != 0;
    Player* const player = globalCtx->mainCamera.player;

    // Important: do NOT gate this on CPAD_ANY. CTRPF's Key::CPad is not the
    // same thing as the raw HID direction bits used by this patch; those bits
    // can flicker around the analogue threshold and were resetting the frame
    // counter before Fast Move reached its speed-write phase.
    if (!zrHeld || !player) {
        heldFrames = 0;
        return;
    }

    volatile u8* const p = (volatile u8*)player;

    if (heldFrames < 3) {
        // Reproduce the original plugin's tiny launch phase. 0x77 is odd, so
        // write the little-endian halfword byte-by-byte.
        p[FAST_MOVE_JUMP_OFFSET] = 0x40;
        p[FAST_MOVE_JUMP_OFFSET + 1] = 0xCB;
        heldFrames++;
    } else {
        // Once ZR has been held for 3 update calls, continuously stamp the
        // original Fast Move value into Link's movement field.
        *(volatile u32*)(p + FAST_MOVE_SPEED_OFFSET) = FAST_MOVE_SPEED_VALUE;
    }
#else
    (void)globalCtx;
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
    InputRemap_ApplyFastMove(globalCtx);
}
