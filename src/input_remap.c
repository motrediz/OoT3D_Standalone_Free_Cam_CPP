#include "input_remap.h"
#include "hid.h"
#include "camera.h"
#include "input.h"

// OoT3D's GameState owns a sampled controller state beginning at offset 0x14.
// The sampler copies its source block at +0x04 into the GameState destination
// at +0x00, so the destination's held/new/released masks begin at
// PlayState+0x14/+0x18/+0x1C. This differs from MM3D's later pad::State layout.
#define PLAY_PAD_BUTTONS_OFFSET          0x14
#define PLAY_PAD_PRESSED_BUTTONS_OFFSET  0x18
#define PLAY_PAD_RELEASED_BUTTONS_OFFSET 0x1C

// Fast Move is copied from Nanquitas' original OoT3D CTRPF plugin and from
// OcarinaCTRComposer's port of the same cheat. Both operate on the live Player
// actor, not on a fixed RAM address:
//   Player + 0x77   <- 0xCB40 for the first 3 held frames
//   Player + 0x222C <- 0x41A00000 afterwards
// Using the live Player pointer is important because its heap address can move.
#define FAST_MOVE_JUMP_OFFSET  0x77
#define FAST_MOVE_SPEED_OFFSET 0x222C
#define FAST_MOVE_SPEED_VALUE  0x41A00000u
#define CPAD_ANY (CPAD_RIGHT | CPAD_LEFT | CPAD_UP | CPAD_DOWN)

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

// Exact Fast Move behaviour used by the established OoT3D cheat plugins.
// Called after the game's normal update so the speed value survives into the
// next movement step. The Player pointer comes from OoT3D's live main Camera;
// this same field is already used throughout this Free Cam project.
static void InputRemap_ApplyFastMove(GlobalContext* globalCtx) {
#if defined(RSTICK) && defined(Version_EUR)
    static u32 jumpFrames = 0;

    const int zrHeld = (rInputCtx.cur.val & BUTTON_ZR) != 0;
    const int moving = (rInputCtx.cur.val & CPAD_ANY) != 0;
    Player* const player = globalCtx->mainCamera.player;

    if (!zrHeld || !moving || !player) {
        jumpFrames = 0;
        return;
    }

    volatile u8* const p = (volatile u8*)player;

    if (jumpFrames < 3) {
        // 0x77 is intentionally odd. Write the little-endian halfword as two
        // bytes instead of performing an unaligned u16 store.
        p[FAST_MOVE_JUMP_OFFSET] = 0x40;
        p[FAST_MOVE_JUMP_OFFSET + 1] = 0xCB;
        jumpFrames++;
    } else {
        *(volatile u32*)(p + FAST_MOVE_SPEED_OFFSET) = FAST_MOVE_SPEED_VALUE;
    }
#else
    (void)globalCtx;
#endif
}

void InputRemap_Update(GlobalContext* globalCtx) {
#ifdef RSTICK
    static ButtonMap sButtonMaps[] = {
        // EUR New 3DS: ZR is reserved for Fast Move instead of mirroring R.
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
