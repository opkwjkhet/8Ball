#pragma once
//
//  ESP.h
//  8 Ball Pool — iOS Hack (Prediction Lines)
//  Converted from Android (jni) + offsets from dump.cs
//

#include <cmath>
#include <cstring>
#include <cstdio>
#include <vector>
#include <array>
#include <algorithm>
#include <mach-o/dyld.h>
#include <dlfcn.h>
#include <objc/runtime.h>
#include <objc/message.h>
#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import "autoclick/wthaxvnpriv.h"
#include "ImGui/imgui.h"
#import "thongbao.h"

// ============================================================================
// Vector / Point types (matching jni/8bp code)
// ============================================================================

typedef uintptr_t ptr;

#define F(type, address) (*(type*)(address))
#define M(type, address, ...) ((type(*)(__VA_ARGS__))(address))

struct Vec2d {
    double x, y;
    Vec2d() : x(0), y(0) {}
    Vec2d(double x, double y) : x(x), y(y) {}
    Vec2d operator+(const Vec2d &v) const { return {x+v.x, y+v.y}; }
    Vec2d operator-(const Vec2d &v) const { return {x-v.x, y-v.y}; }
    Vec2d operator*(double s) const { return {x*s, y*s}; }
    bool operator==(const Vec2d &v) const { return x==v.x && y==v.y; }
    bool operator!=(const Vec2d &v) const { return !(*this==v); }
    operator bool() const { return x != 0.0 || y != 0.0; }
    double square() const { return x*x + y*y; }
    void nullify() { x = 0; y = 0; }
};
typedef Vec2d Point2D;
typedef Vec2d Vector2D;

struct Vec3d {
    double x, y, z;
    Vec3d() : x(0), y(0), z(0) {}
    Vec3d(double x, double y, double z) : x(x), y(y), z(z) {}
    operator bool() const { return x!=0.0||y!=0.0||z!=0.0; }
    void nullify() { x=0; y=0; z=0; }
};

struct Vec4d {
    double x, y, z, w;
    Vec4d() : x(0), y(0), z(0), w(0) {}
};

// ============================================================================
// Game Constants (physics — platform-independent)
// ============================================================================

inline constexpr int MAX_BALLS_COUNT = 16;
inline constexpr double BALL_RADIUS = 3.800475;
inline constexpr double BALL_RADIUS_SQUARE = BALL_RADIUS * BALL_RADIUS;

inline constexpr double TABLE_WIDTH = 254.0;
inline constexpr double TABLE_HEIGHT = 127.0;
inline constexpr double TABLE_HALF_WIDTH = TABLE_WIDTH / 2.0;
inline constexpr double TABLE_HALF_HEIGHT = TABLE_HEIGHT / 2.0;

inline constexpr double TABLE_BOUND_LEFT = -TABLE_HALF_WIDTH + BALL_RADIUS;
inline constexpr double TABLE_BOUND_TOP = -TABLE_HALF_HEIGHT + BALL_RADIUS;
inline constexpr double TABLE_BOUND_RIGHT = TABLE_HALF_WIDTH - BALL_RADIUS;
inline constexpr double TABLE_BOUND_BOTTOM = TABLE_HALF_HEIGHT - BALL_RADIUS;

inline constexpr int TABLE_POCKETS_COUNT = 6;
inline constexpr int TABLE_SHAPE_SIZE = 46;
inline constexpr double POCKET_RADIUS = 8.0;
inline constexpr double POCKET_RADIUS_SQUARE = POCKET_RADIUS * POCKET_RADIUS;
inline constexpr int MAX_SHOT_RESULT_SIZE = 50000;

inline constexpr double TIME_PER_TICK = 0.005;
inline constexpr double MIN_TIME = 1E-11;
inline constexpr double PI_CONST = 3.141592653589793238462643383279502884;
constexpr double maxAngle = 360.0 / (180.0 / PI_CONST);

struct Candidate {
    int idx = -1;
    double angle = 0;
    double score = 0;
    int pocketIndex = -1;
    double power = 0;
    bool operator<(const Candidate& other) const { return score < other.score; }
};

// ============================================================================
// Ball enums
// ============================================================================

namespace BallEnums {
    enum Classification : int {
        ANY = -1,
        CUE_BALL = 0,
        SOLID = 1,
        STRIPE = 2,
        NINE_BALL_RULE = 3,
        EIGHT_BALL = 4,
        ERR_CLASSIFICATION = -8
    };
    enum State : int {
        DEFAULT = 1,
        IN_POCKET = 2,
        UNKNOWN = 3,
        POTTED = 4,
        ERR_STATE = -8
    };
};

// ============================================================================
// Runtime offset resolution via ObjC ivar_getOffset()
// Android offsets DON'T work on iOS — ObjC class layout is different
// ============================================================================

static double CUE_PROPERTIES_SPIN_VALUE = 0.96551722494;
static double g_effectiveMaxPower = 888.85; // backup5 formula base: from UserInfo cue stats
static bool g_cueStatsCached = false;       // reset each time we enter a new game

static struct GameOffsets {
    bool resolved = false;
    // GameManager
    ptrdiff_t gm_mTable = -1;
    ptrdiff_t gm_mVisualCue = -1;
    ptrdiff_t gm_mVisualEnglishControl = -1;
    ptrdiff_t gm_mStateManager = -1;
    ptrdiff_t gm_rules = -1;
    // Table
    ptrdiff_t tbl_mTableProperties = -1;
    ptrdiff_t tbl_mBalls = -1;
    ptrdiff_t tbl_mTableShape = -1;
    ptrdiff_t tbl_frictionProperties = -1;
    ptrdiff_t tbl_mTableCollisionBounds = -1;
    // VisualCue
    ptrdiff_t vc_mVisualGuide = -1;
    ptrdiff_t vc_mPower = -1;
    // VisualEnglishControl
    ptrdiff_t vec_mEnglish = -1;
    ptrdiff_t vec_spinLevel = -1;
    // StateManager
    ptrdiff_t sm_mStateStack = -1;
    // MainManager
    ptrdiff_t mm_mStateManager = -1;
    // TableProperties
    ptrdiff_t tp_mPockets = -1;
    // Ball
    ptrdiff_t ball_physicsProperties = -1; // BallPhysicsProperties struct (contains pos/vel/spin)
    ptrdiff_t ball_classification = -1;    // int (direct ivar)
    ptrdiff_t ball_state = -1;             // int (direct ivar)
    // VisualCue extra — ALL power-related ivars
    ptrdiff_t vc_mInputActive = -1;
    ptrdiff_t vc_powerBarView = -1;
    ptrdiff_t vc_mMaxCuePowerOffset = -1;
    ptrdiff_t vc_mCurrentPowerOffset = -1;
    ptrdiff_t vc_mPowerMode = -1;
    ptrdiff_t vc_powerControlActive = -1;
    ptrdiff_t vc_mPowerSliderVisible = -1;
    ptrdiff_t vc_mControlMode = -1;
    ptrdiff_t vc_mMaxAngleForPowerMode = -1;
    ptrdiff_t vc_mDistanceFromTip = -1;
    ptrdiff_t vc_mAnimatePowerTarget = -1;
    ptrdiff_t vc_mUserSensitivity = -1;
    // VisualEnglishControl extra
    ptrdiff_t vec_mInputRadius = -1;
    // GameManager extra
    ptrdiff_t gm_gameMode = -1;
    ptrdiff_t gm_previousState = -1;
    ptrdiff_t gm_menuStateManager = -1;  // _stateManager (MCMenuStateManager) — used to fire shots directly
    // CCDirector / CCGLView (auto-resolved from dump, previously hardcoded)
    ptrdiff_t dir_view = -1;           // CCDirector.__view (was 0x1F8)
    ptrdiff_t glv_pointBound = -1;     // CCGLView._pointBound (was 0x38)
    ptrdiff_t glv_boundToWidth = -1;   // CCGLView._boundToWidth (was 0x40)
    ptrdiff_t glv_boundToSafeArea = -1;// CCGLView._boundToSafeArea (was 0x41)
} g_off;

#define PHYS_OFF_POSITION  0x00  // Vec2d (16 bytes)
#define PHYS_OFF_VELOCITY  0x10  // Vec2d (16 bytes)
#define PHYS_OFF_RADIUS    0x20  // double (8 bytes)
#define PHYS_OFF_SPIN      0x28  // Vec3d (24 bytes)
#define PHYS_OFF_MASS      0x40  // double
#define PHYS_OFF_VOLUME    0x48  // double

static ptrdiff_t resolveIvar(const char *clsName, const char *ivarName) {
    Class cls = objc_getClass(clsName);
    if (!cls) { NSLog(@"[8BP] Class not found: %s", clsName); return -1; }
    Ivar iv = class_getInstanceVariable(cls, ivarName);
    if (!iv) { NSLog(@"[8BP] Ivar not found: %s.%s", clsName, ivarName); return -1; }
    ptrdiff_t off = ivar_getOffset(iv);
    NSLog(@"[8BP] Resolved %s.%s = 0x%lX", clsName, ivarName, (long)off);
    return off;
}

static bool resolveAllOffsets(void) {
    if (g_off.resolved) return true;

    g_off.gm_mTable = resolveIvar("GameManager", "mTable");
    g_off.gm_mVisualCue = resolveIvar("GameManager", "mVisualCue");
    g_off.gm_mVisualEnglishControl = resolveIvar("GameManager", "mVisualEnglishControl");
    g_off.gm_mStateManager = resolveIvar("GameManager", "mStateManager");
    g_off.gm_rules = resolveIvar("GameManager", "_rules");

    g_off.tbl_mTableProperties = resolveIvar("Table", "mTableProperties");
    g_off.tbl_mBalls = resolveIvar("Table", "mBalls");
    g_off.tbl_mTableShape = resolveIvar("Table", "mTableShape");
    g_off.tbl_frictionProperties = resolveIvar("Table", "_frictionProperties");
    g_off.tbl_mTableCollisionBounds = resolveIvar("Table", "mTableCollisionBounds");

    g_off.vc_mVisualGuide = resolveIvar("VisualCue", "mVisualGuide");
    g_off.vc_mPower = resolveIvar("VisualCue", "mPower");

    g_off.vec_mEnglish = resolveIvar("VisualEnglishControl", "mEnglish");
    g_off.vec_spinLevel = resolveIvar("VisualEnglishControl", "mSpinLevel");

    g_off.sm_mStateStack = resolveIvar("StateManager", "mStateStack");
    g_off.mm_mStateManager = resolveIvar("MainManager", "mStateManager");

    // GameManager._stateManager (MCMenuStateManager) — Android uses GM+0x3b0 to fire shots
    g_off.gm_menuStateManager = resolveIvar("GameManager", "_stateManager");
    NSLog(@"[8BP] GM._stateManager (MCMenuStateManager) = 0x%lX (Android uses 0x3B0)", (long)g_off.gm_menuStateManager);
    g_off.tp_mPockets = resolveIvar("TenByFiveNarrowTableProperties", "mPockets");
    if (g_off.tp_mPockets < 0)
        g_off.tp_mPockets = resolveIvar("TableProperties", "mPockets");

    // Ball ivars — from iOS dump.cs: Ball has _physicsProperties (struct), classification, state
    g_off.ball_physicsProperties = resolveIvar("Ball", "_physicsProperties");
    g_off.ball_classification = resolveIvar("Ball", "classification");
    g_off.ball_state = resolveIvar("Ball", "state");

    g_off.vc_mInputActive = resolveIvar("VisualCue", "mInputActive");
    g_off.vc_powerBarView = resolveIvar("VisualCue", "_powerBarView");
    g_off.vc_mMaxCuePowerOffset = resolveIvar("VisualCue", "mMaxCuePowerOffset");
    g_off.vc_mCurrentPowerOffset = resolveIvar("VisualCue", "mCurrentPowerOffset");
    g_off.vc_mPowerMode = resolveIvar("VisualCue", "mPowerMode");
    g_off.vc_powerControlActive = resolveIvar("VisualCue", "_powerControlActive");
    g_off.vc_mPowerSliderVisible = resolveIvar("VisualCue", "mPowerSliderVisible");
    g_off.vc_mControlMode = resolveIvar("VisualCue", "mControlMode");
    g_off.vc_mMaxAngleForPowerMode = resolveIvar("VisualCue", "mMaxAngleForPowerMode");
    g_off.vc_mDistanceFromTip = resolveIvar("VisualCue", "mDistanceFromTip");
    g_off.vc_mAnimatePowerTarget = resolveIvar("VisualCue", "mAnimatePowerTarget");
    g_off.vc_mUserSensitivity = resolveIvar("VisualCue", "mUserSensitivity");
    g_off.vec_mInputRadius = resolveIvar("VisualEnglishControl", "mInputRadius");
    g_off.gm_gameMode = resolveIvar("GameManager", "mGameMode");
    g_off.gm_previousState = resolveIvar("GameManager", "_previousState");

    // CCDirector / CCGLView — auto-resolve instead of hardcoded offsets
    g_off.dir_view = resolveIvar("CCDirector", "__view");
    if (g_off.dir_view < 0) g_off.dir_view = resolveIvar("CCDirectorMiniclip", "__view");
    if (g_off.dir_view < 0) g_off.dir_view = resolveIvar("CCDirectorMC", "__view");
    g_off.glv_pointBound = resolveIvar("CCGLView", "_pointBound");
    g_off.glv_boundToWidth = resolveIvar("CCGLView", "_boundToWidth");
    g_off.glv_boundToSafeArea = resolveIvar("CCGLView", "_boundToSafeArea");

    bool ok = (g_off.gm_mTable >= 0 && g_off.tbl_mBalls >= 0);
    if (ok) {
        g_off.resolved = true;
        NSLog(@"[8BP] All critical offsets resolved successfully!");
    } else {
        NSLog(@"[8BP] ERROR: Critical offsets missing! gm_mTable=%ld tbl_mBalls=%ld",
              (long)g_off.gm_mTable, (long)g_off.tbl_mBalls);
    }
    return ok;
}

// ============================================================================
// ObjC runtime — find game instances via scene graph
// ============================================================================

static ptr g_poolBase = 0;
static uintptr_t g_poolSlide = 0;
static bool g_initialized = false;

static ptr g_gameManager = 0;
static ptr g_mainManager = 0;
static ptr g_menuManager = 0;
static ptr g_userInfo = 0;

static float g_screenW = 0;
static float g_screenH = 0;

static inline bool isValidPtr(ptr p) {
    return p > 0x100000000ULL && p < 0xFC0000000ULL;
}


static void resolveGameInstances(void) {
    @try {
        // GameManager singleton
        Class gmClass = objc_getClass("GameManager");
        if (gmClass) {
            SEL sel = sel_registerName("sharedGameManager");
            if ([(id)gmClass respondsToSelector:sel]) {
                id gm = ((id(*)(id, SEL))objc_msgSend)((id)gmClass, sel);
                g_gameManager = gm ? (ptr)gm : 0;
            }
        }

        // MainManager singleton
        if (!isValidPtr(g_mainManager)) {
            Class mmClass = objc_getClass("MainManager");
            if (mmClass) {
                SEL sel = sel_registerName("sharedMainManager");
                if ([(id)mmClass respondsToSelector:sel])
                    g_mainManager = (ptr)((id(*)(id, SEL))objc_msgSend)((id)mmClass, sel);
            }
        }

        // UserInfo singleton
        if (!isValidPtr(g_userInfo)) {
            Class uiClass = objc_getClass("UserInfo");
            if (uiClass) {
                SEL sel = sel_registerName("sharedUserInfo");
                if ([(id)uiClass respondsToSelector:sel])
                    g_userInfo = (ptr)((id(*)(id, SEL))objc_msgSend)((id)uiClass, sel);
            }
        }

        // MenuManager singleton
        if (!isValidPtr(g_menuManager)) {
            Class mmClass = objc_getClass("MenuManager");
            if (mmClass) {
                SEL sel = sel_registerName("sharedMenuManager");
                if ([(id)mmClass respondsToSelector:sel])
                    g_menuManager = (ptr)((id(*)(id, SEL))objc_msgSend)((id)mmClass, sel);
            }
        }
    } @catch (...) {}
}


static bool findPoolBinary(void) {
    uint32_t count = _dyld_image_count();
    for (uint32_t i = 0; i < count; i++) {
        const char *name = _dyld_get_image_name(i);
        if (!name) continue;
        if (strstr(name, "/pool") != NULL) {
            g_poolBase = (uint64_t)_dyld_get_image_header(i);
            g_poolSlide = _dyld_get_image_vmaddr_slide(i);
            return true;
        }
    }
    return false;
}

static ptr readPtr(ptr base, ptrdiff_t off) {
    if (!isValidPtr(base) || off < 0) return 0;
    return F(ptr, base + off);
}

static ptr g_ccDirector = 0;

static struct GameCache {
    volatile int mmStateId = -1;
    volatile int gmStateId = -1;
    volatile int gameMode = -1;
    volatile bool is9BallByGameApi = false;
    volatile int ballCount = 0;
    ptr ballPtrs[MAX_BALLS_COUNT] = {};
    // Table screen rect (computed on main thread via ObjC)
    volatile bool tableRectValid = false;
    double tableScrLeft = 0, tableScrRight = 0, tableScrTop = 0, tableScrBottom = 0;
} g_cache;

// Quick check: is ptr likely a valid ObjC object (isa pointer looks sane)?
static bool isObjCObj(ptr p) {
    if (!isValidPtr(p)) return false;
    ptr isa = F(ptr, p) & 0x7FFFFFFFFFF8ULL;
    return isValidPtr(isa);
}
static id getSharedUserInfo();
static void cacheCueStats();
static void updateGameCache(void) {
    @autoreleasepool {
    @try {
        static Class gmClass = nil;
        if (!gmClass) gmClass = objc_getClass("GameManager");
        if (gmClass) {
            BOOL isInit = ((BOOL(*)(id, SEL))objc_msgSend)((id)gmClass, sel_registerName("isGameManagerInitialized"));
            if (isInit) {
                id gm = ((id(*)(id, SEL))objc_msgSend)((id)gmClass, sel_registerName("sharedGameManager"));
                g_gameManager = gm ? (ptr)gm : 0;
            } else {
                g_gameManager = 0;
                g_cache.gmStateId = -1;
                g_cache.ballCount = 0;
                return; // not initialized — skip everything
            }
        }

        SEL stateSel = sel_registerName("getCurrentStateId");

        ptr gmSm = readPtr(g_gameManager, g_off.gm_mStateManager);
        if (isObjCObj(gmSm) && [(__bridge id)(void*)gmSm respondsToSelector:stateSel])
            g_cache.gmStateId = (int)((NSUInteger(*)(id, SEL))objc_msgSend)((__bridge id)(void*)gmSm, stateSel);
        else
            g_cache.gmStateId = -1;

        if (isValidPtr(g_mainManager) && g_off.mm_mStateManager >= 0) {
            ptr mmSm = readPtr(g_mainManager, g_off.mm_mStateManager);
            if (isObjCObj(mmSm) && [(__bridge id)(void*)mmSm respondsToSelector:stateSel])
                g_cache.mmStateId = (int)((NSUInteger(*)(id, SEL))objc_msgSend)((__bridge id)(void*)mmSm, stateSel);
            else
                g_cache.mmStateId = -1;
        }

        g_cache.gameMode = -1;
        g_cache.is9BallByGameApi = false;
        if (isObjCObj(g_gameManager)) {
            id gmObj = (__bridge id)(void *)g_gameManager;
            @try {
                if (g_off.gm_gameMode >= 0) {
                    g_cache.gameMode = F(int, g_gameManager + g_off.gm_gameMode);
                }
            } @catch (...) {}
            @try {
                SEL is9Sel = sel_registerName("is9BallGame");
                if ([gmObj respondsToSelector:is9Sel]) {
                    g_cache.is9BallByGameApi = ((BOOL(*)(id, SEL))objc_msgSend)(gmObj, is9Sel);
                }
            } @catch (...) {}
        }

        // ============================================================================
        // TỰ ĐỘNG CẬP NHẬT CUE STATS NGAY KHI THAY CƠ (mỗi 1 giây)
        // ============================================================================
        static int s_lastCueId = -2;   // -2 để lần đầu luôn chạy

        int currentCueId = -1;
        @try {
            id ui = getSharedUserInfo();
            if (ui) {
                SEL selCue = sel_registerName("selectedCueId");
                if ([ui respondsToSelector:selCue]) {
                    currentCueId = ((int(*)(id, SEL))objc_msgSend)(ui, selCue);
                }
            }
        } @catch (...) {}

        if (currentCueId != s_lastCueId) {
            cacheCueStats();                    // ← CẬP NHẬT NGAY
            s_lastCueId = currentCueId;
            NSLog(@"[8BP] Cue changed → ID: %d | MaxPower: %.2f | Spin: %.6f", 
                  currentCueId, g_effectiveMaxPower, CUE_PROPERTIES_SPIN_VALUE);
        }

        // Always try to compute table screen rect when in game
        ptr table = readPtr(g_gameManager, g_off.gm_mTable);

        // Only read ball data when stably in-game (with 3s grace period)
        static double inGameSince = 0;
        if (g_cache.mmStateId != 4) {
            g_cache.ballCount = 0;
            g_cache.tableRectValid = false;
            inGameSince = 0;
        }
        bool ballsReady = false;
        if (g_cache.mmStateId == 4) {
            if (inGameSince == 0) inGameSince = CACurrentMediaTime();
            ballsReady = (CACurrentMediaTime() - inGameSince >= 3.0);
        }
        if (!ballsReady) g_cache.ballCount = 0;

        // Ball pointers via ObjC NSArray access
        if (ballsReady && isValidPtr(table) && g_off.tbl_mBalls >= 0) {
            ptr ballsArr = F(ptr, table + g_off.tbl_mBalls);
            if (isObjCObj(ballsArr)) {
                id arr = (__bridge id)(void*)ballsArr;
                SEL countSel = sel_registerName("count");
                if ([arr respondsToSelector:countSel]) {
                    int cnt = (int)((NSUInteger(*)(id, SEL))objc_msgSend)(arr, countSel);
                    if (cnt > MAX_BALLS_COUNT) cnt = MAX_BALLS_COUNT;
                    g_cache.ballCount = cnt;
                    SEL objSel = sel_registerName("objectAtIndex:");
                    for (int i = 0; i < cnt; i++) {
                        g_cache.ballPtrs[i] = (ptr)((id(*)(id, SEL, NSUInteger))objc_msgSend)(arr, objSel, (NSUInteger)i);
                    }
                } else { g_cache.ballCount = 0; }
            } else { g_cache.ballCount = 0; }
        } else { g_cache.ballCount = 0; }

        // Ensure screen size is available (may not be set yet by render thread)
        if (g_screenW < 1 || g_screenH < 1) {
            CGRect scrBounds = [[UIScreen mainScreen] bounds];
            if (scrBounds.size.width > scrBounds.size.height) {
                g_screenW = scrBounds.size.width;
                g_screenH = scrBounds.size.height;
            } else {
                g_screenW = scrBounds.size.height;
                g_screenH = scrBounds.size.width;
            }
        }

        // Compute table screen rect via ObjC (main thread safe)
        g_cache.tableRectValid = false;
        if (isValidPtr(table)) {
            @try {
                // Resolve CCDirector if needed
                if (!isValidPtr(g_ccDirector)) {
                    Class dirClass = objc_getClass("CCDirectorMiniclip");
                    if (!dirClass) dirClass = objc_getClass("CCDirectorMC");
                    if (!dirClass) dirClass = objc_getClass("CCDirector");
                    if (dirClass) {
                        id dir = ((id(*)(id, SEL))objc_msgSend)((id)dirClass, sel_registerName("sharedDirector"));
                        if (dir) g_ccDirector = (ptr)dir;
                    }
                }
                if (isObjCObj(g_ccDirector) && g_screenW > 1 && g_screenH > 1) {
                    ptr glView = (g_off.dir_view >= 0) ? F(ptr, g_ccDirector + g_off.dir_view) : 0;
                    double pbWidth = 0, pbHeight = 0;
                    bool hasPointBound = false;
                    if (isValidPtr(glView) && g_off.glv_pointBound >= 0) {
                        pbWidth = F(double, glView + g_off.glv_pointBound);
                        pbHeight = F(double, glView + g_off.glv_pointBound + 8);
                        if (pbWidth >= 1.0 && pbHeight >= 1.0) {
                            hasPointBound = true;
                        }
                    }

                    double sl = 0, sr = 0, st = 0, sb = 0;
                    if (!hasPointBound || g_screenW < 1 || g_screenH < 1) {
                        double v7 = (double)g_screenH / 640.0;
                        double v8 = ((double)g_screenW + v7 * -1280.0) * 0.5;
                        sl = v8 + v7 * 259.0;
                        sr = v8 + v7 * 1020.0;
                        st = v7 * 196.0;
                        sb = v7 * 559.0;
                    } else {
                        double v19 = (double)g_screenH / pbHeight;
                        double v20 = ((double)g_screenW - pbWidth * v19) * 0.5;
                        double v21 = pbWidth / 1280.0;
                        double v22 = pbHeight / 640.0;
                        sl = v20 + v19 * 259.0 * v21;
                        sr = v20 + v19 * 1020.0 * v21;
                        st = v22 * (v19 * 196.0) - 5.0;
                        sb = v22 * (v19 * 559.0) - 5.0;
                    }

                    if (sr - sl > 10 && sb - st > 10) {
                        g_cache.tableScrLeft   = sl;
                        g_cache.tableScrRight  = sr;
                        g_cache.tableScrTop    = st;
                        g_cache.tableScrBottom = sb;
                        g_cache.tableRectValid = true;
                    }
                }
            } @catch (...) {}
        }

    } @catch (...) {
        g_cache.gmStateId = -1;
        g_cache.mmStateId = -1;
        g_cache.ballCount = 0;
    }
    } // @autoreleasepool
}

static ptr GM_Table()        { return readPtr(g_gameManager, g_off.gm_mTable); }
static ptr GM_VisualCue()    { return readPtr(g_gameManager, g_off.gm_mVisualCue); }
static ptr GM_VEnglish()     { return readPtr(g_gameManager, g_off.gm_mVisualEnglishControl); }
static ptr GM_StateManager() { return readPtr(g_gameManager, g_off.gm_mStateManager); }
static ptr GM_Rules()        { return readPtr(g_gameManager, g_off.gm_rules); }

static int32_t getMMStateId() { return g_cache.mmStateId; }
static int32_t getGMStateId() { return g_cache.gmStateId; }
static bool isPlayerTurn() { return g_cache.gmStateId == 4; }
static bool isInGame() {
    return g_cache.mmStateId == 4 && isValidPtr(g_gameManager) && isValidPtr(GM_Table());
}

static int getTableBallCount(ptr table);
static ptr getTableBall(ptr table, int index);
static BallEnums::Classification getPlayerClassification();
static int getPocketNominationMode();

enum KnownMode {
    MODE_LOBBY = -1,
    MODE_9_OR_8 = 5,
    MODE_PASS_N_PLAY = 7,
    MODE_QUICK_FIRE = 13,
    MODE_LUCKY_SHOT = 16,
    MODE_CREATOR = 19,
};

static int getRuntimeGameMode() {
    return g_cache.gameMode;
}

static int countObjectBallsOnTable() {
    int cnt = 0;
    ptr table = GM_Table();
    int total = getTableBallCount(table);
    for (int i = 1; i < total; i++) {
        ptr b = getTableBall(table, i);
        if (!isValidPtr(b)) continue;
        if (g_off.ball_state < 0) continue;
        BallEnums::State st = F(BallEnums::State, b + g_off.ball_state);
        if (st == BallEnums::DEFAULT || st == BallEnums::IN_POCKET) cnt++;
    }
    return cnt;
}

static bool inferNineBallByBallCount() {
    int total = getTableBallCount(GM_Table());
    if (total == 10) return true;   // cue + 1..9
    if (total >= 15) return false;  // cue + 15 object balls
    int objCnt = countObjectBallsOnTable();
    return objCnt > 0 && objCnt <= 9;
}

static bool isNineBallRuntime() {
    if (g_cache.is9BallByGameApi) return true;
    BallEnums::Classification cls = getPlayerClassification();
    if (cls == BallEnums::NINE_BALL_RULE) return true;
    if (cls == BallEnums::SOLID || cls == BallEnums::STRIPE || cls == BallEnums::EIGHT_BALL) return false;
    return inferNineBallByBallCount();
}

static bool isEightBallRuntime() {
    return !isNineBallRuntime();
}

static bool isCallPocketEveryShotMode() {
    int m = getPocketNominationMode();
    return (m == 2);
}

static const char *gameModeName(int mode) {
    switch (mode) {
        case MODE_LOBBY: return "Lobby";
        case MODE_CREATOR: return "Creator";
        case MODE_PASS_N_PLAY: return "Pass n Play";
        case MODE_QUICK_FIRE: return "Quick Fire";
        case MODE_LUCKY_SHOT: return "Lucky Shot";
        case MODE_9_OR_8: return "8/9 Ball Match";
        default: return "Unknown";
    }
}

#define OFF_VG_AIMANGLE 0x28

static double getAimAngle() {
    static double s_lastValidAngle = 0.0;
    constexpr double kDefaultAngle = 0.0;
    auto truncateTo4Places = [](double value) -> double {
        return static_cast<int>(value * 10000.0) / 10000.0;
    };
    ptr vc = GM_VisualCue();
    if (!isValidPtr(vc) || g_off.vc_mVisualGuide < 0) return kDefaultAngle;
    ptr vg = readPtr(vc, g_off.vc_mVisualGuide);
    if (!isValidPtr(vg)) return s_lastValidAngle;
    double angle = F(double, vg + OFF_VG_AIMANGLE);
    if (angle < 0.0 || angle > maxAngle) return kDefaultAngle;
    s_lastValidAngle = truncateTo4Places(angle);
    return s_lastValidAngle;
}

static void setAimAngle(double angle) {
    ptr vc = GM_VisualCue();
    if (!isValidPtr(vc) || g_off.vc_mVisualGuide < 0) return;
    ptr vg = readPtr(vc, g_off.vc_mVisualGuide);
    if (!isValidPtr(vg)) return;
    F(double, vg + OFF_VG_AIMANGLE) = angle;
}

static double getShotPower() {
    @try {
        ptr vc = GM_VisualCue();
        if (!isValidPtr(vc) || g_off.vc_mPower < 0) return 0;
        double power = F(double, vc + g_off.vc_mPower);
        if (power <= 0.0 || power > 1.0) power = 1.0;
        return (1.0 - sqrt(fmax(0.0, 1.0 - power))) * g_effectiveMaxPower;
    } @catch (...) {}
    return 0;
}

static double cueSpinFromUserInfoStats(int *outCueId, bool *outHasStats);

static Vec2d getShotSpin() {
    // Same as poolAndr MemoryManager::VisualCue::getShotSpin: raw english * CueProperties::getCueSpin()
    auto truncateTo4Places = [](double value) -> double {
        return static_cast<int>(value * 10000.0) / 10000.0;
    };
    @try {
        ptr vec = GM_VEnglish();
        if (!isValidPtr(vec) || g_off.vec_mEnglish < 0) return {};
        Vec2d spin = F(Vec2d, vec + g_off.vec_mEnglish);
        if (spin.x == 0.0 && spin.y == 0.0) return spin;
        double cueSpin = g_cueStatsCached ? CUE_PROPERTIES_SPIN_VALUE
                                          : cueSpinFromUserInfoStats(nullptr, nullptr);
        return { truncateTo4Places(spin.x) * cueSpin, truncateTo4Places(spin.y) * cueSpin };
    } @catch (...) {}
    return {};
}


static double ShotPowerToPower(double shotPower) {
    double cueMax = g_effectiveMaxPower;
    if (cueMax <= 0.0) cueMax = 888.85;
    if (shotPower < 0.0) shotPower = 0.0;
    if (shotPower > cueMax) shotPower = cueMax;

    double ratio = 1.0 - (shotPower / cueMax);
    if (ratio < 0.0) ratio = 0.0;
    double power = 1.0 - ratio * ratio;
    if (power < 0.0) power = 0.0;
    if (power > 1.0) power = 1.0;
    return power;
}

static BallEnums::Classification getPlayerClassification() {
    // Read from Ruleset memory (same as Android: rules + 0xC8 = classification vector)
    ptr rules = GM_Rules();
    if (!isValidPtr(rules)) return BallEnums::ANY;
    ptr classVec = F(ptr, rules + 0xC8);
    if (!isValidPtr(classVec)) return BallEnums::ANY;
    bool myTurn = (g_cache.gmStateId == 4);
    return F(BallEnums::Classification, classVec + (myTurn ? 0 : 4));
}

static int getPocketNominationMode() {
    ptr rules = GM_Rules();
    if (!isValidPtr(rules)) return 0;
    return F(int, rules + 0x68);
}
static uint getNominatedPocket() {
    ptr rules = GM_Rules();
    if (!isValidPtr(rules)) return 6;
    return F(uint, rules + 0x118);
}
static void nominatePocket(int pocket) {
    ptr rules = GM_Rules();
    if (!isValidPtr(rules)) return;
    F(uint, rules + 0x118) = pocket;
}

typedef struct {
    unsigned int _power;
    unsigned int _spin;
    unsigned int _aim;
    unsigned int _time;
} CueStats;

static int g_cachedCueId = -1;

static double cueMaxPowerFromUserInfoStats(int *outCueId, bool *outHasStats) {
    double fallback = 888.85;
    static const double kCuePowerTable[] = {
        666.0, 682.0, 699.0, 715.0, 731.0, 747.0, 764.0,
        780.0, 796.0, 813.0, 828.85, 848.85, 868.85, 888.85
    };
    static const int kCuePowerCount = (int)(sizeof(kCuePowerTable) / sizeof(kCuePowerTable[0]));

    if (outCueId) *outCueId = -1;
    if (outHasStats) *outHasStats = false;

    Class userInfoClass = NSClassFromString(@"UserInfo");
    if (!userInfoClass) return fallback;
    id ui = nil;
    @try {
        SEL selSharedUserInfo = sel_registerName("sharedUserInfo");
        if ([userInfoClass respondsToSelector:selSharedUserInfo]) {
            ui = ((id(*)(id, SEL))objc_msgSend)(userInfoClass, selSharedUserInfo);
        }
    } @catch (...) {}
    if (!ui) return fallback;

    int cueId = 0;
    @try {
        SEL selSelectedCueId = sel_registerName("selectedCueId");
        if ([ui respondsToSelector:selSelectedCueId]) {
            cueId = ((int(*)(id, SEL))objc_msgSend)(ui, selSelectedCueId);
        }
    } @catch (...) {}
    if (outCueId) *outCueId = cueId;

    CueStats stats = {0, 0, 0, 0};
    bool hasStats = false;
    // Prefer getCueStatsWithBonus: (includes collection/VIP bonuses) over getCueStats: (base only)
    @try {
        SEL selGetCueStatsBonus = sel_registerName("getCueStatsWithBonus:");
        if ([ui respondsToSelector:selGetCueStatsBonus]) {
            stats = ((CueStats(*)(id, SEL, int))objc_msgSend)(ui, selGetCueStatsBonus, cueId);
            hasStats = true;
        }
    } @catch (...) {}
    if (!hasStats) {
        @try {
            SEL selGetCueStats = sel_registerName("getCueStats:");
            if ([ui respondsToSelector:selGetCueStats]) {
                stats = ((CueStats(*)(id, SEL, int))objc_msgSend)(ui, selGetCueStats, cueId);
                hasStats = true;
            }
        } @catch (...) {}
    }
    if (outHasStats) *outHasStats = hasStats;
    if (!hasStats) return fallback;

    int idx = (int)stats._power;
    if (idx < 0) idx = 0;
    if (idx >= kCuePowerCount) idx = kCuePowerCount - 1;
    return kCuePowerTable[idx];
}


static double cueSpinFromUserInfoStats(int *outCueId, bool *outHasStats) {
    double fallback = 0.96551722494; // Default spin value
    
    // Spin lookup table (điều chỉnh theo game data)
    static const double kCueSpinTable[] = {
        0.37931035305,
        0.48275861247,
        0.58620691299,
        0.68965517241,
        0.71724135300,
        0.74482761580,
        0.77241379639,
        0.79999997698,
        0.82758623978,
        0.85517242037,
        0.88275860096,
        0.91034478155,
        0.93793104435,
        0.96551722494
    };
    static const int kCueSpinCount = (int)(sizeof(kCueSpinTable) / sizeof(kCueSpinTable[0]));

    if (outCueId) *outCueId = -1;
    if (outHasStats) *outHasStats = false;

    Class userInfoClass = NSClassFromString(@"UserInfo");
    if (!userInfoClass) return fallback;
    
    id ui = nil;
    @try {
        SEL selSharedUserInfo = sel_registerName("sharedUserInfo");
        if ([userInfoClass respondsToSelector:selSharedUserInfo]) {
            ui = ((id(*)(id, SEL))objc_msgSend)(userInfoClass, selSharedUserInfo);
        }
    } @catch (...) {}
    if (!ui) return fallback;

    int cueId = 0;
    @try {
        SEL selSelectedCueId = sel_registerName("selectedCueId");
        if ([ui respondsToSelector:selSelectedCueId]) {
            cueId = ((int(*)(id, SEL))objc_msgSend)(ui, selSelectedCueId);
        }
    } @catch (...) {}
    if (outCueId) *outCueId = cueId;

    CueStats stats = {0, 0, 0, 0};
    bool hasStats = false;
    // Prefer getCueStatsWithBonus: (includes collection/VIP bonuses)
    @try {
        SEL selGetCueStatsBonus = sel_registerName("getCueStatsWithBonus:");
        if ([ui respondsToSelector:selGetCueStatsBonus]) {
            stats = ((CueStats(*)(id, SEL, int))objc_msgSend)(ui, selGetCueStatsBonus, cueId);
            hasStats = true;
        }
    } @catch (...) {}
    if (!hasStats) {
        @try {
            SEL selGetCueStats = sel_registerName("getCueStats:");
            if ([ui respondsToSelector:selGetCueStats]) {
                stats = ((CueStats(*)(id, SEL, int))objc_msgSend)(ui, selGetCueStats, cueId);
                hasStats = true;
            }
        } @catch (...) {}
    }
    if (outHasStats) *outHasStats = hasStats;
    if (!hasStats) return fallback;

    int idx = (int)stats._spin;
    if (idx < 0) idx = 0;
    if (idx >= kCueSpinCount) idx = kCueSpinCount - 1;
    return kCueSpinTable[idx];
}
// ══════════════════════════════════════════════════════════════════════════════
// FULL cue power/spin diagnostics — reads EVERY possible source at runtime
// ══════════════════════════════════════════════════════════════════════════════
static int g_powerSource = 0;  // 0=fallback, 1=runtime interp, 2=table lookup
static int g_spinSource = 0;

static struct CueDiag {
    // ── UserInfo server-sent ratios ──
    int lowPowerRatio = 0, highPowerRatio = 0;
    int lowSpinRatio = 0,  highSpinRatio = 0;
    int lowAimRatio = 0,   highAimRatio = 0;
    // ── CueStats (base) ──
    int basePower = -1, baseSpin = -1, baseAim = -1, baseTime = -1;
    // ── CueStatsWithBonus (includes collection/VIP bonuses) ──
    int bonusPower = -1, bonusSpin = -1, bonusAim = -1, bonusTime = -1;
    // ── VisualCue runtime ivars ──
    double vc_mPower = 0.0;               // 0-1 slider position
    double vc_mCurrentPowerOffset = 0.0;   // current offset during pullback
    double vc_mMaxCuePowerOffset = 0.0;    // max pullback offset (always ~100)
    bool   vc_mPowerMode = false;
    bool   vc_powerControlActive = false;
    bool   vc_mPowerSliderVisible = false;
    int    vc_mControlMode = 0;
    double vc_mMaxAngleForPowerMode = 0.0;
    double vc_mDistanceFromTip = 0.0;
    double vc_mAnimatePowerTarget = 0.0;
    double vc_mUserSensitivity = 0.0;
    // ── VisualEnglishControl runtime ivars ──
    double vec_englishX = 0.0, vec_englishY = 0.0;
    uint   vec_spinLevel = 0;
    double vec_mInputRadius = 0.0;
    // ── GamePowerBarView (ObjC methods) ──
    float  pbv_minimumPower = 0.0f;
    float  pbv_sliderDistance = 0.0f;
    // ── Computed values (what ESP actually uses for physics) ──
    double computedShotPower = 0.0;  // getShotPower() result
    double computedSpinX = 0.0, computedSpinY = 0.0;  // getShotSpin() result
    // ── Table power lookup ──
    double tablePowerLookup = 0.0;  // kCuePowerTable[bonusPower]
    double tableSpinLookup = 0.0;   // kCueSpinTable[bonusSpin]
    // ── Interpolated from ratios ──
    double interpPower = 0.0;
    double interpSpin = 0.0;
} g_diag;

// Helper: get UserInfo singleton
static id getSharedUserInfo() {
    @try {
        Class cls = NSClassFromString(@"UserInfo");
        if (!cls) return nil;
        SEL sel = sel_registerName("sharedUserInfo");
        if ([cls respondsToSelector:sel])
            return ((id(*)(id, SEL))objc_msgSend)(cls, sel);
    } @catch (...) {}
    return nil;
}

// Helper: read int/float/double/bool properties from ObjC object
static int readIntProp(id obj, const char *name) {
    @try {
        SEL sel = sel_registerName(name);
        if ([obj respondsToSelector:sel])
            return ((int(*)(id, SEL))objc_msgSend)(obj, sel);
    } @catch (...) {}
    return 0;
}
static float readFloatProp(id obj, const char *name) {
    @try {
        SEL sel = sel_registerName(name);
        if ([obj respondsToSelector:sel])
            return ((float(*)(id, SEL))objc_msgSend)(obj, sel);
    } @catch (...) {}
    return 0.0f;
}

// Helper: safe read double from memory offset
static double safeReadDouble(ptr base, ptrdiff_t off) {
    if (!isValidPtr(base) || off < 0) return 0.0;
    @try { return F(double, base + off); } @catch (...) { return 0.0; }
}
static bool safeReadBool(ptr base, ptrdiff_t off) {
    if (!isValidPtr(base) || off < 0) return false;
    @try { return F(bool, base + off); } @catch (...) { return false; }
}
static int safeReadInt(ptr base, ptrdiff_t off) {
    if (!isValidPtr(base) || off < 0) return 0;
    @try { return F(int, base + off); } @catch (...) { return 0; }
}
static uint safeReadUint(ptr base, ptrdiff_t off) {
    if (!isValidPtr(base) || off < 0) return 0;
    @try { return F(uint, base + off); } @catch (...) { return 0; }
}

// Read cue stats live — reads EVERY power/spin source at runtime.
// Fallback chain: runtime interpolation → preset table lookup → hardcoded default.
static void cacheCueStats() {
    int cueId = -1;
    bool hasStats = false;
    g_powerSource = 0;
    g_spinSource = 0;
    memset(&g_diag, 0, sizeof(g_diag));
    g_diag.basePower = g_diag.baseSpin = g_diag.baseAim = g_diag.baseTime = -1;
    g_diag.bonusPower = g_diag.bonusSpin = g_diag.bonusAim = g_diag.bonusTime = -1;

    // ═══ 1. UserInfo: ratios + CueStats ═══
    id ui = getSharedUserInfo();
    if (ui) {
        cueId = readIntProp(ui, "selectedCueId");

        // Server-sent ratio bounds
        g_diag.lowPowerRatio  = readIntProp(ui, "lowPowerRatio");
        g_diag.highPowerRatio = readIntProp(ui, "highPowerRatio");
        g_diag.lowSpinRatio   = readIntProp(ui, "lowSpinRatio");
        g_diag.highSpinRatio  = readIntProp(ui, "highSpinRatio");
        g_diag.lowAimRatio    = readIntProp(ui, "lowAimRatio");
        g_diag.highAimRatio   = readIntProp(ui, "highAimRatio");

        // CueStatsWithBonus (includes collection/VIP bonuses)
        @try {
            SEL sel = sel_registerName("getCueStatsWithBonus:");
            if ([ui respondsToSelector:sel]) {
                CueStats s = ((CueStats(*)(id, SEL, int))objc_msgSend)(ui, sel, cueId);
                g_diag.bonusPower = (int)s._power; g_diag.bonusSpin = (int)s._spin;
                g_diag.bonusAim = (int)s._aim;     g_diag.bonusTime = (int)s._time;
                hasStats = true;
            }
        } @catch (...) {}
        // CueStats (base, no bonuses)
        @try {
            SEL sel = sel_registerName("getCueStats:");
            if ([ui respondsToSelector:sel]) {
                CueStats s = ((CueStats(*)(id, SEL, int))objc_msgSend)(ui, sel, cueId);
                g_diag.basePower = (int)s._power; g_diag.baseSpin = (int)s._spin;
                g_diag.baseAim = (int)s._aim;     g_diag.baseTime = (int)s._time;
                if (!hasStats) {
                    g_diag.bonusPower = g_diag.basePower; g_diag.bonusSpin = g_diag.baseSpin;
                    g_diag.bonusAim = g_diag.baseAim;     g_diag.bonusTime = g_diag.baseTime;
                    hasStats = true;
                }
            }
        } @catch (...) {}
    }

    // ═══ 2. VisualCue: ALL power ivars ═══
    ptr vc = GM_VisualCue();
    if (isValidPtr(vc)) {
        g_diag.vc_mPower               = safeReadDouble(vc, g_off.vc_mPower);
        g_diag.vc_mCurrentPowerOffset  = safeReadDouble(vc, g_off.vc_mCurrentPowerOffset);
        g_diag.vc_mMaxCuePowerOffset   = safeReadDouble(vc, g_off.vc_mMaxCuePowerOffset);
        g_diag.vc_mPowerMode           = safeReadBool(vc, g_off.vc_mPowerMode);
        g_diag.vc_powerControlActive   = safeReadBool(vc, g_off.vc_powerControlActive);
        g_diag.vc_mPowerSliderVisible  = safeReadBool(vc, g_off.vc_mPowerSliderVisible);
        g_diag.vc_mControlMode         = safeReadInt(vc, g_off.vc_mControlMode);
        g_diag.vc_mMaxAngleForPowerMode= safeReadDouble(vc, g_off.vc_mMaxAngleForPowerMode);
        g_diag.vc_mDistanceFromTip     = safeReadDouble(vc, g_off.vc_mDistanceFromTip);
        g_diag.vc_mAnimatePowerTarget  = safeReadDouble(vc, g_off.vc_mAnimatePowerTarget);
        g_diag.vc_mUserSensitivity     = safeReadDouble(vc, g_off.vc_mUserSensitivity);

        // GamePowerBarView (ObjC child)
        if (g_off.vc_powerBarView >= 0) {
            @try {
                id pbv = (__bridge id)(*(void **)(vc + g_off.vc_powerBarView));
                if (pbv) {
                    g_diag.pbv_minimumPower   = readFloatProp(pbv, "getMinimumPower");
                    g_diag.pbv_sliderDistance  = readFloatProp(pbv, "getPowerSliderDistance");
                }
            } @catch (...) {}
        }
    }

    // ═══ 3. VisualEnglishControl: ALL spin ivars ═══
    ptr vec = GM_VEnglish();
    if (isValidPtr(vec)) {
        if (g_off.vec_mEnglish >= 0) {
            @try {
                Vec2d eng = F(Vec2d, vec + g_off.vec_mEnglish);
                g_diag.vec_englishX = eng.x;
                g_diag.vec_englishY = eng.y;
            } @catch (...) {}
        }
        g_diag.vec_spinLevel   = safeReadUint(vec, g_off.vec_spinLevel);
        g_diag.vec_mInputRadius = safeReadDouble(vec, g_off.vec_mInputRadius);
    }

    // ═══ 4. Table lookups (preset tables — for comparison) ═══
    {
        static const double kPT[] = {666.0,682.0,699.0,715.0,731.0,747.0,764.0,780.0,796.0,813.0,828.85,848.85,868.85,888.85};
        static const double kST[] = {0.37931035305,0.48275861247,0.58620691299,0.68965517241,0.71724135300,0.74482761580,0.77241379639,0.79999997698,0.82758623978,0.85517242037,0.88275860096,0.91034478155,0.93793104435,0.96551722494};
        int pIdx = (g_diag.bonusPower >= 0) ? g_diag.bonusPower : 0;
        if (pIdx > 13) pIdx = 13; if (pIdx < 0) pIdx = 0;
        g_diag.tablePowerLookup = kPT[pIdx];
        int sIdx = (g_diag.bonusSpin >= 0) ? g_diag.bonusSpin : 0;
        if (sIdx > 13) sIdx = 13; if (sIdx < 0) sIdx = 0;
        g_diag.tableSpinLookup = kST[sIdx];
    }

    // ═══ 5. Interpolated from ratios ═══
    static const int kMaxStatLevel = 13;
    if (hasStats && g_diag.lowPowerRatio > 0 && g_diag.highPowerRatio > 0 && g_diag.highPowerRatio >= g_diag.lowPowerRatio) {
        int pLvl = g_diag.bonusPower; if (pLvl < 0) pLvl = 0;
        g_diag.interpPower = g_diag.lowPowerRatio + (double)(g_diag.highPowerRatio - g_diag.lowPowerRatio) * pLvl / kMaxStatLevel;
    }
    if (hasStats && g_diag.highSpinRatio > 0 && g_diag.highSpinRatio >= g_diag.lowSpinRatio) {
        int sLvl = g_diag.bonusSpin; if (sLvl < 0) sLvl = 0;
        g_diag.interpSpin = g_diag.lowSpinRatio + (double)(g_diag.highSpinRatio - g_diag.lowSpinRatio) * sLvl / kMaxStatLevel;
    }

    // ═══ 6. Decide effective power & spin ═══
    // Power priority: interp → table → fallback
    if (g_diag.interpPower > 0.0) {
        g_effectiveMaxPower = g_diag.interpPower;
        g_powerSource = 1;
    } else if (g_diag.tablePowerLookup > 0.0) {
        g_effectiveMaxPower = g_diag.tablePowerLookup;
        g_powerSource = 2;
    } else {
        g_effectiveMaxPower = 888.85;
        g_powerSource = 0;
    }

    // Spin priority: interp → table → fallback
    if (g_diag.interpSpin > 0.0) {
        // Auto-detect scale
        double raw = g_diag.interpSpin;
        double spinMul = (raw > 100.0) ? raw / 1000.0 : (raw > 1.0) ? raw / 100.0 : raw;
        if (spinMul > 0.0 && spinMul <= 2.0) {
            CUE_PROPERTIES_SPIN_VALUE = spinMul;
            g_spinSource = 1;
        }
    }
    if (g_spinSource != 1) {
        if (g_diag.tableSpinLookup > 0.0) {
            CUE_PROPERTIES_SPIN_VALUE = g_diag.tableSpinLookup;
            g_spinSource = 2;
        } else {
            CUE_PROPERTIES_SPIN_VALUE = 0.96551722494;
            g_spinSource = 0;
        }
    }

    // ═══ 7. Computed shot values (what Prediction::initCueBall receives) ═══
    @try { g_diag.computedShotPower = getShotPower(); } @catch (...) {}
    @try {
        Vec2d sp = getShotSpin();
        g_diag.computedSpinX = sp.x;
        g_diag.computedSpinY = sp.y;
    } @catch (...) {}

    g_cueStatsCached = hasStats;
    g_cachedCueId = cueId;
}

static void setPower(double shotPower) {
    ptr vc = GM_VisualCue();
    if (!isValidPtr(vc) || g_off.vc_mPower < 0) return;
    F(double, vc + g_off.vc_mPower) = ShotPowerToPower(shotPower);
    // Zero mCurrentPowerOffset so force-stat swing bonus doesn't add unexpected power
    if (g_off.vc_mCurrentPowerOffset >= 0)
        F(double, vc + g_off.vc_mCurrentPowerOffset) = 0.0;
}

// Fire shot directly via MCMenuStateManager — equivalent to Android's M(void, libmain+0x2dc0c58, void*)
// On iOS we call [MCMenuStateManager shotTaken] or similar selector via objc_msgSend
static bool fireShot() {
    if (g_off.gm_menuStateManager < 0) return false;
    ptr menuSM = readPtr(g_gameManager, g_off.gm_menuStateManager);
    if (!isObjCObj(menuSM)) return false;
    id sm = (__bridge id)(void*)menuSM;
    // Try known selectors for triggering a shot
    static SEL selShotTaken = sel_registerName("shotTaken");
    static SEL selShoot = sel_registerName("shoot");
    static SEL selFireShot = sel_registerName("fireShot");
    static SEL selOnShot = sel_registerName("onShotTaken");
    if ([sm respondsToSelector:selShotTaken]) {
        ((void(*)(id, SEL))objc_msgSend)(sm, selShotTaken);
        return true;
    }
    if ([sm respondsToSelector:selShoot]) {
        ((void(*)(id, SEL))objc_msgSend)(sm, selShoot);
        return true;
    }
    if ([sm respondsToSelector:selFireShot]) {
        ((void(*)(id, SEL))objc_msgSend)(sm, selFireShot);
        return true;
    }
    if ([sm respondsToSelector:selOnShot]) {
        ((void(*)(id, SEL))objc_msgSend)(sm, selOnShot);
        return true;
    }
    return false;
}

// Ball accessor helpers — _physicsProperties struct + internal offsets
static Vec2d ballPosition(ptr ball) {
    if (g_off.ball_physicsProperties < 0) return {};
    return F(Vec2d, ball + g_off.ball_physicsProperties + PHYS_OFF_POSITION);
}
static Vec2d ballVelocity(ptr ball) {
    if (g_off.ball_physicsProperties < 0) return {};
    return F(Vec2d, ball + g_off.ball_physicsProperties + PHYS_OFF_VELOCITY);
}
static Vec3d ballSpin(ptr ball) {
    if (g_off.ball_physicsProperties < 0) return {};
    return F(Vec3d, ball + g_off.ball_physicsProperties + PHYS_OFF_SPIN);
}
static BallEnums::Classification ballClassification(ptr ball) {
    if (g_off.ball_classification < 0) return BallEnums::ERR_CLASSIFICATION;
    return F(BallEnums::Classification, ball + g_off.ball_classification);
}
static BallEnums::State ballState(ptr ball) {
    if (g_off.ball_state < 0) return BallEnums::ERR_STATE;
    return F(BallEnums::State, ball + g_off.ball_state);
}
static bool ballIsOnTable(ptr ball) {
    auto s = ballState(ball);
    return s == BallEnums::DEFAULT || s == BallEnums::IN_POCKET;
}

// Table balls from cache (ball pointers cached on main thread)
static int getTableBallCount(ptr /*table*/) { return g_cache.ballCount; }
static ptr getTableBall(ptr /*table*/, int index) {
    if (index < 0 || index >= g_cache.ballCount) return 0;
    return g_cache.ballPtrs[index];
}
static Vec2d* getTablePockets(ptr table) {
    if (!isValidPtr(table) || g_off.tbl_mTableProperties < 0) return nullptr;
    ptr tp = F(ptr, table + g_off.tbl_mTableProperties);
    if (!isValidPtr(tp) || g_off.tp_mPockets < 0) return nullptr;
    // mPockets is a POINTER to Vec2d[6] array — dereference it
    ptr pocketsPtr = F(ptr, tp + g_off.tp_mPockets);
    if (!isValidPtr(pocketsPtr)) return nullptr;
    return (Vec2d*)pocketsPtr;
}
constexpr double PI = 3.14159265358979;
constexpr double PI_0_5 = PI / 2.0;
constexpr double PI_1_5 = PI * 1.5;
namespace NumberUtils {
    inline double normalizeDoublePrecision(double value, double negativeThreshold = 0.0, double negativeExtraLen = 0.0, size_t maxLen = 7) {
        if (std::abs(value) >= 10000.0) return std::floor(value);
        char buffer[256];
        std::snprintf(buffer, sizeof(buffer), "%lf", value);
        size_t strLen = std::strlen(buffer);
        size_t allowedLen = maxLen;
        if (value < negativeThreshold) allowedLen = maxLen + (size_t)negativeExtraLen;
        if (strLen > allowedLen) buffer[allowedLen] = '\0';
        double result = 0.0;
        std::sscanf(buffer, "%lf", &result);
        return result;
    }

    inline double calcAngle(const Vec2d &delta) {
        double angle;
        if (delta.x == 0.0) {
            angle = PI_1_5;
            if (delta.y >= 0.0)
                angle = PI_0_5;
        }
        else {
            angle = atan(delta.y / delta.x);
            if (delta.x < 0.0)
                angle += PI;
        }
        return angle;
    }

    inline double calcAngle(Vec2d source, Vec2d dest) { return calcAngle(source - dest); }
}


static float g_tableOffsetX = 0.0f;
static float g_tableOffsetY = 0.0f;
static float g_tableScaleFactor = 1.0f;

static double SCREEN_TABLE_LEFT = 0.0;
static double SCREEN_TABLE_TOP = 0.0;
static double SCREEN_TABLE_RIGHT = 0.0;
static double SCREEN_TABLE_BOTTOM = 0.0;
static double SCREEN_TABLE_SCALE = 1.0;

static bool g_autoScaleOK = false;

static ImVec2 WorldToScreen(Vec2d worldPos) {
    double scrX = SCREEN_TABLE_LEFT + (worldPos.x + 127.0) * SCREEN_TABLE_SCALE;
    double scrY = SCREEN_TABLE_BOTTOM - (worldPos.y + 63.5) * SCREEN_TABLE_SCALE;
    return ImVec2((float)scrX, (float)scrY);
}

static void UpdateScreenTable() {
    bool autoOK = false;
    double baseLeft = 0, baseRight = 0, baseTop = 0, baseBottom = 0;
    if (g_cache.tableRectValid) {
        baseLeft   = g_cache.tableScrLeft;
        baseRight  = g_cache.tableScrRight;
        baseTop    = g_cache.tableScrTop;
        baseBottom = g_cache.tableScrBottom;
        autoOK = true;
    }

    g_autoScaleOK = autoOK;

    if (!autoOK) {
        double v7 = (double)g_screenH / 640.0;
        double v8 = ((double)g_screenW + v7 * -1280.0) * 0.5;
        baseLeft   = v8 + v7 * 259.0;
        baseRight  = v8 + v7 * 1020.0;
        baseTop    = v7 * 196.0;
        baseBottom = v7 * 559.0;
    }

    double cx = (baseLeft + baseRight) * 0.5;
    double cy = (baseTop + baseBottom) * 0.5;
    double hw = (baseRight - baseLeft) * 0.5;
    double hh = (baseBottom - baseTop) * 0.5;

    hw *= (double)g_tableScaleFactor;
    hh *= (double)g_tableScaleFactor;

    cx += (double)g_tableOffsetX;
    cy += (double)g_tableOffsetY;

    SCREEN_TABLE_LEFT   = cx - hw;
    SCREEN_TABLE_RIGHT  = cx + hw;
    SCREEN_TABLE_TOP    = cy - hh;
    SCREEN_TABLE_BOTTOM = cy + hh;
    SCREEN_TABLE_SCALE  = (SCREEN_TABLE_RIGHT - SCREEN_TABLE_LEFT) / 254.0;
}


static Vec4d table_bounds;
static bool fastCalc = true;

struct Prediction {
    static bool pocketStatus[TABLE_POCKETS_COUNT];

    struct Ball {
        int index;
        BallEnums::Classification classification;
        BallEnums::State state;
        bool originalOnTable;
        bool onTable;
        int pocketIndex = -1;
        Point2D velocity;
        Vec3d spin;
        Point2D initialPosition;
        Point2D predictedPosition;
        std::vector<Point2D> positions;

        void findNextCollision(void *pData, double *time);
        void calcVelocity();
        void calcVelocityPostCollision(const double &angle);
        void move(const double &time);
        bool isMovingOrSpinning() const;
        bool isBallBallCollision(double *smallestTime, Prediction::Ball &other) const;
        bool willCollideWithTable(const double *smallestTime) const;
        void determineBallTableCollision(void *pData, double *smallestTime);
        bool isBallLineCollision(double *pTime, const Point2D &a, const Point2D &b) const;
        bool isBallPointCollision(double *smallestTime, const Point2D &pt) const;

        Ball() : index(0), classification(BallEnums::ERR_CLASSIFICATION),
                 state(BallEnums::ERR_STATE), originalOnTable(false), onTable(false),
                 velocity(Point2D()), spin(Vec3d()),
                 initialPosition(Point2D()), predictedPosition(Point2D()) {}
    };

    struct Collision {
        bool valid = false;
        enum Type : int { BALL, LINE, POINT } type = POINT;
        double angle = 0.0;
        Point2D point;
        Prediction::Ball *ballA = nullptr;
        Prediction::Ball *ballB = nullptr;
        Prediction::Ball *firstHitBall = nullptr;
    };

    struct SceneData {
        int ballsCount = 0;
        Ball balls[MAX_BALLS_COUNT];
        Collision collision;
        bool shotState = false;
        int railBeforeFirstHitCount = 0;
        int pointBeforeFirstHitCount = 0;
        int railCollisionCount = 0;
        int ballCollisionCount = 0;
        int pointCollisionCount = 0;
        int firstPocketedBallIndex = -1;
        int firstPocketedPocketIndex = -1;
        int pocketedBallsCount = 0;
    } guiData;

    bool firstHitIsTarget = false;
    Candidate m_candidate = {-1};

    bool determineShotResult(bool isAuto, double shotAngle, double shotPower, Vec2d shotSpin = {}, Candidate cand = {-1});
    void initBalls();
    void initCueBall(double shotAngle, double shotPower, const Point2D &shotSpin);
    void determineBallsPositions();
    void handleCollision();
    void handleBallBallCollision() const;
};

static Prediction prediction;
Prediction *gPrediction = &prediction;
bool Prediction::pocketStatus[] = {};

static double prevAngle = 0.0;
static double prevPower = 0.0;
static Point2D prevSpin = {0.0, 0.0};

static void resetPredictionCache() {
    prevAngle = -999.0;
    prevPower = -999.0;
    prevSpin = {-999.0, -999.0};
}

// ── Pocket & Table shape data ──

static std::array<Point2D, TABLE_POCKETS_COUNT>& getPockets() {
    static std::array<Point2D, TABLE_POCKETS_COUNT> POCKET_POSITIONS = {
        Point2D(-130.8, -67.3), Point2D(0, -72), Point2D(130.8, -67.3),
        Point2D(130.8, 67.3),   Point2D(0, 72),  Point2D(-130.8, 67.3)
    };
    return POCKET_POSITIONS;
}

static int g_tableShapeCount = TABLE_SHAPE_SIZE;

static std::array<Point2D, TABLE_SHAPE_SIZE>& getTableShape() {
    static std::array<Point2D, TABLE_SHAPE_SIZE> TABLE_SHAPE = {
        Point2D(-127, 53.5), Point2D(-136.9, 64.1), Point2D(-138.2, 69.2),
        Point2D(-136.7, 73.2), Point2D(-132.7, 74.7), Point2D(-127.6, 73.4),
        Point2D(-117, 63.5), Point2D(-7.8, 63.5), Point2D(-6.1, 68.6),
        Point2D(-5.7, 72.7), Point2D(-3.7, 75.4), Point2D(0, 76.7),
        Point2D(3.7, 75.4), Point2D(5.7, 72.7), Point2D(6.1, 68.6),
        Point2D(7.8, 63.5), Point2D(117, 63.5), Point2D(127.6, 73.4),
        Point2D(132.7, 74.7), Point2D(136.7, 73.2), Point2D(138.2, 69.2),
        Point2D(136.9, 64.1), Point2D(127, 53.5), Point2D(127, -53.5),
        Point2D(136.9, -64.1), Point2D(138.2, -69.2), Point2D(136.7, -73.2),
        Point2D(132.7, -74.7), Point2D(127.6, -73.4), Point2D(117, -63.5),
        Point2D(7.8, -63.5), Point2D(6.1, -68.6), Point2D(5.7, -72.7),
        Point2D(3.7, -75.4), Point2D(0, -76.7), Point2D(-3.7, -75.4),
        Point2D(-5.7, -72.7), Point2D(-6.1, -68.6), Point2D(-7.8, -63.5),
        Point2D(-117, -63.5), Point2D(-127.6, -73.4), Point2D(-132.7, -74.7),
        Point2D(-136.7, -73.2), Point2D(-138.2, -69.2), Point2D(-136.9, -64.1),
        Point2D(-127, -53.5)
    };
    return TABLE_SHAPE;
}

static void refreshRuntimeTableGeometry() {
    ptr table = GM_Table();
    if (!isValidPtr(table)) return;

    // Runtime pockets: copy from TableProperties.mPockets when available.
    Vec2d *pockets = getTablePockets(table);
    if (pockets) {
        auto &dst = getPockets();
        for (int i = 0; i < TABLE_POCKETS_COUNT; i++) {
            dst[i].x = pockets[i].x;
            dst[i].y = pockets[i].y;
        }
    }

    // Runtime table polygon: copy Table.mTableShape vector (backup5 behavior).
    if (g_off.tbl_mTableShape >= 0) {
        ptr vectorPtr = table + g_off.tbl_mTableShape;
        ptr beginPtr = F(ptr, vectorPtr);
        ptr endPtr = F(ptr, vectorPtr + sizeof(ptr));
        if (isValidPtr(beginPtr) && isValidPtr(endPtr) && endPtr > beginPtr) {
            size_t count = (size_t)((endPtr - beginPtr) / sizeof(Vec2d));
            if (count > 0) {
                if (count > TABLE_SHAPE_SIZE) count = TABLE_SHAPE_SIZE;
                auto &shape = getTableShape();
                Vec2d *src = (Vec2d *)beginPtr;
                for (size_t i = 0; i < count; i++) {
                    shape[(int)i].x = src[i].x;
                    shape[(int)i].y = src[i].y;
                }
                g_tableShapeCount = (int)count;
            }
        }
    }
}


bool Prediction::determineShotResult(bool isAuto, double shotAngle, double shotPower, Vec2d shotSpin, Candidate cand) {
    if (shotAngle == prevAngle && shotPower == prevPower && shotSpin == prevSpin) return false;
    prevAngle = shotAngle; prevPower = shotPower; prevSpin = shotSpin;
    fastCalc = isAuto;
    this->m_candidate = cand;
    this->firstHitIsTarget = (cand.idx == -1);
    this->initBalls();
    this->initCueBall(shotAngle, shotPower, shotSpin);
    this->guiData.collision.firstHitBall = nullptr;
    this->guiData.railBeforeFirstHitCount = 0;
    this->guiData.pointBeforeFirstHitCount = 0;
    this->guiData.railCollisionCount = 0;
    this->guiData.ballCollisionCount = 0;
    this->guiData.pointCollisionCount = 0;
    this->guiData.firstPocketedBallIndex = -1;
    this->guiData.firstPocketedPocketIndex = -1;
    this->guiData.pocketedBallsCount = 0;
    for (bool &ps : pocketStatus) ps = false;
    this->determineBallsPositions();
    for (int i = 0; i < this->guiData.ballsCount; i++) {
        Ball &ball = this->guiData.balls[i];
        if (ball.positions.back() != ball.predictedPosition)
            ball.positions.push_back(ball.predictedPosition);
    }
    return true;
}

void Prediction::initBalls() {
    ptr table = GM_Table();
    if (!isValidPtr(table)) return;
    int count = getTableBallCount(table);
    if (count <= 0 || count > MAX_BALLS_COUNT) return;
    this->guiData.ballsCount = count;
    for (int i = 0; i < count; i++) {
        Ball &ball = this->guiData.balls[i];
        ptr bPtr = getTableBall(table, i);
        if (!bPtr) continue;
        ball.index = i;
        ball.state = ballState(bPtr);
        ball.originalOnTable = ballIsOnTable(bPtr);
        ball.onTable = ball.originalOnTable;
        ball.classification = ballClassification(bPtr);
        ball.initialPosition = ballPosition(bPtr);
        ball.predictedPosition = ball.initialPosition;
        ball.velocity.nullify();
        ball.spin.nullify();
        ball.pocketIndex = -1;
        if (!ball.positions.empty()) ball.positions.clear();
        ball.positions.reserve(20);
        ball.positions.push_back(ball.initialPosition);
    }
}

void Prediction::initCueBall(double shotAngle, double shotPower, const Point2D &shotSpin) {
    double roundedAngle = round(shotAngle * 10000.0) / 10000.0;
    double angleCos = round(cos(roundedAngle) * 10000.0) / 10000.0;
    double angleSin = round(sin(roundedAngle) * 10000.0) / 10000.0;
    Ball &cueBall = this->guiData.balls[0];
    cueBall.velocity.x = shotPower * angleCos;
    cueBall.velocity.y = shotPower * angleSin;
    double spinFactor = shotPower / BALL_RADIUS;
    double v31 = -shotSpin.y * spinFactor;
    cueBall.spin.x = -(angleSin * v31);
    cueBall.spin.y = angleCos * v31;
    cueBall.spin.z = shotSpin.x * spinFactor;
}

void Prediction::determineBallsPositions() {
    int i; bool isAnyMoving; double time, time2;
    do {
        time = TIME_PER_TICK;
        do {
            time2 = time;
            this->guiData.collision.valid = false;
            for (i = 0; i < this->guiData.ballsCount; i++) {
                Ball &ball = this->guiData.balls[i];
                if (ball.onTable) ball.findNextCollision(&this->guiData, &time2);
            }
            for (i = 0; i < this->guiData.ballsCount; i++) {
                Ball &ball = this->guiData.balls[i];
                if (ball.onTable && ball.isMovingOrSpinning()) ball.move(time2);
            }
            if (this->guiData.collision.valid) {
                this->handleCollision();
            }
            time -= time2;
        } while (time > MIN_TIME);
        isAnyMoving = false;
        for (i = 0; i < this->guiData.ballsCount; i++) {
            Ball &ball = this->guiData.balls[i];
            if (ball.onTable) {
                ball.calcVelocity();
                if (ball.isMovingOrSpinning()) isAnyMoving = true;
            }
        }
    } while (isAnyMoving);
}

void Prediction::handleCollision() {
    Ball &bA = *(this->guiData.collision.ballA);
    Ball &bB = *(this->guiData.collision.ballB);
    if (!fastCalc) bA.positions.push_back(bA.predictedPosition);
    switch (this->guiData.collision.type) {
        case Collision::Type::BALL:
            this->guiData.ballCollisionCount++;
            this->handleBallBallCollision();
            if (!fastCalc) bB.positions.push_back(bB.predictedPosition);
            if (!this->guiData.collision.firstHitBall) {
                this->guiData.collision.firstHitBall = &bB;
                if (this->m_candidate.idx != -1) {
                    this->firstHitIsTarget = (bB.index == this->m_candidate.idx);
                }
            }
            break;
        case Collision::Type::LINE:
            if (!this->guiData.collision.firstHitBall) this->guiData.railBeforeFirstHitCount++;
            this->guiData.railCollisionCount++;
            bA.calcVelocityPostCollision(this->guiData.collision.angle);
            break;
        default: {
            if (!this->guiData.collision.firstHitBall) this->guiData.pointBeforeFirstHitCount++;
            this->guiData.pointCollisionCount++;
            double v38 = this->guiData.collision.point.y - bA.predictedPosition.y;
            double v39 = this->guiData.collision.point.x - bA.predictedPosition.x;
            double v40 = 0.0;
            if (v38 == 0.0) {
                v40 = (v39 <= 0.0) ? 1.5707963267948966 : -1.5707963267948966;
            } else {
                v40 = round(atan(-v39 / v38) * 10000.0) / 10000.0;
                if (v38 < 0.0) {
                    v40 += 3.141592653589793;
                }
            }
            this->guiData.collision.angle = -v40;
            bA.calcVelocityPostCollision(this->guiData.collision.angle);
        } break;
    }
}

void Prediction::handleBallBallCollision() const {
    Ball &bA = *(this->guiData.collision.ballA);
    Ball &bB = *(this->guiData.collision.ballB);
    Point2D relativePosition = bA.predictedPosition - bB.predictedPosition;
    double invDistance = 1.0 / sqrt(relativePosition.square());
    Point2D collisionNormal = relativePosition * invDistance;
    double velocityComponentA = bA.velocity.x * collisionNormal.x + bA.velocity.y * collisionNormal.y;
    double velocityComponentB = bB.velocity.x * collisionNormal.x + bB.velocity.y * collisionNormal.y;
    Point2D velocityA = collisionNormal * velocityComponentA;
    Point2D velocityB = collisionNormal * velocityComponentB;
    bA.velocity.x = velocityB.x - (velocityA.x - bA.velocity.x);
    bA.velocity.y = velocityB.y - (velocityA.y - bA.velocity.y);
    bB.velocity.x = velocityA.x - (velocityB.x - bB.velocity.x);
    bB.velocity.y = velocityA.y - (velocityB.y - bB.velocity.y);
}

// ── Ball physics methods (pure math) ──

namespace {
// Keep predictor constants aligned 1:1 with Android predictor.
constexpr double kAndroidSlipFactor = 0.00145772594752187;
constexpr double kAndroidRollDecel = 196.0;
constexpr double kAndroidSpinCoupling = 0.6578125102783204;
constexpr double kAndroidPostCollisionBounce = 0.804;
constexpr double kAndroidPostCollisionSpinY = 0.1420875022201172;
}

void Prediction::Ball::calcVelocity() {
    if (!this->isMovingOrSpinning()) return;
    double v15 = BALL_RADIUS * this->spin.x - this->velocity.y;
    double v16 = -this->velocity.x - this->spin.y * BALL_RADIUS;
    double v17 = sqrt(v16*v16 + v15*v15);
    double v18 = v17 * kAndroidSlipFactor;
    if (v18 > MIN_TIME) {
        double v20 = (v18 < TIME_PER_TICK) ? (v17 * kAndroidSlipFactor) : TIME_PER_TICK;
        double v21 = kAndroidRollDecel * v20 / v17;
        double v22 = v16 * v21, v23 = v15 * v21;
        this->velocity.x += v22; this->velocity.y += v23;
        this->spin.x -= v23 * kAndroidSpinCoupling;
        this->spin.y += v22 * kAndroidSpinCoupling;
    }
    if (v18 < TIME_PER_TICK) {
        double v24 = this->velocity.x, v25 = this->velocity.y;
        double v27 = (TIME_PER_TICK - v18) * 10.878;
        double v28 = 1.0 - v27 / sqrt(v25*v25 + v24*v24);
        v28 = (v28 < 0.0) ? 0.0 : v28;
        this->velocity.x = v24*v28; this->velocity.y = v25*v28;
        this->spin.x = v25*v28/BALL_RADIUS; this->spin.y = -(v24*v28)/BALL_RADIUS;
    }
    constexpr double v29 = 9.8 * TIME_PER_TICK;
    this->spin.z = (this->spin.z > 0.0) ? fmax(this->spin.z-v29, 0.0) : fmin(this->spin.z+v29, 0.0);
}

void Prediction::Ball::calcVelocityPostCollision(const double &angle) {
    double ac = cos(angle);
    double as = sin(angle);
    double vx = ac*this->velocity.x - as*this->velocity.y;
    double vy = as*this->velocity.x + ac*this->velocity.y;
    double sf = vx - BALL_RADIUS*this->spin.z;
    double asf = (sf>0.0)?sf:-sf;
    double vf = asf/2.5, avy = (vy>0.0)?vy:-vy;
    double sd = (sf>0.0)?1.0:-1.0;
    double msf = 0.4*avy; if(vf<msf)msf=vf;
    double sc = sd*msf;
    double nvx = vx-sc/2.5, nvy = -kAndroidPostCollisionBounce*vy;
    this->velocity.x = as*nvy+ac*nvx;
    this->velocity.y = ac*nvy-nvx*as;
    double nsx = as*this->spin.x+ac*this->spin.y;
    double nsy = ac*this->spin.x-as*this->spin.y-vy*kAndroidPostCollisionSpinY;
    double nsz = this->spin.z+sc*kAndroidSpinCoupling;
    this->spin.x = as*nsx+ac*nsy;
    this->spin.y = ac*nsx-nsy*as;
    this->spin.z = nsz;
}

bool Prediction::Ball::willCollideWithTable(const double *t) const {
    double cx=this->predictedPosition.x, cy=this->predictedPosition.y;
    double px=cx+this->velocity.x**t, py=cy+this->velocity.y**t;
    double lx=(this->velocity.x>0)?cx:px, rx=(this->velocity.x>0)?px:cx;
    double ty=(this->velocity.y>0)?cy:py, by=(this->velocity.y>0)?py:cy;
    return lx<TABLE_BOUND_LEFT||rx>TABLE_BOUND_RIGHT||ty<TABLE_BOUND_TOP||by>TABLE_BOUND_BOTTOM;
}

bool Prediction::Ball::isBallBallCollision(double *t, Prediction::Ball &other) const {
    Point2D relativePosition = other.predictedPosition - this->predictedPosition;
    Point2D velocityDelta = other.velocity - this->velocity;
    double v19 = (relativePosition.x * velocityDelta.x + relativePosition.y * velocityDelta.y) * 2.0;
    if (v19 >= 0.0) return false;
    double v20 = velocityDelta.square();
    double v21 = (relativePosition.square() - 57.7744409) * (v20 * 4.0);
    if (v21 > v19 * v19) return false;
    double v22 = (-v19 - sqrt(v19 * v19 - v21)) / (v20 + v20);
    if (v22 < 0.0) return false;
    // Exact ninja.c time acceptance logic with DBL_MAX guard
    if (v22 - MIN_TIME == *t) {
        if (v22 == 1.79769313e308) return false;
    } else if (v22 == 1.79769313e308 || v22 - MIN_TIME >= *t) {
        return false;
    }
    *t = v22;
    return true;
}

bool Prediction::Ball::isBallLineCollision(double *pTime, const Point2D &a, const Point2D &b) const {
    if(!this->velocity) return false;
    Point2D delta={b.x-a.x, b.y-a.y};
    double cross=delta.y*this->velocity.x-delta.x*this->velocity.y;
    if(cross==0.0)return false;
    double invD=1.0/sqrt(delta.square());
    double normalX=delta.y*invD, normalY=-(delta.x*invD);
    double offX=this->predictedPosition.x-a.x-normalX*BALL_RADIUS;
    double offY=this->predictedPosition.y-a.y-normalY*BALL_RADIUS;
    double v24=(offX*-this->velocity.y+this->velocity.x*offY)/cross;
    if(v24<=0.0||v24>=1.0)return false;
    double time=(delta.x*offY-delta.y*offX)/cross;
    if(time<=0.0)return false;
    // Exact ninja.c time acceptance logic with DBL_MAX guard
    if(time-MIN_TIME!=*pTime && time-MIN_TIME>=*pTime)return false;
    if(this->velocity.x*normalX+this->velocity.y*normalY>0.0)return false;
    if(time==1.79769313e308)return false;
    *pTime=time; return true;
}

bool Prediction::Ball::isBallPointCollision(double *t, const Point2D &pt) const {
    Point2D d={pt.x-this->predictedPosition.x, pt.y-this->predictedPosition.y};
    double v16=-(this->velocity.x*d.x*2.0)-(this->velocity.y*d.y*2.0);
    if(v16>=0.0)return false;
    double vs=this->velocity.x*this->velocity.x+this->velocity.y*this->velocity.y;
    double ds=d.square();
    if(ds-v16*v16/(vs*4.0)>=BALL_RADIUS_SQUARE)return false;
    double v22=(-v16-sqrt(v16*v16-vs*4.0*(ds-BALL_RADIUS_SQUARE)))/(vs*2.0);
    if(v22<0.0)return false;
    // Exact ninja.c time acceptance logic with DBL_MAX guard
    if(v22-MIN_TIME==*t){
        if(v22==1.79769313e308)return false;
    }else{
        if(v22==1.79769313e308||v22-MIN_TIME>=*t)return false;
    }
    *t=v22; return true;
}

void Prediction::Ball::determineBallTableCollision(void *pData, double *t) {
    auto *data=reinterpret_cast<SceneData*>(pData);
    auto &tableShape = getTableShape();
    int shapeCount = g_tableShapeCount;
    if (shapeCount <= 0 || shapeCount > TABLE_SHAPE_SIZE) shapeCount = TABLE_SHAPE_SIZE;
    for(int i=0;i<shapeCount;i++){
        const Point2D &pt=tableShape[i];
        const Point2D &npt=tableShape[(i+1)%shapeCount];
        if(this->isBallLineCollision(t,pt,npt)){
            double dx = npt.x - pt.x;
            double dy = npt.y - pt.y;
            double angle = 0.0;
            if (dx == 0.0) {
                angle = (dy >= 0.0) ? 1.5707963267948966 : -1.5707963267948966;
            } else {
                angle = round(atan(dy / dx) * 10000.0) / 10000.0;
                if (dx < 0.0) {
                    angle += 3.141592653589793;
                }
            }
            data->collision.valid=true; data->collision.ballA=this;
            data->collision.type=Collision::Type::LINE; data->collision.angle=-angle;
        }else if(this->isBallPointCollision(t,pt)){
            data->collision.valid=true; data->collision.ballA=this;
            data->collision.point=pt; data->collision.type=Collision::Type::POINT;
        }
    }
}

void Prediction::Ball::findNextCollision(void *pData, double *time) {
    auto *data=reinterpret_cast<SceneData*>(pData);
    auto pockets=getPockets();
    if(this->state==BallEnums::DEFAULT){
        for(int i=this->index+1;i<data->ballsCount;i++){
            Ball &ob=data->balls[i];
            if(ob.state==BallEnums::DEFAULT&&this->isBallBallCollision(time,ob)){
                data->collision.valid=true; data->collision.ballA=this;
                data->collision.type=Collision::Type::BALL; data->collision.ballB=&ob;
            }
        }
    }
    if(this->willCollideWithTable(time)){
        if(this->state==BallEnums::IN_POCKET){
            double ut=*time*1.5;
            this->velocity.x-=this->predictedPosition.x*ut;
            this->velocity.y-=this->predictedPosition.y*ut;
        }else if(this->state==BallEnums::DEFAULT){
            for(int i=0;i<TABLE_POCKETS_COUNT;i++){
                Point2D d={pockets[i].x-this->predictedPosition.x, pockets[i].y-this->predictedPosition.y};
                double ds=d.x*d.x+d.y*d.y;
                if(ds<POCKET_RADIUS_SQUARE){
                    double ut=*time*120.0;
                    this->velocity.x+=d.x*ut; this->velocity.y+=d.y*ut;
                    if(ds<BALL_RADIUS_SQUARE){
                        this->state=BallEnums::IN_POCKET;
                        this->pocketIndex=i;
                        Prediction::pocketStatus[i]=true;
                        if (data->firstPocketedBallIndex == -1) {
                            data->firstPocketedBallIndex = this->index;
                            data->firstPocketedPocketIndex = i;
                        }
                        data->pocketedBallsCount++;
                    }
                }
            }
        }
        this->determineBallTableCollision(pData,time);
    }
    if(this->state==BallEnums::IN_POCKET){
        this->state=BallEnums::UNKNOWN; this->onTable=false;
        this->velocity.nullify(); this->spin.nullify();
    }
}

void Prediction::Ball::move(const double &time) {
    if(this->velocity){
        this->predictedPosition.x+=this->velocity.x*time;
        this->predictedPosition.y+=this->velocity.y*time;
        if(!fastCalc){
            auto li=this->positions.size()-1;
            if(li>1){
                auto &a=this->positions[li-1]; auto &b=this->positions[li]; auto &c=this->predictedPosition;
                if (((b.y - a.y) * (c.x - b.x)) == ((c.y - b.y) * (b.x - a.x))) return;
            }
            this->positions.push_back(this->predictedPosition);
        }
    }
}

bool Prediction::Ball::isMovingOrSpinning() const {
    return bool(this->velocity)||bool(this->spin);
}


static double normalizeAngle(double angle) {
    double newAngle = angle;
    if (newAngle >= maxAngle) newAngle = fmod(newAngle, maxAngle);
    else if (newAngle < 0) newAngle = maxAngle - fmod(-newAngle, maxAngle);
    return newAngle;
}

static Candidate g_CurrentCandidate = { -1 };
static Point2D lastFailedCuePos = { -1000.0, -1000.0 };
static ImVec4 g_autoPlayToggleRect = ImVec4(0, 0, 0, 0); // x1,y1,x2,y2

static bool g_autoPlayEnabled = false;

static ImVec2 GetPocketScreenPos(int pocketIdx) {
    auto &pockets = getPockets();
    if (pocketIdx < 0 || pocketIdx >= TABLE_POCKETS_COUNT) return {};
    return WorldToScreen(pockets[pocketIdx]);
}

static bool IsShotValid() {
    auto &cand = g_CurrentCandidate;
    if (cand.idx == -1) return false;
    BallEnums::Classification myclass = getPlayerClassification();
    uint nominatedPocket = getNominatedPocket();
    if (nominatedPocket < 6 && cand.pocketIndex != (int)nominatedPocket) return false;
    if (!gPrediction->guiData.balls[0].onTable) return false;
    if (!gPrediction->guiData.balls[cand.idx].originalOnTable) return false;
    if (gPrediction->guiData.balls[cand.idx].onTable) return false;
    if (gPrediction->guiData.balls[cand.idx].pocketIndex != cand.pocketIndex) return false;
    auto &ball8 = gPrediction->guiData.balls[8];
    if (myclass == BallEnums::ANY && ball8.originalOnTable && !ball8.onTable) return false;
    auto &firstHit = gPrediction->guiData.collision.firstHitBall;
    if (firstHit) {
        if (myclass == BallEnums::ANY) {
            if (firstHit->classification == BallEnums::EIGHT_BALL) return false;
        } else if (firstHit->classification != myclass) return false;
    }
    return true;
}

static NSInteger g_shotTouchId = 0;

static void shotTouchBegin(CGPoint pt) {
    dispatch_async(dispatch_get_main_queue(), ^{
        g_shotTouchId = [wthaxvnpriv fakeTouchId:0 AtPoint:pt withTouchPhase:UITouchPhaseBegan];
    });
}
static void shotTouchMove(CGPoint pt) {
    if (!g_shotTouchId) return;
    NSInteger tid = g_shotTouchId;
    dispatch_async(dispatch_get_main_queue(), ^{
        [wthaxvnpriv fakeTouchId:tid AtPoint:pt withTouchPhase:UITouchPhaseMoved];
    });
}
static void shotTouchEnd(CGPoint pt) {
    if (!g_shotTouchId) return;
    NSInteger tid = g_shotTouchId;
    g_shotTouchId = 0;
    dispatch_async(dispatch_get_main_queue(), ^{
        [wthaxvnpriv fakeTouchId:tid AtPoint:pt withTouchPhase:UITouchPhaseEnded];
    });
}
// Fire touch end then OVERWRITE angle+power AFTER sendEvent returns
// The game's touch handler recalculates angle from touch position during sendEvent.
// By writing our values AFTER sendEvent, we override the game's calculation
// before the next update cycle reads the angle to fire the shot.
static void shotTouchEndWithValues(CGPoint pt, double angle, double power) {
    if (!g_shotTouchId) return;
    NSInteger tid = g_shotTouchId;
    g_shotTouchId = 0;
    dispatch_async(dispatch_get_main_queue(), ^{
        // 1) sendEvent processes touch — game calculates angle from touch position
        [wthaxvnpriv fakeTouchId:tid AtPoint:pt withTouchPhase:UITouchPhaseEnded];
        // 2) AFTER sendEvent returns, overwrite with our exact values
        //    Game hasn't updated yet — next update cycle will read these
        setAimAngle(angle);
        setPower(power);
    });
}

namespace AutoPlay {
    double lastSetAngle = 0.0;
    bool didSetAngle = false;
    bool bAutoPlaying = false;
    bool bAutoAim = false;

    enum State { IDLE, SCANNING, SELECTING, NOMINATING, SHOOTING } state = IDLE;

    void showStatus(const char *msg) {
        dispatch_async(dispatch_get_main_queue(), ^{
            thongbao([NSString stringWithUTF8String:msg]);
        });
    }
    void updateState(State newState, const char *msg = nullptr) {
        if (newState == state) return;
        state = newState;
        if (msg) showStatus(msg);
    }

    double pendingShotPower = 0.0;
    double pendingShotAngle = 0.0;
    int nominationFrameCounter = 0;
    int shootingFrameCounter = 0;

    // Scan pipeline: 7s collect all simulated samples + 1s choose best from buffered results.
    static constexpr double kScanCollectSeconds = 7.0;
    static constexpr double kScanSelectSeconds = 1.0;
    static constexpr double kScanStepPrimary = 0.0062;
    static bool g_scanActive = false;
    static CFTimeInterval g_scanStart = 0.0;
    static CFTimeInterval g_selectStart = 0.0;
    static int g_scanSamplesDone = 0;
    static int g_scanTotalSamples = 0;
    static Candidate g_bestShot = {-1};
    static double g_bestQuality = 1e18;
    static Point2D g_scanCuePos = {-1000.0, -1000.0};
    static std::vector<Candidate> g_scanResults;


    // Max power: use same calculation as prediction (getShotPower) with slider at max
    static double getFixedPower() {
        ptr vc = GM_VisualCue();
        if (isValidPtr(vc) && g_off.vc_mPower >= 0) {
            double savedPower = F(double, vc + g_off.vc_mPower);
            F(double, vc + g_off.vc_mPower) = 1.0; // slider max
            double result = getShotPower();          // exact same as prediction
            F(double, vc + g_off.vc_mPower) = savedPower; // restore
            return result;
        }
        return g_effectiveMaxPower; // fallback
    }

    void ClearState() {
        g_CurrentCandidate.idx = -1;
        lastFailedCuePos = { -1000.0, -1000.0 };
        resetPredictionCache();
        g_scanActive = false;
        g_scanStart = 0.0;
        g_selectStart = 0.0;
        g_scanSamplesDone = 0;
        g_scanTotalSamples = 0;
        g_bestShot = {-1};
        g_bestQuality = 1e18;
        g_scanResults.clear();
    }

    CGPoint getShotTouchPoint() {
        return CGPointMake(g_screenW - 120.0, g_screenH * 0.35);
    }

    // ── Path obstruction check ──
    // Returns true if no other ball blocks the straight line from→to
    static bool isPathClearWithFactor(Vec2d from, Vec2d to, int ignoreBall1, int ignoreBall2, double clearanceFactor) {
        Vec2d d = to - from;
        double dist = sqrt(d.square());
        if (dist < 0.01) return true;
        Vec2d n = d * (1.0 / dist);
        double clearanceRadius = BALL_RADIUS * clearanceFactor; // lenient in relaxed fallback passes
        for (int i = 0; i < gPrediction->guiData.ballsCount; i++) {
            if (i == ignoreBall1 || i == ignoreBall2) continue;
            auto &b = gPrediction->guiData.balls[i];
            if (!b.originalOnTable) continue;
            Vec2d tb = b.initialPosition - from;
            double proj = tb.x * n.x + tb.y * n.y;
            if (proj < BALL_RADIUS || proj > dist - BALL_RADIUS) continue;
            double perpSq = tb.square() - proj * proj;
            if (perpSq < clearanceRadius * clearanceRadius) return false;
        }
        return true;
    }

    static bool isPathClear(Vec2d from, Vec2d to, int ignoreBall1, int ignoreBall2) {
        return isPathClearWithFactor(from, to, ignoreBall1, ignoreBall2, 1.90);
    }

    static bool hasRemainingBallsByClass(BallEnums::Classification cls) {
        for (int i = 1; i < gPrediction->guiData.ballsCount; i++) {
            if (i == 8) continue;
            auto &b = gPrediction->guiData.balls[i];
            if (!b.originalOnTable) continue;
            if (b.classification == cls) return true;
        }
        return false;
    }

    static bool mustShootEightNow(BallEnums::Classification myclass, bool isNineBall) {
        if (isNineBall) return false;
        if (myclass == BallEnums::EIGHT_BALL) return true;
        if (myclass != BallEnums::SOLID && myclass != BallEnums::STRIPE) return false;
        return !hasRemainingBallsByClass(myclass);
    }

    static bool isEightBallOpeningBreak(BallEnums::Classification myclass, bool isNineBall);

    static bool isTargetAllowed(BallEnums::Classification myclass, bool isNineBall, int ballIdx, int lowestIdx) {
        if (ballIdx <= 0 || ballIdx >= gPrediction->guiData.ballsCount) return false;
        auto &ball = gPrediction->guiData.balls[ballIdx];
        if (!ball.originalOnTable) return false;
        if (isEightBallOpeningBreak(myclass, isNineBall)) return ballIdx != 8;
        if (isNineBall) return true; // Any ball on table can be pocketed via combo
        if (mustShootEightNow(myclass, isNineBall)) return ballIdx == 8;
        if (myclass == BallEnums::ANY) return ballIdx != 8;
        return ball.classification == myclass;
    }

    // Direct target check for 9-ball geometric finder (only aims at lowest ball)
    static bool isDirectTarget9Ball(int ballIdx, int lowestIdx) {
        return (lowestIdx == -1) || (ballIdx == lowestIdx);
    }

    static double pathLength(const std::vector<Point2D> &pts);
    static double shotQualityScore(bool isNineBall, int targetIdx, int lowestIdx, double cutAngle, int railHits, int ballHits, int pointHits);
    static bool canExecuteCandidateNow(const Candidate &cand, BallEnums::Classification myclass, bool isNineBall, int lowestIdx, uint nominatedPocket);

    static bool isPocketMouthDangerPoint(const Vec2d &pt, int intendedPocket) {
        auto &pockets = getPockets();
        const double avoidR = POCKET_RADIUS * 1.45;
        const double avoidRSq = avoidR * avoidR;
        for (int i = 0; i < TABLE_POCKETS_COUNT; i++) {
            if (i == intendedPocket) continue;
            Vec2d d = pt - pockets[i];
            if (d.square() < avoidRSq) return true;
        }
        return false;
    }

    static int countPocketMouthHazards(const std::vector<Point2D> &pts, int intendedPocket) {
        int hazards = 0;
        for (const auto &p : pts) {
            if (isPocketMouthDangerPoint(p, intendedPocket)) hazards++;
        }
        return hazards;
    }

    static bool isNineBallOpeningBreak() {
        if (!isNineBallRuntime()) return false;
        if (gPrediction->guiData.ballsCount <= 9) return false;
        for (int i = 1; i <= 9; i++) {
            auto &b = gPrediction->guiData.balls[i];
            if (!b.originalOnTable) return false;
        }
        return true;
    }

    static bool isEightBallOpeningBreak(BallEnums::Classification myclass, bool isNineBall) {
        (void)myclass;
        if (isNineBall) return false;
        if (gPrediction->guiData.ballsCount <= 8) return false;
        for (int i = 1; i < gPrediction->guiData.ballsCount; i++) {
            auto &b = gPrediction->guiData.balls[i];
            if (!b.originalOnTable) return false;
        }
        return true;
    }

    static bool isOpeningBreakNoRailRule(BallEnums::Classification myclass, bool isNineBall) {
        // Keep API for compatibility, but do not hard-block break shots by rail/point collisions.
        // Collision quality is already handled by scoring penalties, which is safer and avoids dead scans.
        (void)myclass;
        (void)isNineBall;
        return false;
    }

    // Call-pocket modes: search all pockets for best predicted drop, then nominate that pocket before shooting.
    static bool autoPlayIgnoreNominatedPocketWhenSearching() {
        int m = getPocketNominationMode();
        if (m == 2) return true;
        BallEnums::Classification myclass = getPlayerClassification();
        if (m == 1 && myclass == BallEnums::EIGHT_BALL) return true;
        if (isNineBallRuntime() && m != 0 && !isNineBallOpeningBreak()) return true;
        return false;
    }

    static Candidate findOpeningBreakFallbackShot(BallEnums::Classification myclass, bool isNineBall, int lowestIdx) {
        Candidate best = {-1};
        auto &cue = gPrediction->guiData.balls[0];
        double bestDist = 1e18;
        for (int i = 1; i < gPrediction->guiData.ballsCount; i++) {
            auto &b = gPrediction->guiData.balls[i];
            if (!b.originalOnTable) continue;
            if (isNineBall) {
                if (lowestIdx != -1 && i != lowestIdx) continue;
            } else {
                if (b.classification == BallEnums::EIGHT_BALL) continue;
            }
            Vec2d d = b.initialPosition - cue.initialPosition;
            double dsq = d.square();
            if (dsq < 0.01) continue;
            if (dsq < bestDist) {
                bestDist = dsq;
                double a = atan2(d.y, d.x);
                if (a < 0) a += 2.0 * PI_CONST;
                best = {i, a, 9e17, 0, fmax(220.0, g_effectiveMaxPower * 0.92)};
            }
        }
        return best;
    }

    // ── Cut angle between incoming line and pocket direction ──
    static double calcCutAngle(Vec2d cuePos, Vec2d targetPos, Vec2d pocketPos) {
        Vec2d inDir = targetPos - cuePos;
        Vec2d outDir = pocketPos - targetPos;
        double d1 = sqrt(inDir.square()), d2 = sqrt(outDir.square());
        if (d1 < 0.01 || d2 < 0.01) return PI_CONST;
        double dot = (inDir.x * outDir.x + inDir.y * outDir.y) / (d1 * d2);
        if (dot > 1.0) dot = 1.0;
        if (dot < -1.0) dot = -1.0;
        return acos(dot);
    }

    // ── Validate a fully-simulated shot result ──
    static bool validateShot(int targetIdx, int targetPocket, BallEnums::Classification myclass, bool isNineBall) {
        // Cue ball must stay on table
        if (!gPrediction->guiData.balls[0].onTable) return false;
        if (isOpeningBreakNoRailRule(myclass, isNineBall)) {
            if (gPrediction->guiData.railCollisionCount > 0) return false;
            if (gPrediction->guiData.pointCollisionCount > 0) return false;
        }

        // 8-ball opening break: keep turn if any object ball except the 8 is pocketed (any pocket); first hit cannot be the 8.
        if (!isNineBall && isEightBallOpeningBreak(myclass, isNineBall)) {
            bool okPot = false;
            for (int i = 1; i < gPrediction->guiData.ballsCount; i++) {
                if (i == 8) continue;
                auto &b = gPrediction->guiData.balls[i];
                if (b.originalOnTable && !b.onTable) { okPot = true; break; }
            }
            if (!okPot) return false;
            auto *fh = gPrediction->guiData.collision.firstHitBall;
            if (!fh) return false;
            if (fh->classification == BallEnums::EIGHT_BALL) return false;
            if (gPrediction->guiData.ballsCount > 8) {
                auto &eightBall = gPrediction->guiData.balls[8];
                if (eightBall.originalOnTable && !eightBall.onTable) return false;
            }
            return true;
        }

        auto &target = gPrediction->guiData.balls[targetIdx];
        bool anyPot = (gPrediction->guiData.pocketedBallsCount > 0);
        if (!isNineBall) {
            // In 8-ball flow we require the selected target to be potted into chosen pocket.
            if (target.onTable) return false;
            if (target.pocketIndex != targetPocket) return false;
        } else {
            // In 9-ball, after legal first contact, any potted ball can keep turn.
            if (!anyPot) return false;
        }

        // First-hit validation
        auto *firstHit = gPrediction->guiData.collision.firstHitBall;
        if (!firstHit) return false;

        if (isNineBall) {
            // Must hit lowest numbered ball first
            int lowestIdx = -1;
            for (int i = 1; i < gPrediction->guiData.ballsCount; i++) {
                if (gPrediction->guiData.balls[i].originalOnTable) { lowestIdx = i; break; }
            }
            if (lowestIdx != -1 && firstHit->index != lowestIdx) return false;
            if (targetPocket < 0 || targetPocket >= 6) return false;
            if (!isNineBallOpeningBreak()) {
                int firstPocket = gPrediction->guiData.firstPocketedPocketIndex;
                if (firstPocket < 0 || firstPocket >= 6) return false;
                if (firstPocket != targetPocket) return false;
            }
        } else {
            bool eightOnly = mustShootEightNow(myclass, isNineBall);
            if (eightOnly) {
                if (targetIdx != 8) return false;
                if (firstHit->index != 8) return false;
            } else if (myclass == BallEnums::ANY) {
                if (firstHit->classification == BallEnums::EIGHT_BALL) return false;
                if (targetIdx == 8) return false;
            } else {
                if (firstHit->classification != myclass) return false;
                if (target.classification != myclass) return false;
            }
        }

        // Don't accidentally pot the 8-ball when not shooting for it
        if (!isNineBall && !mustShootEightNow(myclass, isNineBall) && gPrediction->guiData.ballsCount > 8) {
            auto &eightBall = gPrediction->guiData.balls[8];
            if (eightBall.originalOnTable && !eightBall.onTable) return false;
        }

        return true;
    }

    static bool evaluateShotResultLegality(bool isNineBall, BallEnums::Classification myclass, int lowestIdx) {
        if (!gPrediction->guiData.balls[0].onTable) return false;
        auto *firstHit = gPrediction->guiData.collision.firstHitBall;
        if (!firstHit) return false;
        if (isOpeningBreakNoRailRule(myclass, isNineBall)) {
            if (gPrediction->guiData.railCollisionCount > 0) return false;
            if (gPrediction->guiData.pointCollisionCount > 0) return false;
        }

        if (isNineBall) {
            if (lowestIdx != -1 && firstHit->index != lowestIdx) return false;
            return gPrediction->guiData.pocketedBallsCount > 0;
        }

        bool eightOnly = mustShootEightNow(myclass, isNineBall);
        bool eightOpenBreak = isEightBallOpeningBreak(myclass, isNineBall);
        if (eightOpenBreak) {
            if (firstHit->index == 8) return false;
            if (gPrediction->guiData.ballsCount > 8) {
                auto &eightBall = gPrediction->guiData.balls[8];
                if (eightBall.originalOnTable && !eightBall.onTable) return false;
            }
            for (int i = 1; i < gPrediction->guiData.ballsCount; i++) {
                if (i == 8) continue;
                auto &b = gPrediction->guiData.balls[i];
                if (b.originalOnTable && !b.onTable) return true;
            }
            return false;
        }
        if (eightOnly) {
            if (firstHit->index != 8) return false;
            auto &eightBall = gPrediction->guiData.balls[8];
            return eightBall.originalOnTable && !eightBall.onTable;
        }

        if (myclass == BallEnums::ANY) {
            if (firstHit->classification == BallEnums::EIGHT_BALL) return false;
            for (int i = 1; i < gPrediction->guiData.ballsCount; i++) {
                if (i == 8) continue;
                auto &b = gPrediction->guiData.balls[i];
                if (b.originalOnTable && !b.onTable) return true;
            }
            return false;
        } else if (firstHit->classification != myclass) {
            return false;
        } else {
            for (int i = 1; i < gPrediction->guiData.ballsCount; i++) {
                if (i == 8) continue;
                auto &b = gPrediction->guiData.balls[i];
                if (b.classification == myclass && b.originalOnTable && !b.onTable) return true;
            }
            return false;
        }
    }

    static Candidate findBestDirectShot(BallEnums::Classification myclass, bool isNineBall, int lowestIdx, uint nominatedPocket, bool strictStraightOnly) {
        Candidate best = {-1};
        auto &pockets = getPockets();
        auto &cueBall = gPrediction->guiData.balls[0];
        double bestQuality = 1e18;
        Vec2d spin = getShotSpin();
        const double maxCutAngle = strictStraightOnly ? 1.3963 : 1.5359; // 80deg strict, ~88deg relaxed
        const double pathClearance = strictStraightOnly ? 1.90 : 1.55;

        struct GeoCand {
            int bi;
            int pi;
            double angle;
            double basePower;
            double cut;
            double seedScore;
            bool operator<(const GeoCand &o) const { return seedScore < o.seedScore; }
        };
        std::vector<GeoCand> pre;
        pre.reserve(64);

        for (int bi = 1; bi < gPrediction->guiData.ballsCount; bi++) {
            // 9-ball direct: only target lowest ball; combos handled by findBest9BallComboShot
            if (isNineBall) {
                if (bi <= 0 || bi >= gPrediction->guiData.ballsCount) continue;
                if (!gPrediction->guiData.balls[bi].originalOnTable) continue;
                if (!isDirectTarget9Ball(bi, lowestIdx)) continue;
            } else {
                if (!isTargetAllowed(myclass, isNineBall, bi, lowestIdx)) continue;
            }
            auto &ball = gPrediction->guiData.balls[bi];
            for (int pi = 0; pi < TABLE_POCKETS_COUNT; pi++) {
                if (nominatedPocket < 6 && pi != (int)nominatedPocket) continue;
                Vec2d pocket = pockets[pi];
                Vec2d toPocket = pocket - ball.initialPosition;
                double distTP = sqrt(toPocket.square());
                if (distTP < 0.5) continue;
                Vec2d pocketDir = toPocket * (1.0 / distTP);
                Vec2d ghostPos = ball.initialPosition - pocketDir * (BALL_RADIUS * 2.0);
                Vec2d shotLine = ghostPos - cueBall.initialPosition;
                double distCG = sqrt(shotLine.square());
                if (distCG < 1.0) continue;
                double cutAngle = calcCutAngle(cueBall.initialPosition, ball.initialPosition, pocket);
                if (cutAngle > maxCutAngle) continue;
                if (!isPathClearWithFactor(cueBall.initialPosition, ghostPos, 0, bi, pathClearance)) continue;
                if (!isPathClearWithFactor(ball.initialPosition, pocket, bi, 0, pathClearance)) continue;

                double angle = atan2(shotLine.y, shotLine.x);
                if (angle < 0) angle += 2.0 * PI_CONST;
                double cosTransfer = cos(cutAngle);
                if (cosTransfer < 0.10) continue;
                double power = getFixedPower(); // always max power
                double seedScore = cutAngle * 120.0 + distCG * 0.55 + distTP * 0.35;
                pre.push_back({bi, pi, angle, power, cutAngle, seedScore});
            }
        }

        std::sort(pre.begin(), pre.end());
        const int tryN = strictStraightOnly ? (int)fmin((size_t)12, pre.size()) : (int)fmin((size_t)22, pre.size());
        static const double offsets[] = {0.0, -0.0012, 0.0012, -0.0026, 0.0026};
        for (int c = 0; c < tryN; c++) {
            auto &cand = pre[c];
            double p = cand.basePower; // fixed 90% power
            for (double off : offsets) {
                double a = normalizeAngle(cand.angle + off);
                Candidate simCand = {cand.bi, a, 0, cand.pi, p};
                gPrediction->determineShotResult(true, a, p, spin, simCand);
                if (!gPrediction->firstHitIsTarget) continue;
                if (!validateShot(cand.bi, cand.pi, myclass, isNineBall)) continue;
                int railHits = gPrediction->guiData.railCollisionCount;
                int ballHits = gPrediction->guiData.ballCollisionCount;
                int pointHits = gPrediction->guiData.pointCollisionCount;
                double cuePath = pathLength(gPrediction->guiData.balls[0].positions);
                double objPath = pathLength(gPrediction->guiData.balls[cand.bi].positions);
                double q = shotQualityScore(isNineBall, cand.bi, lowestIdx, cand.cut, railHits, ballHits, pointHits);
                int cueHazards = countPocketMouthHazards(gPrediction->guiData.balls[0].positions, cand.pi);
                int objHazards = countPocketMouthHazards(gPrediction->guiData.balls[cand.bi].positions, cand.pi);
                q += cuePath * 0.8 + objPath * 0.3;
                q += cueHazards * 420.0 + objHazards * 260.0;
                if ((railHits > 0 || pointHits > 0) && (cueHazards > 0 || objHazards > 0)) q += 3200.0;
                // 8-ball: penalize potting opponent's balls
                if (!isNineBall && myclass != BallEnums::ANY && !mustShootEightNow(myclass, isNineBall)) {
                    for (int obi = 1; obi < gPrediction->guiData.ballsCount; obi++) {
                        if (obi == 8) continue;
                        auto &ob = gPrediction->guiData.balls[obi];
                        if (ob.originalOnTable && !ob.onTable && ob.classification != myclass && ob.classification != BallEnums::EIGHT_BALL)
                            q += 1500.0;
                    }
                }
                if (q < bestQuality) {
                    bestQuality = q;
                    best = {cand.bi, a, q, cand.pi, p};
                }
            }
        }
        return best;
    }

    // ── 9-Ball Combo Shot Finder ──
    // Finds: cue → hits lowest ball → lowest deflects → hits target → target falls into pocket
    static Candidate findBest9BallComboShot(int lowestIdx, uint nominatedPocket) {
        if (lowestIdx <= 0) return {-1};
        Candidate best = {-1};
        auto &pockets = getPockets();
        auto &cueBall = gPrediction->guiData.balls[0];
        auto &lowestBall = gPrediction->guiData.balls[lowestIdx];
        if (!lowestBall.originalOnTable || !cueBall.originalOnTable) return {-1};
        double bestQuality = 1e18;
        Vec2d spin = getShotSpin();

        struct ComboCand {
            int targetBallIdx, pocketIdx;
            double angle, power, cut1, cut2, seedScore;
            bool operator<(const ComboCand &o) const { return seedScore < o.seedScore; }
        };
        std::vector<ComboCand> pre;
        pre.reserve(64);

        for (int bi = 1; bi < gPrediction->guiData.ballsCount; bi++) {
            if (bi == lowestIdx) continue;
            auto &targetBall = gPrediction->guiData.balls[bi];
            if (!targetBall.originalOnTable) continue;

            for (int pi = 0; pi < TABLE_POCKETS_COUNT; pi++) {
                if (nominatedPocket < 6 && pi != (int)nominatedPocket) continue;
                Vec2d pocket = pockets[pi];

                // Step 1: Ghost position for target ball (where to hit it into pocket)
                Vec2d tTP = pocket - targetBall.initialPosition;
                double dTP = sqrt(tTP.square());
                if (dTP < 0.5) continue;
                Vec2d pDir = tTP * (1.0 / dTP);
                Vec2d tGhost = targetBall.initialPosition - pDir * (BALL_RADIUS * 2.0);

                // Step 2: Ghost position for lowest ball (where to hit it toward target ghost)
                Vec2d lTG = tGhost - lowestBall.initialPosition;
                double dLG = sqrt(lTG.square());
                if (dLG < 1.0) continue;
                Vec2d lDir = lTG * (1.0 / dLG);
                Vec2d lGhost = lowestBall.initialPosition - lDir * (BALL_RADIUS * 2.0);

                // Step 3: Shot angle from cue to lowest ghost
                Vec2d sLine = lGhost - cueBall.initialPosition;
                double dCG = sqrt(sLine.square());
                if (dCG < 1.0) continue;

                // Cut angles
                double c1 = calcCutAngle(cueBall.initialPosition, lowestBall.initialPosition, tGhost);
                double c2 = calcCutAngle(lowestBall.initialPosition, targetBall.initialPosition, pocket);
                if (c1 > 1.2217 || c2 > 1.2217) continue; // max ~70°

                // Path clearance
                if (!isPathClearWithFactor(cueBall.initialPosition, lGhost, 0, lowestIdx, 1.75)) continue;
                if (!isPathClearWithFactor(lowestBall.initialPosition, tGhost, lowestIdx, bi, 1.75)) continue;
                if (!isPathClearWithFactor(targetBall.initialPosition, pocket, bi, lowestIdx, 1.75)) continue;

                double angle = atan2(sLine.y, sLine.x);
                if (angle < 0) angle += 2.0 * PI_CONST;
                double power = getFixedPower();
                double seed = (c1 + c2) * 200.0 + dCG * 0.4 + dLG * 0.5 + dTP * 0.3;
                if (bi == 9) seed -= 3000.0; // Huge bonus for 9-ball combo win
                pre.push_back({bi, pi, angle, power, c1, c2, seed});
            }
        }

        std::sort(pre.begin(), pre.end());
        const int tryN = (int)fmin((size_t)15, pre.size());
        static const double offsets[] = {0.0, -0.0010, 0.0010, -0.0022, 0.0022, -0.0036, 0.0036};

        for (int c = 0; c < tryN; c++) {
            auto &cc = pre[c];
            for (double off : offsets) {
                double a = normalizeAngle(cc.angle + off);
                Candidate simCand = {lowestIdx, a, 0, cc.pocketIdx, cc.power};
                gPrediction->determineShotResult(true, a, cc.power, spin, simCand);

                // Must hit lowest first
                auto *fh = gPrediction->guiData.collision.firstHitBall;
                if (!fh || fh->index != lowestIdx) continue;
                if (!gPrediction->guiData.balls[0].onTable) continue;
                if (gPrediction->guiData.pocketedBallsCount <= 0) continue;

                int potIdx = gPrediction->guiData.firstPocketedBallIndex;
                int potPocket = gPrediction->guiData.firstPocketedPocketIndex;
                if (potIdx < 1 || potPocket < 0 || potPocket >= 6) continue;
                if (nominatedPocket < 6 && potPocket != (int)nominatedPocket) continue;

                // Score
                auto &cue = gPrediction->guiData.balls[0];
                double cuePath = pathLength(cue.positions);
                int rH = gPrediction->guiData.railCollisionCount;
                int bH = gPrediction->guiData.ballCollisionCount;
                int pH = gPrediction->guiData.pointCollisionCount;
                double q = shotQualityScore(true, potIdx, lowestIdx, cc.cut1 + cc.cut2, rH, bH, pH);
                q += 300.0; // Base combo penalty (prefer direct when equal)
                q += cuePath * 0.8;
                q += countPocketMouthHazards(cue.positions, potPocket) * 620.0;

                // Full re-validate
                Candidate evalCand = {potIdx, a, q, potPocket, cc.power};
                gPrediction->determineShotResult(true, a, cc.power, spin, evalCand);
                auto *fh2 = gPrediction->guiData.collision.firstHitBall;
                if (!fh2 || (lowestIdx > 0 && fh2->index != lowestIdx)) continue;
                if (!gPrediction->guiData.balls[0].onTable) continue;
                if (gPrediction->guiData.pocketedBallsCount <= 0) continue;
                BallEnums::Classification mc = getPlayerClassification();
                if (!validateShot(potIdx, potPocket, mc, true)) continue;

                if (q < bestQuality) {
                    bestQuality = q;
                    best = evalCand;
                }
            }
        }
        return best;
    }

    static double pathLength(const std::vector<Point2D> &pts) {
        if (pts.size() < 2) return 0.0;
        double sum = 0.0;
        for (size_t i = 1; i < pts.size(); i++) {
            Vec2d d = pts[i] - pts[i - 1];
            sum += sqrt(d.square());
        }
        return sum;
    }

    static double shotQualityScore(bool isNineBall, int targetIdx, int lowestIdx, double cutAngle, int railHits, int ballHits, int pointHits) {
        double score = 0.0;
        // Heavily penalize collisions — prefer least bouncy path
        score += railHits * 420.0;
        score += ballHits * 300.0;
        score += pointHits * 520.0;
        if (railHits >= 4) score += 1600.0;
        // Exponential penalty for extreme cut angles — safer shots preferred
        score += pow(cutAngle, 1.5) * 580.0;
        if (isNineBall) {
            // Winning combo to 9-ball has top priority when legal.
            if (targetIdx == 9) score -= 5200.0;
            // Always aim at lowest ball on table (9-ball): score picks among all simulated shots.
            if (lowestIdx > 0 && targetIdx == lowestIdx) score -= 900.0;
            if (lowestIdx > 0 && targetIdx != lowestIdx) score += 4000.0;
        }
        return score;
    }

    static bool canExecuteCandidateNow(const Candidate &cand, BallEnums::Classification myclass, bool isNineBall, int lowestIdx, uint nominatedPocket) {
        if (cand.idx <= 0 || cand.pocketIndex < 0 || cand.pocketIndex >= TABLE_POCKETS_COUNT) return false;
        if (nominatedPocket < 6 && cand.pocketIndex != (int)nominatedPocket) return false;
        if (!isTargetAllowed(myclass, isNineBall, cand.idx, lowestIdx)) return false;
        Vec2d spin = getShotSpin();
        gPrediction->determineShotResult(true, cand.angle, cand.power, spin, cand);
        if (isNineBall) {
            // 9-ball combo: first hit must be lowest ball, not necessarily the pocketed ball
            auto *fh = gPrediction->guiData.collision.firstHitBall;
            if (!fh || (lowestIdx > 0 && fh->index != lowestIdx)) return false;
        } else {
            if (!gPrediction->firstHitIsTarget) return false;
        }
        return validateShot(cand.idx, cand.pocketIndex, myclass, isNineBall);
    }

    // ── Direct memory cue activation/deactivation ──
    static void activateCue() {
        ptr vc = GM_VisualCue();
        if (!isValidPtr(vc) || g_off.vc_mInputActive < 0) return;
        F(bool, vc + g_off.vc_mInputActive) = true;
    }
    static void deactivateCue() {
        ptr vc = GM_VisualCue();
        if (!isValidPtr(vc) || g_off.vc_mInputActive < 0) return;
        F(bool, vc + g_off.vc_mInputActive) = false;
    }

    // Set angle+power via memory, then enter SHOOTING state to fire via touch
    void takeShot(double angle, double power) {
        // Re-read cue stats right before firing so cue swaps update power immediately.
        cacheCueStats();
        angle = normalizeAngle(angle);
        pendingShotAngle = angle;
        pendingShotPower = power;
        lastSetAngle = angle;
        setAimAngle(angle);
        setPower(power);
        resetPredictionCache();
        gPrediction->determineShotResult(false, angle, power);
        updateState(SHOOTING, "🎱 Shooting!");
        shootingFrameCounter = 0;
    }

    void Shoot(double angle, double power) {
        angle = normalizeAngle(angle);
        pendingShotAngle = angle;
        pendingShotPower = power;
        lastSetAngle = angle;

        bool nominating = false;
        int nominationMode = getPocketNominationMode();
        auto myclass = getPlayerClassification();
        bool isNineBall = isNineBallRuntime();
        bool openingNineBreak = isNineBallOpeningBreak();
        bool needsCallPocketFlow =
            (nominationMode == 1 && myclass == BallEnums::EIGHT_BALL) ||
            (nominationMode == 2) ||
            (isNineBall && nominationMode != 0 && !openingNineBreak);
        if (needsCallPocketFlow && g_CurrentCandidate.idx != -1 &&
            g_CurrentCandidate.pocketIndex >= 0 && g_CurrentCandidate.pocketIndex < 6) {
            nominatePocket(g_CurrentCandidate.pocketIndex);
        }
        if (needsCallPocketFlow) {
            if (g_CurrentCandidate.idx != -1 && getNominatedPocket() != (uint)g_CurrentCandidate.pocketIndex) {
                nominating = true;
            }
        }

        if (nominating) {
            state = NOMINATING;
            nominationFrameCounter = 0;
        } else {
            takeShot(angle, power);
        }
    }

    void DrawToggleButton() {
        ImGuiIO &io = ImGui::GetIO();
        float button_size = ImGui::GetFrameHeight() * 2.3f;
        float windowWidth = button_size;
        float windowHeight = button_size;

        ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - windowWidth, io.DisplaySize.y - windowHeight), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(windowWidth, windowHeight), ImGuiCond_Always);

        ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32(0, 0, 0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 5.0f);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        if (ImGui::Begin("##AutoPlay", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus)) {
            ImVec2 pos = ImGui::GetCursorScreenPos();
            ImVec2 size(button_size, button_size);
            ImVec2 center(pos.x + size.x * 0.5f, pos.y + size.y * 0.5f);
            g_autoPlayToggleRect = ImVec4(pos.x, pos.y, pos.x + size.x, pos.y + size.y);

            ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(50, 50, 50, 180));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(80, 80, 80, 200));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(100, 100, 100, 200));

            bool clicked = ImGui::Button("##AutoPlayBtn", size);
            ImDrawList *dl = ImGui::GetWindowDrawList();
            float h = size.y * 0.4f, w = h * 0.8f;

            if (bAutoPlaying) {
                float bar_w = w * 0.35f, gap = w * 0.3f;
                dl->AddRectFilled(ImVec2(center.x - gap/2 - bar_w, center.y - h/2), ImVec2(center.x - gap/2, center.y + h/2), IM_COL32(255, 255, 255, 180));
                dl->AddRectFilled(ImVec2(center.x + gap/2, center.y - h/2), ImVec2(center.x + gap/2 + bar_w, center.y + h/2), IM_COL32(255, 255, 255, 180));
            } else {
                float off_x = h * 0.3f;
                dl->AddTriangleFilled(ImVec2(center.x - off_x, center.y - h/2), ImVec2(center.x - off_x, center.y + h/2), ImVec2(center.x + off_x * 1.5f, center.y), IM_COL32(255, 255, 255, 180));
            }

            ImGui::GetForegroundDrawList()->AddRect(pos, ImVec2(pos.x + size.x, pos.y + size.y), IM_COL32(200, 200, 200, 255), 5.0f, 0, 2.0f);
            ImGui::PopStyleColor(3);

            if (clicked) {
                bAutoPlaying = !bAutoPlaying;
                if (bAutoPlaying) ClearState();
            }
        } ImGui::End();

        ImGui::PopStyleVar();
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(2);
    }

    void UpdateAutoAimOnly() {
        if (!bAutoAim || bAutoPlaying) return;
        if (!isInGame() || !isPlayerTurn()) return;
        if (g_cache.ballCount <= 0) return;

        cacheCueStats();
        refreshRuntimeTableGeometry();
        ClearState();

        // Run a quick direct-shot scan for auto-aim only
        BallEnums::Classification myclass = getPlayerClassification();
        bool isNineBall = isNineBallRuntime();
        uint nominatedPocket = getNominatedPocket();
        if ((isNineBall && isNineBallOpeningBreak()) || autoPlayIgnoreNominatedPocketWhenSearching())
            nominatedPocket = 6;
        int lowestIdx = -1;
        for (int i = 1; i < gPrediction->guiData.ballsCount; i++) {
            if (gPrediction->guiData.balls[i].originalOnTable) { lowestIdx = i; break; }
        }
        Candidate directShot = findBestDirectShot(myclass, isNineBall, lowestIdx, nominatedPocket, true);
        if (directShot.idx == -1) directShot = findBestDirectShot(myclass, isNineBall, lowestIdx, nominatedPocket, false);

        // 9-ball: try combo shots if no direct shot found
        Candidate bestShot = directShot;
        if (isNineBall && lowestIdx > 0) {
            Candidate comboShot = findBest9BallComboShot(lowestIdx, nominatedPocket);
            if (comboShot.idx != -1 && (bestShot.idx == -1 || comboShot.score < bestShot.score)) {
                bestShot = comboShot;
            }
        }

        if (bestShot.idx != -1) {
            g_CurrentCandidate = bestShot;
            double alignedAngle = normalizeAngle(bestShot.angle);
            setAimAngle(alignedAngle);
            lastSetAngle = alignedAngle;
        }
        updateState(IDLE, "🎯 Auto Aim ready");
    }

    // ── Hidden scan: 6s full-angle sweep, single power (90%) ──
    static void BeginScan() {
        g_scanActive = true;
        g_scanStart = CACurrentMediaTime();
        g_selectStart = 0.0;
        g_scanSamplesDone = 0;
        g_scanTotalSamples = (int)ceil(maxAngle / kScanStepPrimary);
        if (g_scanTotalSamples < 1) g_scanTotalSamples = 1;
        g_bestShot = {-1};
        g_bestQuality = 1e18;
        g_scanResults.clear();
        g_scanResults.reserve((size_t)g_scanTotalSamples + 128);
        g_scanCuePos = {-1000.0, -1000.0};
        resetPredictionCache();
        gPrediction->determineShotResult(true, getAimAngle(), 300.0);
        g_scanCuePos = gPrediction->guiData.balls[0].initialPosition;
    }

    static Candidate pickBestBufferedCandidate(BallEnums::Classification myclass, bool isNineBall, int lowestIdx, uint nominatedPocket) {
        Candidate best = {-1};
        double bestQ = 1e18;
        for (const auto &cand : g_scanResults) {
            if (cand.score >= bestQ) continue;
            if (!canExecuteCandidateNow(cand, myclass, isNineBall, lowestIdx, nominatedPocket)) continue;
            bestQ = cand.score;
            best = cand;
        }
        return best;
    }

    static bool StepScanAndMaybeShoot() {
        if (!g_scanActive) BeginScan();

        BallEnums::Classification myclass = getPlayerClassification();
        bool isNineBall = isNineBallRuntime();
        uint nominatedPocket = getNominatedPocket();
        if ((isNineBall && isNineBallOpeningBreak()) || autoPlayIgnoreNominatedPocketWhenSearching())
            nominatedPocket = 6;
        int lowestIdx = -1;
        for (int i = 1; i < gPrediction->guiData.ballsCount; i++) {
            if (gPrediction->guiData.balls[i].originalOnTable) { lowestIdx = i; break; }
        }

        // Seed from direct-shot geometry, but final decision waits full 6s scan budget.
        if (g_scanSamplesDone == 0) {
            Candidate directShot = findBestDirectShot(myclass, isNineBall, lowestIdx, nominatedPocket, true);
            if (directShot.idx == -1) directShot = findBestDirectShot(myclass, isNineBall, lowestIdx, nominatedPocket, false);
            if (directShot.idx != -1 && directShot.score < g_bestQuality) {
                g_bestShot = directShot;
                g_bestQuality = directShot.score;
            }
            // 9-ball combo seed: find geometric combo shots (cue→lowest→target→pocket)
            if (isNineBall && lowestIdx > 0) {
                Candidate comboShot = findBest9BallComboShot(lowestIdx, nominatedPocket);
                if (comboShot.idx != -1 && comboShot.score < g_bestQuality) {
                    g_bestShot = comboShot;
                    g_bestQuality = comboShot.score;
                }
            }
        }

        Vec2d spin = getShotSpin();
        CFTimeInterval frameStart = CACurrentMediaTime();
        // Fixed single power: max
        const double fixedPower = getFixedPower();

        auto evaluateOne = [&](double angle) {
            gPrediction->determineShotResult(true, angle, fixedPower, spin);
            if (!evaluateShotResultLegality(isNineBall, myclass, lowestIdx)) return;
            if (!isNineBall && !mustShootEightNow(myclass, isNineBall) && gPrediction->guiData.ballsCount > 8) {
                auto &eb = gPrediction->guiData.balls[8];
                if (eb.originalOnTable && !eb.onTable) return;
            }

            int targetIdx = -1;
            int targetPocket = -1;
            if (isNineBall) {
                int firstPocketBall = gPrediction->guiData.firstPocketedBallIndex;
                int firstPocket = gPrediction->guiData.firstPocketedPocketIndex;
                if (firstPocketBall > 0 && firstPocket >= 0 && firstPocket < 6) {
                    if (nominatedPocket < 6 && firstPocket != (int)nominatedPocket) return;
                    targetIdx = firstPocketBall;
                    targetPocket = firstPocket;
                }
            } else {
                for (int i = 1; i < gPrediction->guiData.ballsCount; i++) {
                    auto &ball = gPrediction->guiData.balls[i];
                    if (!ball.originalOnTable || ball.onTable) continue;
                    bool valid = isTargetAllowed(myclass, isNineBall, i, lowestIdx);
                    if (nominatedPocket < 6 && ball.pocketIndex != (int)nominatedPocket) valid = false;
                    if (valid) { targetIdx = i; targetPocket = ball.pocketIndex; break; }
                }
            }
            if (targetIdx < 0 || targetPocket < 0) return;

            auto &cue = gPrediction->guiData.balls[0];
            auto &target = gPrediction->guiData.balls[targetIdx];
            auto &pockets = getPockets();
            Vec2d pocketPos = pockets[targetPocket];
            double cutAngle = calcCutAngle(cue.initialPosition, target.initialPosition, pocketPos);
            double cuePath = pathLength(cue.positions);
            double objPath = pathLength(target.positions);
            int railHits = gPrediction->guiData.railCollisionCount;
            int ballHits = gPrediction->guiData.ballCollisionCount;
            int pointHits = gPrediction->guiData.pointCollisionCount;
            double q = shotQualityScore(isNineBall, targetIdx, lowestIdx, cutAngle, railHits, ballHits, pointHits);
            int cueHazards = countPocketMouthHazards(cue.positions, targetPocket);
            int objHazards = countPocketMouthHazards(target.positions, targetPocket);
            q += cuePath * 0.8 + objPath * 0.3;
            q += cueHazards * 620.0 + objHazards * 420.0;
            if ((cueHazards + objHazards) > 0 && (railHits > 0 || ballHits > 1 || pointHits > 0)) q += 3800.0;
            // 8-ball: penalize potting opponent's balls (gives them free clearance)
            if (!isNineBall && myclass != BallEnums::ANY && !mustShootEightNow(myclass, isNineBall)) {
                for (int obi = 1; obi < gPrediction->guiData.ballsCount; obi++) {
                    if (obi == 8) continue;
                    auto &ob = gPrediction->guiData.balls[obi];
                    if (ob.originalOnTable && !ob.onTable && ob.classification != myclass && ob.classification != BallEnums::EIGHT_BALL)
                        q += 1500.0;
                }
            }
            Candidate evalCand = {targetIdx, angle, q, targetPocket, fixedPower};
            if (!canExecuteCandidateNow(evalCand, myclass, isNineBall, lowestIdx, nominatedPocket)) return;
            g_scanResults.push_back(evalCand);
            if (q < g_bestQuality) {
                g_bestQuality = q;
                g_bestShot = evalCand;
            }
        };

        CFTimeInterval elapsed = frameStart - g_scanStart;
        if (elapsed < 0.0) elapsed = 0.0;
        if (elapsed > kScanCollectSeconds) elapsed = kScanCollectSeconds;
        int shouldDoneByNow = (int)floor((elapsed / kScanCollectSeconds) * g_scanTotalSamples);
        if (shouldDoneByNow > g_scanTotalSamples) shouldDoneByNow = g_scanTotalSamples;
        if (shouldDoneByNow < g_scanSamplesDone) shouldDoneByNow = g_scanSamplesDone;

        // Catch up to required progress: ensures all sample angles are covered by 6s.
        while (g_scanSamplesDone < shouldDoneByNow) {
            double angle = normalizeAngle((double)g_scanSamplesDone * kScanStepPrimary);
            evaluateOne(angle);
            g_scanSamplesDone++;
        }

        CFTimeInterval now = CACurrentMediaTime();
        bool collectFinished = (now - g_scanStart) >= kScanCollectSeconds;
        if (collectFinished && g_scanSamplesDone < g_scanTotalSamples) {
            while (g_scanSamplesDone < g_scanTotalSamples) {
                double angle = normalizeAngle((double)g_scanSamplesDone * kScanStepPrimary);
                evaluateOne(angle);
                g_scanSamplesDone++;
            }
        }
        if (!collectFinished) return false;

        if (g_selectStart <= 0.0) {
            g_selectStart = now;
            updateState(SELECTING, "🧠 Selecting best shot...");
        }

        // Keep filtering from buffered data for a full 1s window before firing.
        Candidate bufferedBest = pickBestBufferedCandidate(myclass, isNineBall, lowestIdx, nominatedPocket);
        if (bufferedBest.idx != -1) {
            g_bestShot = bufferedBest;
            g_bestQuality = bufferedBest.score;
        }

        if ((now - g_selectStart) < kScanSelectSeconds) return false;

        g_scanActive = false;
        if (g_bestShot.idx != -1 && canExecuteCandidateNow(g_bestShot, myclass, isNineBall, lowestIdx, nominatedPocket)) {
            g_CurrentCandidate = g_bestShot;
            Shoot(g_bestShot.angle, g_bestShot.power);
        } else {
            lastFailedCuePos = g_scanCuePos;
            updateState(IDLE, "⏸ No shot found");
        }
        return true;
    }

    void Update() {
        DrawToggleButton();

        if (!isInGame() || g_cache.ballCount <= 0) {
            if (g_cueStatsCached) {
                g_cueStatsCached = false;
                g_cachedCueId = -1;
                g_effectiveMaxPower = 888.85;
            }
            return;
        }

        if (bAutoAim && !bAutoPlaying) {
            UpdateAutoAimOnly();
            return;
        }
        cacheCueStats();
        refreshRuntimeTableGeometry();

        if (!bAutoPlaying || !isPlayerTurn()) {
            if (state == SHOOTING || state == SCANNING || state == SELECTING) {
                deactivateCue();
            }
            updateState(IDLE, "⏸ Idle");
            return;
        }

        if (state == IDLE) {
            ClearState();
            resetPredictionCache();
            updateState(SCANNING, "Scanning...");
        }
        if (state == SCANNING || state == SELECTING) {
            StepScanAndMaybeShoot();
        }
        if (state == NOMINATING) {
            nominationFrameCounter++;
            if (nominationFrameCounter == 5) {
                if (g_CurrentCandidate.pocketIndex >= 0 && g_CurrentCandidate.pocketIndex < 6) {
                    nominatePocket(g_CurrentCandidate.pocketIndex);
                }
            }
            if (nominationFrameCounter > 10 && g_CurrentCandidate.pocketIndex >= 0 && g_CurrentCandidate.pocketIndex < 6 &&
                getNominatedPocket() == (uint)g_CurrentCandidate.pocketIndex) {
                takeShot(pendingShotAngle, pendingShotPower);
            } else if (nominationFrameCounter > 90) {
                // Pocket nomination did not synchronize in time; abort this cycle and rescan.
                ClearState();
                updateState(IDLE, "⏸ Pocket sync retry");
            }
        }
        if (state == SHOOTING) {
            shootingFrameCounter++;
            CGPoint shotPt = getShotTouchPoint();
            if (shootingFrameCounter == 1) {
                setAimAngle(pendingShotAngle);
                setPower(pendingShotPower);
                shotTouchBegin(shotPt);
            }
            if (shootingFrameCounter >= 2 && shootingFrameCounter <= 9) {
                setAimAngle(pendingShotAngle);
                setPower(pendingShotPower);
            }
            if (shootingFrameCounter == 10) {
                setAimAngle(pendingShotAngle);
                setPower(pendingShotPower);
                shotTouchEndWithValues(shotPt, pendingShotAngle, pendingShotPower);
            }
            if (shootingFrameCounter > 10 && shootingFrameCounter <= 25) {
                setAimAngle(pendingShotAngle);
                setPower(pendingShotPower);
            }
            if (shootingFrameCounter > 30 && !isPlayerTurn()) {
                ClearState();
                state = IDLE;
            }
            if (shootingFrameCounter > 300) {
                ClearState();
                state = IDLE;
            }
        }
    }
}; // namespace AutoPlay


namespace AutoQueue {
    bool bAutoQueue = false;
    int queueMode = 0;           // 0 = rematch last tier, 1 = auto tier by bet %
    int betPercent = 50;         // % of coins to bet (mode 1)
    bool counting = false;
    CFTimeInterval countdownStart = 0;
    CFTimeInterval lastCallTime = 0;

    // Tier bets (matching Android jni/AutoQueue.h modeBets)
    struct TierBet { const char *tier; int64_t bet; };
    static const TierBet kTierBets[] = {
        {"M1",        50},
        {"M2",       100},
        {"M3",       500},
        {"M4",      2500},
        {"M5",     10000},
        {"M6",     50000},
        {"M7",    100000},
        {"M8",    250000},
        {"M9",    500000},
        {"M10",  1000000},
        {"M11",  2500000},
        {"M12",  4000000},
        {"M13",  5000000},
        {"M14", 10000000},
        {"M15", 15000000},
        {"M16", 25000000},
        {"M17",100000000},
    };
    static const int kNumTiers = sizeof(kTierBets) / sizeof(kTierBets[0]);

    // Read current menu state id from MenuManager
    static int getMenuStateId() {
        if (!isValidPtr(g_menuManager)) return -1;
        @try {
            id mm = (__bridge id)(void *)g_menuManager;
            SEL sel = sel_registerName("getMenuStateId");
            if ([mm respondsToSelector:sel]) {
                return (int)((NSInteger(*)(id, SEL))objc_msgSend)(mm, sel);
            }
        } @catch (...) {}
        return -1;
    }

    // Read user coins from UserInfo singleton
    static int64_t getUserCoins() {
        if (!isValidPtr(g_userInfo)) return 0;
        @try {
            id ui = (__bridge id)(void *)g_userInfo;
            SEL sel = sel_registerName("coins");
            if ([ui respondsToSelector:sel]) {
                return (int64_t)((NSUInteger(*)(id, SEL))objc_msgSend)(ui, sel);
            }
        } @catch (...) {}
        return 0;
    }

    // Pop menu state (close popup)
    static void popMenuState(int stateId) {
        if (!isValidPtr(g_menuManager)) return;
        @try {
            id mm = (__bridge id)(void *)g_menuManager;
            SEL sel = sel_registerName("popMenuState:");
            if ([mm respondsToSelector:sel]) {
                ((void(*)(id, SEL, NSInteger))objc_msgSend)(mm, sel, (NSInteger)stateId);
            }
        } @catch (...) {}
    }

    // Walk the menu state stack, find first instance of given class name,
    // and call a no-arg selector on it. Dispatches to main thread for safety.
    static bool callSelectorOnMenuStateOfClass(const char *className, const char *selName) {
        if (!isValidPtr(g_menuManager)) return false;
        NSString *cls = [NSString stringWithUTF8String:className];
        NSString *sel = [NSString stringWithUTF8String:selName];
        dispatch_async(dispatch_get_main_queue(), ^{
            @try {
                if (!isValidPtr(g_menuManager)) return;
                id mm = (__bridge id)(void *)g_menuManager;
                if (!isObjCObj((ptr)mm)) return;
                SEL smSel = sel_registerName("getMenuStateManager");
                if (![mm respondsToSelector:smSel]) return;
                id sm = ((id(*)(id, SEL))objc_msgSend)(mm, smSel);
                if (!sm || !isObjCObj((ptr)sm)) return;

                SEL stackSel = sel_registerName("currentMenuStateStack");
                if (![sm respondsToSelector:stackSel]) return;
                NSArray *stack = ((NSArray*(*)(id, SEL))objc_msgSend)(sm, stackSel);
                if (!stack || ![stack isKindOfClass:[NSArray class]]) return;

                Class target = objc_getClass([cls UTF8String]);
                if (!target) return;

                SEL callSel = sel_registerName([sel UTF8String]);
                for (id state in stack) {
                    if (!state || !isObjCObj((ptr)state)) continue;
                    if ([state isKindOfClass:target]) {
                        if ([state respondsToSelector:callSel]) {
                            ((void(*)(id, SEL))objc_msgSend)(state, callSel);
                            return;
                        }
                    }
                }
            } @catch (...) {}
        });
        return true;
    }

    // Pick tier code based on bet % of coins (mode 1) — SAME as Android
    static const char *pickTierForBetPercent(int64_t coins, int percent) {
        int64_t maxBet = coins * percent / 100;
        const char *selected = kTierBets[0].tier;
        for (int i = 0; i < kNumTiers; i++) {
            if (maxBet >= kTierBets[i].bet) selected = kTierBets[i].tier;
        }
        return selected;
    }

    static void startNextMatch() {
        callSelectorOnMenuStateOfClass("MenuPlayerMatch", "rematchButtonCallback");
    }

    // Main update — called every frame while menu is shown (not in game)
    void Update() {
        if (!bAutoQueue) {
            counting = false;
            return;
        }
        // Only trigger when not in game AND not in queue
        if (isInGame()) { counting = false; return; }

        int menuState = getMenuStateId();
        bool inQueue = (menuState == 12); // same as Android
        if (inQueue) { counting = false; return; }

        CFTimeInterval now = CACurrentMediaTime();
        if ((now - lastCallTime) > 0.5) counting = false;
        lastCallTime = now;

        if (!counting) {
            counting = true;
            countdownStart = now;
        }

        int remainingMs = (int)(3000.0 - (now - countdownStart) * 1000.0);

        if (remainingMs <= 0) {
            // Pop state 13 (match end popup) if present, like Android
            if (menuState == 13) popMenuState(13);
            startNextMatch();
            counting = false;
            return;
        }

        // Draw countdown overlay
        ImGuiIO &io = ImGui::GetIO();
        ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x/2, io.DisplaySize.y/2), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(360, 200), ImGuiCond_Always);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.10f, 0.10f, 0.12f, 0.95f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 16.0f);
        if (ImGui::Begin("##AutoQueue", nullptr,
            ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoSavedSettings))
        {
            ImDrawList *dl = ImGui::GetWindowDrawList();
            ImVec2 p = ImGui::GetWindowPos();
            ImVec2 s = ImGui::GetWindowSize();
            dl->AddRectFilled(p, ImVec2(p.x+s.x, p.y+50), IM_COL32(40,100,180,255), 16.0f, ImDrawFlags_RoundCornersTop);
            ImGui::Dummy(ImVec2(0, 8));
            const char *title = "Auto Queue";
            ImVec2 ts = ImGui::CalcTextSize(title);
            ImGui::SetCursorPosX((s.x - ts.x) * 0.5f);
            ImGui::TextColored(ImVec4(1,1,1,1), "%s", title);

            ImGui::Dummy(ImVec2(0, 20));
            char buf[16]; snprintf(buf, sizeof(buf), "%d", (remainingMs / 1000) + 1);
            ImGui::SetWindowFontScale(3.0f);
            ImVec2 cs = ImGui::CalcTextSize(buf);
            ImGui::SetCursorPosX((s.x - cs.x) * 0.5f);
            ImGui::TextColored(ImVec4(0.35f, 0.7f, 1.0f, 1.0f), "%s", buf);
            ImGui::SetWindowFontScale(1.0f);

            ImGui::Dummy(ImVec2(0, 10));
            ImGui::SetCursorPosX((s.x - 120) * 0.5f);
            if (ImGui::Button("Cancel", ImVec2(120, 36))) {
                bAutoQueue = false;
                counting = false;
            }
        }
        ImGui::End();
        ImGui::PopStyleVar();
        ImGui::PopStyleColor();
    }
}; // namespace AutoQueue

// ============================================================================
// Colors for ball prediction lines
// ============================================================================

static const ImColor g_ballColors[16] = {
    ImColor(1.f,1.f,1.f), ImColor(1.f,1.f,0.f), ImColor(0.f,0.f,1.f),
    ImColor(1.f,0.f,0.f), ImColor(0.5f,0.f,0.5f), ImColor(1.f,0.65f,0.f),
    ImColor(0.f,0.5f,0.f), ImColor(0.5f,0.f,0.f), ImColor(0.f,0.f,0.f),
    ImColor(1.f,1.f,0.f), ImColor(0.f,0.f,1.f), ImColor(1.f,0.f,0.f),
    ImColor(0.5f,0.f,0.5f), ImColor(1.f,0.65f,0.f), ImColor(0.f,0.5f,0.f),
    ImColor(0.5f,0.f,0.f),
};

// ============================================================================
// Settings
// ============================================================================

static int g_menuTab = 0;
static bool g_espEnabled = true;
static bool g_drawPredictionLine = true;
static bool g_drawPockets = true;
static bool g_drawPocketsShotState = true;
static float g_lineWidth = 1.0f;
static bool g_noScreenShot = false;

// Preview helpers: pocket / cue line coloring (player turn prediction only)
static bool espPreviewMustShootEightNow(BallEnums::Classification myclass, bool isNineBall) {
    if (isNineBall) return false;
    if (myclass == BallEnums::EIGHT_BALL) return true;
    if (myclass != BallEnums::SOLID && myclass != BallEnums::STRIPE) return false;
    for (int i = 1; i < gPrediction->guiData.ballsCount; i++) {
        if (i == 8) continue;
        auto &b = gPrediction->guiData.balls[i];
        if (b.originalOnTable && b.classification == myclass) return false;
    }
    return true;
}

static bool espPreviewIsIllegalObjectPot(BallEnums::Classification myclass, bool isNine, int ballIdx) {
    if (ballIdx <= 0 || ballIdx >= gPrediction->guiData.ballsCount) return false;
    auto &ball = gPrediction->guiData.balls[ballIdx];
    if (isNine) return false;
    if (ballIdx == 8)
        return !espPreviewMustShootEightNow(myclass, isNine);
    if (myclass == BallEnums::SOLID && ball.classification == BallEnums::STRIPE) return true;
    if (myclass == BallEnums::STRIPE && ball.classification == BallEnums::SOLID) return true;
    return false;
}

static bool espPreviewPocketIsIllegal(int pocketIdx) {
    if (pocketIdx < 0 || pocketIdx >= TABLE_POCKETS_COUNT) return false;
    auto &cue = gPrediction->guiData.balls[0];
    if (!cue.onTable && cue.pocketIndex == pocketIdx) return true;
    BallEnums::Classification myclass = getPlayerClassification();
    bool isNine = isNineBallRuntime();
    for (int b = 1; b < gPrediction->guiData.ballsCount; b++) {
        auto &ball = gPrediction->guiData.balls[b];
        if (ball.onTable || !ball.originalOnTable) continue;
        if (ball.pocketIndex != pocketIdx) continue;
        if (espPreviewIsIllegalObjectPot(myclass, isNine, b)) return true;
    }
    return false;
}

static bool espPreviewNineBallWrongFirstHit() {
    if (!isNineBallRuntime()) return false;
    int low = -1;
    for (int i = 1; i < gPrediction->guiData.ballsCount; i++) {
        if (gPrediction->guiData.balls[i].originalOnTable) {
            low = i;
            break;
        }
    }
    if (low < 0) return false;
    auto *fh = gPrediction->guiData.collision.firstHitBall;
    return !fh || fh->index != low;
}

static void autoAimScan(void) {
    // Called from main thread timer — no-op, AutoPlay runs in render loop
}

// ============================================================================
// ESP_Update — main frame update (called every frame from Metal render loop)
// ============================================================================

static void ESP_Update(ImDrawList *draw) {
    ImGuiIO &io = ImGui::GetIO();
    g_screenW = io.DisplaySize.x;
    g_screenH = io.DisplaySize.y;

    // Singletons resolved on main thread (see ESP_InitPool)
    // NO ObjC calls here — render thread only does raw memory reads

    if (!isInGame()) return;
    if (g_cache.ballCount <= 0) return;

    // Extra safety: verify first ball pointer is readable
    if (!isValidPtr(g_cache.ballPtrs[0])) return;

    @try {

    UpdateScreenTable();

    ptr table = GM_Table();
    if (!isValidPtr(table)) return;

    // Draw pockets (use static positions — they never change)
    auto &pockets = getPockets();
    if (g_drawPockets) {
        for (int i = 0; i < 6; i++) {
            auto sp = WorldToScreen(pockets[i]);
            float pr = fmaxf(4.f, (float)(POCKET_RADIUS * SCREEN_TABLE_SCALE * 0.6));
            draw->AddCircle(sp, pr, IM_COL32(255,255,255,180), 0, 1.5f);
        }
    }


    int32_t stateId = g_cache.gmStateId;
    if (stateId == 4) {
        // Player's turn — run prediction
        double angle = normalizeAngle(getAimAngle());
        double power = getShotPower();
        Vec2d spin = getShotSpin();
        gPrediction->determineShotResult(false, angle, power, spin);
    }

    // Don't draw during ball rolling or opponent turn
    if (stateId == 6 || stateId == 7 || stateId == 8) return;

    // Draw pocket shot state: green = legal pot preview, red = scratch or illegal group pot
    bool hidePlanningPreview = false;
    if (g_drawPocketsShotState && !hidePlanningPreview) {
        for (int i = 0; i < 6; i++) {
            if (Prediction::pocketStatus[i]) {
                auto sp = WorldToScreen(pockets[i]);
                float pr = fmaxf(4.f, (float)(POCKET_RADIUS * SCREEN_TABLE_SCALE * 0.6));
                bool bad = (stateId == 4) && espPreviewPocketIsIllegal(i);
                ImU32 pc = bad ? IM_COL32(255, 55, 55, 255) : IM_COL32(0, 255, 0, 255);
                draw->AddCircle(sp, pr, pc, 0, 2.f);
            }
        }
    }

    // Draw prediction lines (9-ball: cue path thick red if first hit is not lowest ball on table)
    if (g_drawPredictionLine && !hidePlanningPreview) {
        const bool nineWrongFirst = (stateId == 4 && espPreviewNineBallWrongFirstHit());
        for (int i = 0; i < gPrediction->guiData.ballsCount; i++) {
            auto &ball = gPrediction->guiData.balls[i];
            if (ball.initialPosition == ball.predictedPosition) continue;

            const bool cueBadNine = (i == 0 && nineWrongFirst);
            ImU32 col = cueBadNine ? IM_COL32(255, 60, 60, 255) : (ImU32)g_ballColors[i % 16];
            const float lineW = cueBadNine ? (g_lineWidth * 3.0f) : g_lineWidth;

            // Draw trajectory line
            ImVec2 lastPos = {};
            for (size_t j = 1; j < ball.positions.size(); j++) {
                auto pt = WorldToScreen(ball.positions[j]);
                if (lastPos.x != 0 || lastPos.y != 0)
                    draw->AddLine(lastPos, pt, col, lineW);
                lastPos = pt;
            }

            // Draw start/end markers
            float br = fmaxf(3.f, (float)(BALL_RADIUS * SCREEN_TABLE_SCALE * 0.5));
            draw->AddCircle(WorldToScreen(ball.initialPosition), br, col, 0, 1.5f);
            draw->AddCircleFilled(WorldToScreen(ball.predictedPosition), br, col);
        }
    }

    } @catch (NSException *e) {
        NSLog(@"[8BP] ESP_Update exception: %@", e);
    }
}

// ============================================================================
// Style
// ============================================================================

static void ESP_ApplyStyle(void) {
    ImGuiStyle &s = ImGui::GetStyle();
    s.WindowPadding     = ImVec2(12, 12);
    s.FramePadding      = ImVec2(8, 4);
    s.ItemSpacing       = ImVec2(8, 6);
    s.ItemInnerSpacing  = ImVec2(6, 4);
    s.ScrollbarSize     = 12.f;
    s.GrabMinSize       = 8.f;
    s.WindowBorderSize  = 1.f;
    s.FrameBorderSize   = 0.f;
    s.TabBorderSize     = 0.f;
    s.WindowRounding    = 10.f;
    s.FrameRounding     = 6.f;
    s.GrabRounding      = 6.f;
    s.ScrollbarRounding = 6.f;
    s.TabRounding       = 6.f;
    s.Alpha = 0.97f;

    ImVec4 *c = s.Colors;
    // Background
    c[ImGuiCol_WindowBg]             = ImVec4(0.08f, 0.08f, 0.10f, 0.96f);
    c[ImGuiCol_ChildBg]              = ImVec4(0.10f, 0.10f, 0.12f, 0.60f);
    c[ImGuiCol_PopupBg]              = ImVec4(0.08f, 0.08f, 0.10f, 0.96f);
    // Title
    c[ImGuiCol_TitleBg]              = ImVec4(0.06f, 0.06f, 0.08f, 1.00f);
    c[ImGuiCol_TitleBgActive]        = ImVec4(0.10f, 0.10f, 0.14f, 1.00f);
    c[ImGuiCol_TitleBgCollapsed]     = ImVec4(0.06f, 0.06f, 0.08f, 0.60f);
    // Tabs
    c[ImGuiCol_Tab]                  = ImVec4(0.12f, 0.12f, 0.16f, 1.00f);
    c[ImGuiCol_TabHovered]           = ImVec4(0.24f, 0.42f, 0.72f, 0.80f);
    c[ImGuiCol_TabActive]            = ImVec4(0.20f, 0.36f, 0.64f, 1.00f);
    c[ImGuiCol_TabUnfocused]         = ImVec4(0.10f, 0.10f, 0.14f, 1.00f);
    c[ImGuiCol_TabUnfocusedActive]   = ImVec4(0.16f, 0.28f, 0.50f, 1.00f);
    // Frame
    c[ImGuiCol_FrameBg]              = ImVec4(0.14f, 0.14f, 0.18f, 1.00f);
    c[ImGuiCol_FrameBgHovered]       = ImVec4(0.20f, 0.20f, 0.26f, 1.00f);
    c[ImGuiCol_FrameBgActive]        = ImVec4(0.24f, 0.24f, 0.30f, 1.00f);
    // Button
    c[ImGuiCol_Button]               = ImVec4(0.20f, 0.36f, 0.64f, 1.00f);
    c[ImGuiCol_ButtonHovered]        = ImVec4(0.26f, 0.44f, 0.74f, 1.00f);
    c[ImGuiCol_ButtonActive]         = ImVec4(0.16f, 0.30f, 0.54f, 1.00f);
    // Check/Slider
    c[ImGuiCol_CheckMark]            = ImVec4(0.40f, 0.70f, 1.00f, 1.00f);
    c[ImGuiCol_SliderGrab]           = ImVec4(0.30f, 0.55f, 0.90f, 1.00f);
    c[ImGuiCol_SliderGrabActive]     = ImVec4(0.40f, 0.65f, 1.00f, 1.00f);
    // Header
    c[ImGuiCol_Header]               = ImVec4(0.16f, 0.28f, 0.50f, 0.60f);
    c[ImGuiCol_HeaderHovered]        = ImVec4(0.20f, 0.36f, 0.64f, 0.80f);
    c[ImGuiCol_HeaderActive]         = ImVec4(0.24f, 0.42f, 0.72f, 1.00f);
    // Border / Separator
    c[ImGuiCol_Border]               = ImVec4(0.20f, 0.20f, 0.26f, 0.50f);
    c[ImGuiCol_BorderShadow]         = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    c[ImGuiCol_Separator]            = ImVec4(0.24f, 0.24f, 0.30f, 0.60f);
    c[ImGuiCol_SeparatorHovered]     = ImVec4(0.30f, 0.55f, 0.90f, 0.80f);
    c[ImGuiCol_SeparatorActive]      = ImVec4(0.40f, 0.70f, 1.00f, 1.00f);
    // Scrollbar
    c[ImGuiCol_ScrollbarBg]          = ImVec4(0.06f, 0.06f, 0.08f, 0.40f);
    c[ImGuiCol_ScrollbarGrab]        = ImVec4(0.24f, 0.24f, 0.30f, 1.00f);
    c[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.30f, 0.30f, 0.38f, 1.00f);
    c[ImGuiCol_ScrollbarGrabActive]  = ImVec4(0.36f, 0.36f, 0.44f, 1.00f);
    // Resize grip
    c[ImGuiCol_ResizeGrip]           = ImVec4(0.20f, 0.36f, 0.64f, 0.40f);
    c[ImGuiCol_ResizeGripHovered]    = ImVec4(0.26f, 0.44f, 0.74f, 0.60f);
    c[ImGuiCol_ResizeGripActive]     = ImVec4(0.40f, 0.70f, 1.00f, 0.80f);
    // Text
    c[ImGuiCol_Text]                 = ImVec4(0.90f, 0.90f, 0.94f, 1.00f);
    c[ImGuiCol_TextDisabled]         = ImVec4(0.45f, 0.45f, 0.50f, 1.00f);
}

// ============================================================================
// Menu
// ============================================================================

static void ESP_DrawMenu(void) {
    if (ImGui::BeginTabBar("##8BPTabs")) {

        if (ImGui::BeginTabItem(ICON_FA_EYE " ESP")) {
            ImGui::Spacing();
            ImGui::Checkbox("Enable ESP", &g_espEnabled);
            ImGui::Separator(); ImGui::Spacing();
            ImGui::Checkbox("Prediction Lines", &g_drawPredictionLine);
            ImGui::Checkbox("Draw Pockets", &g_drawPockets);
            ImGui::Checkbox("Pocket Shot State", &g_drawPocketsShotState);
            ImGui::Separator(); ImGui::Spacing();
            ImGui::SliderFloat("Line Width", &g_lineWidth, 0.5f, 5.0f, "%.1f");
            ImGui::Separator(); ImGui::Spacing();
            ImGui::Text("Table Calibration (Offsets)");
            ImGui::SliderFloat("Offset X", &g_tableOffsetX, -50.0f, 50.0f, "%.1f");
            ImGui::SliderFloat("Offset Y", &g_tableOffsetY, -50.0f, 50.0f, "%.1f");
            ImGui::SliderFloat("Scale Factor", &g_tableScaleFactor, 0.900f, 1.100f, "%.3f");
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem(ICON_FA_GAMEPAD " Auto")) {
            ImGui::Spacing();
            ImGui::Checkbox("Auto Aim (only rotate cue)", &AutoPlay::bAutoAim);
            ImGui::Checkbox("AutoPlay", &AutoPlay::bAutoPlaying);
            if (ImGui::IsItemClicked() && AutoPlay::bAutoPlaying) AutoPlay::ClearState();
            ImGui::TextColored(ImVec4(0.5f,0.5f,0.5f,1.0f), "State: %s",
                AutoPlay::state == AutoPlay::IDLE ? "Idle" :
                AutoPlay::state == AutoPlay::SCANNING ? "Scanning" :
                AutoPlay::state == AutoPlay::SELECTING ? "Selecting" :
                AutoPlay::state == AutoPlay::NOMINATING ? "Nominating" : "Shooting");
            ImGui::TextColored(ImVec4(0.5f,0.8f,1.0f,1.0f), "Detected Rule: %s",
                isNineBallRuntime() ? "9-Ball" : "8-Ball");
            ImGui::TextColored(ImVec4(0.6f,0.6f,0.6f,1.0f), "Call pocket every shot: %s",
                isCallPocketEveryShotMode() ? "YES" : "NO");
            ImGui::TextColored(ImVec4(0.5f,0.5f,0.5f,1.0f), "Power: MAX | Scan: 7s + Select: 1s");
            if (g_CurrentCandidate.idx != -1) {
                ImGui::TextColored(ImVec4(0.3f,1.f,0.3f,1.f), "Target: Ball %d -> Pocket %d",
                    g_CurrentCandidate.idx, g_CurrentCandidate.pocketIndex);
            }

            ImGui::Separator(); ImGui::Spacing();
            ImGui::Checkbox("AutoQueue (Rematch)", &AutoQueue::bAutoQueue);
            if (AutoQueue::bAutoQueue) {
                ImGui::TextColored(ImVec4(0.6f,0.6f,0.6f,1.0f), "Auto-rematch sau khi match kết thúc");
                int ms = AutoQueue::getMenuStateId();
                ImGui::TextColored(ImVec4(0.5f,0.5f,0.5f,1.0f), "MenuState: %d", ms);
            }
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem(ICON_FA_COG " Misc")) {
            ImGui::Spacing();
            ImGui::Checkbox("I'm Not Hacker", &g_noScreenShot);
            ImGui::Separator(); ImGui::Spacing();
            ImGui::Text("GameManager: 0x%llX %s", (unsigned long long)g_gameManager, isValidPtr(g_gameManager) ? "OK" : "N/A");
            ImGui::Text("MainManager: 0x%llX", (unsigned long long)g_mainManager);
            ImGui::Text("In Game: %s", isInGame() ? "Yes" : "No");
            ImGui::Text("Player Turn: %s", isPlayerTurn() ? "Yes" : "No");
            ImGui::Text("GM StateID: %d", g_cache.gmStateId);
            ImGui::Text("MM StateID: %d", g_cache.mmStateId);
            ImGui::Text("mGameMode: %d", getRuntimeGameMode());
            ImGui::Text("Mode name: %s", gameModeName(getRuntimeGameMode()));
            ImGui::Text("is9BallGame(): %s", g_cache.is9BallByGameApi ? "true" : "false");
            ImGui::Text("Rule detect: %s", isNineBallRuntime() ? "9-Ball" : "8-Ball");
            ImGui::Text("Call pocket mode: %s", isCallPocketEveryShotMode() ? "All shots" : "Normal");
            ImGui::Text("Table: 0x%llX", (unsigned long long)GM_Table());
            ImGui::Text("Balls: %d", getTableBallCount(GM_Table()));
            ImGui::Text("FPS: %.0f", ImGui::GetIO().Framerate);
            ImGui::Separator();
            if (ImGui::TreeNode("Cue Power/Spin (FULL DUMP)")) {
                const char *srcName[] = {"Fallback", "Interp", "Table"};
                auto &d = g_diag;
                // ── EFFECTIVE (what ESP uses) ──
                ImGui::TextColored(ImVec4(0.3f,1.0f,0.3f,1.0f), "EFFECTIVE MaxPower: %.4f [%s]", g_effectiveMaxPower, srcName[g_powerSource]);
                ImGui::TextColored(ImVec4(0.3f,1.0f,0.3f,1.0f), "EFFECTIVE SpinMul:  %.11f [%s]", CUE_PROPERTIES_SPIN_VALUE, srcName[g_spinSource]);
                ImGui::TextColored(ImVec4(0.3f,1.0f,0.3f,1.0f), "ShotPower (computed): %.4f", d.computedShotPower);
                ImGui::TextColored(ImVec4(0.3f,1.0f,0.3f,1.0f), "ShotSpin  (computed): (%.6f, %.6f)", d.computedSpinX, d.computedSpinY);
                ImGui::Separator();
                // ── UserInfo server-sent ratios ──
                ImGui::TextColored(ImVec4(1.0f,0.8f,0.3f,1.0f), "UserInfo Ratios (server-sent):");
                ImGui::Text("  Power: low=%d high=%d", d.lowPowerRatio, d.highPowerRatio);
                ImGui::Text("  Spin:  low=%d high=%d", d.lowSpinRatio, d.highSpinRatio);
                ImGui::Text("  Aim:   low=%d high=%d", d.lowAimRatio, d.highAimRatio);
                ImGui::Text("  Interp Power: %.4f", d.interpPower);
                ImGui::Text("  Interp Spin:  %.4f (raw int)", d.interpSpin);
                ImGui::Separator();
                // ── CueStats ──
                ImGui::TextColored(ImVec4(0.5f,0.8f,1.0f,1.0f), "CueId: %d", g_cachedCueId);
                ImGui::Text("Base  (getCueStats):       P=%d S=%d A=%d T=%d", d.basePower, d.baseSpin, d.baseAim, d.baseTime);
                ImGui::Text("Bonus (getCueStatsBonus):  P=%d S=%d A=%d T=%d", d.bonusPower, d.bonusSpin, d.bonusAim, d.bonusTime);
                ImGui::Text("Table Power[%d]: %.4f", d.bonusPower, d.tablePowerLookup);
                ImGui::Text("Table Spin[%d]:  %.11f", d.bonusSpin, d.tableSpinLookup);
                ImGui::Separator();
                // ── VisualCue (ALL power ivars) ──
                ImGui::TextColored(ImVec4(1.0f,0.6f,0.6f,1.0f), "VisualCue ivars:");
                ImGui::Text("  mPower: %.6f (0-1)", d.vc_mPower);
                ImGui::Text("  mCurrentPowerOffset: %.4f", d.vc_mCurrentPowerOffset);
                ImGui::Text("  mMaxCuePowerOffset:  %.4f", d.vc_mMaxCuePowerOffset);
                ImGui::Text("  mPowerMode: %s", d.vc_mPowerMode ? "Y" : "N");
                ImGui::Text("  powerControlActive: %s", d.vc_powerControlActive ? "Y" : "N");
                ImGui::Text("  mPowerSliderVisible: %s", d.vc_mPowerSliderVisible ? "Y" : "N");
                ImGui::Text("  mControlMode: %d", d.vc_mControlMode);
                ImGui::Text("  mMaxAngleForPowerMode: %.4f", d.vc_mMaxAngleForPowerMode);
                ImGui::Text("  mDistanceFromTip: %.4f", d.vc_mDistanceFromTip);
                ImGui::Text("  mAnimatePowerTarget: %.6f", d.vc_mAnimatePowerTarget);
                ImGui::Text("  mUserSensitivity: %.4f", d.vc_mUserSensitivity);
                ImGui::Separator();
                // ── GamePowerBarView ──
                ImGui::TextColored(ImVec4(1.0f,0.6f,0.6f,1.0f), "GamePowerBarView:");
                ImGui::Text("  getMinimumPower: %.6f", d.pbv_minimumPower);
                ImGui::Text("  getPowerSliderDistance: %.4f", d.pbv_sliderDistance);
                ImGui::Separator();
                // ── VisualEnglishControl (ALL spin ivars) ──
                ImGui::TextColored(ImVec4(0.6f,0.6f,1.0f,1.0f), "VisualEnglishControl ivars:");
                ImGui::Text("  mEnglish: (%.6f, %.6f)", d.vec_englishX, d.vec_englishY);
                ImGui::Text("  mSpinLevel: %u", d.vec_spinLevel);
                ImGui::Text("  mInputRadius: %.4f", d.vec_mInputRadius);
                ImGui::TreePop();
            }
            ImGui::Separator();
            if (ImGui::TreeNode("Resolved Offsets")) {
                ImGui::Text("GM.mTable: 0x%lX", (long)g_off.gm_mTable);
                ImGui::Text("GM.mVisualCue: 0x%lX", (long)g_off.gm_mVisualCue);
                ImGui::Text("GM.mStateManager: 0x%lX", (long)g_off.gm_mStateManager);
                ImGui::Text("Tbl.mBalls: 0x%lX", (long)g_off.tbl_mBalls);
                ImGui::Text("Tbl.mTableProperties: 0x%lX", (long)g_off.tbl_mTableProperties);
                ImGui::Text("Ball._physicsProperties: 0x%lX", (long)g_off.ball_physicsProperties);
                ImGui::Text("Ball.classification: 0x%lX", (long)g_off.ball_classification);
                ImGui::Text("Ball.state: 0x%lX", (long)g_off.ball_state);
                ImGui::Text("VC.mVisualGuide: 0x%lX", (long)g_off.vc_mVisualGuide);
                ImGui::Text("VC.mPower: 0x%lX", (long)g_off.vc_mPower);
                ImGui::Text("SM.mStateStack: 0x%lX", (long)g_off.sm_mStateStack);
                ImGui::Text("MM.mStateManager: 0x%lX", (long)g_off.mm_mStateManager);
                ImGui::Text("VC.mMaxCuePowerOffset: 0x%lX", (long)g_off.vc_mMaxCuePowerOffset);
                ImGui::Text("VEC.mSpinLevel: 0x%lX", (long)g_off.vec_spinLevel);
                ImGui::TreePop();
            }
            ImGui::Separator(); ImGui::Spacing();
            static bool logSaved = false;
            static char logStatus[128] = "";
            if (ImGui::Button("Save Log to Documents", ImVec2(-1, 40))) {
                @try {
                    NSMutableString *log = [NSMutableString string];
                    [log appendFormat:@"=== 8BP Debug Log ===\n"];
                    [log appendFormat:@"Date: %@\n", [NSDate date]];
                    [log appendFormat:@"poolBase: 0x%llX  slide: 0x%lX\n", (unsigned long long)g_poolBase, g_poolSlide];
                    [log appendFormat:@"GameManager: 0x%llX\n", (unsigned long long)g_gameManager];
                    [log appendFormat:@"MainManager: 0x%llX\n", (unsigned long long)g_mainManager];
                    [log appendFormat:@"MenuManager: 0x%llX\n", (unsigned long long)g_menuManager];
                    [log appendFormat:@"UserInfo: 0x%llX\n", (unsigned long long)g_userInfo];
                    [log appendFormat:@"InGame: %d  PlayerTurn: %d\n", isInGame(), isPlayerTurn()];
                    [log appendFormat:@"GM_StateID: %d  MM_StateID: %d\n", g_cache.gmStateId, g_cache.mmStateId];
                    [log appendFormat:@"mGameMode: %d (%s)\n", getRuntimeGameMode(), gameModeName(getRuntimeGameMode())];
                    [log appendFormat:@"is9BallGame(): %d  RuleDetected: %s\n", g_cache.is9BallByGameApi ? 1 : 0, isNineBallRuntime() ? "9-Ball" : "8-Ball"];
                    [log appendFormat:@"CallPocketEveryShot: %d\n", isCallPocketEveryShotMode() ? 1 : 0];
                    [log appendFormat:@"Table: 0x%llX  Balls: %d\n", (unsigned long long)GM_Table(), getTableBallCount(GM_Table())];
                    [log appendFormat:@"\n=== Resolved Offsets ===\n"];
                    [log appendFormat:@"GM.mTable: 0x%lX\n", (long)g_off.gm_mTable];
                    [log appendFormat:@"GM.mVisualCue: 0x%lX\n", (long)g_off.gm_mVisualCue];
                    [log appendFormat:@"GM.mVisualEnglishControl: 0x%lX\n", (long)g_off.gm_mVisualEnglishControl];
                    [log appendFormat:@"GM.mStateManager: 0x%lX\n", (long)g_off.gm_mStateManager];
                    [log appendFormat:@"GM._rules: 0x%lX\n", (long)g_off.gm_rules];
                    [log appendFormat:@"Tbl.mTableProperties: 0x%lX\n", (long)g_off.tbl_mTableProperties];
                    [log appendFormat:@"Tbl.mBalls: 0x%lX\n", (long)g_off.tbl_mBalls];
                    [log appendFormat:@"Tbl._frictionProperties: 0x%lX\n", (long)g_off.tbl_frictionProperties];
                    [log appendFormat:@"Tbl.mTableCollisionBounds: 0x%lX\n", (long)g_off.tbl_mTableCollisionBounds];
                    [log appendFormat:@"VC.mVisualGuide: 0x%lX\n", (long)g_off.vc_mVisualGuide];
                    [log appendFormat:@"VC.mPower: 0x%lX\n", (long)g_off.vc_mPower];
                    [log appendFormat:@"VEC.mEnglish: 0x%lX\n", (long)g_off.vec_mEnglish];
                    [log appendFormat:@"SM.mStateStack: 0x%lX\n", (long)g_off.sm_mStateStack];
                    [log appendFormat:@"MM.mStateManager: 0x%lX\n", (long)g_off.mm_mStateManager];
                    [log appendFormat:@"TP.mPockets: 0x%lX\n", (long)g_off.tp_mPockets];
                    [log appendFormat:@"Ball._physicsProperties: 0x%lX\n", (long)g_off.ball_physicsProperties];
                    [log appendFormat:@"Ball.classification: 0x%lX\n", (long)g_off.ball_classification];
                    [log appendFormat:@"Ball.state: 0x%lX\n", (long)g_off.ball_state];

                    // Cue Power/Spin FULL debug
                    {
                        const char *src[] = {"Fallback", "Interp", "Table"};
                        auto &d = g_diag;
                        [log appendFormat:@"\n=== Cue Power/Spin (FULL DUMP) ===\n"];
                        [log appendFormat:@"EFFECTIVE MaxPower: %.4f [%s]\n", g_effectiveMaxPower, src[g_powerSource]];
                        [log appendFormat:@"EFFECTIVE SpinMul: %.11f [%s]\n", CUE_PROPERTIES_SPIN_VALUE, src[g_spinSource]];
                        [log appendFormat:@"ShotPower (computed): %.4f\n", d.computedShotPower];
                        [log appendFormat:@"ShotSpin  (computed): (%.6f, %.6f)\n", d.computedSpinX, d.computedSpinY];
                        [log appendFormat:@"CueId: %d\n", g_cachedCueId];
                        [log appendFormat:@"--- UserInfo Ratios ---\n"];
                        [log appendFormat:@"Power: low=%d high=%d\n", d.lowPowerRatio, d.highPowerRatio];
                        [log appendFormat:@"Spin:  low=%d high=%d\n", d.lowSpinRatio, d.highSpinRatio];
                        [log appendFormat:@"Aim:   low=%d high=%d\n", d.lowAimRatio, d.highAimRatio];
                        [log appendFormat:@"Interp Power: %.4f  Spin: %.4f\n", d.interpPower, d.interpSpin];
                        [log appendFormat:@"--- CueStats ---\n"];
                        [log appendFormat:@"Base:  P=%d S=%d A=%d T=%d\n", d.basePower, d.baseSpin, d.baseAim, d.baseTime];
                        [log appendFormat:@"Bonus: P=%d S=%d A=%d T=%d\n", d.bonusPower, d.bonusSpin, d.bonusAim, d.bonusTime];
                        [log appendFormat:@"Table Power[%d]: %.4f\n", d.bonusPower, d.tablePowerLookup];
                        [log appendFormat:@"Table Spin[%d]: %.11f\n", d.bonusSpin, d.tableSpinLookup];
                        [log appendFormat:@"--- VisualCue ---\n"];
                        [log appendFormat:@"mPower: %.6f  mCurrentPowerOffset: %.4f  mMaxCuePowerOffset: %.4f\n", d.vc_mPower, d.vc_mCurrentPowerOffset, d.vc_mMaxCuePowerOffset];
                        [log appendFormat:@"mPowerMode: %d  powerControlActive: %d  mPowerSliderVisible: %d\n", d.vc_mPowerMode, d.vc_powerControlActive, d.vc_mPowerSliderVisible];
                        [log appendFormat:@"mControlMode: %d  mMaxAngleForPowerMode: %.4f\n", d.vc_mControlMode, d.vc_mMaxAngleForPowerMode];
                        [log appendFormat:@"mDistanceFromTip: %.4f  mAnimatePowerTarget: %.6f  mUserSensitivity: %.4f\n", d.vc_mDistanceFromTip, d.vc_mAnimatePowerTarget, d.vc_mUserSensitivity];
                        [log appendFormat:@"--- GamePowerBarView ---\n"];
                        [log appendFormat:@"getMinimumPower: %.6f  getPowerSliderDistance: %.4f\n", d.pbv_minimumPower, d.pbv_sliderDistance];
                        [log appendFormat:@"--- VisualEnglishControl ---\n"];
                        [log appendFormat:@"mEnglish: (%.6f, %.6f)  mSpinLevel: %u  mInputRadius: %.4f\n", d.vec_englishX, d.vec_englishY, d.vec_spinLevel, d.vec_mInputRadius];
                    }

                    // Debug GM+0x3B0 (Android's shot trigger object)
                    if (isValidPtr(g_gameManager)) {
                        ptr obj3b0 = F(ptr, g_gameManager + 0x3b0);
                        [log appendFormat:@"\n=== Shot Debug ===\n"];
                        [log appendFormat:@"GM+0x3B0: 0x%llX\n", (unsigned long long)obj3b0];
                        [log appendFormat:@"AimAngle: %.6f\n", getAimAngle()];
                        [log appendFormat:@"Classification: %d\n", (int)getPlayerClassification()];
                    }

                    // Ball data if in game
                    ptr table = GM_Table();
                    int bc = getTableBallCount(table);
                    if (bc > 0) {
                        [log appendFormat:@"\n=== Ball Data (%d balls) ===\n", bc];
                        for (int i = 0; i < bc && i < 16; i++) {
                            ptr b = getTableBall(table, i);
                            if (!b) continue;
                            Vec2d pos = ballPosition(b);
                            auto cls = ballClassification(b);
                            auto st = ballState(b);
                            [log appendFormat:@"Ball[%d]: pos=(%.2f,%.2f) class=%d state=%d\n", i, pos.x, pos.y, cls, st];
                        }
                    }

                    // Write to Documents
                    NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
                    NSString *docDir = [paths firstObject];
                    NSString *filePath = [docDir stringByAppendingPathComponent:@"8bp_debug.log"];
                    NSError *err = nil;
                    [log writeToFile:filePath atomically:YES encoding:NSUTF8StringEncoding error:&err];
                    if (err) {
                        snprintf(logStatus, sizeof(logStatus), "Error: %s", [[err localizedDescription] UTF8String]);
                    } else {
                        snprintf(logStatus, sizeof(logStatus), "Saved: %s", [filePath UTF8String]);
                        logSaved = true;
                    }
                } @catch (NSException *e) {
                    snprintf(logStatus, sizeof(logStatus), "Exception: %s", [[e reason] UTF8String]);
                }
            }
            if (logStatus[0]) {
                ImGui::TextColored(logSaved ? ImVec4(0.3f,1.f,0.3f,1.f) : ImVec4(1.f,0.3f,0.3f,1.f), "%s", logStatus);
            }
            ImGui::Spacing();
            ImGui::TextWrapped("3-Finger Double Tap to toggle menu");
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(0.5f,0.5f,0.5f,1.0f), "8 Ball Pool Hack v1.0 (iOS)");
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }
}

// ============================================================================
// Init
// ============================================================================

static void ESP_SaveSettings(void);

static void ESP_BlockAds(void); // forward declaration
static void ESP_RegisterAdBlockProtocol(void); // forward declaration
static void ESP_InitPool(void) {
    if (g_initialized) return;
    if (!findPoolBinary()) {
        NSLog(@"[8BP] FATAL: pool binary not found");
        return;
    }
    NSLog(@"[8BP] Pool binary found at 0x%llX, slide=0x%lX", (unsigned long long)g_poolBase, g_poolSlide);

    // Resolve all ObjC ivar offsets
    resolveAllOffsets();

    // Initial singleton resolution (on main thread)
    resolveGameInstances();

    // Periodic refresh on MAIN thread (safe for ObjC calls)
    // Updates singletons + state IDs + ball pointers every 200ms
    // MUST be static — otherwise ARC releases it when function returns!
    static dispatch_source_t timer;
    timer = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, dispatch_get_main_queue());
    dispatch_source_set_timer(timer, dispatch_time(DISPATCH_TIME_NOW, 0), 1 * NSEC_PER_SEC, 500 * NSEC_PER_MSEC);
    dispatch_source_set_event_handler(timer, ^{
        updateGameCache();
        autoAimScan();
        // Auto-save settings every ~10s
        static int saveCounter = 0;
        if (++saveCounter >= 10) { saveCounter = 0; ESP_SaveSettings(); }
    });
    dispatch_resume(timer);

    g_initialized = true;
    NSLog(@"[8BP] Initialization complete");
}

// ============================================================================
// Block Ads — hook MAdsAdsManagementWrapper to prevent ad loading/showing
// ============================================================================

static BOOL _blocked_isReady(id self, SEL _cmd, id uniqueId) { return NO; }
static BOOL _blocked_loadAd(id self, SEL _cmd, id uniqueId) { return NO; }
static BOOL _blocked_showAd(id self, SEL _cmd, id uniqueId, id params) { return NO; }
static void _blocked_createAd(id self, SEL _cmd, id fmt, id uid, id name, id evtL, id loadL) { }
static void _blocked_void(id self, SEL _cmd, id arg) { }
static void _blocked_present(id self, SEL _cmd, id vc, BOOL animated, id completion) { }

static void _hookClassMethod(Class cls, const char *selName, IMP newImp) {
    Method m = class_getClassMethod(cls, sel_registerName(selName));
    if (m) method_setImplementation(m, newImp);
}
static void _hookInstanceMethod(Class cls, const char *selName, IMP newImp) {
    Method m = class_getInstanceMethod(cls, sel_registerName(selName));
    if (m) method_setImplementation(m, newImp);
}

static void ESP_BlockAds(void) {
    int hooked = 0;

    // 1) Hook MAdsAdsManagementWrapper — the game's central ad manager
    Class adClass = objc_getClass("MAdsAdsManagementWrapper");
    if (adClass) {
        _hookClassMethod(adClass, "isReadyWithUniqueId:", (IMP)_blocked_isReady);
        _hookClassMethod(adClass, "loadAdWithUniqueId:", (IMP)_blocked_loadAd);
        _hookClassMethod(adClass, "showAdWithUniqueId:customParameters:", (IMP)_blocked_showAd);
        _hookClassMethod(adClass, "createAdWithAdFormat:uniqueId:placementName:eventListener:loadListener:", (IMP)_blocked_createAd);
        _hookClassMethod(adClass, "destroyAdWithUniqueId:", (IMP)_blocked_void);
        // Also try instance methods in case dump was wrong about static
        _hookInstanceMethod(adClass, "isReadyWithUniqueId:", (IMP)_blocked_isReady);
        _hookInstanceMethod(adClass, "loadAdWithUniqueId:", (IMP)_blocked_loadAd);
        _hookInstanceMethod(adClass, "showAdWithUniqueId:customParameters:", (IMP)_blocked_showAd);
        _hookInstanceMethod(adClass, "createAdWithAdFormat:uniqueId:placementName:eventListener:loadListener:", (IMP)_blocked_createAd);
        hooked++;
        NSLog(@"[8BP] Ads blocked: MAdsAdsManagementWrapper");
    }

    // 2) Hook MAdsSDKSwift — prevent mediation SDK from starting
    Class sdkClass = objc_getClass("MAdsSDKSwift");
    if (sdkClass) {
        _hookClassMethod(sdkClass, "startMediationFromBuilder:appKey:segmentName:segments:mediationDelegate:", (IMP)_blocked_createAd);
        _hookInstanceMethod(sdkClass, "startMediationFromBuilder:appKey:segmentName:segments:mediationDelegate:", (IMP)_blocked_createAd);
        hooked++;
        NSLog(@"[8BP] Ads blocked: MAdsSDKSwift");
    }

    // 3) Hook IronSource (IS) ad loading/showing
    const char *isClasses[] = {"IronSource", "ISRewardedVideo", "ISInterstitial", "ISBannerView"};
    for (auto name : isClasses) {
        Class c = objc_getClass(name);
        if (!c) continue;
        _hookClassMethod(c, "showRewardedVideoWithViewController:", (IMP)_blocked_void);
        _hookClassMethod(c, "showRewardedVideoWithViewController:placement:", (IMP)_blocked_showAd);
        _hookClassMethod(c, "loadInterstitial", (IMP)_blocked_void);
        _hookClassMethod(c, "showInterstitialWithViewController:", (IMP)_blocked_void);
        _hookClassMethod(c, "showInterstitialWithViewController:placement:", (IMP)_blocked_showAd);
        _hookClassMethod(c, "loadBanner:withViewController:", (IMP)_blocked_showAd);
        hooked++;
    }

    // 4) Hook AppLovin MAX (ALInterstitialAd, ALIncentivizedInterstitialAd)
    Class alInter = objc_getClass("ALInterstitialAd");
    if (alInter) {
        _hookInstanceMethod(alInter, "show", (IMP)_blocked_void);
        _hookInstanceMethod(alInter, "showOver:", (IMP)_blocked_void);
        hooked++;
    }
    Class alRewarded = objc_getClass("ALIncentivizedInterstitialAd");
    if (alRewarded) {
        _hookInstanceMethod(alRewarded, "showAndNotify:", (IMP)_blocked_void);
        _hookInstanceMethod(alRewarded, "show", (IMP)_blocked_void);
        hooked++;
    }

    // 5) Hook Google AdMob (GADInterstitialAd, GADBannerView)
    Class gadInter = objc_getClass("GADInterstitialAd");
    if (gadInter) {
        _hookClassMethod(gadInter, "loadWithAdUnitID:request:completionHandler:", (IMP)_blocked_createAd);
        _hookInstanceMethod(gadInter, "presentFromRootViewController:", (IMP)_blocked_void);
        hooked++;
    }
    Class gadBanner = objc_getClass("GADBannerView");
    if (gadBanner) {
        _hookInstanceMethod(gadBanner, "loadRequest:", (IMP)_blocked_void);
        hooked++;
    }

    NSLog(@"[8BP] Ad blocking complete — %d SDK groups hooked", hooked);
}

// Run ad blocking at dylib load time — BEFORE the app initializes ads
__attribute__((constructor)) static void ESP_EarlyAdBlock(void) {
    // Immediate attempt
    ESP_BlockAds();
    // Retry after 1s in case classes aren't loaded yet
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(1.0 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
        ESP_BlockAds();
    });
    // Retry again at 3s
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(3.0 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
        ESP_BlockAds();
    });

    // Register URL protocol to block ad network requests
    ESP_RegisterAdBlockProtocol();
}

static BOOL _adblock_canInit(id self, SEL _cmd, NSURLRequest *request) {
    NSString *host = request.URL.host.lowercaseString;
    if (!host) return NO;
    static NSArray *adDomains = @[
        @"googleads", @"doubleclick", @"googlesyndication", @"admob",
        @"google-analytics", @"googleadservices", @"pagead2",
        @"adservice.google", @"app-measurement",
        @"applovin.com", @"maxcdn.com", @"adjust.com",
        @"mintegral", @"rayjump",
        @"ironsrc.com", @"supersonicads",
        @"unityads", @"unity3d.com",
        @"tapjoy.com", @"liveop.com",
        @"vungle.com", @"inmobi.com",
        @"chartboost.com",
        @"an.facebook.com", @"audience"
    ];
    for (NSString *domain in adDomains) {
        if ([host containsString:domain]) return YES;
    }
    NSString *path = request.URL.absoluteString.lowercaseString;
    if ([path containsString:@"/ads/"] || [path containsString:@"/ad/"] ||
        [path containsString:@"pagead"] || [path containsString:@"adunit"]) return YES;
    return NO;
}

static NSURLRequest* _adblock_canonical(id self, SEL _cmd, NSURLRequest *request) {
    return request;
}

static void _adblock_startLoading(NSURLProtocol *self, SEL _cmd) {
    id<NSURLProtocolClient> client = self.client;
    NSError *err = [NSError errorWithDomain:NSURLErrorDomain code:NSURLErrorCancelled userInfo:nil];
    [client URLProtocol:self didFailWithError:err];
}

static void _adblock_stopLoading(id self, SEL _cmd) { }

static void ESP_RegisterAdBlockProtocol(void) {
    Class proto = objc_allocateClassPair([NSURLProtocol class], "ESPAdBlockProtocol", 0);
    if (!proto) return;

    Class metaCls = object_getClass(proto);
    class_addMethod(metaCls, sel_registerName("canInitWithRequest:"), (IMP)_adblock_canInit, "c@:@");
    class_addMethod(metaCls, sel_registerName("canonicalRequestForRequest:"), (IMP)_adblock_canonical, "@@:@");
    class_addMethod(proto, sel_registerName("startLoading"), (IMP)_adblock_startLoading, "v@:");
    class_addMethod(proto, sel_registerName("stopLoading"), (IMP)_adblock_stopLoading, "v@:");

    objc_registerClassPair(proto);
    [NSURLProtocol registerClass:proto];
    NSLog(@"[8BP] URL-level ad blocking registered (ESPAdBlockProtocol)");
}

// ============================================================================
// Save/Load Settings
// ============================================================================

#define ESP_PREFS_KEY @"com.8bp.hack.settings"

static void ESP_SaveSettings(void) {
    NSMutableDictionary *d = [NSMutableDictionary dictionary];
    d[@"espEnabled"] = @(g_espEnabled);
    d[@"drawPredictionLine"] = @(g_drawPredictionLine);
    d[@"drawPockets"] = @(g_drawPockets);
    d[@"drawPocketsShotState"] = @(g_drawPocketsShotState);
    d[@"noScreenShot"] = @(g_noScreenShot);
    d[@"lineWidth"] = @(g_lineWidth);
    d[@"autoAim"] = @(AutoPlay::bAutoAim);
    d[@"autoPlay"] = @(AutoPlay::bAutoPlaying);
    d[@"autoQueue"] = @(AutoQueue::bAutoQueue);
    d[@"queueMode"] = @(AutoQueue::queueMode);
    d[@"betPercent"] = @(AutoQueue::betPercent);
    d[@"tableOffsetX"] = @(g_tableOffsetX);
    d[@"tableOffsetY"] = @(g_tableOffsetY);
    d[@"tableScaleFactor"] = @(g_tableScaleFactor);
    [[NSUserDefaults standardUserDefaults] setObject:d forKey:ESP_PREFS_KEY];
    [[NSUserDefaults standardUserDefaults] synchronize];
}

static void ESP_LoadSettings(void) {
    NSDictionary *d = [[NSUserDefaults standardUserDefaults] dictionaryForKey:ESP_PREFS_KEY];
    if (!d) return;
    if (d[@"espEnabled"]) g_espEnabled = [d[@"espEnabled"] boolValue];
    if (d[@"drawPredictionLine"]) g_drawPredictionLine = [d[@"drawPredictionLine"] boolValue];
    if (d[@"drawPockets"]) g_drawPockets = [d[@"drawPockets"] boolValue];
    if (d[@"drawPocketsShotState"]) g_drawPocketsShotState = [d[@"drawPocketsShotState"] boolValue];
    if (d[@"noScreenShot"]) g_noScreenShot = [d[@"noScreenShot"] boolValue];
    if (d[@"lineWidth"]) g_lineWidth = [d[@"lineWidth"] floatValue];
    if (d[@"autoAim"]) AutoPlay::bAutoAim = [d[@"autoAim"] boolValue];
    if (d[@"autoPlay"]) AutoPlay::bAutoPlaying = [d[@"autoPlay"] boolValue];
    if (d[@"autoQueue"]) AutoQueue::bAutoQueue = [d[@"autoQueue"] boolValue];
    if (d[@"queueMode"]) AutoQueue::queueMode = [d[@"queueMode"] intValue];
    if (d[@"betPercent"]) AutoQueue::betPercent = [d[@"betPercent"] intValue];
    if (d[@"tableOffsetX"]) g_tableOffsetX = [d[@"tableOffsetX"] floatValue];
    if (d[@"tableOffsetY"]) g_tableOffsetY = [d[@"tableOffsetY"] floatValue];
    if (d[@"tableScaleFactor"]) g_tableScaleFactor = [d[@"tableScaleFactor"] floatValue];
    NSLog(@"[8BP] Settings loaded");
}

static void ESP_AutoSave(void) {
    static float timer = 0.f;
    timer += ImGui::GetIO().DeltaTime;
    if (timer >= 2.0f) { timer = 0.f; ESP_SaveSettings(); }
}
