#include "input_remap.h"
#include "hid.h"
#include "camera.h"
#include "input.h"
#include "3ds/svc.h"

#define PLAY_PAD_BUTTONS_OFFSET          0x14
#define PLAY_PAD_PRESSED_BUTTONS_OFFSET  0x18
#define PLAY_PAD_RELEASED_BUTTONS_OFFSET 0x1C

// Native Luma cheat proven working on this EUR build writes 20.0f to Link+0x222C.
// The crucial difference from our previous attempts is timing: the native cheat engine
// writes asynchronously, while our old code wrote once at a deterministic point in the
// game update and OoT3D could overwrite the value before movement consumed it.
#define FAST_MOVE_SPEED_OFFSET 0x222C
#define FAST_MOVE_SPEED_VALUE  0x41A00000u
#define FAST_MOVE_THREAD_STACK 0x400
#define FAST_MOVE_SLEEP_NS     (1LL * 1000 * 1000) // 1 ms

typedef struct {
    u32 sourceButton;
    u32 targetButton;
} ButtonMap;

#if defined(RSTICK) && defined(Version_EUR)
static volatile Player* gFastMovePlayer = 0;
static volatile u32 gFastMoveZRHeld = 0;
static Handle gFastMoveThreadHandle = 0;
static u8 gFastMoveThreadStack[FAST_MOVE_THREAD_STACK] __attribute__((aligned(8)));
static u8 gFastMoveThreadStarted = 0;

static void FastMoveThread(void* arg) {
    (void)arg;
    for (;;) {
        Player* player = (Player*)gFastMovePlayer;
        if (gFastMoveZRHeld && player) {
            *(volatile u32*)((volatile u8*)player + FAST_MOVE_SPEED_OFFSET) = FAST_MOVE_SPEED_VALUE;
        }
        svcSleepThread(FAST_MOVE_SLEEP_NS);
    }
}

void InputRemap_StartFastMoveThread(void) {
    if (gFastMoveThreadStarted) return;
    if (svcCreateThread(&gFastMoveThreadHandle,
                        FastMoveThread,
                        0,
                        (u32*)(gFastMoveThreadStack + sizeof(gFastMoveThreadStack)),
                        0x28,
                        -1) == 0) {
        gFastMoveThreadStarted = 1;
    }
}
#else
void InputRemap_StartFastMoveThread(void) {}
#endif

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

#if defined(RSTICK) && defined(Version_EUR)
    // This hook is known-good for input because the original ZR->R mapping worked here.
    // Publish only state to the asynchronous worker; do not write movement from the game thread.
    gFastMovePlayer = globalCtx ? globalCtx->mainCamera.player : 0;
    gFastMoveZRHeld = (rInputCtx.cur.val & BUTTON_ZR) ? 1u : 0u;
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
}
