// Exact-build extensions to the existing hideAds option. Helpers deliberately
// distinguish Void/Object error callbacks from a synchronous Bool rejection.
// No success callback receives a fabricated or nil advertisement.
static NSMutableDictionary *kcAdditionalAdStatus;

static BOOL kcAdTypeIs(const char *type, char expected) {
    if(!type)return NO;
    while(*type && strchr("rnNoORV",*type))type++;
    return *type==expected || (expected=='B' && *type=='c');
}
static BOOL kcAdMethodSignature(Method method, char result, NSUInteger objectArguments) {
    if(!method || method_getNumberOfArguments(method)!=objectArguments+2)return NO;
    char type[128];method_getReturnType(method,type,sizeof(type));
    if(!kcAdTypeIs(type,result))return NO;
    for(NSUInteger i=0;i<objectArguments;i++) {
        method_getArgumentType(method,(unsigned int)i+2,type,sizeof(type));
        if(!kcAdTypeIs(type,'@') || strstr(type,"@?"))return NO;
    }
    return YES;
}
static void kcAdNotifyFailure(id loader, NSString *callbackName, BOOL errorOnly) {
    dispatch_async(dispatch_get_main_queue(),^{
        SEL getter=NSSelectorFromString(@"delegate");
        Method get=class_getInstanceMethod([loader class],getter);
        if(!kcAdMethodSignature(get,'@',0))return;
        id delegate=((id(*)(id,SEL))objc_msgSend)(loader,getter);
        SEL callback=NSSelectorFromString(callbackName);
        Method method=class_getInstanceMethod([delegate class],callback);
        if(!kcAdMethodSignature(method,'v',errorOnly?1:2))return;
        NSError *error=[NSError errorWithDomain:NSURLErrorDomain code:NSURLErrorCancelled
            userInfo:@{NSLocalizedDescriptionKey:@"Advertisement disabled locally"}];
        if(errorOnly)((void(*)(id,SEL,id))objc_msgSend)(delegate,callback,error);
        else ((void(*)(id,SEL,id,id))objc_msgSend)(delegate,callback,loader,error);
    });
}
static BOOL kcInstallAdVoid(NSString *className, NSString *selectorName, NSUInteger arguments,
                            NSString *failure, BOOL errorOnly) {
    Class cls=NSClassFromString(className);SEL sel=NSSelectorFromString(selectorName);
    Method method=class_getInstanceMethod(cls,sel);
    if(!kcAdMethodSignature(method,'v',arguments))return NO;
    IMP replacement=NULL;
    void (^blocked)(id)=^(id loader){
        event([@"ads.blocked." stringByAppendingString:className]);
        if(failure)kcAdNotifyFailure(loader,failure,errorOnly);
    };
    // Use exact argument lists instead of invoking a no-argument block through
    // every SDK method's ABI. Values are intentionally not inspected.
    if(arguments==0)replacement=imp_implementationWithBlock(^(id loader){blocked(loader);});
    else if(arguments==2)replacement=imp_implementationWithBlock(^(id loader,id keyword,id region){blocked(loader);});
    else return NO;
    replace(cls,sel,replacement,method_getTypeEncoding(method));return YES;
}
static BOOL kcInstallAdBool(NSString *className, NSString *selectorName) {
    Class cls=NSClassFromString(className);SEL sel=NSSelectorFromString(selectorName);
    Method method=class_getInstanceMethod(cls,sel);
    if(!kcAdMethodSignature(method,'B',3))return NO;
    replace(cls,sel,imp_implementationWithBlock(^BOOL(id loader,id user,id content,id duplicate){
        event(@"ads.blocked.bizboardAdditionalLoad");
        // Synchronous rejection, not a fake successful asynchronous request.
        return NO;
    }),method_getTypeEncoding(method));return YES;
}

#ifndef KC_AD_TESTING
static const uint8_t kcAdSDKUUID[]={0x3b,0xfc,0x9c,0x52,0x67,0x14,0x37,0x8e,0x8b,0x5e,0x84,0x7b,0x0f,0xc1,0x52,0x90};
static const struct mach_header_64 *kcAdSDKImage(void) {
    for(uint32_t i=0;i<_dyld_image_count();i++) {
        const char *path=_dyld_get_image_name(i);
        if(!path || ![[NSString stringWithUTF8String:path] hasSuffix:@"/AdFitPM.framework/AdFitPM"])continue;
        const struct mach_header_64 *h=(const void *)_dyld_get_image_header(i);
        if(h->magic!=MH_MAGIC_64)return NULL;
        const uint8_t *cursor=(const void *)(h+1),*end=cursor+h->sizeofcmds;
        for(uint32_t j=0;j<h->ncmds && cursor+sizeof(struct load_command)<=end;j++) {
            const struct load_command *c=(const void *)cursor;
            if(c->cmdsize<sizeof(*c) || cursor+c->cmdsize>end)return NULL;
            if(c->cmd==LC_UUID && c->cmdsize==sizeof(struct uuid_command))
                return memcmp(((const struct uuid_command *)c)->uuid,kcAdSDKUUID,16)==0?h:NULL;
            cursor+=c->cmdsize;
        }
    }
    return NULL;
}
static void kcAdNativeBlocked(uint32_t route) {
    if(route<2)event(route==0?@"ads.blocked.threadNative":@"ads.blocked.focusNative");
}
static BOOL kcAdEnableNative(const uint8_t *base) {
    if(memcmp(base+0x33f0,"GCADS001",8)!=0)return NO;
    // Check both redirects and their relocated prologues before arming either.
    const uint32_t threadBranch=0x14000000u|((uint32_t)((0x3000-0x131b40)/4)&0x03ffffffu);
    const uint32_t focusBranch=0x14000000u|((uint32_t)((0x3080-0x28b4d0)/4)&0x03ffffffu);
    if(*(const uint32_t *)(base+0x131b40)!=threadBranch || *(const uint32_t *)(base+0x28b4d0)!=focusBranch ||
       *(const uint32_t *)(base+0x3030)!=0xa9bc5ff8 || *(const uint32_t *)(base+0x30b0)!=0xa9ba6ffc)return NO;
    uint64_t *flags=(void *)(base+0x507fe0);void **callback=(void *)(base+0x507fe8);
    if(!ginppaiWritableRegion(flags,16) || *flags!=0 || *callback!=NULL)return NO;
    *callback=(void *)&kcAdNativeBlocked;
    *flags=1;return YES;
}
static void kcAdCollapseView(NSString *className) {
    Class cls=NSClassFromString(className);
    if(!cls || ![cls isSubclassOfClass:UIView.class])return;
    hideClass(className);
    kcAdInstallViewSizing(className);
    kcAdditionalAdStatus[[@"view." stringByAppendingString:className]]=@YES;
}
static void installAdditionalAdBlocking(void) {
    kcAdditionalAdStatus=[NSMutableDictionary dictionaryWithDictionary:@{@"enabled":@(option(@"hideAds")),
        @"nativeThreadFocus":@NO,@"policyFlags":@"unresolved; original policy retained"}];
    if(!option(@"hideAds"))return;
    // These additional SDK paths are verified only for this app/SDK pair.
    if(kcNativeProfile!=2){kcAdditionalAdStatus[@"reason"]=@"Additional routes require verified KakaoTalk 26.8.0";return;}
    const struct mach_header_64 *sdk=kcAdSDKImage();
    if(!sdk){kcAdditionalAdStatus[@"reason"]=@"AdFitPM UUID mismatch or SDK not loaded";return;}
    BOOL native=kcAdEnableNative((const void *)sdk);
    kcAdditionalAdStatus[@"nativeThreadFocus"]=@(native);
    if(!native)kcAdditionalAdStatus[@"nativeReason"]=@"Prepare original IPA with --ad-features; no direct Swift hook installed";
    kcAdditionalAdStatus[@"bizboardAdditionalObjCLoad"]=@(kcInstallAdBool(@"AdFitPM.BizBoardProvider",@"loadWithUserObject:contentObject:duplicateKey:"));
    kcAdditionalAdStatus[@"quickViewObjCLoad"]=@(kcInstallAdVoid(@"AdFitPM.AdFitQuickViewAdLoader",@"loadAdWithKeyword:regionId:",2,@"quickViewAdLoaderDidFailToReceiveAd:error:",NO));
    kcAdditionalAdStatus[@"brandChipObjCLoad"]=@(kcInstallAdVoid(@"AdFitPM.BrandChipAdLoader",@"loadAd",0,@"brandChipAdLoaderDidFailToReceiveAdWithError:",YES));
    kcAdditionalAdStatus[@"nasObjCLoad"]=@(kcInstallAdVoid(@"AdFitPM.NasAdLoader",@"loadAd",0,@"didFailToReceiveAdWithError:",YES));
    for(NSString *name in @[@"OpenLink.OpenChatBizBoardView",@"OpenLink.OpenChatsFeedADTableViewCell",
        @"OpenLink.OpenChatFeedADMediaContainerView",@"OpenLink.OpenChatFeedADSkeletonView",
        @"OpenLink.OpenChatAdCell",@"KakaoTalk.GlobalBizBoardCell",@"Friends.PlusFriendsAdSectionCell",
        @"FriendsFeedPresentation.FriendFeedADCell",@"FriendsFeedPresentation.FeedADHeaderView"])
        kcAdCollapseView(name);
    // Do not remove birthday entries or entire Pay/profile webviews. Policy
    // flag getters remain unresolved; exact ad row sizing is handled in AdSpacing.inc.
}
#endif
