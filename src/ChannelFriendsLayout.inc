// Friends/Channels peer presentation for the exact 26.9.2 appearance profile.
// Native friend/feed protocol storage, records and fetch predicates are untouched.
#include <math.h>
static const char KCCFFriendsStateKey, KCCFOwnedChannelKey;
static NSInteger (*KCCFOriginalChannelSectionCount)(id,SEL,id);
@class KCCFFriendsPaneState;
static void KCCFRefreshFriends(UIViewController *host);
static BOOL KCCFSetFriendsPane(KCCFFriendsPaneState *state, BOOL channels);

@interface KCCFFriendsSegments : KCNavigationChipSelector
@end
@implementation KCCFFriendsSegments
- (BOOL)pointInside:(CGPoint)point withEvent:(UIEvent *)event {
    if(self.hidden || !self.enabled || !self.userInteractionEnabled)return NO;
    CGFloat extra=MAX(0,(44-CGRectGetHeight(self.bounds))/2);
    return CGRectContainsPoint(CGRectInset(self.bounds,0,-extra),point);
}
@end

@interface KCCFFriendsPaneState : NSObject
@property(nonatomic,weak) UIViewController *host;
@property(nonatomic,weak) UIView *header;
@property(nonatomic,weak) UICollectionView *nativeCollection;
@property(nonatomic,weak) UIView *coveredNativeView;
@property(nonatomic,copy) NSDictionary *coveredNativeFlags;
@property(nonatomic,copy) NSDictionary *collectionFlags;
@property(nonatomic,strong) KCCFFriendsSegments *segments;
@property(nonatomic,strong) UIViewController *channels;
@property(nonatomic,strong) UIView *channelSurface;
@property(nonatomic) BOOL channelsSelected;
@property(nonatomic) BOOL updating;
@property(nonatomic) BOOL normalizationPending;
@property(nonatomic) BOOL normalizationAttempted;
@property(nonatomic) BOOL nativeBuildFailed;
- (void)segmentChanged:(KCNavigationChipSelector *)sender;
@end
@implementation KCCFFriendsPaneState
- (void)segmentChanged:(KCNavigationChipSelector *)sender {
    if(sender!=self.segments || self.updating)return;
    BOOL requested=sender.selectedSegmentIndex==1;
    if(!KCCFSetFriendsPane(self,requested))sender.selectedSegmentIndex=self.channelsSelected?1:0;
}
@end

static BOOL KCCFMethod(Method method, const char *result, const char *arguments) {
    NSUInteger count=strlen(arguments);
    if(!method || method_getNumberOfArguments(method)!=count+2)return NO;
    char type[128];method_getReturnType(method,type,sizeof(type));if(strcmp(type,result))return NO;
    for(NSUInteger index=0;index<count;index++) {
        method_getArgumentType(method,(unsigned)index+2,type,sizeof(type));
        if(type[0]!=arguments[index])return NO;
    }
    return YES;
}
static id KCCFField(id object, NSString *name) {
    if(!object || !name)return nil;
    Class bridge=NSClassFromString(@"KCChannelNavigationBridge");SEL selector=NSSelectorFromString(@"field:name:");
    if(!KCCFMethod(class_getClassMethod(bridge,selector),"@","@@"))return nil;
    return ((id(*)(id,SEL,id,id))objc_msgSend)(bridge,selector,object,name);
}
static BOOL KCCFControllerWithin(UIViewController *controller, UIViewController *ancestor) {
    for(NSUInteger depth=0;controller && depth<16;controller=controller.parentViewController,depth++)if(controller==ancestor)return YES;
    return NO;
}
static BOOL KCCFHostVisible(UIViewController *host) {
    if(!host || ![NSThread isMainThread] || !kcAppearanceOnlyBuild ||
       ![NSStringFromClass(host.class) isEqualToString:@"FriendsFeedPresentation.FriendsTabContainerViewController"] ||
       !host.isViewLoaded || !host.view.window || host.presentedViewController || host.transitionCoordinator ||
       host.navigationController.presentedViewController || host.tabBarController.presentedViewController)return NO;
    for(UIView *view=host.view;view;view=view.superview)if(view.hidden || view.alpha<0.01)return NO;
    UIViewController *top=host.navigationController.topViewController;
    if(top && !KCCFControllerWithin(host,top))return NO;
    UIViewController *selected=host.tabBarController.selectedViewController;
    if(selected && !KCCFControllerWithin(host,selected))return NO;
    return YES;
}
static UIViewController *KCCFNativeFriends(UIViewController *host, UIView *content) {
    UIViewController *native=host.childViewControllerForStatusBarStyle;
    if(!native || ![NSStringFromClass(native.class) isEqualToString:@"FriendsFeedPresentation.FriendsListViewController"] ||
       !native.isViewLoaded || native.isEditing || native.parentViewController!=host || !native.view.window ||
       ![native.view isDescendantOfView:content])return nil;
    return native;
}
static void KCCFFindHeaderCollections(UIView *view, UIView *header, NSMutableArray *result, NSUInteger depth) {
    if(depth>8 || result.count>1)return;
    if([view isKindOfClass:UICollectionView.class] && ((UICollectionView *)view).delegate==(id)header) {
        [result addObject:view];return;
    }
    for(UIView *child in view.subviews)KCCFFindHeaderCollections(child,header,result,depth+1);
}
static UICollectionView *KCCFHeaderCollection(UIView *header) {
    NSMutableArray *collections=[NSMutableArray new];KCCFFindHeaderCollections(header,header,collections,0);
    if(collections.count!=1)return nil;
    UICollectionView *collection=collections.firstObject;
    id source=ginppaiFriendSource(collection),snapshot=ginppaiCall(source,@"snapshot");
    NSArray *items=ginppaiFriendItems(snapshot);
    if(items.count!=2 || ginppaiFriendListIndex(items)<0 || collection.numberOfSections!=1 ||
       [collection numberOfItemsInSection:0]!=2)return nil;
    return collection;
}
static UIView *KCCFMainBar(UIView *view, NSUInteger depth) {
    if(!view || depth>10)return nil;
    if([NSStringFromClass(view.class) isEqualToString:@"KakaoTalk.MainTabBarView"])return view;
    for(UIView *child in view.subviews){UIView *found=KCCFMainBar(child,depth+1);if(found)return found;}
    return nil;
}
static UIFont *KCCFFirstFont(UIView *view, NSUInteger depth) {
    if(depth>8)return nil;
    if([view isKindOfClass:UILabel.class])return ((UILabel *)view).font;
    for(UIView *child in view.subviews){UIFont *font=KCCFFirstFont(child,depth+1);if(font)return font;}
    return nil;
}
static BOOL KCCFChannelContextReady(UIViewController *host) {
    Class bridge=NSClassFromString(@"KCChannelNavigationBridge");SEL selector=NSSelectorFromString(@"canBuildChannelFriendsFrom:");
    if(!KCCFMethod(class_getClassMethod(bridge,selector),@encode(BOOL),"@"))return NO;
    return ((BOOL(*)(id,SEL,id))objc_msgSend)(bridge,selector,host);
}
static void KCCFRestoreNativeView(KCCFFriendsPaneState *state) {
    UIView *view=state.coveredNativeView;NSDictionary *flags=state.coveredNativeFlags;
    if(view && flags) {
        if(view.hidden)view.hidden=[flags[@"hidden"] boolValue];
        if(!view.userInteractionEnabled)view.userInteractionEnabled=[flags[@"interaction"] boolValue];
        if(view.accessibilityElementsHidden)view.accessibilityElementsHidden=[flags[@"accessibility"] boolValue];
    }
    state.coveredNativeView=nil;state.coveredNativeFlags=nil;
}
static void KCCFCoverNativeView(KCCFFriendsPaneState *state, UIView *view) {
    if(state.coveredNativeView!=view){KCCFRestoreNativeView(state);state.coveredNativeView=view;
        state.coveredNativeFlags=@{@"hidden":@(view.hidden),@"interaction":@(view.userInteractionEnabled),@"accessibility":@(view.accessibilityElementsHidden)};}
    view.hidden=YES;view.userInteractionEnabled=NO;view.accessibilityElementsHidden=YES;
}
static BOOL KCCFPlaceChannels(KCCFFriendsPaneState *state, UIView *content) {
    UIViewController *host=state.host;UICollectionView *collection=state.nativeCollection;
    if(!collection || collection.window!=content.window || ![collection isDescendantOfView:host.view])return NO;
    CGRect chipRect=[collection convertRect:collection.bounds toView:content];
    CGFloat top=MAX(CGRectGetMinY(content.bounds),CGRectGetMaxY(chipRect));
    CGFloat bottom=CGRectGetMaxY(content.bounds)-MAX(0,host.view.safeAreaInsets.bottom);
    UIView *bar=KCCFMainBar(host.tabBarController.view,0);
    if(bar.window==content.window && !bar.hidden && bar.alpha>0.01) {
        CGRect barRect=[bar convertRect:bar.bounds toView:content];
        if(CGRectIntersectsRect(barRect,content.bounds))bottom=MIN(bottom,CGRectGetMinY(barRect));
    }
    CGRect frame=CGRectMake(CGRectGetMinX(content.bounds),top,CGRectGetWidth(content.bounds),bottom-top);
    if(!isfinite(frame.origin.y) || !isfinite(frame.size.height) || frame.size.width<100 || frame.size.height<100)return NO;
    if(!state.channelSurface) {
        state.channelSurface=[UIView new];state.channelSurface.clipsToBounds=YES;
        state.channelSurface.backgroundColor=content.backgroundColor?:UIColor.systemBackgroundColor;
    }
    if(state.channelSurface.superview!=content){[state.channelSurface removeFromSuperview];[content addSubview:state.channelSurface];}
    if(!CGRectEqualToRect(state.channelSurface.frame,frame))state.channelSurface.frame=frame;
    [content bringSubviewToFront:state.channelSurface];
    UIView *childView=state.channels.view;
    childView.autoresizingMask=UIViewAutoresizingFlexibleWidth|UIViewAutoresizingFlexibleHeight;
    if(childView.superview!=state.channelSurface){[childView removeFromSuperview];[state.channelSurface addSubview:childView];}
    if(!CGRectEqualToRect(childView.frame,state.channelSurface.bounds))childView.frame=state.channelSurface.bounds;
    return YES;
}
static void KCCFRemoveChannelChild(KCCFFriendsPaneState *state, BOOL appearance) {
    UIViewController *child=state.channels;
    if(child.parentViewController==state.host) {
        [child willMoveToParentViewController:nil];
        if(appearance)[child beginAppearanceTransition:NO animated:NO];
        [child.view removeFromSuperview];[child removeFromParentViewController];
        if(appearance)[child endAppearanceTransition];
    }
    [state.channelSurface removeFromSuperview];KCCFRestoreNativeView(state);
    state.channelsSelected=NO;state.segments.selectedSegmentIndex=0;
}
static void KCCFRestoreHeaderCollection(KCCFFriendsPaneState *state) {
    UICollectionView *collection=state.nativeCollection;NSDictionary *flags=state.collectionFlags;
    if(collection && flags) {
        if(collection.hidden)collection.hidden=[flags[@"hidden"] boolValue];
        if(!collection.userInteractionEnabled)collection.userInteractionEnabled=[flags[@"interaction"] boolValue];
        if(collection.accessibilityElementsHidden)collection.accessibilityElementsHidden=[flags[@"accessibility"] boolValue];
    }
}
static void KCCFRestoreUnsupportedLayout(KCCFFriendsPaneState *state) {
    if(!state || state.updating)return;
    state.updating=YES;
    KCSetFollowedChannelsReady(state.host,NO);
    @try {KCCFRemoveChannelChild(state,NO);KCCFRestoreHeaderCollection(state);[state.segments removeFromSuperview];}
    @finally {state.updating=NO;}
}
static BOOL KCCFSetFriendsPane(KCCFFriendsPaneState *state, BOOL channels) {
    UIViewController *host=state.host;
    if(state.updating || !KCCFHostVisible(host))return NO;
    UIView *content=KCCFField(host,@"contentView");
    if(![content isKindOfClass:UIView.class] || ![content isDescendantOfView:host.view])return NO;
    UIViewController *native=KCCFNativeFriends(host,content);
    if(!native)return NO;
    state.updating=YES;
    __block BOOL appearing=NO;
    @try {
        if(!channels) {
            KCCFRemoveChannelChild(state,YES);
            event(@"navigation.friends.normal-selected");return YES;
        }
        // The healthy Friends mount prepares this controller before its entry
        // is removed from the native list. Selection never starts another fetch.
        if(!state.channels || state.nativeBuildFailed)return NO;
        UIViewController *child=state.channels;
        if(child.parentViewController && child.parentViewController!=host)return NO;
        BOOL attach=child.parentViewController==nil;
        if(attach){[host addChildViewController:child];[child beginAppearanceTransition:YES animated:NO];appearing=YES;}
        if(!KCCFPlaceChannels(state,content)) {
            if(attach){[child willMoveToParentViewController:nil];[child removeFromParentViewController];[child endAppearanceTransition];appearing=NO;}
            [state.channelSurface removeFromSuperview];return NO;
        }
        if(attach){[child didMoveToParentViewController:host];[child endAppearanceTransition];appearing=NO;}
        KCCFCoverNativeView(state,native.view);
        BOOL changed=!state.channelsSelected;state.channelsSelected=YES;state.segments.selectedSegmentIndex=1;
        if(changed)event(@"navigation.friends.channels-selected");return YES;
    } @catch(NSException *exception) {
        // Native Objective-C exception does not authorize a replacement model.
        @try {if(appearing)[state.channels endAppearanceTransition];KCCFRemoveChannelChild(state,NO);} @catch(NSException *cleanup) {}
        state.channels=nil;state.nativeBuildFailed=YES;KCSetFollowedChannelsReady(host,NO);
        event(@"navigation.friends.native-channel-unavailable");KCCFRestoreHeaderCollection(state);[state.segments removeFromSuperview];return NO;
    } @finally {state.updating=NO;}
}
static BOOL KCCFPrepareChannels(KCCFFriendsPaneState *state) {
    UIViewController *host=state.host;
    if(state.nativeBuildFailed || !KCCFHostVisible(host))return NO;
    if(state.channels){KCSetFollowedChannelsReady(host,YES);return YES;}
    if(!KCCFChannelContextReady(host)) {
        KCSetFollowedChannelsReady(host,NO);return NO;
    }
    Class cls=NSClassFromString(@"KakaoTalk.MyPlusFriendsViewController");
    if(!cls || ![cls isSubclassOfClass:UIViewController.class] ||
       !KCCFMethod(class_getInstanceMethod(cls,@selector(init)),"@","")) {
        KCSetFollowedChannelsReady(host,NO);return NO;
    }
    state.updating=YES;
    @try {
        // The verified native initializer performs its existing read-only FRC
        // fetch. Do not load its view, attach it or send appearance callbacks.
        UIViewController *child=[[cls alloc] init];
        if(![NSStringFromClass(child.class) isEqualToString:@"KakaoTalk.MyPlusFriendsViewController"] ||
           child.parentViewController) {
            state.nativeBuildFailed=YES;KCSetFollowedChannelsReady(host,NO);return NO;
        }
        objc_setAssociatedObject(child,&KCCFOwnedChannelKey,@YES,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        state.channels=child;
        KCSetFollowedChannelsReady(host,YES);
        event(@"navigation.friends.native-channel-prepared");return YES;
    } @catch(NSException *exception) {
        state.nativeBuildFailed=YES;state.channels=nil;KCSetFollowedChannelsReady(host,NO);
        event(@"navigation.friends.native-channel-unavailable");return NO;
    } @finally {state.updating=NO;}
}
static void KCCFNormalizeFriends(KCCFFriendsPaneState *state) {
    UIViewController *host=state.host;
    if(state.normalizationPending || state.normalizationAttempted || !KCCFHostVisible(host) ||
       ![NSStringFromClass(host.childViewControllerForStatusBarStyle.class) isEqualToString:@"FriendsFeedPresentation.FriendsFeedViewController"])return;
    state.normalizationPending=YES;state.normalizationAttempted=YES;
    __weak KCCFFriendsPaneState *weakState=state;
    dispatch_async(dispatch_get_main_queue(),^{
        KCCFFriendsPaneState *current=weakState;if(!current)return;
        current.normalizationPending=NO;UIViewController *host=current.host;
        if(!KCCFHostVisible(host))return;
        UIView *header=KCCFField(host,@"friendsTabHeaderView");UICollectionView *collection=KCCFHeaderCollection(header);
        if(!header || !collection)return;
        id source=ginppaiFriendSource(collection),snapshot=ginppaiCall(source,@"snapshot");NSArray *items=ginppaiFriendItems(snapshot);
        NSInteger list=ginppaiFriendListIndex(items);SEL select=NSSelectorFromString(@"collectionView:didSelectItemAtIndexPath:");
        if(list<0 || !KCCFMethod(class_getInstanceMethod(header.class,select),"v","@@"))return;
        NSIndexPath *path=[NSIndexPath indexPathForItem:list inSection:0];
        ((void(*)(id,SEL,id,id))objc_msgSend)(header,select,collection,path);
        [collection selectItemAtIndexPath:path animated:NO scrollPosition:UICollectionViewScrollPositionNone];
        event(@"navigation.friends.native-list-normalized");
        dispatch_async(dispatch_get_main_queue(),^{KCCFRefreshFriends(host);});
    });
}
static void KCCFRefreshFriends(UIViewController *host) {
    if(!KCCFHostVisible(host))return;
    KCCFFriendsPaneState *state=objc_getAssociatedObject(host,&KCCFFriendsStateKey);
    if(state.updating)return;
    UIView *header=KCCFField(host,@"friendsTabHeaderView"),*content=KCCFField(host,@"contentView");
    if(![header isKindOfClass:UIView.class] || ![NSStringFromClass(header.class) isEqualToString:@"FriendsFeedPresentation.FriendsTabHeaderView"] ||
       ![content isKindOfClass:UIView.class] || ![header isDescendantOfView:host.view] || ![content isDescendantOfView:host.view]){KCCFRestoreUnsupportedLayout(state);return;}
    UICollectionView *collection=KCCFHeaderCollection(header);if(!collection || collection.window!=host.view.window){KCCFRestoreUnsupportedLayout(state);return;}
    CGRect frame=[collection convertRect:collection.bounds toView:header];
    if(!isfinite(frame.origin.x) || !isfinite(frame.origin.y) || CGRectGetWidth(frame)<100 || CGRectGetHeight(frame)<20 || CGRectGetHeight(frame)>100){KCCFRestoreUnsupportedLayout(state);return;}
    if(!state && (collection.hidden || collection.alpha<0.01 || !collection.userInteractionEnabled || collection.accessibilityElementsHidden))return;
    if(!state){state=[KCCFFriendsPaneState new];state.host=host;objc_setAssociatedObject(host,&KCCFFriendsStateKey,state,OBJC_ASSOCIATION_RETAIN_NONATOMIC);}
    if(state.nativeCollection && state.nativeCollection!=collection)KCCFRestoreHeaderCollection(state);
    if(state.nativeCollection!=collection)state.collectionFlags=@{@"hidden":@(collection.hidden),@"interaction":@(collection.userInteractionEnabled),@"accessibility":@(collection.accessibilityElementsHidden)};
    state.header=header;state.nativeCollection=collection;
    if(!KCCFNativeFriends(host,content)){KCCFRestoreUnsupportedLayout(state);KCCFNormalizeFriends(state);return;}
    if(!KCCFPrepareChannels(state)){KCCFRestoreUnsupportedLayout(state);return;}
    if(!state.segments) {
        BOOL korean=[NSBundle.mainBundle.preferredLocalizations.firstObject hasPrefix:@"ko"];
        state.segments=[[KCCFFriendsSegments alloc] initWithItems:korean?@[@"친구",@"채널"]:@[@"Friends",@"Channels"]];
        state.segments.accessibilityIdentifier=@"Ginppai.FriendsChannels";
        [state.segments addTarget:state action:@selector(segmentChanged:) forControlEvents:UIControlEventValueChanged];
        state.segments.selectedSegmentIndex=state.channelsSelected?1:0;
        event(@"navigation.friends.channel-selector-mounted");
    }
    UIFont *font=KCCFFirstFont(collection,0);
    if(font){[state.segments setTitleTextAttributes:@{NSFontAttributeName:font} forState:UIControlStateNormal];[state.segments setTitleTextAttributes:@{NSFontAttributeName:font} forState:UIControlStateSelected];}
    if(state.segments.superview!=header){[state.segments removeFromSuperview];[header addSubview:state.segments];}
    state.segments.frame=frame;[header bringSubviewToFront:state.segments];
    collection.hidden=YES;collection.userInteractionEnabled=NO;collection.accessibilityElementsHidden=YES;
    if(state.channelsSelected)KCCFSetFriendsPane(state,YES);
}
static BOOL KCCFOwnedChannelAdSection(id controller, UITableView *table, NSInteger section) {
    if(section!=0 || !kcAppearanceOnlyBuild || !objc_getAssociatedObject(controller,&KCCFOwnedChannelKey) ||
       ![NSStringFromClass([controller class]) isEqualToString:@"KakaoTalk.MyPlusFriendsViewController"] ||
       table.dataSource!=controller || table.delegate!=controller || !KCCFOriginalChannelSectionCount)return NO;
    NSInteger count=KCCFOriginalChannelSectionCount(controller,@selector(numberOfSectionsInTableView:),table);
    return count>=2 && count<=4;
}
static void KCCFRefreshAfterAppearance(UIViewController *host) {
    __weak UIViewController *weakHost=host;
    dispatch_async(dispatch_get_main_queue(),^{KCCFRefreshFriends(weakHost);});
    id<UIViewControllerTransitionCoordinator> transition=host.transitionCoordinator;
    if(transition)[transition animateAlongsideTransition:nil completion:^(id<UIViewControllerTransitionCoordinatorContext> context){
        dispatch_async(dispatch_get_main_queue(),^{KCCFRefreshFriends(weakHost);});
    }];
}
static BOOL KCInstallChannelFriendsLayout(void) {
    if(!kcAppearanceOnlyBuild)return NO;
    Class host=NSClassFromString(@"FriendsFeedPresentation.FriendsTabContainerViewController");
    Class header=NSClassFromString(@"FriendsFeedPresentation.FriendsTabHeaderView");
    Class channels=NSClassFromString(@"KakaoTalk.MyPlusFriendsViewController");
    if(![host isSubclassOfClass:UIViewController.class] || ![header isSubclassOfClass:UIView.class] ||
       ![channels isSubclassOfClass:UIViewController.class])return NO;
    Method layout=class_getInstanceMethod(host,@selector(viewDidLayoutSubviews));
    Method appear=class_getInstanceMethod(host,@selector(viewDidAppear:));
    Method headerLayout=class_getInstanceMethod(header,@selector(layoutSubviews));
    if(!KCCFMethod(layout,"v","") || !KCCFMethod(appear,"v","B") || !KCCFMethod(headerLayout,"v",""))return NO;
    IMP originalLayout=method_getImplementation(layout),originalAppear=method_getImplementation(appear),originalHeaderLayout=method_getImplementation(headerLayout);
    replace(host,@selector(viewDidLayoutSubviews),imp_implementationWithBlock(^(UIViewController *vc){((void(*)(id,SEL))originalLayout)(vc,@selector(viewDidLayoutSubviews));KCCFRefreshFriends(vc);}),method_getTypeEncoding(layout));
    replace(host,@selector(viewDidAppear:),imp_implementationWithBlock(^(UIViewController *vc,BOOL animated){((void(*)(id,SEL,BOOL))originalAppear)(vc,@selector(viewDidAppear:),animated);KCCFFriendsPaneState *state=objc_getAssociatedObject(vc,&KCCFFriendsStateKey);state.normalizationAttempted=NO;KCCFRefreshFriends(vc);KCCFRefreshAfterAppearance(vc);}),method_getTypeEncoding(appear));
    replace(header,@selector(layoutSubviews),imp_implementationWithBlock(^(UIView *view){
        ((void(*)(id,SEL))originalHeaderLayout)(view,@selector(layoutSubviews));
        for(UIResponder *r=view.nextResponder;r;r=r.nextResponder)if([NSStringFromClass(r.class) isEqualToString:@"FriendsFeedPresentation.FriendsTabContainerViewController"]){KCCFRefreshFriends((id)r);break;}
    }),method_getTypeEncoding(headerLayout));
    Method sections=class_getInstanceMethod(channels,@selector(numberOfSectionsInTableView:));
    if(KCCFMethod(sections,@encode(NSInteger),"@")) {
        KCCFOriginalChannelSectionCount=(NSInteger(*)(id,SEL,id))method_getImplementation(sections);
        // Preserve native row indices and FRC/update accounting. Collapse only
        // the presentation of the verified native first advertising section.
        for(NSString *name in @[@"tableView:heightForRowAtIndexPath:",@"tableView:estimatedHeightForRowAtIndexPath:"]) {
            SEL selector=NSSelectorFromString(name);Method method=class_getInstanceMethod(channels,selector);
            if(!KCCFMethod(method,@encode(CGFloat),"@@"))continue;IMP original=method_getImplementation(method);
            replace(channels,selector,imp_implementationWithBlock(^CGFloat(id vc,UITableView *table,NSIndexPath *path){
                return path && KCCFOwnedChannelAdSection(vc,table,path.section)?0:((CGFloat(*)(id,SEL,id,id))original)(vc,selector,table,path);
            }),method_getTypeEncoding(method));
        }
        for(NSString *name in @[@"tableView:heightForHeaderInSection:",@"tableView:heightForFooterInSection:"]) {
            SEL selector=NSSelectorFromString(name);Method method=class_getInstanceMethod(channels,selector);if(!KCCFMethod(method,@encode(CGFloat),"@q"))continue;IMP original=method_getImplementation(method);
            replace(channels,selector,imp_implementationWithBlock(^CGFloat(id vc,UITableView *table,NSInteger section){return KCCFOwnedChannelAdSection(vc,table,section)?CGFLOAT_MIN:((CGFloat(*)(id,SEL,id,NSInteger))original)(vc,selector,table,section);}),method_getTypeEncoding(method));
        }
        for(NSString *name in @[@"tableView:viewForHeaderInSection:",@"tableView:viewForFooterInSection:"]) {
            SEL selector=NSSelectorFromString(name);Method method=class_getInstanceMethod(channels,selector);if(!KCCFMethod(method,"@","@q"))continue;IMP original=method_getImplementation(method);
            replace(channels,selector,imp_implementationWithBlock(^id(id vc,UITableView *table,NSInteger section){return KCCFOwnedChannelAdSection(vc,table,section)?nil:((id(*)(id,SEL,id,NSInteger))original)(vc,selector,table,section);}),method_getTypeEncoding(method));
        }
    }
    return YES;
}
