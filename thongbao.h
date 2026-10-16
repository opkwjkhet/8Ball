#include <UIKit/UIKit.h>

void NSLogC(NSString *format, ...) {
    static NSInteger callCount = 0;
    callCount++;

    // Xử lý chuỗi định dạng với các tham số
    va_list args;
    va_start(args, format);
    NSString *userMessage = [[NSString alloc] initWithFormat:format arguments:args];
    va_end(args);

    // Tạo thông báo đầy đủ, bao gồm số lần gọi
    NSString *fullMessage = [NSString stringWithFormat:@"Hàm này đang được gọi %ld lần\n%@", (long)callCount, userMessage];

    dispatch_async(dispatch_get_main_queue(), ^{
        UIWindow *keyWindow = [UIApplication sharedApplication].keyWindow;
        if (!keyWindow) return;

        UILabel *toastLabel = [[UILabel alloc] init];
        toastLabel.text = fullMessage;
        toastLabel.textColor = [UIColor whiteColor];
        toastLabel.backgroundColor = [[UIColor blackColor] colorWithAlphaComponent:0.7];
        toastLabel.textAlignment = NSTextAlignmentCenter;
        toastLabel.font = [UIFont systemFontOfSize:14.0];
        toastLabel.numberOfLines = 0;
        toastLabel.alpha = 0.0;
        toastLabel.layer.cornerRadius = 10;
        toastLabel.clipsToBounds = YES;

        CGFloat padding = 20;
        CGFloat maxWidth = keyWindow.frame.size.width - 2 * padding;
        CGSize maxSize = CGSizeMake(maxWidth, CGFLOAT_MAX);
        CGSize expectedSize = [toastLabel sizeThatFits:maxSize];

        toastLabel.frame = CGRectMake(padding,
                                      keyWindow.frame.size.height - expectedSize.height - 100,
                                      maxWidth,
                                      expectedSize.height + 20);

        [keyWindow addSubview:toastLabel];

        [UIView animateWithDuration:0.4 animations:^{
            toastLabel.alpha = 1.0;
        } completion:^(BOOL finished) {
            [UIView animateWithDuration:0.4 delay:2.0 options:0 animations:^{
                toastLabel.alpha = 0.0;
            } completion:^(BOOL finished) {
                [toastLabel removeFromSuperview];
            }];
        }];
    });
}

void thongbao(NSString *messageText, ...)
{
    NSDictionary *attributes = @{NSFontAttributeName: [UIFont systemFontOfSize:12]};
    CGSize textSize = [messageText sizeWithAttributes:attributes];

    CGFloat padding = 10;
    CGFloat screenWidth = [UIScreen mainScreen].bounds.size.width;
    CGFloat alertWidth = textSize.width + padding * 2;
    CGFloat alertHeight = textSize.height + padding;
    CGFloat alertX = (screenWidth - alertWidth) / 2;
    CGFloat alertY = 30;

    UIView *alertView = [[UIView alloc] initWithFrame:CGRectMake(alertX, alertY, alertWidth, alertHeight)];
    alertView.backgroundColor = [[UIColor blackColor] colorWithAlphaComponent:0.7];
    alertView.layer.cornerRadius = 8;
    alertView.layer.masksToBounds = YES;
    alertView.alpha = 0.0;

    UILabel *messageLabel = [[UILabel alloc] initWithFrame:CGRectMake(padding, 0, alertWidth - padding * 2, textSize.height)];
    messageLabel.text = messageText;
    messageLabel.textColor = [UIColor whiteColor];
    messageLabel.font = [UIFont systemFontOfSize:12];
    messageLabel.textAlignment = NSTextAlignmentCenter;
    messageLabel.numberOfLines = 0;
    [messageLabel sizeToFit];
    messageLabel.center = CGPointMake(alertWidth / 2, alertHeight / 2);

    [alertView addSubview:messageLabel];

    UIWindow *keyWindow = UIApplication.sharedApplication.keyWindow;
    [keyWindow addSubview:alertView];

    [UIView animateWithDuration:0.3 animations:^{
        alertView.alpha = 1.0;
    } completion:^(BOOL finished) {
        [UIView animateWithDuration:0.3
                              delay:5.0
                            options:UIViewAnimationOptionCurveEaseOut
                         animations:^{
            alertView.alpha = 0.0;
        } completion:^(BOOL finished) {
            [alertView removeFromSuperview];
        }];
    }];
}
