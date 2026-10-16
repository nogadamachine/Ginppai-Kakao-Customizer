#import <Foundation/Foundation.h>
#import <objc/runtime.h>
#import <objc/message.h>
#define KC_AD_TESTING 1
static NSMutableDictionary *counts;
static void event(NSString *key){counts[key]=@([counts[key] integerValue]+1);}
static void replace(Class cls,SEL sel,IMP imp,const char *types){
    if(!class_addMethod(cls,sel,imp,types))class_replaceMethod(cls,sel,imp,types);
}
#include "AdditionalAds.inc"

@interface TestAdDelegate:NSObject
@property(nonatomic,strong) id loader;
@property(nonatomic,strong) NSError *error;
@property(nonatomic) NSUInteger received;
@end
@implementation TestAdDelegate
- (void)two:(id)loader error:(NSError *)error{self.loader=loader;self.error=error;self.received++;}
- (void)one:(NSError *)error{self.error=error;self.received++;}
@end
@interface TestAdLoader:NSObject
@property(nonatomic,strong) TestAdDelegate *delegate;
@property(nonatomic) NSUInteger originalLoads;
@end
@implementation TestAdLoader
- (void)load{self.originalLoads++;}
- (void)load:(id)keyword region:(id)region{self.originalLoads++;}
- (BOOL)load:(id)user content:(id)content duplicate:(id)duplicate{self.originalLoads++;return YES;}
- (id)wrong:(id)user content:(id)content duplicate:(id)duplicate{return self;}
- (void)scalar:(NSInteger)value{self.originalLoads++;}
@end
static void drain(void){
    NSDate *deadline=[NSDate dateWithTimeIntervalSinceNow:0.1];
    while([deadline timeIntervalSinceNow]>0)[NSRunLoop.currentRunLoop runUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.005]];
}
static void require(BOOL result,NSString *name){if(!result){NSLog(@"FAIL: %@",name);exit(1);}}
int main(void){@autoreleasepool{
    counts=[NSMutableDictionary new];
    TestAdLoader *loader=[TestAdLoader new];loader.delegate=[TestAdDelegate new];
    require(!kcInstallAdBool(@"TestAdLoader",@"wrong:content:duplicate:"),@"object-return method must not be replaced as Bool");
    require(!kcInstallAdVoid(@"TestAdLoader",@"scalar:",1,nil,NO),@"scalar signature must be rejected");
    require(!kcInstallAdVoid(@"MissingAdClass",@"load",0,nil,NO),@"absent SDK class must be harmless");
    require(kcInstallAdVoid(@"TestAdLoader",@"load:region:",2,@"two:error:",NO),@"install two-object route");
    [loader load:@"keyword" region:@"region"];drain();
    require(loader.originalLoads==0 && loader.delegate.received==1,@"no original request; one callback");
    require(loader.delegate.loader==loader && loader.delegate.error.code==NSURLErrorCancelled,@"callback has correct loader and error");
    require(kcInstallAdVoid(@"TestAdLoader",@"load",0,@"one:",YES),@"install one-error route");
    [loader load];drain();
    require(loader.originalLoads==0 && loader.delegate.received==2 && loader.delegate.error!=nil,@"single argument is error object");
    require(kcInstallAdBool(@"TestAdLoader",@"load:content:duplicate:"),@"install synchronous Bool route");
    require(![loader load:nil content:nil duplicate:nil] && loader.originalLoads==0,@"Bool rejection does not start request");
    loader.delegate=nil;[loader load];drain();
    require([counts[@"ads.blocked.bizboardAdditionalLoad"] integerValue]==1,@"diagnostic event");
    puts("Additional advertisement ABI and callback tests passed");
}return 0;}
