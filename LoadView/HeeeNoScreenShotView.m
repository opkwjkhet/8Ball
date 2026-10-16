//
//  HeeeNoScreenShotView.m
//  HeeeNoScreenShotView-demo-OC
//
//  Created by Heee on 2021/5/18.
//  注意：这个方法仅适用于iOS13及以后的系统

#import "HeeeNoScreenShotView.h"

@interface HeeeNoScreenShotView ()
@property (nonatomic,strong) UITextField *textField;
@property (nonatomic,strong) UIView *clearView;

@end

@implementation HeeeNoScreenShotView

- (instancetype)init {
  self = [super init];
  if (self) {
    _passTouchToContent = NO;
    [self setupUI];
  }
  return self;
}

- (instancetype)initWithFrame:(CGRect)frame {
  self = [super initWithFrame:frame];
  if (self) {
    _passTouchToContent = NO;
    [self setupUI];
  }
  return self;
}

- (instancetype)initWithCoder:(NSCoder *)coder {
  self = [super initWithCoder:coder];
  if (self) {
    _passTouchToContent = NO;
    [self setupUI];
  }
  return self;
}

- (void)layoutSubviews {
  [super layoutSubviews];
  self.textField.frame = self.bounds;
  self.clearView.frame = self.bounds;
}

- (void)setMaskContent:(BOOL)maskContent {
  self.textField.secureTextEntry = maskContent;
}

- (BOOL)maskContent {
  return _textField.secureTextEntry;
}

- (void)setupUI {
  [self addSubview:self.textField];
  self.textField.subviews.firstObject.userInteractionEnabled = YES;
  [self.textField.subviews.firstObject addSubview:self.clearView];
  // Ensure all layers pass touch
  self.userInteractionEnabled = YES;
  self.multipleTouchEnabled = YES;
  self.textField.userInteractionEnabled = YES;
  self.textField.multipleTouchEnabled = YES;
  self.clearView.userInteractionEnabled = YES;
  self.clearView.multipleTouchEnabled = YES;
}

- (void)addSubview:(UIView *)view {
  [super addSubview:view];
  if (self.textField != view) {
    [self.clearView addSubview:view];
  }
}

// Touch passthrough: when passTouchToContent=NO, return nil so touch goes
// to the game view underneath. When YES, forward to content normally.
- (UIView *)hitTest:(CGPoint)point withEvent:(UIEvent *)event {
  if (!self.passTouchToContent) {
    // Keep a tiny interactive corner for AutoPlay toggle when menu is hidden.
    // The ImGui toggle lives at bottom-right and needs touch even in passthrough mode.
    CGFloat kToggleSize = 96.0;
    CGRect cornerRect = CGRectMake(CGRectGetWidth(self.bounds) - kToggleSize,
                                   CGRectGetHeight(self.bounds) - kToggleSize,
                                   kToggleSize, kToggleSize);
    if (CGRectContainsPoint(cornerRect, point)) {
      return [super hitTest:point withEvent:event];
    }
    return nil; // Touch passes through to game view below
  }
  return [super hitTest:point withEvent:event];
}

- (UITextField *)textField {
  if (!_textField) {
    _textField = [[UITextField alloc] init];
    _textField.secureTextEntry = YES;
  }

  return _textField;
}

- (UIView *)clearView {
  if (!_clearView) {
    _clearView = [[UIView alloc] init];
  }

  return _clearView;
}

@end
