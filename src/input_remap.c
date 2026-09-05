#include "input_remap.h"
#include "hid.h"
#include "camera.h"
#include "input.h"

#define PLAY_PAD_BUTTONS_OFFSET          0x14
#define PLAY_PAD_PRESSED_BUTTONS_OFFSET  0x18
#define PLAY_PAD_RELEASED_BUTTONS_OFFSET 0x1C

// Mailbox value read by Luma's native cheat engine.
// This lives inside the patch's own linked data region, so we don't have to
// guess a free game RAM address or touch Link's movement structures directly.
#define ZR_MAILBOX_MAGIC_ON  0x5A525A52u  // "ZRZR"
#define ZR_MAILBOX_MAGIC_OFF 0x5A524F46u  // "ZROF"

// Deliberately non-static so the build workflow can resolve the exact linked
// address with arm-none-eabi-nm and generate the matching cheats.txt snippet.
volatile u32 gZRMailbox __attribute__((used, section(".data"))) = ZR_MAILBOX_MAGIC_OFF;

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
    // ZR does not write movement memory anymore. It only flips our private
    // mailbox; Luma's native cheat engine performs the already-proven Fast Move
    // write when this mailbox contains ZR_MAILBOX_MAGIC_ON.
    gZRMailbox = (rInputCtx.cur.val & BUTTON_ZR) ? ZR_MAILBOX_MAGIC_ON : ZR_MAILBOX_MAGIC_OFF;
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

void InputRemap_StartFastMoveThread(void) {
    // Compatibility no-op. No worker thread is used by the mailbox design.
}
