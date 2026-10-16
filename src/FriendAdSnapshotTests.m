#import <Foundation/Foundation.h>
#import <objc/message.h>
static BOOL enabled,ginppaiNativeBuildVerified;
static BOOL option(NSString *key){return enabled;}
static int64_t ginppaiInt(id object,NSString *name){return ((NSInteger(*)(id,SEL))objc_msgSend)(object,NSSelectorFromString(name));}
static void event(NSString *name){}
#include "FriendAdSnapshot.inc"
// Raw pointers make the C ownership contract explicit for the Swift harness.
void *KCTestFriendAdSnapshot(void *input,BOOL hideAds,BOOL verified) {
    @autoreleasepool {
        enabled=hideAds;ginppaiNativeBuildVerified=verified;
        return (void *)CFBridgingRetain(ginppaiFriendPromotionSnapshot((__bridge id)input));
    }
}
