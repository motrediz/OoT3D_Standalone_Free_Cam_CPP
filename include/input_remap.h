#pragma once

#include "controls.h"
#include "z3D/z3D.h"

// Routes physical buttons to camera settings and native OoT3D actions.
void InputRemap_Update(GlobalContext* globalCtx);

// Applies actions that must run after OoT3D's own GlobalContext update.
void InputRemap_AfterUpdate(GlobalContext* globalCtx);
