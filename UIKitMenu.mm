#import "UIKitMenu.h"
#import <objc/runtime.h>

// g_instaPocket is defined as /*export*/ (non-static) in ESP.h — ImGuiDrawView.mm's TU owns it
extern bool g_instaPocket;

// ── ImGui (for ImGui::GetIO().Framerate only) ──
#include "ImGui/imgui.h"

// ── ESP vars extern (defined in ImGuiDrawView.mm via ESP.h) ──
// Do NOT #include "ESP.h" here — it would cause duplicate symbols at link time.
extern bool g_espEnabled;
extern bool g_drawPredictionLine;
extern bool g_drawPockets;
extern bool g_drawPocketsShotState;
extern float g_lineWidth;
extern float g_tableOffsetX;
extern float g_tableOffsetY;
extern float g_tableScaleFactor;
extern bool g_noScreenShot;

// AutoPlay / AutoQueue — also defined in ESP.h (included by ImGuiDrawView.mm)
namespace AutoPlay {
    extern bool bAutoAim;
    extern bool bAutoPlaying;
    extern void ClearState();
}
namespace AutoQueue {
    extern bool bAutoQueue;
}

// Game state helpers — defined in ESP.h
extern bool isInGame();
extern bool isPlayerTurn();
extern bool isNineBallRuntime();
extern int  getTableBallCount(uintptr_t table);
extern uintptr_t GM_Table();
extern void ESP_SaveSettings();

// Cache struct forward (for g_cache)
struct ESPCache_t;
extern ESPCache_t g_cache;

// ── Aim Assist + Native Power + Auto Win (defined in ESP.h) ──────────────
extern bool  g_aimAssistEnabled;
extern float g_aimGainX;
extern float g_aimGainY;
extern float g_aimGainZ;
extern float g_aimLineThickness;
extern int   g_aimLineColor;      // 0=Black 1=Red 2=Green 3=Purple 4=White
extern bool  g_nativePowerEnabled;
extern float g_nativePowerValue;  // 0.0 - 1.0
extern bool  g_autoWinPending;    // one-shot flag

// ESP LINE/BOX/NAME
extern bool  g_espLines;
extern bool  g_espBox;
extern bool  g_espName;

// ════════════════════════════════════════════════════════════════════════════
// MARK: - Palette  (sampled from the reference screenshots)
// ════════════════════════════════════════════════════════════════════════════
static UIColor *MKRGB(int r, int g, int b) {
    return [UIColor colorWithRed:r/255.0 green:g/255.0 blue:b/255.0 alpha:1];
}
static UIColor *MK_BG()        { return MKRGB(11, 14, 21);  }   // panel + sidebar
static UIColor *MK_ROW()       { return MKRGB(25, 32, 40);  }   // rows, segmented bg, dropdown
static UIColor *MK_ACCENT()    { return MKRGB(254, 85, 20); }   // orange
static UIColor *MK_ACCENT_BG() { return MKRGB(62, 41, 36);  }   // selected dropdown option
static UIColor *MK_TEXT()      { return MKRGB(237, 237, 245); }
static UIColor *MK_DIM()       { return MKRGB(150, 158, 172); }
static UIColor *MK_SEP()       { return MKRGB(31, 34, 41);  }   // dividers
static UIColor *MK_TRACK_OFF() { return MKRGB(44, 53, 70);  }   // switch track (off)
static UIColor *MK_KNOB_OFF()  { return MKRGB(11, 14, 19);  }

// ── Metrics: screenshot is 1280x588 = iPhone landscape 874x402 pt (x1.464) ──
static const CGFloat kW      = 480;   // panel width
static const CGFloat kH      = 374;   // panel height
static const CGFloat kSideW  = 106;   // sidebar width
static const CGFloat kHeadH  = 70;    // header height (title row)
static const CGFloat kRH     = 42;    // row height
static const CGFloat kSegH   = 42;    // segmented control height
static const CGFloat kPadL   = 14;    // left padding of rows inside content
static const CGFloat kRowW   = 330;   // row width
static const CGFloat kGap    = 13;    // gap between rows
static const CGFloat kR      = 12;    // panel corner radius
static const CGFloat kRR     = 10;    // row corner radius

static char kGapKey;

static UIImage *MKSym(NSString *name, CGFloat pt, UIImageSymbolWeight w) {
    UIImageSymbolConfiguration *c = [UIImageSymbolConfiguration configurationWithPointSize:pt weight:w];
    UIImage *i = [UIImage systemImageNamed:name withConfiguration:c];
    if (!i) i = [UIImage systemImageNamed:@"circle" withConfiguration:c];
    return [i imageWithRenderingMode:UIImageRenderingModeAlwaysTemplate];
}
static CGFloat MKGap(UIView *v) {
    NSNumber *n = objc_getAssociatedObject(v, &kGapKey);
    return n ? n.doubleValue : kGap;
}
static UIView *MKSetGap(UIView *v, CGFloat g) {
    objc_setAssociatedObject(v, &kGapKey, @(g), OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    return v;
}

// UI-only for now: "Play mode" dropdown (0 = Safe, 1 = Brutal). Not wired to ESP.h logic.
static int s_playMode = 0;

// ════════════════════════════════════════════════════════════════════════════
// MARK: - BoolRow  (checkbox or switch)
// ════════════════════════════════════════════════════════════════════════════
@interface BoolRow : UIView
- (instancetype)initWithTitle:(NSString *)t boolPtr:(bool *)ptr width:(CGFloat)w asSwitch:(BOOL)sw;
- (void)refresh;
@end
@implementation BoolRow {
    bool    *_ptr;
    BOOL     _switch;
    UIView  *_box;      // checkbox / switch track
    UIView  *_knob;     // switch knob
    UIImageView *_check;
    CGFloat  _w;
}
- (instancetype)initWithTitle:(NSString *)t boolPtr:(bool *)ptr width:(CGFloat)w asSwitch:(BOOL)sw {
    self = [super initWithFrame:CGRectMake(0, 0, w, kRH)];
    if (!self) return nil;
    _ptr = ptr; _switch = sw; _w = w;
    self.backgroundColor    = MK_ROW();
    self.layer.cornerRadius = kRR;

    UILabel *lbl = [[UILabel alloc] initWithFrame:CGRectMake(16, 0, w-16-80, kRH)];
    lbl.text = t; lbl.textColor = MK_TEXT();
    lbl.font = [UIFont systemFontOfSize:15 weight:UIFontWeightMedium];
    [self addSubview:lbl];

    if (sw) {
        _box = [[UIView alloc] initWithFrame:CGRectMake(w-16-44, (kRH-24)/2, 44, 24)];
        _box.layer.cornerRadius = 12;
        _knob = [[UIView alloc] initWithFrame:CGRectMake(3, 3, 18, 18)];
        _knob.layer.cornerRadius = 9;
        [_box addSubview:_knob];
    } else {
        _box = [[UIView alloc] initWithFrame:CGRectMake(w-18-23, (kRH-23)/2, 23, 23)];
        _box.layer.cornerRadius = 6;
        _check = [[UIImageView alloc] initWithImage:MKSym(@"checkmark", 12, UIImageSymbolWeightHeavy)];
        _check.tintColor = [UIColor whiteColor];
        _check.contentMode = UIViewContentModeCenter;
        _check.frame = _box.bounds;
        [_box addSubview:_check];
    }
    _box.userInteractionEnabled = NO;
    [self addSubview:_box];

    [self addGestureRecognizer:[[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(tapped)]];
    [self refresh];
    return self;
}
- (void)tapped { if (_ptr) { *_ptr = !(*_ptr); [self refresh]; ESP_SaveSettings(); } }
- (void)refresh {
    BOOL on = _ptr ? *_ptr : NO;
    if (_switch) {
        _box.backgroundColor  = on ? MK_ACCENT() : MK_TRACK_OFF();
        _knob.backgroundColor = on ? [UIColor whiteColor] : MK_KNOB_OFF();
        CGRect kf = _knob.frame; kf.origin.x = on ? 44-18-3 : 3;
        if (!CGRectEqualToRect(kf, _knob.frame)) {
            [UIView animateWithDuration:0.15 animations:^{ self->_knob.frame = kf; }];
        }
    } else {
        _box.backgroundColor = on ? MK_ACCENT() : MK_BG();
        _check.hidden = !on;
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
    self = [super initWithFrame:CGRectMake(0, 0, w, 62)];
    if (!self) return nil;
    _ptr = ptr; _fmt = fmt;
    self.backgroundColor    = MK_ROW();
    self.layer.cornerRadius = kRR;

    UILabel *tl = [[UILabel alloc] initWithFrame:CGRectMake(16, 8, w-32-70, 20)];
    tl.text = t; tl.textColor = MK_TEXT();
    tl.font = [UIFont systemFontOfSize:15 weight:UIFontWeightMedium];
    [self addSubview:tl];

    _valLbl = [[UILabel alloc] initWithFrame:CGRectMake(w-16-70, 8, 70, 20)];
    _valLbl.textAlignment = NSTextAlignmentRight;
    _valLbl.textColor = MK_ACCENT();
    _valLbl.font = [UIFont systemFontOfSize:14 weight:UIFontWeightMedium];
    [self addSubview:_valLbl];

    UISlider *sl = [[UISlider alloc] initWithFrame:CGRectMake(16, 32, w-32, 22)];
    sl.minimumValue = mn; sl.maximumValue = mx;
    sl.value = ptr ? *ptr : mn;
    sl.minimumTrackTintColor = MK_ACCENT();
    sl.maximumTrackTintColor = MK_TRACK_OFF();
    sl.thumbTintColor = [UIColor whiteColor];
    objc_setAssociatedObject(sl, "ptr", [NSValue valueWithPointer:ptr], OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    [sl addTarget:self action:@selector(slChanged:) forControlEvents:UIControlEventValueChanged];
    [sl addTarget:self action:@selector(slEnd:)     forControlEvents:UIControlEventTouchUpInside | UIControlEventTouchUpOutside];
    [self addSubview:sl];

    [self updateVal:ptr ? *ptr : mn];
    return self;
}
- (void)updateVal:(float)v { _valLbl.text = [NSString stringWithFormat:_fmt, v]; }
- (void)slChanged:(UISlider *)s {
    float *ptr = (float *)[objc_getAssociatedObject(s, "ptr") pointerValue];
    if (ptr) *ptr = s.value;
    [self updateVal:s.value];
}
- (void)slEnd:(UISlider *)s { ESP_SaveSettings(); }
@end

// ════════════════════════════════════════════════════════════════════════════
// MARK: - InfoRow  (read-only: title left, orange value right)
// ════════════════════════════════════════════════════════════════════════════
@interface InfoRow : UIView
@property (nonatomic, strong) UILabel *valueLabel;
- (instancetype)initWithTitle:(NSString *)t width:(CGFloat)w;
@end
@implementation InfoRow
- (instancetype)initWithTitle:(NSString *)t width:(CGFloat)w {
    self = [super initWithFrame:CGRectMake(0, 0, w, kRH)];
    if (!self) return nil;
    self.backgroundColor    = MK_ROW();
    self.layer.cornerRadius = kRR;

    UILabel *tl = [[UILabel alloc] initWithFrame:CGRectMake(16, 0, w/2, kRH)];
    tl.text = t; tl.textColor = MK_TEXT();
    tl.font = [UIFont systemFontOfSize:15 weight:UIFontWeightMedium];
    [self addSubview:tl];

    self.valueLabel = [[UILabel alloc] initWithFrame:CGRectMake(w/2, 0, w/2-16, kRH)];
    self.valueLabel.textAlignment = NSTextAlignmentRight;
    self.valueLabel.textColor = MK_ACCENT();
    self.valueLabel.font = [UIFont systemFontOfSize:15 weight:UIFontWeightMedium];
    [self addSubview:self.valueLabel];
    return self;
}
@end

// ════════════════════════════════════════════════════════════════════════════
// MARK: - ActionRow  (orange icon + white text, like "Feedback" / "Logout")
// ════════════════════════════════════════════════════════════════════════════
typedef void(^ActionHandler)(void);
@interface ActionRow : UIView
- (instancetype)initWithTitle:(NSString *)t symbol:(NSString *)sym width:(CGFloat)w action:(ActionHandler)h;
@end
@implementation ActionRow {
    ActionHandler _h;
}
- (instancetype)initWithTitle:(NSString *)t symbol:(NSString *)sym width:(CGFloat)w action:(ActionHandler)h {
    self = [super initWithFrame:CGRectMake(0, 0, w, kRH)];
    if (!self) return nil;
    _h = [h copy];
    self.backgroundColor    = MK_ROW();
    self.layer.cornerRadius = kRR;

    UIImageView *iv = [[UIImageView alloc] initWithImage:MKSym(sym, 17, UIImageSymbolWeightRegular)];
    iv.tintColor = MK_ACCENT();
    iv.contentMode = UIViewContentModeCenter;
    iv.frame = CGRectMake(16, 0, 26, kRH);
    [self addSubview:iv];

    UILabel *lbl = [[UILabel alloc] initWithFrame:CGRectMake(16+26+10, 0, w-60, kRH)];
    lbl.text = t; lbl.textColor = MK_TEXT();
    lbl.font = [UIFont systemFontOfSize:15 weight:UIFontWeightMedium];
    [self addSubview:lbl];

    [self addGestureRecognizer:[[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(tapped)]];
    return self;
}
- (void)tapped {
    self.alpha = 0.5;
    [UIView animateWithDuration:0.25 animations:^{ self.alpha = 1; }];
    if (_h) _h();
}
@end

// ════════════════════════════════════════════════════════════════════════════
// MARK: - DropdownRow  (collapsed row + expanding option list)
// ════════════════════════════════════════════════════════════════════════════
@interface DropdownRow : UIView
@property (nonatomic, copy) void (^onResize)(void);
- (instancetype)initWithOptions:(NSArray<NSString *> *)opts indexPtr:(int *)p width:(CGFloat)w;
@end
@implementation DropdownRow {
    NSArray   *_opts;
    int       *_idx;
    CGFloat    _w;
    BOOL       _open;
    UIView    *_head;
    UILabel   *_val;
    UIImageView *_chev;
    UIView    *_list;
}
- (instancetype)initWithOptions:(NSArray<NSString *> *)opts indexPtr:(int *)p width:(CGFloat)w {
    self = [super initWithFrame:CGRectMake(0, 0, w, kRH)];
    if (!self) return nil;
    _opts = opts; _idx = p; _w = w;

    _head = [[UIView alloc] initWithFrame:CGRectMake(0, 0, w, kRH)];
    _head.backgroundColor = MK_ROW();
    _head.layer.cornerRadius = kRR;
    [self addSubview:_head];

    _val = [[UILabel alloc] initWithFrame:CGRectMake(16, 0, w-60, kRH)];
    _val.textColor = MK_TEXT();
    _val.font = [UIFont systemFontOfSize:15 weight:UIFontWeightMedium];
    [_head addSubview:_val];

    _chev = [[UIImageView alloc] init];
    _chev.tintColor = MK_ACCENT();
    _chev.contentMode = UIViewContentModeCenter;
    _chev.frame = CGRectMake(w-28-12, 0, 24, kRH);
    [_head addSubview:_chev];

    [_head addGestureRecognizer:[[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(toggle)]];
    [self sync];
    return self;
}
- (void)sync {
    int i = _idx ? *_idx : 0;
    if (i < 0 || i >= (int)_opts.count) i = 0;
    _val.text = _opts[i];
    _chev.image = MKSym(_open ? @"chevron.up" : @"chevron.down", 13, UIImageSymbolWeightSemibold);
}
- (void)buildList {
    [_list removeFromSuperview];
    CGFloat oh = kRH, og = 4, pad = 8;
    CGFloat lh = pad*2 + _opts.count*oh + (_opts.count-1)*og;
    _list = [[UIView alloc] initWithFrame:CGRectMake(0, kRH+7, _w, lh)];
    _list.backgroundColor = MK_ROW();
    _list.layer.cornerRadius = kRR;
    _list.layer.borderWidth = 1;
    _list.layer.borderColor = MKRGB(38, 46, 58).CGColor;
    int sel = _idx ? *_idx : 0;
    for (NSUInteger i = 0; i < _opts.count; i++) {
        UIButton *b = [UIButton buttonWithType:UIButtonTypeCustom];
        b.frame = CGRectMake(pad, pad + i*(oh+og), _w-pad*2, oh);
        b.layer.cornerRadius = 8;
        b.tag = i;
        BOOL on = ((int)i == sel);
        b.backgroundColor = on ? MK_ACCENT_BG() : [UIColor clearColor];
        [b setTitle:_opts[i] forState:UIControlStateNormal];
        [b setTitleColor:on ? MK_ACCENT() : MK_TEXT() forState:UIControlStateNormal];
        b.titleLabel.font = [UIFont systemFontOfSize:15 weight:UIFontWeightMedium];
        b.contentHorizontalAlignment = UIControlContentHorizontalAlignmentLeft;
        b.contentEdgeInsets = UIEdgeInsetsMake(0, 12, 0, 0);
        if (on) {
            UIImageView *ck = [[UIImageView alloc] initWithImage:MKSym(@"checkmark", 14, UIImageSymbolWeightSemibold)];
            ck.tintColor = MK_ACCENT();
            ck.contentMode = UIViewContentModeCenter;
            ck.frame = CGRectMake(b.bounds.size.width-36, 0, 24, oh);
            [b addSubview:ck];
        }
        [b addTarget:self action:@selector(pick:) forControlEvents:UIControlEventTouchUpInside];
        [_list addSubview:b];
    }
    [self addSubview:_list];
}
- (void)toggle { [self setOpen:!_open]; }
- (void)setOpen:(BOOL)open {
    _open = open;
    CGRect f = self.frame;
    if (open) {
        [self buildList];
        f.size.height = kRH + 7 + _list.frame.size.height;
    } else {
        [_list removeFromSuperview]; _list = nil;
        f.size.height = kRH;
    }
    self.frame = f;
    [self sync];
    if (self.onResize) self.onResize();
}
- (void)pick:(UIButton *)b {
    if (_idx) *_idx = (int)b.tag;
    [self setOpen:NO];
    ESP_SaveSettings();
}
@end

// ════════════════════════════════════════════════════════════════════════════
// MARK: - SegmentView  (Play | Aim | Queue)
// ════════════════════════════════════════════════════════════════════════════
@interface SegmentView : UIView
@property (nonatomic, copy) void (^onSelect)(NSInteger);
- (instancetype)initWithTitles:(NSArray<NSString *> *)t width:(CGFloat)w;
- (void)setSelected:(NSInteger)i animated:(BOOL)a;
@end
@implementation SegmentView {
    NSArray<UILabel *> *_labels;
    UIView   *_pill;
    NSUInteger _n;
    CGFloat   _w;
}
- (instancetype)initWithTitles:(NSArray<NSString *> *)t width:(CGFloat)w {
    self = [super initWithFrame:CGRectMake(0, 0, w, kSegH)];
    if (!self) return nil;
    _n = t.count; _w = w;
    self.backgroundColor = MK_ROW();
    self.layer.cornerRadius = kRR;

    CGFloat inset = 3, sw = (w - inset*2) / _n;
    _pill = [[UIView alloc] initWithFrame:CGRectMake(inset, inset, sw, kSegH-inset*2)];
    _pill.backgroundColor = MK_ACCENT();
    _pill.layer.cornerRadius = kRR - 2;
    [self addSubview:_pill];

    NSMutableArray *ls = [NSMutableArray array];
    for (NSUInteger i = 0; i < _n; i++) {
        UILabel *l = [[UILabel alloc] initWithFrame:CGRectMake(inset + i*sw, 0, sw, kSegH)];
        l.text = t[i]; l.textAlignment = NSTextAlignmentCenter;
        l.font = [UIFont systemFontOfSize:15 weight:UIFontWeightSemibold];
        [self addSubview:l]; [ls addObject:l];
    }
    _labels = ls;
    [self addGestureRecognizer:[[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(tapped:)]];
    [self setSelected:0 animated:NO];
    return self;
}
- (void)tapped:(UITapGestureRecognizer *)g {
    CGFloat x = [g locationInView:self].x;
    NSInteger i = (NSInteger)floor((x - 3) / ((_w - 6) / _n));
    i = MAX(0, MIN((NSInteger)_n - 1, i));
    [self setSelected:i animated:YES];
    if (self.onSelect) self.onSelect(i);
}
- (void)setSelected:(NSInteger)i animated:(BOOL)a {
    CGFloat sw = (_w - 6) / _n;
    void (^apply)(void) = ^{
        CGRect f = self->_pill.frame; f.origin.x = 3 + i*sw; self->_pill.frame = f;
        for (NSUInteger k = 0; k < self->_n; k++)
            self->_labels[k].textColor = ((NSInteger)k == i) ? [UIColor whiteColor] : MK_DIM();
    };
    if (a) [UIView animateWithDuration:0.2 animations:apply]; else apply();
}
@end

// ════════════════════════════════════════════════════════════════════════════
// MARK: - Text helpers
// ════════════════════════════════════════════════════════════════════════════
// Small white label above a control ("Play mode")
static UIView *mkLabel(NSString *t) {
    UILabel *l = [[UILabel alloc] initWithFrame:CGRectMake(0, 0, kRowW, 20)];
    l.text = t; l.textColor = MK_TEXT();
    l.font = [UIFont systemFontOfSize:15 weight:UIFontWeightRegular];
    return MKSetGap(l, 17);
}
// Paragraph with orange highlighted words
static UIView *mkNote(NSString *text, NSArray<NSString *> *highlights) {
    NSMutableAttributedString *a = [[NSMutableAttributedString alloc] initWithString:text attributes:@{
        NSForegroundColorAttributeName: MK_TEXT(),
        NSFontAttributeName: [UIFont systemFontOfSize:15 weight:UIFontWeightRegular]}];
    for (NSString *h in highlights) {
        NSRange r = [text rangeOfString:h];
        if (r.location != NSNotFound) [a addAttribute:NSForegroundColorAttributeName value:MK_ACCENT() range:r];
    }
    UILabel *l = [[UILabel alloc] initWithFrame:CGRectMake(0, 0, kRowW, 10)];
    l.numberOfLines = 0;
    l.attributedText = a;
    CGSize s = [l sizeThatFits:CGSizeMake(kRowW, CGFLOAT_MAX)];
    l.frame = CGRectMake(0, 0, kRowW, ceil(s.height));
    return l;
}

// ════════════════════════════════════════════════════════════════════════════
// MARK: - MenuPage  (one sidebar page = one scroll view, optional segments)
// ════════════════════════════════════════════════════════════════════════════
@interface MenuPage : NSObject
@property (nonatomic, strong) UIScrollView *scroll;
@property (nonatomic, strong) NSString *title;
@property (nonatomic, strong) NSString *symbol;
- (instancetype)initWithTitle:(NSString *)t symbol:(NSString *)s frame:(CGRect)f;
- (void)addTop:(UIView *)v;
- (void)setSegments:(NSArray<NSString *> *)titles;
- (void)add:(UIView *)v group:(NSInteger)g;
- (void)relayout;
@end
@implementation MenuPage {
    NSMutableArray<UIView *> *_top;
    NSMutableArray<NSMutableArray<UIView *> *> *_groups;
    SegmentView *_segment;
    UIView      *_divider;
    NSInteger    _cur;
}
- (instancetype)initWithTitle:(NSString *)t symbol:(NSString *)s frame:(CGRect)f {
    self = [super init];
    _title = t; _symbol = s;
    _top = [NSMutableArray array];
    _groups = [NSMutableArray arrayWithObject:[NSMutableArray array]];
    _scroll = [[UIScrollView alloc] initWithFrame:f];
    _scroll.backgroundColor = [UIColor clearColor];
    _scroll.alwaysBounceVertical = YES;
    _scroll.showsVerticalScrollIndicator = YES;
    _scroll.indicatorStyle = UIScrollViewIndicatorStyleWhite;
    _scroll.clipsToBounds = YES;
    return self;
}
- (void)addTop:(UIView *)v { [_top addObject:v]; [_scroll addSubview:v]; [self relayout]; }
- (void)setSegments:(NSArray<NSString *> *)titles {
    __weak MenuPage *ws = self;
    _segment = [[SegmentView alloc] initWithTitles:titles width:kRowW];
    _segment.onSelect = ^(NSInteger i) { MenuPage *s = ws; if (!s) return; s->_cur = i; [s relayout]; };
    _divider = [[UIView alloc] initWithFrame:CGRectMake(0, 0, kRowW, 1)];
    _divider.backgroundColor = MK_SEP();
    [_scroll addSubview:_segment]; [_scroll addSubview:_divider];
    while (_groups.count < titles.count) [_groups addObject:[NSMutableArray array]];
    [self relayout];
}
- (void)add:(UIView *)v group:(NSInteger)g {
    while ((NSInteger)_groups.count <= g) [_groups addObject:[NSMutableArray array]];
    [_groups[g] addObject:v];
    [_scroll addSubview:v];
    if ([v isKindOfClass:[DropdownRow class]]) {
        __weak MenuPage *ws = self;
        ((DropdownRow *)v).onResize = ^{ [ws relayout]; };
    }
    [self relayout];
}
- (void)relayout {
    CGFloat y = 0; BOOL first = YES;
    for (UIView *v in _top) {
        y += first ? 0 : MKGap(v);
        v.frame = CGRectMake(kPadL, y, kRowW, v.frame.size.height);
        y += v.frame.size.height; first = NO;
    }
    if (_segment) {
        y += first ? 0 : 14;
        _segment.frame = CGRectMake(kPadL, y, kRowW, kSegH);
        y += kSegH + 14;
        _divider.frame = CGRectMake(kPadL, y, kRowW, 1);
        y += 1 + 15;
        first = YES;
    }
    CGFloat maxY = y;
    for (NSInteger g = 0; g < (NSInteger)_groups.count; g++) {
        BOOL cur = (g == _cur);
        CGFloat yy = y; BOOL f = first;
        for (UIView *v in _groups[g]) {
            v.hidden = !cur;
            if (!cur) continue;
            yy += f ? 0 : MKGap(v);
            v.frame = CGRectMake(kPadL, yy, kRowW, v.frame.size.height);
            yy += v.frame.size.height; f = NO;
        }
        if (cur) maxY = yy;
    }
    _scroll.contentSize = CGSizeMake(_scroll.bounds.size.width, maxY + 14);
}
@end

// ════════════════════════════════════════════════════════════════════════════
// MARK: - UIKitMenu
// ════════════════════════════════════════════════════════════════════════════
@interface UIKitMenu ()
@property (nonatomic, assign) NSInteger        currentTab;
@property (nonatomic, strong) NSArray<MenuPage *> *pages;
@property (nonatomic, strong) NSArray<UIButton *> *sideBtns;
@property (nonatomic, strong) UIImageView      *headIcon;
@property (nonatomic, strong) UILabel          *headTitle;
@property (nonatomic, strong) InfoRow          *irInGame;
@property (nonatomic, strong) InfoRow          *irTurn;
@property (nonatomic, strong) InfoRow          *irBalls;
@property (nonatomic, strong) InfoRow          *irFPS;
@property (nonatomic, strong) InfoRow          *irRule;
@property (nonatomic, strong) CADisplayLink    *refreshLink;
@property (nonatomic, assign) CGPoint          dragAnchor;
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

- (BoolRow *)boolRow:(NSString *)t ptr:(bool *)p asSwitch:(BOOL)sw {
    BoolRow *r = [[BoolRow alloc] initWithTitle:t boolPtr:p width:kRowW asSwitch:sw];
    [_boolRows addObject:r];
    return r;
}

// ── Shell ──────────────────────────────────────────────────────────────────
- (void)buildShell {
    self.backgroundColor     = MK_BG();
    self.layer.cornerRadius  = kR;
    self.layer.masksToBounds = NO;
    self.layer.shadowColor   = [UIColor blackColor].CGColor;
    self.layer.shadowRadius  = 24;
    self.layer.shadowOpacity = 0.65;
    self.layer.shadowOffset  = CGSizeMake(0, 12);

    // Sidebar / content divider
    UIView *vsep = [[UIView alloc] initWithFrame:CGRectMake(kSideW, 0, 1, kH)];
    vsep.backgroundColor = MK_SEP();
    [self addSubview:vsep];

    // Header: icon + title (orange), close button
    _headIcon = [[UIImageView alloc] initWithFrame:CGRectMake(kSideW+kPadL, (kHeadH-26)/2, 26, 26)];
    _headIcon.tintColor = MK_ACCENT();
    _headIcon.contentMode = UIViewContentModeCenter;
    [self addSubview:_headIcon];

    _headTitle = [[UILabel alloc] initWithFrame:CGRectMake(kSideW+kPadL+26+8, 0, 200, kHeadH)];
    _headTitle.textColor = MK_ACCENT();
    _headTitle.font = [UIFont systemFontOfSize:16 weight:UIFontWeightMedium];
    [self addSubview:_headTitle];

    UIButton *close = [UIButton buttonWithType:UIButtonTypeCustom];
    close.frame = CGRectMake(kW-35-22, (kHeadH-44)/2, 44, 44);
    [close setImage:MKSym(@"xmark", 20, UIImageSymbolWeightMedium) forState:UIControlStateNormal];
    close.tintColor = MK_ACCENT();
    [close addTarget:self action:@selector(hide) forControlEvents:UIControlEventTouchUpInside];
    [self addSubview:close];

    // Drag handle = header strip (right of sidebar, left of close)
    UIView *drag = [[UIView alloc] initWithFrame:CGRectMake(kSideW+1, 0, kW-kSideW-1-60, kHeadH)];
    drag.backgroundColor = [UIColor clearColor];
    [drag addGestureRecognizer:[[UIPanGestureRecognizer alloc] initWithTarget:self action:@selector(handlePan:)]];
    [self insertSubview:drag belowSubview:close];

    // Pages
    CGRect sf = CGRectMake(kSideW+1, kHeadH, kW-kSideW-1, kH-kHeadH-14);
    MenuPage *pAuto = [[MenuPage alloc] initWithTitle:@"AUTO"     symbol:@"scope"          frame:sf];
    MenuPage *pAim  = [[MenuPage alloc] initWithTitle:@"AIM"      symbol:@"target"         frame:sf];
    MenuPage *pVis  = [[MenuPage alloc] initWithTitle:@"VISUALS"  symbol:@"eye"            frame:sf];
    MenuPage *pMisc = [[MenuPage alloc] initWithTitle:@"MISC"     symbol:@"shippingbox"    frame:sf];
    MenuPage *pSet  = [[MenuPage alloc] initWithTitle:@"SETTINGS" symbol:@"gearshape"      frame:sf];
    MenuPage *pAcc  = [[MenuPage alloc] initWithTitle:@"ACCOUNT"  symbol:@"person"         frame:sf];
    self.pages = @[pAuto, pAim, pVis, pMisc, pSet, pAcc];
    for (MenuPage *p in self.pages) { p.scroll.hidden = YES; [self addSubview:p.scroll]; }
    [self buildAuto:pAuto]; [self buildAim:pAim]; [self buildVisuals:pVis];
    [self buildMisc:pMisc]; [self buildSettings:pSet]; [self buildAccount:pAcc];

    // Sidebar buttons: icon centre at 35pt, pitch 70.7pt, label 22pt below icon
    NSMutableArray *btns = [NSMutableArray array];
    for (NSUInteger i = 0; i < self.pages.count; i++) {
        MenuPage *p = self.pages[i];
        CGFloat iconY = 35 + i * 70.7;
        UIButton *b = [UIButton buttonWithType:UIButtonTypeCustom];
        b.frame = CGRectMake(0, iconY - 24, kSideW, 66);
        b.tag = i;
        UIImageView *iv = [[UIImageView alloc] initWithImage:MKSym(p.symbol, 21, UIImageSymbolWeightRegular)];
        iv.contentMode = UIViewContentModeCenter;
        iv.frame = CGRectMake(0, 0, kSideW, 28);
        iv.tag = 101; iv.userInteractionEnabled = NO;
        [b addSubview:iv];
        UILabel *l = [[UILabel alloc] initWithFrame:CGRectMake(0, 30, kSideW, 20)];
        l.text = p.title.capitalizedString;
        l.textAlignment = NSTextAlignmentCenter;
        l.font = [UIFont systemFontOfSize:14 weight:UIFontWeightMedium];
        l.tag = 102; l.userInteractionEnabled = NO;
        [b addSubview:l];
        [b addTarget:self action:@selector(sideTapped:) forControlEvents:UIControlEventTouchUpInside];
        [self addSubview:b]; [btns addObject:b];
    }
    self.sideBtns = btns;
    [self selectTab:0];
}

// ── Tab selection ──────────────────────────────────────────────────────────
- (void)sideTapped:(UIButton *)b { [self selectTab:b.tag]; }
- (void)selectTab:(NSInteger)idx {
    _currentTab = idx;
    for (NSUInteger i = 0; i < self.pages.count; i++) {
        MenuPage *p = self.pages[i];
        BOOL sel = (NSInteger)i == idx;
        p.scroll.hidden = !sel;
        UIButton *b = self.sideBtns[i];
        UIColor *c = sel ? MK_ACCENT() : MK_TEXT();
        ((UIImageView *)[b viewWithTag:101]).tintColor = c;
        ((UILabel *)[b viewWithTag:102]).textColor = c;
        if (sel) {
            self.headIcon.image = MKSym(p.symbol, 21, UIImageSymbolWeightRegular);
            self.headTitle.text = p.title;
        }
    }
}

// ── Page 0: Auto (Play | Aim | Queue) ──────────────────────────────────────
- (void)buildAuto:(MenuPage *)p {
    [p setSegments:@[@"Play", @"Aim", @"Queue"]];
    // Play
    [p add:[self boolRow:@"AutoPlay" ptr:&AutoPlay::bAutoPlaying asSwitch:YES] group:0];
    [p add:mkLabel(@"Play mode") group:0];
    [p add:MKSetGap([[DropdownRow alloc] initWithOptions:@[@"Safe Mode", @"Brutal Mode"]
                                                indexPtr:&s_playMode width:kRowW], 9) group:0];
    // ── Insta Pocket ──────────────────────────────────────────────────────
    [p add:[[ActionRow alloc] initWithTitle:@"💥  Insta Pocket (all balls)"
                                    symbol:@"arrow.down.circle.fill"
                                     width:kRowW
                                    action:^{
        // Trigger one-shot teleport on next render frame
        g_instaPocket = true;
    }] group:0];
    // Aim
    [p add:[self boolRow:@"Auto Aim (rotate cue)" ptr:&AutoPlay::bAutoAim asSwitch:YES] group:1];
    // Queue
    [p add:[self boolRow:@"AutoQueue (Rematch)" ptr:&AutoQueue::bAutoQueue asSwitch:YES] group:2];
}

// ── Page 1: AIM ASSIST + Native Power + Auto Win ───────────────────────────
- (void)buildAim:(MenuPage *)p {
    [p setSegments:@[@"Aim Assist", @"Power", @"Win"]];

    // ── Aim Assist ────────────────────────────────────────────────────────
    [p add:[self boolRow:@"Aim Assist" ptr:&g_aimAssistEnabled asSwitch:YES] group:0];
    [p add:mkLabel(@"Gain") group:0];
    [p add:[[FloatRow alloc] initWithTitle:@"Gain X"
                                      ptr:&g_aimGainX min:0.0 max:5.0 fmt:@"%.2f"
                                    width:kRowW] group:0];
    [p add:[[FloatRow alloc] initWithTitle:@"Gain Y"
                                      ptr:&g_aimGainY min:0.0 max:5.0 fmt:@"%.2f"
                                    width:kRowW] group:0];
    [p add:[[FloatRow alloc] initWithTitle:@"Gain Z"
                                      ptr:&g_aimGainZ min:0.0 max:5.0 fmt:@"%.2f"
                                    width:kRowW] group:0];
    [p add:mkLabel(@"Line Style") group:0];
    [p add:[[FloatRow alloc] initWithTitle:@"Thickness"
                                      ptr:&g_aimLineThickness min:0.5 max:8.0 fmt:@"%.1f"
                                    width:kRowW] group:0];
    [p add:MKSetGap([[DropdownRow alloc]
                     initWithOptions:@[@"⬛ Black", @"🔴 Red", @"🟢 Green", @"🟣 Purple", @"⬜ White"]
                            indexPtr:&g_aimLineColor width:kRowW], 9) group:0];

    // ── Native Power ──────────────────────────────────────────────────────
    [p add:[self boolRow:@"Native Power" ptr:&g_nativePowerEnabled asSwitch:YES] group:1];
    [p add:mkLabel(@"Power level") group:1];
    [p add:[[FloatRow alloc] initWithTitle:@"Power"
                                      ptr:&g_nativePowerValue min:0.0 max:1.0 fmt:@"%.2f"
                                    width:kRowW] group:1];

    // ── Auto Win ──────────────────────────────────────────────────────────
    [p addTop:mkNote(@"One-shot: teleports all balls into pockets immediately.",
                     @[@"One-shot", @"immediately"])];
    [p add:[[ActionRow alloc]
            initWithTitle:@"🏆  AUTO WIN (One Shot)"
                   symbol:@"trophy.fill"
                    width:kRowW
                   action:^{
        g_autoWinPending = true;
        ESP_SaveSettings();
    }] group:2];
}

// ── Page 2: Visuals ────────────────────────────────────────────────────────
- (void)buildVisuals:(MenuPage *)p {
    [p setSegments:@[@"Prediction", @"ESP Overlay"]];

    // ── Prediction Lines / Pockets ────────────────────────────────────────
    [p add:[self boolRow:@"Enable ESP"        ptr:&g_espEnabled           asSwitch:NO] group:0];
    [p add:[self boolRow:@"Prediction Lines"  ptr:&g_drawPredictionLine   asSwitch:NO] group:0];
    [p add:[self boolRow:@"Draw Pockets"      ptr:&g_drawPockets          asSwitch:NO] group:0];
    [p add:[self boolRow:@"Pocket Shot State" ptr:&g_drawPocketsShotState asSwitch:NO] group:0];
    [p add:mkLabel(@"Line") group:0];
    [p add:MKSetGap([[FloatRow alloc] initWithTitle:@"Line Width"
                                               ptr:&g_lineWidth min:0.5 max:5.0 fmt:@"%.1f"
                                             width:kRowW], 9) group:0];

    // ── ESP LINE / BOX / NAME (สีดำ ด้านบนจอ) ────────────────────────────
    [p add:mkNote(@"Lines drawn from top of screen. Color: Black.",
                  @[@"Black"]) group:1];
    [p add:[self boolRow:@"ESP Lines (top → enemy)"  ptr:&g_espLines asSwitch:YES] group:1];
    [p add:[self boolRow:@"ESP Box"                  ptr:&g_espBox   asSwitch:YES] group:1];
    [p add:[self boolRow:@"ESP Name (above box)"     ptr:&g_espName  asSwitch:YES] group:1];
}

// ── Page 2: Misc (Protection | Debug) ──────────────────────────────────────
- (void)buildMisc:(MenuPage *)p {
    [p addTop:mkNote(@"Some options in this section may not be entirely safe. Use with caution and at your own risk.",
                     @[@"not be entirely safe", @"caution", @"your own risk"])];
    [p setSegments:@[@"Protection", @"Debug"]];
    [p add:[self boolRow:@"I'm Not Hacker (stream)" ptr:&g_noScreenShot asSwitch:NO] group:0];
    _irFPS = [[InfoRow alloc] initWithTitle:@"FPS" width:kRowW];
    [p add:_irFPS group:1];
    [p add:[[ActionRow alloc] initWithTitle:@"Save Log to Documents" symbol:@"square.and.arrow.down" width:kRowW
        action:^{
            @try {
                NSMutableString *log = [NSMutableString string];
                [log appendFormat:@"=== 8BP Debug Log ===\n"];
                [log appendFormat:@"Date: %@\n", [NSDate date]];
                [log appendFormat:@"InGame: %d  PlayerTurn: %d\n", isInGame(), isPlayerTurn()];
                [log appendFormat:@"Rule: %s\n", isNineBallRuntime() ? "9-Ball" : "8-Ball"];
                [log appendFormat:@"Balls: %d\n", getTableBallCount(GM_Table())];
                NSString *path = [NSSearchPathForDirectoriesInDomains(
                    NSDocumentDirectory, NSUserDomainMask, YES).firstObject
                    stringByAppendingPathComponent:@"8bp_debug.txt"];
                [log writeToFile:path atomically:YES encoding:NSUTF8StringEncoding error:nil];
            } @catch (...) {}
        }] group:1];
}

// ── Page 3: Settings (table calibration) ───────────────────────────────────
- (void)buildSettings:(MenuPage *)p {
    [p add:[[FloatRow alloc] initWithTitle:@"Offset X"      ptr:&g_tableOffsetX     min:-50  max:50  fmt:@"%.1f" width:kRowW] group:0];
    [p add:[[FloatRow alloc] initWithTitle:@"Offset Y"      ptr:&g_tableOffsetY     min:-50  max:50  fmt:@"%.1f" width:kRowW] group:0];
    [p add:[[FloatRow alloc] initWithTitle:@"Scale Factor"  ptr:&g_tableScaleFactor min:0.9 max:1.1 fmt:@"%.3f" width:kRowW] group:0];
}

// ── Page 4: Account (live game status) ─────────────────────────────────────
- (void)buildAccount:(MenuPage *)p {
    _irInGame = [[InfoRow alloc] initWithTitle:@"In Game" width:kRowW];        [p add:_irInGame group:0];
    _irTurn   = [[InfoRow alloc] initWithTitle:@"Player Turn" width:kRowW];    [p add:_irTurn   group:0];
    _irRule   = [[InfoRow alloc] initWithTitle:@"Rule" width:kRowW];           [p add:_irRule   group:0];
    _irBalls  = [[InfoRow alloc] initWithTitle:@"Balls on table" width:kRowW]; [p add:_irBalls  group:0];
}

// ── Show / Hide / Toggle ───────────────────────────────────────────────────
- (CGFloat)fitScaleIn:(UIView *)root {
    CGFloat sw = (root.bounds.size.width  - 8) / kW;
    CGFloat sh = (root.bounds.size.height - 8) / kH;
    return MIN(1.0, MIN(sw, sh));
}

- (void)show {
    if (self.superview) {
        self.hidden = NO; [self.superview bringSubviewToFront:self];
        for (BoolRow *r in _boolRows) [r refresh];
        [self startRefresh];
        return;
    }
    UIWindow *w = [UIApplication sharedApplication].keyWindow
                  ?: [UIApplication sharedApplication].windows.firstObject;
    UIView *root = w.rootViewController.view;
    if (!root) return;
    CGFloat fit = [self fitScaleIn:root];
    self.center = CGPointMake(root.bounds.size.width/2, root.bounds.size.height/2);
    [root addSubview:self];
    self.alpha = 0; self.transform = CGAffineTransformMakeScale(0.92*fit, 0.92*fit);
    [UIView animateWithDuration:0.25 delay:0
         usingSpringWithDamping:0.8 initialSpringVelocity:0
                        options:0
                     animations:^{ self.alpha = 1; self.transform = CGAffineTransformMakeScale(fit, fit); }
                     completion:nil];
    [self startRefresh];
}

- (void)hide {
    CGFloat fit = self.superview ? [self fitScaleIn:self.superview] : 1.0;
    [UIView animateWithDuration:0.18 animations:^{
        self.alpha = 0; self.transform = CGAffineTransformMakeScale(0.94*fit, 0.94*fit);
    } completion:^(BOOL f) {
        self.hidden = YES; self.alpha = 1; self.transform = CGAffineTransformMakeScale(fit, fit);
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
        _irInGame.valueLabel.text  = isInGame()      ? @"Yes" : @"No";
        _irTurn.valueLabel.text    = isPlayerTurn()  ? @"Yes" : @"No";
        _irRule.valueLabel.text    = isNineBallRuntime() ? @"9-Ball" : @"8-Ball";
        _irBalls.valueLabel.text   = [NSString stringWithFormat:@"%d", getTableBallCount(GM_Table())];
        _irFPS.valueLabel.text     = [NSString stringWithFormat:@"%.0f", ImGui::GetIO().Framerate];
        for (BoolRow *r in _boolRows) [r refresh];
    } @catch (...) {}
}

@end
