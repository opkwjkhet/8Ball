// ESP_Bridge.mm — thin C wrappers around ESP.h static vars/functions
// Include ESP.h ONLY here (already included in ImGuiDrawView.mm, but
// ESP_Bridge.mm is a separate TU so static vars are its own copies —
// that's fine: UIKitMenu reads/writes through these wrappers which
// operate on the same in-process memory via the pointers below).
//
// ⚠️  Because ESP.h uses "static", each TU gets its own copy of each var.
//     The real solution is to make UIKitMenu call these bridges which
//     forward to the SAME static storage as ImGuiDrawView.mm's TU.
//     We achieve this by having ImGuiDrawView.mm call
//     Bridge_BindPointers() once at startup, giving us the addresses.

#include "ImGui/imgui.h"
#include "ImGui/Icon.h"
#include "ImGui/Iconcpp.h"
#include "ImGui/ico_font.h"
#include "ESP.h"
#include "ESP_Bridge.h"

// ── Pointer slots (filled by Bridge_BindPointers from ImGuiDrawView.mm) ──
static bool  *p_espEnabled          = nullptr;
static bool  *p_drawPredictionLine  = nullptr;
static bool  *p_drawPockets         = nullptr;
static bool  *p_drawPocketsShotState= nullptr;
static float *p_lineWidth           = nullptr;
static float *p_tableOffsetX        = nullptr;
static float *p_tableOffsetY        = nullptr;
static float *p_tableScaleFactor    = nullptr;
static bool  *p_noScreenShot        = nullptr;
static bool  *p_autoAim             = nullptr;
static bool  *p_autoPlaying         = nullptr;
static bool  *p_autoQueue           = nullptr;

extern "C" void Bridge_BindPointers(
    bool *espEnabled, bool *predLine, bool *pockets, bool *pocketState,
    float *lineW, float *offX, float *offY, float *scale,
    bool *noScreen, bool *autoAim, bool *autoPlay, bool *autoQueue)
{
    p_espEnabled           = espEnabled;
    p_drawPredictionLine   = predLine;
    p_drawPockets          = pockets;
    p_drawPocketsShotState = pocketState;
    p_lineWidth            = lineW;
    p_tableOffsetX         = offX;
    p_tableOffsetY         = offY;
    p_tableScaleFactor     = scale;
    p_noScreenShot         = noScreen;
    p_autoAim              = autoAim;
    p_autoPlaying          = autoPlay;
    p_autoQueue            = autoQueue;
}

// ── Getters / Setters ──
#define BRIDGE_BOOL(name, ptr) \
    extern "C" bool  Bridge_Get##name(void) { return ptr ? *ptr : false; } \
    extern "C" void  Bridge_Set##name(bool v) { if (ptr) *ptr = v; }

#define BRIDGE_FLOAT(name, ptr) \
    extern "C" float Bridge_Get##name(void) { return ptr ? *ptr : 0.f; } \
    extern "C" void  Bridge_Set##name(float v) { if (ptr) *ptr = v; }

BRIDGE_BOOL(EspEnabled,          p_espEnabled)
BRIDGE_BOOL(DrawPredictionLine,  p_drawPredictionLine)
BRIDGE_BOOL(DrawPockets,         p_drawPockets)
BRIDGE_BOOL(DrawPocketsShotState,p_drawPocketsShotState)
BRIDGE_FLOAT(LineWidth,          p_lineWidth)
BRIDGE_FLOAT(TableOffsetX,       p_tableOffsetX)
BRIDGE_FLOAT(TableOffsetY,       p_tableOffsetY)
BRIDGE_FLOAT(TableScaleFactor,   p_tableScaleFactor)
BRIDGE_BOOL(NoScreenShot,        p_noScreenShot)
BRIDGE_BOOL(AutoAim,             p_autoAim)
BRIDGE_BOOL(AutoPlaying,         p_autoPlaying)
BRIDGE_BOOL(AutoQueue,           p_autoQueue)

extern "C" bool  Bridge_IsInGame(void)      { @try { return isInGame();           } @catch(...){ return false; } }
extern "C" bool  Bridge_IsPlayerTurn(void)  { @try { return isPlayerTurn();       } @catch(...){ return false; } }
extern "C" bool  Bridge_IsNineBall(void)    { @try { return isNineBallRuntime();  } @catch(...){ return false; } }
extern "C" int   Bridge_GetBallCount(void)  { @try { return getTableBallCount(GM_Table()); } @catch(...){ return 0; } }
extern "C" float Bridge_GetFPS(void)        { return (float)ImGui::GetIO().Framerate; }
extern "C" void  Bridge_SaveSettings(void)      { @try { ESP_SaveSettings(); } @catch(...){} }
extern "C" void  Bridge_TriggerInstaPocket(void){ g_instaPocket = true; }
