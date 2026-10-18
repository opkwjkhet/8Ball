#pragma once
#import <Foundation/Foundation.h>
#import <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ── ESP vars ──
bool   Bridge_GetEspEnabled(void);
void   Bridge_SetEspEnabled(bool v);
bool   Bridge_GetDrawPredictionLine(void);
void   Bridge_SetDrawPredictionLine(bool v);
bool   Bridge_GetDrawPockets(void);
void   Bridge_SetDrawPockets(bool v);
bool   Bridge_GetDrawPocketsShotState(void);
void   Bridge_SetDrawPocketsShotState(bool v);
float  Bridge_GetLineWidth(void);
void   Bridge_SetLineWidth(float v);
float  Bridge_GetTableOffsetX(void);
void   Bridge_SetTableOffsetX(float v);
float  Bridge_GetTableOffsetY(void);
void   Bridge_SetTableOffsetY(float v);
float  Bridge_GetTableScaleFactor(void);
void   Bridge_SetTableScaleFactor(float v);
bool   Bridge_GetNoScreenShot(void);
void   Bridge_SetNoScreenShot(bool v);

// ── AutoPlay ──
bool   Bridge_GetAutoAim(void);
void   Bridge_SetAutoAim(bool v);
bool   Bridge_GetAutoPlaying(void);
void   Bridge_SetAutoPlaying(bool v);

// ── AutoQueue ──
bool   Bridge_GetAutoQueue(void);
void   Bridge_SetAutoQueue(bool v);

// ── Game state ──
bool   Bridge_IsInGame(void);
bool   Bridge_IsPlayerTurn(void);
bool   Bridge_IsNineBall(void);
int    Bridge_GetBallCount(void);
float  Bridge_GetFPS(void);

// ── Persist ──
void   Bridge_SaveSettings(void);

// ── InstaPocket ──
void   Bridge_TriggerInstaPocket(void);

#ifdef __cplusplus
}
#endif
