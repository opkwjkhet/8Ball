#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>
#import <mach-o/dyld.h>
#import <objc/runtime.h>
#import <dlfcn.h>

// ── LoadView infrastructure ──
#import "LoadView/ImGuiDrawView.h"
#import "LoadView/ImGuiLoad.h"
#import "LoadView/HeeeNoScreenShotView.h"

// ── ImGui ──
#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_metal.h"

// ── Fonts ──
#include "ImGui/zzz.h"
#include "ImGui/Icon.h"
#include "ImGui/Iconcpp.h"
#include "ImGui/ico_font.h"

// ── ESP system ──
#include "ESP.h"

// ── Metal device for ESP (if needed) ──
id<MTLDevice> g_espMetalDevice = nil;

// ============================================================================
// Globals
// ============================================================================

static bool g_menuVisible = false;
HeeeNoScreenShotView *hideViewSylphin3107 = nil;

// ── Gesture state ──
static id                       g_gestureTarget         = nil;
static __weak UIWindow         *g_gestureAttachedWindow = nil;
static UITapGestureRecognizer  *g_menuGesture           = nil;

// ============================================================================
// Find the game's render view (MTKView, UnityView, etc.)
// ============================================================================

static UIView *GetGameRenderView(UIView *root) {
    if (!root) return nil;
    static NSArray<NSString*> *names;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        names = @[@"MTKView", @"EAGLView", @"GLKView", @"UnityView", @"SCNView"];
    });
    for (NSString *name in names) {
        Class c = NSClassFromString(name);
        if (c && [root isKindOfClass:c]) {
            return root;
        }
    }
    for (UIView *sub in root.subviews) {
        UIView *v = GetGameRenderView(sub);
        if (v) return v;
    }
    return nil;
}

// ============================================================================
// Enable multitouch on window + game view so 3-finger gesture can fire
// ============================================================================

static void EnableMultiTouchForGameAndImGui(UIView *imguiView) {
    UIWindow *window = [UIApplication sharedApplication].keyWindow
                       ?: [UIApplication sharedApplication].windows.firstObject;
    if (window) {
        window.multipleTouchEnabled = YES;
    }
    UIView *gameRootView = window.rootViewController.view;
    if (gameRootView) {
        gameRootView.multipleTouchEnabled = YES;
        UIView *gameRenderView = GetGameRenderView(gameRootView);
        if (gameRenderView) {
            gameRenderView.multipleTouchEnabled = YES;
            gameRenderView.exclusiveTouch = NO;
        }
    }
    if (imguiView) {
        imguiView.multipleTouchEnabled = YES;
        imguiView.exclusiveTouch = NO;
    }
}

// ============================================================================
// Gesture recognizer — 3-finger double-tap on UIWindow
// ============================================================================

static void gestureFired(id self, SEL _cmd, id sender) {
    dispatch_async(dispatch_get_main_queue(), ^{
        g_menuVisible = !g_menuVisible;
        NSLog(@"[ESP-AI] Menu toggled: %s", g_menuVisible ? "VISIBLE" : "HIDDEN");
    });
}

static BOOL isGestureStillAttached(void) {
    UIWindow *win = g_gestureAttachedWindow;
    if (!win || !g_menuGesture) return NO;
    return [win.gestureRecognizers containsObject:g_menuGesture];
}

static void attachGestureToWindow(UIWindow *window) {
    if (!window) return;

    if (!g_gestureTarget) {
        g_gestureTarget = [[NSObject alloc] init];
        SEL sel = sel_registerName("gestureFired:");
        if (!class_getInstanceMethod([NSObject class], sel)) {
            class_addMethod([NSObject class], sel, (IMP)gestureFired, "v@:@");
        }
    }

    // Remove old gesture
    if (g_menuGesture) {
        UIWindow *oldWin = g_gestureAttachedWindow;
        if (oldWin) {
            [oldWin removeGestureRecognizer:g_menuGesture];
        }
        g_menuGesture = nil;
        g_gestureAttachedWindow = nil;
    }

    SEL sel = sel_registerName("gestureFired:");
    UITapGestureRecognizer *tap = [[UITapGestureRecognizer alloc] init];
    tap.numberOfTapsRequired    = 2;
    tap.numberOfTouchesRequired = 3;
    tap.cancelsTouchesInView    = NO;
    tap.delaysTouchesBegan      = NO;
    tap.delaysTouchesEnded      = NO;
    [tap addTarget:g_gestureTarget action:sel];
    [window addGestureRecognizer:tap];

    g_menuGesture = tap;
    g_gestureAttachedWindow = window;
    NSLog(@"[ESP-AI] Gesture recognizer attached to window");
}

static void setupGestureRecognizer(void) {
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(1 * NSEC_PER_SEC)),
                   dispatch_get_main_queue(), ^{
        UIWindow *window = [UIApplication sharedApplication].keyWindow;
        if (!window) return;

        attachGestureToWindow(window);

        static dispatch_once_t observerOnce;
        dispatch_once(&observerOnce, ^{
            [[NSNotificationCenter defaultCenter]
             addObserverForName:UIApplicationDidBecomeActiveNotification
             object:nil
             queue:[NSOperationQueue mainQueue]
             usingBlock:^(NSNotification *note) {
                dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(0.5 * NSEC_PER_SEC)),
                               dispatch_get_main_queue(), ^{
                    UIWindow *currentWindow = [UIApplication sharedApplication].keyWindow;
                    if (!currentWindow) return;
                    if (!isGestureStillAttached() || g_gestureAttachedWindow != currentWindow) {
                        attachGestureToWindow(currentWindow);
                    }
                    EnableMultiTouchForGameAndImGui(nil);
                });
            }];
        });
    });
}


static ImGuiDrawView *_sharedImGuiDrawView = nil;

static void setupGestureAndShowImGui(void) {
    dispatch_async(dispatch_get_main_queue(), ^{
        if (!_sharedImGuiDrawView) {
            _sharedImGuiDrawView = [[ImGuiDrawView alloc] init];
        }

        UIWindow *window = [UIApplication sharedApplication].keyWindow
                           ?: [UIApplication sharedApplication].windows.firstObject;
        UIView *rootView = window.rootViewController.view;
        if (!rootView) return;

        if (!hideViewSylphin3107) {
            CGRect screenBounds = [UIScreen mainScreen].bounds;
            hideViewSylphin3107 = [[HeeeNoScreenShotView alloc] initWithFrame:screenBounds];
            hideViewSylphin3107.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
            // Start with maskContent OFF (normal mode), touch passthrough
            hideViewSylphin3107.maskContent = NO;
            hideViewSylphin3107.passTouchToContent = NO;
            [rootView addSubview:hideViewSylphin3107];
        }

        if (_sharedImGuiDrawView.view.superview == nil) {
            [hideViewSylphin3107 addSubview:_sharedImGuiDrawView.view];
            _sharedImGuiDrawView.view.userInteractionEnabled = YES;
            _sharedImGuiDrawView.view.multipleTouchEnabled = YES;
        }

        EnableMultiTouchForGameAndImGui(_sharedImGuiDrawView.view);
        setupGestureRecognizer();

        // Show menu after a short delay
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(3 * NSEC_PER_SEC)),
                       dispatch_get_main_queue(), ^{
            g_menuVisible = true;
        });
    });
}


@interface ImGuiDrawView () <MTKViewDelegate>
@property (nonatomic, strong) id<MTLDevice>       device;
@property (nonatomic, strong) id<MTLCommandQueue>  commandQueue;
@property (nonatomic, assign) CFTimeInterval       lastFrameTime;
@end

@implementation ImGuiDrawView

+ (void)load {
    [super load];
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(5 * NSEC_PER_SEC)),
                   dispatch_get_main_queue(), ^{
        // Test/enterprise mode: bypass paid API + anti-crack checks, boot UI directly.
        ESP_InitPool();
        ESP_LoadSettings();
        setupGestureAndShowImGui();
    });
}

+ (void)showChange:(BOOL)open {
    g_menuVisible = open;
}

- (MTKView *)mtkView {
    return (MTKView *)self.view;
}

- (void)loadView {
    CGFloat w = [UIApplication sharedApplication].windows[0].rootViewController.view.frame.size.width;
    CGFloat h = [UIApplication sharedApplication].windows[0].rootViewController.view.frame.size.height;
    self.view = [[MTKView alloc] initWithFrame:CGRectMake(0, 0, w, h)];
}

- (instancetype)init {
    self = [super init];
    if (self) {
        _device = MTLCreateSystemDefaultDevice();
        _commandQueue = [_device newCommandQueue];

        if (!_device) {
            NSLog(@"[ESP-AI] ERROR: Metal not available!");
            return self;
        }

        // ── ImGui init ──
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();

        ImGuiIO &io = ImGui::GetIO();
        io.IniFilename = NULL;
        io.LogFilename = NULL;

        // ── Style ──
        ImGui::StyleColorsDark();
        ESP_ApplyStyle();

        // ── Fonts ──
        ImFont* font = io.Fonts->AddFontFromMemoryCompressedTTF((void*)zzz_compressed_data, zzz_compressed_size, 40, NULL, io.Fonts->GetGlyphRangesVietnamese());

        // Font Awesome icon font config
        static const ImWchar icons_ranges[] = { 0xf000, 0xf999, 0 };
        ImFontConfig icons_config;
        icons_config.MergeMode = true;
        icons_config.PixelSnapH = true;

        ImFont* fontAwesome = io.Fonts->AddFontFromMemoryCompressedTTF(font_awesome_data, font_awesome_size, 25, &icons_config, icons_ranges);
        font->FontSize = 5;
        ImGui_ImplMetal_Init(_device);

        // ── Pass Metal device to ESP ──
        g_espMetalDevice = _device;

        self.lastFrameTime = CACurrentMediaTime();

        NSLog(@"[ESP-AI] ImGuiDrawView initialized");
    }
    return self;
}

- (void)viewDidLoad {
    [super viewDidLoad];
    self.mtkView.device = self.device;
    self.mtkView.delegate = self;
    self.mtkView.clearColor = MTLClearColorMake(0, 0, 0, 0);
    self.mtkView.backgroundColor = [UIColor colorWithRed:0 green:0 blue:0 alpha:0];
    self.mtkView.clipsToBounds = YES;

    self.mtkView.preferredFramesPerSecond = 60;

    // Use event-driven rendering to reduce wakeups when possible
    self.mtkView.enableSetNeedsDisplay = NO;  // We want continuous rendering for ESP
    self.mtkView.paused = NO;
}

- (void)Guest2 {
    // Placeholder — reset data if needed
}

- (void)updateIOWithTouchEvent:(UIEvent *)event {
    UITouch *anyTouch = event.allTouches.anyObject;
    CGPoint touchLocation = [anyTouch locationInView:self.view];
    ImGuiIO &io = ImGui::GetIO();
    io.MousePos = ImVec2(touchLocation.x, touchLocation.y);

    BOOL hasActiveTouch = NO;
    for (UITouch *touch in event.allTouches) {
        if (touch.phase != UITouchPhaseEnded && touch.phase != UITouchPhaseCancelled) {
            hasActiveTouch = YES;
            break;
        }
    }
    io.MouseDown[0] = hasActiveTouch;
}

- (void)touchesBegan:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    [self updateIOWithTouchEvent:event];
}

- (void)touchesMoved:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    [self updateIOWithTouchEvent:event];
}

- (void)touchesCancelled:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    [self updateIOWithTouchEvent:event];
}

- (void)touchesEnded:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    [self updateIOWithTouchEvent:event];
}

// ============================================================================
// MTKViewDelegate — render loop
// ============================================================================

- (void)mtkView:(MTKView *)view drawableSizeWillChange:(CGSize)size {
    // Nothing needed
}

- (void)drawInMTKView:(MTKView *)view {
    @autoreleasepool {
        ImGuiIO &io = ImGui::GetIO();
        io.DisplaySize.x = view.bounds.size.width;
        io.DisplaySize.y = view.bounds.size.height;

        CFTimeInterval currentTime = CACurrentMediaTime();
        float deltaTime = (float)(currentTime - self.lastFrameTime);
        if (deltaTime <= 0.0f || deltaTime > 0.1f) deltaTime = 1.0f / 60.0f;
        io.DeltaTime = deltaTime;
        self.lastFrameTime = currentTime;

        static CGFloat cachedScale = 0;
        if (cachedScale == 0) {
            cachedScale = view.window.screen.nativeScale ?: [UIScreen mainScreen].nativeScale;
        }
        io.DisplayFramebufferScale = ImVec2(cachedScale, cachedScale);

        // ── NoScreenShot toggle (just flip maskContent, no view moving) ──
        static bool wasNoScreenShot = false;
        if (g_noScreenShot != wasNoScreenShot) {
            wasNoScreenShot = g_noScreenShot;
            dispatch_async(dispatch_get_main_queue(), ^{
                if (hideViewSylphin3107) {
                    hideViewSylphin3107.maskContent = g_noScreenShot;
                    NSLog(@"[ESP-AI] NoScreenShot: %s", g_noScreenShot ? "ON" : "OFF");
                }
            });
        }
        // Touch control:
        // - When menu visible: hideView passes touch to MTKView (ImGui handles it)
        // - When menu hidden: hideView returns nil in hitTest, touch goes to game
        if (hideViewSylphin3107) {
            hideViewSylphin3107.passTouchToContent = g_menuVisible;
        }
        [self.view setUserInteractionEnabled:YES];

        // ── Render ──
        MTLRenderPassDescriptor *rpd = view.currentRenderPassDescriptor;
        if (rpd == nil) return;

        id<MTLCommandBuffer> commandBuffer = [self.commandQueue commandBuffer];
        if (!commandBuffer) return;

        id<MTLRenderCommandEncoder> encoder = [commandBuffer renderCommandEncoderWithDescriptor:rpd];
        [encoder pushDebugGroup:@"ImGui ESP-AI"];

        ImGui_ImplMetal_NewFrame(rpd);
        ImGui::NewFrame();

        // Scale font once
        static dispatch_once_t fontScaleOnce;
        dispatch_once(&fontScaleOnce, ^{
            ImFont *font = ImGui::GetFont();
            font->Scale = 15.f / font->FontSize;
        });

        // ── AutoPlay update ──
        @try {
            AutoPlay::Update();
        } @catch (NSException *e) {
            NSLog(@"[ESP-AI] AutoPlay exception: %@", e);
        }

        // ── AutoQueue update ──
        @try {
            AutoQueue::Update();
        } @catch (NSException *e) {
            NSLog(@"[ESP-AI] AutoQueue exception: %@", e);
        }

        // ── ESP overlay (always drawn) ──
        ImDrawList *bgDrawList = ImGui::GetBackgroundDrawList();
        @try {
            ESP_Update(bgDrawList);
        } @catch (NSException *e) {
            NSLog(@"[ESP-AI] ESP_Update exception: %@", e);
        }

        // ── Menu window ── (width +50% vs original 340pt for longer debug lines)
        if (g_menuVisible) {
            const CGFloat kESPMenuW = 340.f * 1.5f;
            const CGFloat kESPMenuH = 380.f;
            CGFloat x = (view.bounds.size.width - kESPMenuW) / 2;
            CGFloat y = (view.bounds.size.height - kESPMenuH) / 2;
            ImGui::SetNextWindowPos(ImVec2(x, y), ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowSize(ImVec2(kESPMenuW, kESPMenuH), ImGuiCond_Always);

            ImGui::Begin(ICON_FA_EYE " 8 Ball Pool Hack", &g_menuVisible,
                        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize);

            ESP_DrawMenu();

            ImGui::End();
        }

        // ── Finalize ──
        ImGui::Render();
        ImGui_ImplMetal_RenderDrawData(ImGui::GetDrawData(), commandBuffer, encoder);

        [encoder popDebugGroup];
        [encoder endEncoding];

        id<CAMetalDrawable> drawable = view.currentDrawable;
        if (drawable) {
            [commandBuffer presentDrawable:drawable];
        }
        [commandBuffer commit];
    }
}

@end
