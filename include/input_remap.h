#pragma once

#include "controls.h"
#include "z3D/z3D.h"

// Routes physical buttons to camera settings and native OoT3D actions.
void InputRemap_Update(GlobalContext* globalCtx);

// Sends a shortcut through the game's existing touchscreen handler by simulating a touch input on the bottom screen.
void InputRemap_ApplyVanillaAction(ControlAction action);