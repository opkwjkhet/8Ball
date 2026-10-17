#pragma once
#import <UIKit/UIKit.h>

// Forward declare ESP.h vars — defined there as static
// We access them via extern bridge in UIKitMenu.mm
@interface UIKitMenu : UIView
+ (instancetype)shared;
- (void)show;
- (void)hide;
- (void)toggle;
@end
