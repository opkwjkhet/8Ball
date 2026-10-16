#import "NutLog.h"
#import <objc/runtime.h>
#import <objc/message.h>

@implementation SwitchView {
    UISwitch *crashSwitch;
}

static SwitchView *sharedInstance = nil;

+ (void)showInView:(UIView *)parentView {
    if (!sharedInstance) {
        CGFloat switchWidth = 40;
        CGFloat switchHeight = 50;
        CGFloat marginTop = 20;
        CGRect frame = CGRectMake((parentView.frame.size.width - switchWidth) / 2,
                                  marginTop,
                                  switchWidth,
                                  switchHeight);
        sharedInstance = [[SwitchView alloc] initWithFrame:frame];
        [parentView addSubview:sharedInstance];
    }
}

+ (void)removeFromParentView {
    if (sharedInstance) {
        [sharedInstance removeFromSuperview];
        sharedInstance = nil;
    }
}

- (instancetype)initWithFrame:(CGRect)frame {
    self = [super initWithFrame:frame];
    if (self) {
        crashSwitch = [[UISwitch alloc] init];
        crashSwitch.transform = CGAffineTransformMakeScale(0.6, 0.6);
        crashSwitch.center = CGPointMake(frame.size.width / 2, frame.size.height / 2);
        
        [crashSwitch addTarget:self action:@selector(switchToggled:) forControlEvents:UIControlEventValueChanged];
        [self addSubview:crashSwitch];
        
        crashSwitch.onTintColor = [UIColor redColor];
        crashSwitch.tintColor = [UIColor grayColor];
    }
    return self;
}


- (void)OpenGame {
    Class LSApplicationWorkspace = objc_getClass("LSApplicationWorkspace");
    SEL selector = NSSelectorFromString(@"defaultWorkspace");
    id workspace = ((id (*)(Class, SEL))objc_msgSend)(LSApplicationWorkspace, selector);
    
    SEL openAppSelector = NSSelectorFromString(@"openApplicationWithBundleID:");
    ((void (*)(id, SEL, id))objc_msgSend)(workspace, openAppSelector, @"com.garena.game.kgvn");
}

- (void)switchToggled:(UISwitch *)sender {
    
    [self OpenGame]; 
}

@end