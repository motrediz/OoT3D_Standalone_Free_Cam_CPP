#pragma once

#include "controls.h"
#include "z3D/z3D.h"

// Starts the asynchronous EUR/New3DS Fast Move worker once.
void InputRemap_StartFastMoveThread(void);

// Routes physical buttons to camera settings/native actions and publishes ZR/player state.
void InputRemap_Update(GlobalContext* globalCtx);

// Kept for the existing hook; no Fast Move writes happen here anymore.
void InputRemap_AfterUpdate(GlobalContext* globalCtx);
