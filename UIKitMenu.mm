#import "UIKitMenu.h"
#import <objc/runtime.h>

// ── Icon fonts (required by ESP.h) ──
#include "ImGui/imgui.h"
#include "ImGui/Icon.h"
#include "ImGui/Iconcpp.h"
#include "ImGui/ico_font.h"

// ── Pull in all ESP vars & AutoPlay/AutoQueue ──
#include "ESP.h"

// ════════════════════════════════════════════════════════════════════════════
// MARK: - Palette
// ════════════════════════════════════════════════════════════════════════════
static UIColor *MK_BG()       { return [UIColor colorWithRed:0.07 green:0.07 blue:0.08 alpha:0.97]; }
static UIColor *MK_PANEL()    { return [UIColor colorWithRed:0.11 green:0.11 blue:0.13 alpha:1.00]; }
static UIColor *MK_ACCENT()   { return [UIColor colorWithRed:0.20 green:0.60 blue:1.00 alpha:1.00]; }
static UIColor *MK_TEXT()     { return [UIColor colorWithWhite:0.95 alpha:1.00]; }
static UIColor *MK_DIM()      { return [UIColor colorWithWhite:0.50 alpha:1.00]; }
static UIColor *MK_SEP()      { return [UIColor colorWithWhite:1.00 alpha:0.07]; }
static UIColor *MK_ROW()      { return [UIColor colorWithRed:0.13 green:0.13 blue:0.15 alpha:1.00]; }
static UIColor *MK_WARN()     { return [UIColor colorWithRed:1.00 green:0.80 blue:0.20 alpha:1.00]; }

static const CGFloat kW   = 340;
static const CGFloat kH   = 460;
static const CGFloat kRH  = 48;   // row height
static const CGFloat kPad = 14;
static const CGFloat kR   = 16;   // corner radius menu
static const CGFloat kRR  = 12;   // corner radius row

// ════════════════════════════════════════════════════════════════════════════
// MARK: - BoolRow  (toggle checkbox)
// ════════════════════════════════════════════════════════════════════════════
typedef void(^BoolHandler)(BOOL);
@interface BoolRow : UIView
- (instancetype)initWithTitle:(NSString *)t boolPtr:(bool *)ptr width:(CGFloat)w;
- (void)refresh;
@end
@implementation BoolRow {
    bool        *_ptr;
    UIView      *_box;
    UILabel     *_lbl;
}
- (instancetype)initWithTitle:(NSString *)t boolPtr:(bool *)ptr width:(CGFloat)w {
    self = [super initWithFrame:CGRectMake(0, 0, w, kRH)];
    if (!self) return nil;
    _ptr = ptr;
    self.backgroundColor    = MK_ROW();
    self.layer.cornerRadius = kRR;

    // checkbox
    CGFloat bs = 24;
    _box = [[UIView alloc] initWithFrame:CGRectMake(kPad, (kRH-bs)/2, bs, bs)];
    _box.layer.cornerRadius = 6;
    _box.layer.borderWidth  = 2;
    _box.layer.borderColor  = MK_ACCENT().CGColor;
    [self addSubview:_box];

    _lbl = [[UILabel alloc] initWithFrame:CGRectMake(kPad+bs+10, 0, w-kPad*2-bs-10-kPad, kRH)];
    _lbl.text = t; _lbl.textColor = MK_TEXT();
    _lbl.font = [UIFont systemFontOfSize:15 weight:UIFontWeightMedium];
    [self addSubview:_lbl];

    UITapGestureRecognizer *tap = [[UITapGestureRecognizer alloc]
        initWithTarget:self action:@selector(tapped)];
    [self addGestureRecognizer:tap];
    self.userInteractionEnabled = YES;
    [self refresh];
    return self;
}
- (void)tapped { if (_ptr) { *_ptr = !(*_ptr); [self refresh]; ESP_SaveSettings(); } }
- (void)refresh {
    BOOL on = _ptr ? *_ptr : NO;
    _box.backgroundColor = on ? MK_ACCENT() : [UIColor clearColor];
    // checkmark
    for (UIView *v in _box.subviews) [v removeFromSuperview];
    if (on) {
        UILabel *ck = [[UILabel alloc] initWithFrame:_box.bounds];
        ck.text = @"✓"; ck.textAlignment = NSTextAlignmentCenter;
        ck.textColor = [UIColor whiteColor];
        ck.font = [UIFont systemFontOfSize:14 weight:UIFontWeightBold];
        [_box addSubview:ck];
    }
}
@end

// ════════════════════════════════════════════════════════════════════════════
// MARK: - FloatRow  (slider)
// ════════════════════════════════════════════════════════════════════════════
@interface FloatRow : UIView
- (instancetype)initWithTitle:(NSString *)t ptr:(float *)ptr min:(float)mn max:(float)mx fmt:(NSString *)fmt width:(CGFloat)w;
@end
@implementation FloatRow {
    float      *_ptr;
    NSString   *_fmt;
    UILabel    *_valLbl;
}
- (instancetype)initWithTitle:(NSString *)t ptr:(float *)ptr min:(float)mn max:(float)mx fmt:(NSString *)fmt width:(CGFloat)w {
    self = [super initWithFrame:CGRectMake(0, 0, w, 66)];
    if (!self) return nil;
    _ptr = ptr; _fmt = fmt;
    self.backgroundColor    = MK_ROW();
    self.layer.cornerRadius = kRR;

    UILabel *tl = [[UILabel alloc] initWithFrame:CGRectMake(kPad, 8, w-kPad*2-60, 20)];
    tl.text = t; tl.textColor = MK_TEXT();
    tl.font = [UIFont systemFontOfSize:14 weight:UIFontWeightMedium];
    [self addSubview:tl];

    _valLbl = [[UILabel alloc] initWithFrame:CGRectMake(w-kPad-56, 8, 56, 20)];
    _valLbl.textAlignment = NSTextAlignmentRight;
    _valLbl.textColor = MK_DIM();
    _valLbl.font = [UIFont systemFontOfSize:13 weight:UIFontWeightRegular];
    [self addSubview:_valLbl];

    UISlider *sl = [[UISlider alloc] initWithFrame:CGRectMake(kPad, 36, w-kPad*2, 20)];
    sl.minimumValue = mn; sl.maximumValue = mx;
    sl.value = ptr ? *ptr : mn;
    sl.minimumTrackTintColor = MK_ACCENT();
    sl.maximumTrackTintColor = [UIColor colorWithWhite:0.25 alpha:1];
    sl.thumbTintColor = [UIColor whiteColor];
    objc_setAssociatedObject(sl, "ptr", [NSValue valueWithPointer:ptr], OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    objc_setAssociatedObject(sl, "vl",  _valLbl, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    objc_setAssociatedObject(sl, "fmt", fmt,     OBJC_ASSOCIATION_COPY_NONATOMIC);
    [sl addTarget:self action:@selector(slChanged:) forControlEvents:UIControlEventValueChanged];
    [sl addTarget:self action:@selector(slEnd:)     forControlEvents:UIControlEventTouchUpInside | UIControlEventTouchUpOutside];
    [self addSubview:sl];

    [self updateVal:ptr ? *ptr : mn];
    return self;
}
- (void)updateVal:(float)v {
    _valLbl.text = [NSString stringWithFormat:_fmt, v];
}
- (void)slChanged:(UISlider *)s {
    float *ptr = (float *)[objc_getAssociatedObject(s, "ptr") pointerValue];
    if (ptr) *ptr = s.value;
    [self updateVal:s.value];
}
- (void)slEnd:(UISlider *)s { ESP_SaveSettings(); }
@end

// ════════════════════════════════════════════════════════════════════════════
// MARK: - InfoRow  (read-only text)
// ════════════════════════════════════════════════════════════════════════════
@interface InfoRow : UIView
@property (nonatomic, strong) UILabel *valueLabel;
- (instancetype)initWithTitle:(NSString *)t width:(CGFloat)w;
@end
@implementation InfoRow
- (instancetype)initWithTitle:(NSString *)t width:(CGFloat)w {
    self = [super initWithFrame:CGRectMake(0, 0, w, 38)];
    if (!self) return nil;
    self.backgroundColor    = [UIColor clearColor];

    UILabel *tl = [[UILabel alloc] initWithFrame:CGRectMake(kPad+4, 0, w/2-kPad, 38)];
    tl.text = t; tl.textColor = MK_DIM();
    tl.font = [UIFont systemFontOfSize:12 weight:UIFontWeightRegular];
    [self addSubview:tl];

    self.valueLabel = [[UILabel alloc] initWithFrame:CGRectMake(w/2, 0, w/2-kPad, 38)];
    self.valueLabel.textAlignment = NSTextAlignmentRight;
    self.valueLabel.textColor = MK_TEXT();
    self.valueLabel.font = [UIFont systemFontOfSize:12 weight:UIFontWeightMedium];
    [self addSubview:self.valueLabel];
    return self;
}
@end

// ════════════════════════════════════════════════════════════════════════════
// MARK: - ActionRow  (button)
// ════════════════════════════════════════════════════════════════════════════
typedef void(^ActionHandler)(void);
@interface ActionRow : UIView
- (instancetype)initWithTitle:(NSString *)t color:(UIColor *)col width:(CGFloat)w action:(ActionHandler)h;
@end
@implementation ActionRow
- (instancetype)initWithTitle:(NSString *)t color:(UIColor *)col width:(CGFloat)w action:(ActionHandler)h {
    self = [super initWithFrame:CGRectMake(0, 0, w, kRH)];
    if (!self) return nil;
    UIButton *btn = [UIButton buttonWithType:UIButtonTypeCustom];
    btn.frame = self.bounds;
    btn.backgroundColor    = col ?: MK_ACCENT();
    btn.layer.cornerRadius = kRR;
    [btn setTitle:t forState:UIControlStateNormal];
    [btn setTitleColor:[UIColor whiteColor] forState:UIControlStateNormal];
    btn.titleLabel.font = [UIFont systemFontOfSize:14 weight:UIFontWeightSemibold];
    objc_setAssociatedObject(btn, "h", h, OBJC_ASSOCIATION_COPY_NONATOMIC);
    [btn addTarget:self action:@selector(tapped:) forControlEvents:UIControlEventTouchUpInside];
    [self addSubview:btn];
    return self;
}
- (void)tapped:(UIButton *)b { ActionHandler h = objc_getAssociatedObject(b, "h"); if (h) h(); }
@end

// ════════════════════════════════════════════════════════════════════════════
// MARK: - SectionHeader
// ════════════════════════════════════════════════════════════════════════════
static UILabel *mkSection(NSString *t, CGFloat w) {
    UILabel *l = [[UILabel alloc] initWithFrame:CGRectMake(kPad+4, 0, w-kPad*2, 28)];
    l.text = t; l.textColor = MK_ACCENT();
    l.font = [UIFont systemFontOfSize:12 weight:UIFontWeightBold];
    UIView *wrapper = [[UIView alloc] initWithFrame:CGRectMake(0,0,w,28)];
    [wrapper addSubview:l];
    return (UILabel *)wrapper;
}

// ════════════════════════════════════════════════════════════════════════════
// MARK: - Tab scroll builder
// ════════════════════════════════════════════════════════════════════════════
@interface TabBuilder : NSObject
@property (nonatomic, strong) UIScrollView *scrollView;
@property (nonatomic, assign) CGFloat       y;
@property (nonatomic, assign) CGFloat       w;
- (instancetype)initWithScrollView:(UIScrollView *)sv width:(CGFloat)w;
- (void)addView:(UIView *)v;
- (void)addSection:(NSString *)t;
- (void)addSpacer:(CGFloat)h;
- (void)finalize;
@end
@implementation TabBuilder
- (instancetype)initWithScrollView:(UIScrollView *)sv width:(CGFloat)w {
    self = [super init];
    _scrollView = sv; _w = w; _y = 10;
    return self;
}
- (void)addView:(UIView *)v {
    v.frame = CGRectMake(kPad, _y, _w - kPad*2, v.frame.size.height);
    [_scrollView addSubview:v];
    _y += v.frame.size.height + 8;
}
- (void)addSection:(NSString *)t {
    UIView *s = mkSection(t, _w);
    s.frame = CGRectMake(0, _y, _w, 28);
    [_scrollView addSubview:s];
    _y += 32;
}
- (void)addSpacer:(CGFloat)h { _y += h; }
- (void)finalize { _scrollView.contentSize = CGSizeMake(_w, _y + 16); }
@end

// ════════════════════════════════════════════════════════════════════════════
// MARK: - UIKitMenu
// ════════════════════════════════════════════════════════════════════════════
@interface UIKitMenu ()
@property (nonatomic, assign) NSInteger        currentTab;
@property (nonatomic, strong) NSArray          *tabScrolls;   // UIScrollView per tab
@property (nonatomic, strong) NSArray          *tabBtns;
@property (nonatomic, strong) UIView           *contentArea;
// Info rows for Misc tab — refreshed every frame via CADisplayLink
@property (nonatomic, strong) InfoRow          *irInGame;
@property (nonatomic, strong) InfoRow          *irTurn;
@property (nonatomic, strong) InfoRow          *irBalls;
@property (nonatomic, strong) InfoRow          *irFPS;
@property (nonatomic, strong) InfoRow          *irRule;
@property (nonatomic, strong) CADisplayLink    *refreshLink;
@property (nonatomic, assign) CGPoint          dragAnchor;
// BoolRows that need periodic refresh
@property (nonatomic, strong) NSMutableArray   *boolRows;
@end

@implementation UIKitMenu

static UIKitMenu *_shared;
+ (instancetype)shared {
    static dispatch_once_t once;
    dispatch_once(&once, ^{ _shared = [[UIKitMenu alloc] init]; });
    return _shared;
}

- (instancetype)init {
    self = [super initWithFrame:CGRectMake(0, 0, kW, kH)];
    if (!self) return nil;
    _boolRows = [NSMutableArray array];
    [self buildShell];
    return self;
}

// ── Shell ──────────────────────────────────────────────────────────────────
- (void)buildShell {
    self.backgroundColor    = MK_BG();
    self.layer.cornerRadius = kR;
    self.layer.masksToBounds = NO;
    self.layer.shadowColor  = [UIColor blackColor].CGColor;
    self.layer.shadowRadius = 24;
    self.layer.shadowOpacity = 0.65;
    self.layer.shadowOffset = CGSizeMake(0, 12);

    // Title bar
    CGFloat tH = 50;
    UIView *titleBar = [[UIView alloc] initWithFrame:CGRectMake(0, 0, kW, tH)];
    titleBar.backgroundColor = MK_PANEL();
    titleBar.layer.cornerRadius = kR;
    [self addSubview:titleBar];
    // mask bottom corners of title bar
    UIView *titleFill = [[UIView alloc] initWithFrame:CGRectMake(0, kR, kW, tH-kR)];
    titleFill.backgroundColor = MK_PANEL();
    [self addSubview:titleFill];

    UILabel *title = [[UILabel alloc] initWithFrame:CGRectMake(0, 0, kW, tH)];
    title.text = @"8 Ball Pool";
    title.textAlignment = NSTextAlignmentCenter;
    title.textColor = MK_TEXT();
    title.font = [UIFont systemFontOfSize:17 weight:UIFontWeightBold];
    [self addSubview:title];

    // Close button
    UIButton *close = [UIButton buttonWithType:UIButtonTypeSystem];
    close.frame = CGRectMake(kW-44, 8, 36, 34);
    [close setTitle:@"✕" forState:UIControlStateNormal];
    [close setTitleColor:MK_DIM() forState:UIControlStateNormal];
    close.titleLabel.font = [UIFont systemFontOfSize:18];
    [close addTarget:self action:@selector(hide) forControlEvents:UIControlEventTouchUpInside];
    [self addSubview:close];

    // Drag
    UIPanGestureRecognizer *pan = [[UIPanGestureRecognizer alloc]
        initWithTarget:self action:@selector(handlePan:)];
    [titleBar addGestureRecognizer:pan];

    // Separator
    UIView *sep = [[UIView alloc] initWithFrame:CGRectMake(0, tH, kW, 0.5)];
    sep.backgroundColor = MK_SEP();
    [self addSubview:sep];

    // Tab bar
    CGFloat tabBarH = 40;
    NSArray *tabs = @[@"ESP", @"Auto", @"Misc"];
    NSMutableArray *btns = [NSMutableArray array];
    CGFloat tw = kW / tabs.count;
    UIView *tabBar = [[UIView alloc] initWithFrame:CGRectMake(0, tH+0.5, kW, tabBarH)];
    tabBar.backgroundColor = MK_PANEL();
    [self addSubview:tabBar];
    for (NSUInteger i = 0; i < tabs.count; i++) {
        UIButton *btn = [UIButton buttonWithType:UIButtonTypeCustom];
        btn.frame = CGRectMake(i*tw, 0, tw, tabBarH);
        btn.tag = i;
        [btn setTitle:tabs[i] forState:UIControlStateNormal];
        btn.titleLabel.font = [UIFont systemFontOfSize:13 weight:UIFontWeightSemibold];
        [btn addTarget:self action:@selector(tabTapped:) forControlEvents:UIControlEventTouchUpInside];
        [tabBar addSubview:btn];
        [btns addObject:btn];
    }
    // tab bottom indicator
    UIView *tabSep = [[UIView alloc] initWithFrame:CGRectMake(0, tabBarH, kW, 0.5)];
    tabSep.backgroundColor = MK_SEP();
    [tabBar addSubview:tabSep];
    self.tabBtns = btns;

    // Content area
    CGFloat contentY = tH + 0.5 + tabBarH + 1;
    CGFloat contentH = kH - contentY;
    self.contentArea = [[UIView alloc] initWithFrame:CGRectMake(0, contentY, kW, contentH)];
    self.contentArea.backgroundColor = [UIColor clearColor];
    self.contentArea.clipsToBounds = YES;
    [self addSubview:self.contentArea];

    // Build tab pages
    NSMutableArray *scrolls = [NSMutableArray array];
    for (NSUInteger i = 0; i < tabs.count; i++) {
        UIScrollView *sv = [[UIScrollView alloc] initWithFrame:self.contentArea.bounds];
        sv.backgroundColor = [UIColor clearColor];
        sv.showsVerticalScrollIndicator = YES;
        sv.alwaysBounceVertical = YES;
        sv.hidden = (i != 0);
        [self.contentArea addSubview:sv];
        [scrolls addObject:sv];
    }
    self.tabScrolls = scrolls;
    [self buildESP];
    [self buildAuto];
    [self buildMisc];
    [self selectTab:0];
}

// ── Tab selection ──────────────────────────────────────────────────────────
- (void)tabTapped:(UIButton *)b { [self selectTab:b.tag]; }
- (void)selectTab:(NSInteger)idx {
    _currentTab = idx;
    for (NSUInteger i = 0; i < self.tabScrolls.count; i++) {
        ((UIScrollView *)self.tabScrolls[i]).hidden = (NSInteger)i != idx;
    }
    for (NSUInteger i = 0; i < self.tabBtns.count; i++) {
        UIButton *b = self.tabBtns[i];
        BOOL sel = (NSInteger)i == idx;
        [b setTitleColor:sel ? MK_ACCENT() : MK_DIM() forState:UIControlStateNormal];
        b.titleLabel.font = [UIFont systemFontOfSize:13
            weight:sel ? UIFontWeightBold : UIFontWeightSemibold];
    }
}

// ── Tab 0: ESP ─────────────────────────────────────────────────────────────
- (void)buildESP {
    UIScrollView *sv = self.tabScrolls[0];
    TabBuilder *tb = [[TabBuilder alloc] initWithScrollView:sv width:kW];
    [tb addSection:@"ESP"];
    BoolRow *r1 = [[BoolRow alloc] initWithTitle:@"Enable ESP" boolPtr:&g_espEnabled width:kW-kPad*2];
    [tb addView:r1]; [_boolRows addObject:r1];
    BoolRow *r2 = [[BoolRow alloc] initWithTitle:@"Prediction Lines" boolPtr:&g_drawPredictionLine width:kW-kPad*2];
    [tb addView:r2]; [_boolRows addObject:r2];
    BoolRow *r3 = [[BoolRow alloc] initWithTitle:@"Draw Pockets" boolPtr:&g_drawPockets width:kW-kPad*2];
    [tb addView:r3]; [_boolRows addObject:r3];
    BoolRow *r4 = [[BoolRow alloc] initWithTitle:@"Pocket Shot State" boolPtr:&g_drawPocketsShotState width:kW-kPad*2];
    [tb addView:r4]; [_boolRows addObject:r4];
    [tb addSpacer:4];
    [tb addSection:@"LINE"];
    [tb addView:[[FloatRow alloc] initWithTitle:@"Line Width" ptr:&g_lineWidth min:0.5 max:5.0 fmt:@"%.1f" width:kW-kPad*2]];
    [tb addSpacer:4];
    [tb addSection:@"TABLE CALIBRATION"];
    [tb addView:[[FloatRow alloc] initWithTitle:@"Offset X" ptr:&g_tableOffsetX min:-50 max:50 fmt:@"%.1f" width:kW-kPad*2]];
    [tb addView:[[FloatRow alloc] initWithTitle:@"Offset Y" ptr:&g_tableOffsetY min:-50 max:50 fmt:@"%.1f" width:kW-kPad*2]];
    [tb addView:[[FloatRow alloc] initWithTitle:@"Scale Factor" ptr:&g_tableScaleFactor min:0.9 max:1.1 fmt:@"%.3f" width:kW-kPad*2]];
    [tb finalize];
}

// ── Tab 1: Auto ────────────────────────────────────────────────────────────
- (void)buildAuto {
    UIScrollView *sv = self.tabScrolls[1];
    TabBuilder *tb = [[TabBuilder alloc] initWithScrollView:sv width:kW];
    [tb addSection:@"AUTO AIM"];
    BoolRow *r1 = [[BoolRow alloc] initWithTitle:@"Auto Aim (rotate cue)" boolPtr:&AutoPlay::bAutoAim width:kW-kPad*2];
    [tb addView:r1]; [_boolRows addObject:r1];
    BoolRow *r2 = [[BoolRow alloc] initWithTitle:@"AutoPlay" boolPtr:&AutoPlay::bAutoPlaying width:kW-kPad*2];
    [tb addView:r2]; [_boolRows addObject:r2];
    [tb addSpacer:4];
    [tb addSection:@"AUTO QUEUE"];
    BoolRow *r3 = [[BoolRow alloc] initWithTitle:@"AutoQueue (Rematch)" boolPtr:&AutoQueue::bAutoQueue width:kW-kPad*2];
    [tb addView:r3]; [_boolRows addObject:r3];
    [tb addSpacer:8];
    // Status info
    _irInGame = [[InfoRow alloc] initWithTitle:@"In Game" width:kW-kPad*2];
    [tb addView:_irInGame];
    _irTurn = [[InfoRow alloc] initWithTitle:@"Player Turn" width:kW-kPad*2];
    [tb addView:_irTurn];
    _irRule = [[InfoRow alloc] initWithTitle:@"Rule" width:kW-kPad*2];
    [tb addView:_irRule];
    _irBalls = [[InfoRow alloc] initWithTitle:@"Balls on table" width:kW-kPad*2];
    [tb addView:_irBalls];
    [tb finalize];
}

// ── Tab 2: Misc ────────────────────────────────────────────────────────────
- (void)buildMisc {
    UIScrollView *sv = self.tabScrolls[2];
    TabBuilder *tb = [[TabBuilder alloc] initWithScrollView:sv width:kW];
    [tb addSection:@"PROTECTION"];
    BoolRow *r1 = [[BoolRow alloc] initWithTitle:@"I'm Not Hacker (stream)" boolPtr:&g_noScreenShot width:kW-kPad*2];
    [tb addView:r1]; [_boolRows addObject:r1];
    [tb addSpacer:4];
    [tb addSection:@"DEBUG"];
    _irFPS = [[InfoRow alloc] initWithTitle:@"FPS" width:kW-kPad*2];
    [tb addView:_irFPS];
    // Save log button
    [tb addSpacer:8];
    [tb addView:[[ActionRow alloc] initWithTitle:@"Save Log to Documents"
        color:[UIColor colorWithRed:0.2 green:0.5 blue:0.9 alpha:1]
        width:kW-kPad*2
        action:^{
            @try {
                NSMutableString *log = [NSMutableString string];
                [log appendFormat:@"=== 8BP Debug Log ===\n"];
                [log appendFormat:@"Date: %@\n", [NSDate date]];
                [log appendFormat:@"InGame: %d  PlayerTurn: %d\n", isInGame(), isPlayerTurn()];
                [log appendFormat:@"GM_StateID: %d  MM_StateID: %d\n", g_cache.gmStateId, g_cache.mmStateId];
                [log appendFormat:@"Rule: %s\n", isNineBallRuntime() ? "9-Ball" : "8-Ball"];
                [log appendFormat:@"Balls: %d\n", getTableBallCount(GM_Table())];
                NSString *path = [NSSearchPathForDirectoriesInDomains(
                    NSDocumentDirectory, NSUserDomainMask, YES).firstObject
                    stringByAppendingPathComponent:@"8bp_debug.txt"];
                [log writeToFile:path atomically:YES encoding:NSUTF8StringEncoding error:nil];
            } @catch (...) {}
        }]];
    [tb finalize];
}

// ── Show / Hide / Toggle ───────────────────────────────────────────────────
- (void)show {
    if (self.superview) { self.hidden = NO; [self.superview bringSubviewToFront:self]; return; }
    UIWindow *w = [UIApplication sharedApplication].keyWindow
                  ?: [UIApplication sharedApplication].windows.firstObject;
    UIView *root = w.rootViewController.view;
    if (!root) return;
    self.center = CGPointMake(root.bounds.size.width/2, root.bounds.size.height/2);
    [root addSubview:self];
    self.alpha = 0; self.transform = CGAffineTransformMakeScale(0.92, 0.92);
    [UIView animateWithDuration:0.25 delay:0
         usingSpringWithDamping:0.8 initialSpringVelocity:0
                        options:0
                     animations:^{ self.alpha = 1; self.transform = CGAffineTransformIdentity; }
                     completion:nil];
    [self startRefresh];
}

- (void)hide {
    [UIView animateWithDuration:0.18 animations:^{
        self.alpha = 0; self.transform = CGAffineTransformMakeScale(0.94, 0.94);
    } completion:^(BOOL f) {
        self.hidden = YES; self.alpha = 1; self.transform = CGAffineTransformIdentity;
        [self stopRefresh];
    }];
}

- (void)toggle { (self.hidden || !self.superview) ? [self show] : [self hide]; }

// ── Drag ───────────────────────────────────────────────────────────────────
- (void)handlePan:(UIPanGestureRecognizer *)g {
    CGPoint t = [g translationInView:self.superview];
    if (g.state == UIGestureRecognizerStateBegan) _dragAnchor = self.center;
    self.center = CGPointMake(_dragAnchor.x + t.x, _dragAnchor.y + t.y);
}

// ── Refresh info labels (via CADisplayLink ~2fps) ──────────────────────────
- (void)startRefresh {
    if (_refreshLink) return;
    _refreshLink = [CADisplayLink displayLinkWithTarget:self selector:@selector(refreshInfo)];
    _refreshLink.preferredFramesPerSecond = 2;
    [_refreshLink addToRunLoop:[NSRunLoop mainRunLoop] forMode:NSRunLoopCommonModes];
}
- (void)stopRefresh { [_refreshLink invalidate]; _refreshLink = nil; }

- (void)refreshInfo {
    @try {
        _irInGame.valueLabel.text  = isInGame()      ? @"✅ Yes" : @"No";
        _irTurn.valueLabel.text    = isPlayerTurn()  ? @"✅ Yes" : @"No";
        _irRule.valueLabel.text    = isNineBallRuntime() ? @"9-Ball" : @"8-Ball";
        _irBalls.valueLabel.text   = [NSString stringWithFormat:@"%d", getTableBallCount(GM_Table())];
        _irFPS.valueLabel.text     = [NSString stringWithFormat:@"%.0f", ImGui::GetIO().Framerate];
    } @catch (...) {}
}

@end
