//
//  HeeeNoScreenShotView.h
//  HeeeNoScreenShotView-demo-OC
//
//  Created by Heee on 2021/5/18.
//

#import <UIKit/UIKit.h>

NS_ASSUME_NONNULL_BEGIN

@interface HeeeNoScreenShotView : UIView
/// Khi YES, nội dung bên trong sẽ bị che (secureTextEntry) khi chụp màn hình.
@property (nonatomic, assign) BOOL maskContent;
/// Khi YES, hitTest trả về MTKView con. Khi NO, hitTest trả nil (touch xuyên qua).
@property (nonatomic, assign) BOOL passTouchToContent;
@end

NS_ASSUME_NONNULL_END
