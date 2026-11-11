// Exact-build presentation partition. Native managed objects, identities,
// friendship state and unread counters are never edited.
#import <CoreData/CoreData.h>
#import <ptrauth.h>
static const char KCChannelRouteKey,KCNormalFetchPredicateKey,KCChannelTabStateKey;
static const char KCNowNormalizePendingKey;
static __weak id KCChannelCoordinator;
static UIViewController *KCQuietController;
static BOOL KCQuietModelReady;
static BOOL KCPrepareQuietController(void);
static const char KCNormalNativePredicateKey,KCNormalQuietPredicateKey;
typedef struct KCChannelRouteScope {
    __unsafe_unretained UINavigationController *origin;
    __unsafe_unretained UINavigationController *destination;
    __unsafe_unretained UIViewController *owner;
    BOOL redirected;
} KCChannelRouteScope;
static __thread KCChannelRouteScope *KCActiveChannelRoute;
static BOOL KCScopedNativeChannelRoute(UIViewController *owner,id objectID,id coordinator) {
    if(![NSThread isMainThread] || !owner.view.window || owner.isEditing)return NO;
    UIViewController *normal=KCNALField(coordinator,@"chatsTableViewController");
    UINavigationController *origin=normal.navigationController,*destination=owner.navigationController;
    UITabBarController *main=owner.tabBarController;
    if(!origin || !destination || origin==destination || ![tabKind(main.selectedViewController) isEqualToString:@"now"] ||
       destination.presentedViewController || destination.transitionCoordinator || origin.transitionCoordinator ||
       ![owner.view isDescendantOfView:destination.topViewController.view])return NO;
    KCChannelRouteScope scope={origin,destination,owner,NO},*previous=KCActiveChannelRoute;
    KCActiveChannelRoute=&scope;
    @try {
        ((BOOL(*)(id,SEL,id,id,id))objc_msgSend)(NSClassFromString(@"KCChannelNavigationBridge"),NSSelectorFromString(@"routeChannelObjectID:from:coordinator:"),objectID,owner,coordinator);
    }@finally {KCActiveChannelRoute=previous;}
    return scope.redirected;
}
static void KCInstallChannelPushRouting(void) {
    Class cls=UINavigationController.class;SEL selector=@selector(pushViewController:animated:);Method method=class_getInstanceMethod(cls,selector);
    if(!method || method_getNumberOfArguments(method)!=4)return;char type[64];method_getReturnType(method,type,sizeof(type));if(strcmp(type,"v"))return;
    method_getArgumentType(method,2,type,sizeof(type));if(type[0]!='@')return;method_getArgumentType(method,3,type,sizeof(type));if(type[0]!='B' && type[0]!='c')return;
    IMP original=method_getImplementation(method);
    replace(cls,selector,imp_implementationWithBlock(^(UINavigationController *self,UIViewController *controller,BOOL animated){
        KCChannelRouteScope *scope=KCActiveChannelRoute;
        if([NSThread isMainThread] && scope && !scope->redirected && self==scope->origin && scope->owner.view.window &&
           scope->destination.topViewController && !scope->destination.presentedViewController && !scope->destination.transitionCoordinator && !self.transitionCoordinator &&
           [tabKind(scope->owner.tabBarController.selectedViewController) isEqualToString:@"now"] &&
           [scope->owner.view isDescendantOfView:scope->destination.topViewController.view] && controller && !controller.parentViewController) {
            scope->redirected=YES;
            [scope->destination pushViewController:controller animated:animated];
            event(@"navigation.channels.route-on-third-tab");return;
        }
        ((void(*)(id,SEL,id,BOOL))original)(self,selector,controller,animated);
    }),method_getTypeEncoding(method));
}
static uintptr_t KCBadgeSnapshotAppCallers[2],KCBadgeSnapshotOpenCallers[2];
static NSHashTable *KCReadyFollowedSources,*KCTrackedFollowedSources;
static const char KCFriendUnfilteredSnapshotKey,KCFriendFilteredSnapshotKey,KCFriendSourceGenerationKey,KCFriendReplayTokensKey;
static BOOL KCFollowedSourceReady(id source) {
    @synchronized(NSClassFromString(@"KCChannelNavigationBridge")){return [KCReadyFollowedSources containsObject:source];}
}
static UIViewController *KCFindController(UIViewController *root,NSString *name,NSUInteger depth);
static BOOL KCBadgeSnapshotCaller(void *address,BOOL app) {
    uintptr_t pc=(uintptr_t)ptrauth_strip(address,ptrauth_key_return_address);
    uintptr_t *allowed=app?KCBadgeSnapshotAppCallers:KCBadgeSnapshotOpenCallers;
    return allowed[0] && (pc==allowed[0] || pc==allowed[1]);
}
static id KCNavigationFilterFriendChannels(id source,id snapshot) {
    if(!kcAppearanceOnlyBuild)return snapshot;
    BOOL ready=NO;
    @synchronized(NSClassFromString(@"KCChannelNavigationBridge")) {
        if(![KCTrackedFollowedSources containsObject:source])return snapshot;
        ready=[KCReadyFollowedSources containsObject:source];
    }
    __block BOOL replay=NO,staleReplay=NO;
    @synchronized(source) {
        NSMutableArray *tokens=objc_getAssociatedObject(source,&KCFriendReplayTokensKey);
        NSArray *sections=KCNALObjectGetter(snapshot,@"sectionIdentifiers"),*items=KCNALObjectGetter(snapshot,@"itemIdentifiers");
        NSUInteger generation=[objc_getAssociatedObject(source,&KCFriendSourceGenerationKey) unsignedIntegerValue];
        for(NSDictionary *token in [tokens copy])if([sections isEqualToArray:token[@"sections"]] && [items isEqualToArray:token[@"items"]]) {
            replay=YES;staleReplay=[token[@"generation"] unsignedIntegerValue]!=generation || [token[@"ready"] boolValue]!=ready;
            [tokens removeObjectIdenticalTo:token];break;
        }
        if(!replay)objc_setAssociatedObject(source,&KCFriendSourceGenerationKey,@(generation+1),OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    // UIKit may clone a snapshot and lose associated markers. Source-scoped
    // replay tokens match opaque identifier arrays, preserve the native-input
    // generation/cache, and discard an obsolete replay at the final apply hook.
    if(staleReplay)return KCNALObjectGetter(source,@"snapshot")?:snapshot;
    if(!ready)return snapshot;
    @try {
        NSArray *items=KCNALObjectGetter(snapshot,@"itemIdentifiers");if(![items isKindOfClass:NSArray.class] || items.count>20000)return snapshot;
        Class bridge=NSClassFromString(@"KCChannelNavigationBridge");SEL s=NSSelectorFromString(@"friendChannelItemIndexes:");
        NSArray *values=((id(*)(id,SEL,id))objc_msgSend)(bridge,s,items);NSIndexSet *indexes=KCNALIndexes(values,items.count);
        SEL remove=NSSelectorFromString(@"deleteItemsWithIdentifiers:");
        if(![snapshot respondsToSelector:@selector(copyWithZone:)] || !KCNALSignature(class_getInstanceMethod([snapshot class],remove),"v",1))return snapshot;
        if(!replay && !objc_getAssociatedObject(snapshot,&KCFriendFilteredSnapshotKey))objc_setAssociatedObject(source,&KCFriendUnfilteredSnapshotKey,[snapshot copy],OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        if(!indexes.count)return snapshot;
        id copy=[snapshot copy];if(copy==snapshot)return snapshot;
        NSArray *sections=KCNALObjectGetter(snapshot,@"sectionIdentifiers");
        ((void(*)(id,SEL,id))objc_msgSend)(copy,remove,[items objectsAtIndexes:indexes]);
        NSMutableArray *expected=[items mutableCopy];[expected removeObjectsAtIndexes:indexes];
        if(![KCNALObjectGetter(copy,@"itemIdentifiers") isEqualToArray:expected] || ![KCNALObjectGetter(copy,@"sectionIdentifiers") isEqualToArray:sections])return snapshot;
        objc_setAssociatedObject(copy,&KCFriendFilteredSnapshotKey,@YES,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        event(@"navigation.friends.channels-separated");return copy;
    }@catch(NSException *exception){return snapshot;}
}
static void KCSetFollowedChannelsReady(UIViewController *host,BOOL ready) {
    UIViewController *list=KCFindController(host,@"FriendsFeedPresentation.FriendsListViewController",0);
    id wrapper=KCNALField(list,@"dataSource");Class bridge=NSClassFromString(@"KCChannelNavigationBridge");
    id source=wrapper?((id(*)(id,SEL,id))objc_msgSend)(bridge,NSSelectorFromString(@"diffableImplementation:"),wrapper):nil;
    if(!source)return;
    @synchronized(NSClassFromString(@"KCChannelNavigationBridge")) {
        if(!KCReadyFollowedSources)KCReadyFollowedSources=[NSHashTable weakObjectsHashTable];
        if(!KCTrackedFollowedSources)KCTrackedFollowedSources=[NSHashTable weakObjectsHashTable];
        [KCTrackedFollowedSources addObject:source];
        if([KCReadyFollowedSources containsObject:source]==ready)return;
        if(ready)[KCReadyFollowedSources addObject:source];else[KCReadyFollowedSources removeObject:source];
    }
    __block NSUInteger expectedGeneration;
    @synchronized(source){expectedGeneration=[objc_getAssociatedObject(source,&KCFriendSourceGenerationKey) unsignedIntegerValue];}
    SEL apply=NSSelectorFromString(@"applyDifferencesFromSnapshot:animatingDifferences:completion:");
    Method method=class_getInstanceMethod([source class],apply);char type[64];
    if(!method || method_getNumberOfArguments(method)!=5)return;
    method_getReturnType(method,type,sizeof(type));if(strcmp(type,"v"))return;
    for(unsigned i=2;i<5;i++){method_getArgumentType(method,i,type,sizeof(type));if(i==3){if(type[0]!='B' && type[0]!='c')return;}else if(type[0]!='@')return;}
    __weak id weak=source;
    dispatch_async(dispatch_get_main_queue(),^{
        id current=weak;if(!current || KCFollowedSourceReady(current)!=ready)return;
        id snapshot=nil;
        __block NSUInteger replayGeneration=expectedGeneration;
        if(ready) {
            @synchronized(current){replayGeneration=[objc_getAssociatedObject(current,&KCFriendSourceGenerationKey) unsignedIntegerValue];}
            snapshot=KCNALObjectGetter(current,@"snapshot");
        }
        else @synchronized(current) {
            if([objc_getAssociatedObject(current,&KCFriendSourceGenerationKey) unsignedIntegerValue]!=expectedGeneration)return;
            snapshot=objc_getAssociatedObject(current,&KCFriendUnfilteredSnapshotKey);
        }
        if(!snapshot)return;
        NSArray *sections=KCNALObjectGetter(snapshot,@"sectionIdentifiers"),*items=KCNALObjectGetter(snapshot,@"itemIdentifiers");
        if(![sections isKindOfClass:NSArray.class] || ![items isKindOfClass:NSArray.class])return;
        __block NSDictionary *token;
        @synchronized(current) {
            if([objc_getAssociatedObject(current,&KCFriendSourceGenerationKey) unsignedIntegerValue]!=replayGeneration)return;
            NSMutableArray *tokens=objc_getAssociatedObject(current,&KCFriendReplayTokensKey);
            if(!tokens){tokens=[NSMutableArray new];objc_setAssociatedObject(current,&KCFriendReplayTokensKey,tokens,OBJC_ASSOCIATION_RETAIN_NONATOMIC);}
            token=@{@"sections":sections,@"items":items,@"generation":@(replayGeneration),@"ready":@(ready)};
            [tokens addObject:token];
        }
        void (^completion)(void)=^{id target=weak;if(!target)return;@synchronized(target){[objc_getAssociatedObject(target,&KCFriendReplayTokensKey) removeObjectIdenticalTo:token];}};
        ((void(*)(id,SEL,id,BOOL,id))objc_msgSend)(current,apply,snapshot,NO,completion);
    });
}
static NSInteger KCChannelKind(id chat) {
    Class bridge=NSClassFromString(@"KCChannelNavigationBridge");SEL selector=NSSelectorFromString(@"channelKind:");
    if(!KCNALSignature(class_getClassMethod(bridge,selector),@encode(NSInteger),1))return -1;
    return ((NSInteger(*)(id,SEL,id))objc_msgSend)(bridge,selector,chat);
}
static BOOL KCPrepareNormalFetch(NSFetchedResultsController *frc) {
    // Legacy normal Chats has its own FRC. Open, archive and channel controllers
    // are distinct classes; never modify a shared request or cached controller.
    if(![NSStringFromClass([frc.delegate class]) isEqualToString:@"KakaoTalk.ChatsTableViewController"])return NO;
    if(frc.cacheName){event(@"navigation.normal.cached-request-unsupported");return NO;}
    NSString *entityName=frc.fetchRequest.entityName,*className=frc.fetchRequest.entity.managedObjectClassName;
    BOOL chatEntity=[entityName isEqualToString:@"Chat"] || (className && NSClassFromString(className)==NSClassFromString(@"Chat"));
    if(frc.managedObjectContext.concurrencyType!=NSMainQueueConcurrencyType || !chatEntity){event(@"navigation.normal.entity-or-context-unsupported");return NO;}
    Class bridge=NSClassFromString(@"KCChannelNavigationBridge");
    SEL factory=NSSelectorFromString(@"excludingChannelPredicate");
    if(!KCNALSignature(class_getClassMethod(bridge,factory),"@",0))return NO;
    NSPredicate *exclude=((id(*)(id,SEL))objc_msgSend)(bridge,factory);
    if(![exclude isKindOfClass:NSPredicate.class])return NO;
    NSPredicate *quiet=nil;
    if(KCQuietModelReady) {
        SEL selector=NSSelectorFromString(@"excludingQuietEntryInContext:");
        if(KCNALSignature(class_getClassMethod(bridge,selector),"@",1))quiet=((id(*)(id,SEL,id))objc_msgSend)(bridge,selector,frc.managedObjectContext);
    }
    NSPredicate *current=frc.fetchRequest.predicate,*last=objc_getAssociatedObject(frc,&KCNormalFetchPredicateKey);
    if(current==last && [objc_getAssociatedObject(frc,&KCNormalQuietPredicateKey) boolValue]==(quiet!=nil))return NO;
    id base=current;
    if(current==last){base=objc_getAssociatedObject(frc,&KCNormalNativePredicateKey);if(base==NSNull.null)base=nil;}
    else objc_setAssociatedObject(frc,&KCNormalNativePredicateKey,current?:NSNull.null,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    // A native folder change may replace the request predicate. Partition that
    // new predicate too, without retaining an obsolete folder restriction.
    NSMutableArray *parts=[NSMutableArray arrayWithObject:exclude];if(base)[parts addObject:base];if(quiet)[parts addObject:quiet];
    NSPredicate *partition=[NSCompoundPredicate andPredicateWithSubpredicates:parts];
    frc.fetchRequest.predicate=partition;
    objc_setAssociatedObject(frc,&KCNormalFetchPredicateKey,partition,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    objc_setAssociatedObject(frc,&KCNormalQuietPredicateKey,@(quiet!=nil),OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    if(quiet)event(@"navigation.normal.quiet-entry-moved");
    event(@"navigation.normal.channel-predicate");return YES;
}
static void KCRefreshNormalController(UIViewController *controller) {
    id frc=KCNALField(controller,@"fetchedResultsController");
    if(![frc isKindOfClass:NSFetchedResultsController.class] || !KCPrepareNormalFetch(frc))return;
    NSError *error=nil;
    if([frc performFetch:&error] && [controller isKindOfClass:UITableViewController.class]) {
        [((UITableViewController *)controller).tableView reloadData];
        event(@"navigation.normal.partition-refreshed");
    }
}
static void KCInstallNormalPartition(void) {
    Class cls=NSFetchedResultsController.class;SEL selector=@selector(performFetch:);
    Method method=class_getInstanceMethod(cls,selector);
    if(method && method_getNumberOfArguments(method)==3) {
        char ret[32],arg[32];method_getReturnType(method,ret,sizeof(ret));method_getArgumentType(method,2,arg,sizeof(arg));
        if((!strcmp(ret,"B") || !strcmp(ret,"c")) && !strcmp(arg,"^@")) {
            IMP old=method_getImplementation(method);
            replace(cls,selector,imp_implementationWithBlock(^BOOL(NSFetchedResultsController *self,NSError **error){
                KCPrepareNormalFetch(self);return ((BOOL(*)(id,SEL,NSError **))old)(self,selector,error);
            }),method_getTypeEncoding(method));
        }
    }
    Class normal=NSClassFromString(@"KakaoTalk.ChatsTableViewController");
    SEL appear=@selector(viewWillAppear:);Method m=class_getInstanceMethod(normal,appear);
    if(KCNALBoolLifecycle(m)){IMP old=method_getImplementation(m);replace(normal,appear,imp_implementationWithBlock(^(UIViewController *self,BOOL animated){
        ((void(*)(id,SEL,BOOL))old)(self,appear,animated);KCPrepareQuietController();KCRefreshNormalController(self);
    }),method_getTypeEncoding(m));}
    Class host=NSClassFromString(@"KakaoTalk.ChatsViewController");SEL load=@selector(viewDidLoad);m=class_getInstanceMethod(host,load);
    if(KCNALSignature(m,"v",0)){IMP old=method_getImplementation(m);replace(host,load,imp_implementationWithBlock(^(UIViewController *self){
        ((void(*)(id,SEL))old)(self,load);KCChannelCoordinator=KCNALField(self,@"contentCoordinator");
    }),method_getTypeEncoding(m));}
    // Preserve the native datasource's row indices and update accounting. Only
    // the known section-zero advertisement row's measured/estimated height is
    // collapsed; a late native reload still has a valid row to address.
    for(NSString *name in @[@"tableView:heightForRowAtIndexPath:",@"tableView:estimatedHeightForRowAtIndexPath:"]) {
        SEL s=NSSelectorFromString(name);m=class_getInstanceMethod(normal,s);
        if(!KCNALSignature(m,@encode(CGFloat),2))continue;IMP old=method_getImplementation(m);
        replace(normal,s,imp_implementationWithBlock(^CGFloat(id self,UITableView *table,NSIndexPath *path){
            if(path.section==0){event(@"navigation.normal.bizboard-collapsed");return 0;}
            return ((CGFloat(*)(id,SEL,id,id))old)(self,s,table,path);
        }),method_getTypeEncoding(m));
    }
    for(NSString *name in @[@"tableView:heightForHeaderInSection:",@"tableView:heightForFooterInSection:",@"tableView:estimatedHeightForHeaderInSection:",@"tableView:estimatedHeightForFooterInSection:"]) {
        SEL s=NSSelectorFromString(name);m=class_getInstanceMethod(normal,s);if(!m || strcmp(method_getTypeEncoding(m),"d32@0:8@16q24"))continue;IMP old=method_getImplementation(m);
        replace(normal,s,imp_implementationWithBlock(^CGFloat(id self,UITableView *table,NSInteger section){return section==0?CGFLOAT_MIN:((CGFloat(*)(id,SEL,id,NSInteger))old)(self,s,table,section);}),method_getTypeEncoding(m));
    }
}
static void KCInstallBadgePartition(void) {
    // Read-only caller identity, gated by the already verified exact main UUID.
    // The native getter also participates in write/update comparisons elsewhere;
    // changing its result there would affect account state. Only the four
    // statically verified badge-snapshot calls receive presentation overrides.
    for(uint32_t i=0;i<_dyld_image_count();i++) {
        const char *path=_dyld_get_image_name(i);
        if(!path || strcmp(strrchr(path,'/')?:path,"/KakaoTalk"))continue;
        uintptr_t base=(uintptr_t)_dyld_get_image_header(i);
        KCBadgeSnapshotAppCallers[0]=base+0x2d8be8;KCBadgeSnapshotAppCallers[1]=base+0x2d8d28;
        KCBadgeSnapshotOpenCallers[0]=base+0x2d8bf4;KCBadgeSnapshotOpenCallers[1]=base+0x2d8d34;break;
    }
    Class cls=NSClassFromString(@"Chat");
    for(NSString *name in @[@"isExcludeFromAppBadge",@"isExcludeFromOpenChatTabBadge"]) {
        SEL selector=NSSelectorFromString(name);Method m=class_getInstanceMethod(cls,selector);
        if(!m || strcmp(method_getTypeEncoding(m),"B16@0:8"))continue;
        IMP old=method_getImplementation(m);BOOL app=[name isEqualToString:@"isExcludeFromAppBadge"];
        replace(cls,selector,imp_implementationWithBlock(^BOOL(id self){
            void *caller=__builtin_return_address(0);
            if(!KCBadgeSnapshotCaller(caller,app))return ((BOOL(*)(id,SEL))old)(self,selector);
            event(app?@"navigation.badge.app-snapshot":@"navigation.badge.internal-snapshot");
            NSInteger kind=KCChannelKind(self);
            if(kind==1 || (app && kind==2))return YES;
            // Nested native Open→App getter calls have a different return PC,
            // and therefore automatically use their original semantics.
            return ((BOOL(*)(id,SEL))old)(self,selector);
        }),method_getTypeEncoding(m));
    }
    // Leave internal native Open unread publishers intact. Only badges under
    // the mapped third bottom button are hidden, including dot-style badges.
    for(NSString *name in @[@"TalkDesignSystemUIKit.UnreadBadge",@"TalkDesignSystemUIKit.UnreadDotBadge"]) {
        Class badge=NSClassFromString(name);SEL s=@selector(layoutSubviews);Method m=class_getInstanceMethod(badge,s);
        if(!KCNALSignature(m,"v",0))continue;IMP old=method_getImplementation(m);
        replace(badge,s,imp_implementationWithBlock(^(UIView *self){
            ((void(*)(id,SEL))old)(self,s);
            UIView *button=self.superview;while(button && ![NSStringFromClass(button.class) isEqualToString:@"KakaoTalk.MainTabBarButton"])button=button.superview;
            if(!button)return;UIView *bar=nativeMainBar(button.superview);if(!bar)bar=button.superview;
            UITabBarController *main=mainController(bar);NSUInteger index=tabButtonIndex(bar,(id)button,main);
            if(index==NSNotFound || index>=main.viewControllers.count || ![tabKind(main.viewControllers[index]) isEqualToString:@"now"])return;
            self.hidden=YES;self.accessibilityElementsHidden=YES;
        }),method_getTypeEncoding(m));
    }
}
static UIViewController *KCFindController(UIViewController *root,NSString *name,NSUInteger depth) {
    if(!root || depth>12)return nil;if([NSStringFromClass(root.class) isEqualToString:name])return root;
    for(UIViewController *child in root.childViewControllers){UIViewController *found=KCFindController(child,name,depth+1);if(found)return found;}return nil;
}
static UIView *KCFindNamedView(UIView *root,NSString *name,NSUInteger depth) {
    if(!root || depth>18)return nil;if([NSStringFromClass(root.class) isEqualToString:name])return root;
    for(UIView *child in root.subviews){UIView *found=KCFindNamedView(child,name,depth+1);if(found)return found;}return nil;
}
static BOOL KCPrepareQuietController(void) {
    if(KCQuietController) {
        id frc=KCNALField(KCQuietController,@"fetchedResultsController");
        if([frc isKindOfClass:NSFetchedResultsController.class] && ((NSFetchedResultsController *)frc).managedObjectContext.concurrencyType==NSMainQueueConcurrencyType)KCQuietModelReady=YES;
        return YES;
    }
    if(![NSThread isMainThread] || !KCChannelCoordinator)return NO;
    Class bridge=NSClassFromString(@"KCChannelNavigationBridge");SEL ready=NSSelectorFromString(@"canBuildChannelChatsFromCoordinator:");
    if(!KCNALSignature(class_getClassMethod(bridge,ready),@encode(BOOL),1) || !((BOOL(*)(id,SEL,id))objc_msgSend)(bridge,ready,KCChannelCoordinator))return NO;
    Class cls=NSClassFromString(@"KakaoTalk.ChatsArchiveTableViewController");Method init=class_getInstanceMethod(cls,@selector(initWithStyle:));
    if(![cls isSubclassOfClass:UITableViewController.class] || !init || strcmp(method_getTypeEncoding(init),"@24@0:8q16"))return NO;
    @try {
        UITableViewController *controller=[(UITableViewController *)[cls alloc] initWithStyle:UITableViewStylePlain];if(controller.parentViewController)return NO;
        [controller loadViewIfNeeded];
        id frc=KCNALField(controller,@"fetchedResultsController");
        KCQuietModelReady=[frc isKindOfClass:NSFetchedResultsController.class] && ((NSFetchedResultsController *)frc).managedObjectContext.concurrencyType==NSMainQueueConcurrencyType;
        KCQuietController=controller;event(@"navigation.quiet.prepared");return YES;
    }@catch(NSException *exception){event(@"navigation.quiet.unavailable");return NO;}
}
@interface KCChannelTabState:NSObject
@property(nonatomic,weak) UIViewController *host;
@property(nonatomic,weak) UIViewController *openController;
@property(nonatomic,strong) UIViewController *channels;
@property(nonatomic,strong) UIViewController *quiet;
@property(nonatomic,strong) KCNavigationChipSelector *selector;
@property(nonatomic,weak) UIView *nativeHeader;
@property(nonatomic,strong) NSTimer *timer;
@property(nonatomic) NSInteger selectedKind;
@property(nonatomic) BOOL creationFailed;
- (BOOL)prepareChannels;
- (void)select:(KCNavigationChipSelector *)sender;
- (void)refresh;
@end
@implementation KCChannelTabState
- (void)dealloc {[_timer invalidate];}
- (BOOL)prepareChannels {
    if(self.channels)return YES;
    if(self.creationFailed || !self.host.view.window || !KCChannelCoordinator)return NO;
    Class bridge=NSClassFromString(@"KCChannelNavigationBridge");SEL ready=NSSelectorFromString(@"canBuildChannelChatsFromCoordinator:");
    if(!KCNALSignature(class_getClassMethod(bridge,ready),@encode(BOOL),1) || !((BOOL(*)(id,SEL,id))objc_msgSend)(bridge,ready,KCChannelCoordinator))return NO;
    @try {
        Class cls=NSClassFromString(@"KakaoTalk.ChannelChatsViewController");
        Method init=class_getInstanceMethod(cls,@selector(initWithNibName:bundle:));
        if(![cls isSubclassOfClass:UIViewController.class] || !KCNALSignature(init,"@",2))return NO;
        UIViewController *vc=[[cls alloc] initWithNibName:nil bundle:nil];
        if(vc.parentViewController)return NO;
        objc_setAssociatedObject(vc,&KCChannelRouteKey,KCChannelCoordinator,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        [vc loadViewIfNeeded];self.channels=vc;event(@"navigation.channels.created");return YES;
    }@catch(NSException *exception){self.creationFailed=YES;event(@"navigation.channels.unavailable");return NO;}
}
- (void)select:(KCNavigationChipSelector *)sender {
    NSInteger selected=sender.selectedSegmentIndex;
    if(selected==1 && ![self prepareChannels]){sender.selectedSegmentIndex=self.selectedKind;return;}
    if(selected==2 && !KCPrepareQuietController()){sender.selectedSegmentIndex=self.selectedKind;return;}
    self.quiet=KCQuietController;self.selectedKind=selected;
    NSArray *panes=@[self.channels?:NSNull.null,self.quiet?:NSNull.null];
    for(NSUInteger i=0;i<panes.count;i++) {
        id object=panes[i];if(object==NSNull.null)continue;UIViewController *pane=object;BOOL active=selected==(NSInteger)i+1;
        if(active && !pane.parentViewController) {
            [self.host addChildViewController:pane];[pane beginAppearanceTransition:YES animated:NO];
            [self.host.view insertSubview:pane.view belowSubview:self.selector];[pane didMoveToParentViewController:self.host];[pane endAppearanceTransition];
        }else if(!active && pane.parentViewController==self.host) {
            [pane willMoveToParentViewController:nil];[pane beginAppearanceTransition:NO animated:NO];
            [pane.view removeFromSuperview];[pane removeFromParentViewController];[pane endAppearanceTransition];
        }
        pane.view.hidden=!active;pane.view.accessibilityElementsHidden=!active;
    }
    self.openController.view.hidden=selected!=0;self.openController.view.accessibilityElementsHidden=selected!=0;
    [self refresh];event(selected==1?@"navigation.channels.selected":(selected==2?@"navigation.quiet.selected":@"navigation.openchat.selected"));
}
- (void)refresh {
    UIViewController *host=self.host;if(!host.isViewLoaded || !host.view.window)return;
    [self prepareChannels];
    if(KCPrepareQuietController())self.quiet=KCQuietController;
    UIView *header=self.nativeHeader;
    header.alpha=0;
    self.selector.frame=[host.view convertRect:header.bounds fromView:header];
    self.selector.hidden=header.hidden;
    for(id object in @[self.channels?:NSNull.null,self.quiet?:NSNull.null]) {
        if(object==NSNull.null)continue;UIViewController *pane=object;
        CGRect frame=[host.view convertRect:self.openController.view.bounds fromView:self.openController.view];
        CGFloat top=CGRectGetMaxY(self.selector.frame);CGFloat bottom=CGRectGetMaxY(frame);
        pane.view.frame=CGRectMake(frame.origin.x,top,frame.size.width,MAX(0,bottom-top));
        [host.view bringSubviewToFront:self.selector];
    }
    Class bridge=NSClassFromString(@"KCChannelNavigationBridge");SEL s=NSSelectorFromString(@"presentationUnread:");
    for(NSInteger index=0;index<3;index++) {
        id vc=index==0?self.openController:(index==1?self.channels:self.quiet);NSInteger count=vc?((NSInteger(*)(id,SEL,id))objc_msgSend)(bridge,s,vc):-1;
        NSString *title=index==0?@"오픈채팅":(index==1?@"채널채팅":@"조용한 채팅");
        [self.selector setTitle:title forSegmentAtIndex:index];
        [self.selector setUnreadCount:count forSegmentAtIndex:index];
    }
}
@end
static void KCUpdateChannelTabs(UIViewController *host) {
    if(!host.isViewLoaded || !host.view.window)return;
    KCChannelTabState *state=objc_getAssociatedObject(host,&KCChannelTabStateKey);
    if(state){[state refresh];return;}
    UIView *header=KCFindNamedView(host.view,@"TalkAppBase.NowTabHeaderChipSelectionView",0);
    UIViewController *open=KCFindController(host,@"KakaoTalk.OpenChatsTableViewController",0);
    if(!header || !open || header.bounds.size.height<30 || header.bounds.size.height>80)return;
    // A sibling overlay replaces only the native peer-selector region. Native
    // Open content and routing stay in their original controller hierarchy.
    state=[KCChannelTabState new];state.host=host;state.openController=open;state.nativeHeader=header;
    state.selector=[[KCNavigationChipSelector alloc] initWithItems:@[@"오픈채팅",@"채널채팅",@"조용한 채팅"]];state.selector.selectedSegmentIndex=0;state.selector.horizontalInset=16;
    state.selector.accessibilityIdentifier=@"KCOpenAndChannelTabs";
    [state.selector addTarget:state action:@selector(select:) forControlEvents:UIControlEventValueChanged];
    header.userInteractionEnabled=NO;header.accessibilityElementsHidden=YES;
    header.alpha=0;
    state.selector.backgroundColor=host.view.backgroundColor?:UIColor.systemBackgroundColor;
    [host.view addSubview:state.selector];objc_setAssociatedObject(host,&KCChannelTabStateKey,state,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    __weak KCChannelTabState *weak=state;
    state.timer=[NSTimer scheduledTimerWithTimeInterval:2 repeats:YES block:^(NSTimer *timer){KCChannelTabState *current=weak;if(current)[current refresh];else[timer invalidate];}];
    [state refresh];event(@"navigation.open-channel.header");
}
static void KCInstallChannelTabs(void) {
    Class host=NSClassFromString(@"KakaoTalk.OpenChatsViewController");
    for(NSString *name in @[@"viewDidLayoutSubviews",@"viewDidAppear:"]) {
        SEL s=NSSelectorFromString(name);Method m=class_getInstanceMethod(host,s);
        if([name hasSuffix:@":"]){if(!KCNALBoolLifecycle(m))continue;IMP old=method_getImplementation(m);replace(host,s,imp_implementationWithBlock(^(UIViewController *self,BOOL animated){((void(*)(id,SEL,BOOL))old)(self,s,animated);KCUpdateChannelTabs(self);}),method_getTypeEncoding(m));}
        else {if(!KCNALSignature(m,"v",0))continue;IMP old=method_getImplementation(m);replace(host,s,imp_implementationWithBlock(^(UIViewController *self){((void(*)(id,SEL))old)(self,s);KCUpdateChannelTabs(self);}),method_getTypeEncoding(m));}
    }
    Class channels=NSClassFromString(@"KakaoTalk.ChannelChatsViewController");SEL s=@selector(collectionView:didSelectItemAtIndexPath:);Method m=class_getInstanceMethod(channels,s);
    if(!KCNALSignature(m,"v",2))return;IMP old=method_getImplementation(m);
    replace(channels,s,imp_implementationWithBlock(^(UIViewController *self,UICollectionView *collection,NSIndexPath *path){
        id coordinator=objc_getAssociatedObject(self,&KCChannelRouteKey);
        if(coordinator) {
            id source=KCNALField(self,@"dataSource");Class bridge=NSClassFromString(@"KCChannelNavigationBridge");
            id impl=source?((id(*)(id,SEL,id))objc_msgSend)(bridge,NSSelectorFromString(@"diffableImplementation:"),source):nil;
            SEL item=NSSelectorFromString(@"itemIdentifierForIndexPath:");
            if(KCNALSignature(class_getInstanceMethod([impl class],item),"@",1)) {
                id identifier=((id(*)(id,SEL,id))objc_msgSend)(impl,item,path);
                id objectID=((id(*)(id,SEL,id))objc_msgSend)(bridge,NSSelectorFromString(@"channelChatObjectID:"),identifier);
                if(objectID && KCScopedNativeChannelRoute(self,objectID,coordinator)) {
                    [collection deselectItemAtIndexPath:path animated:YES];event(@"navigation.channels.native-route");return;
                }
            }
        }
        ((void(*)(id,SEL,id,id))old)(self,s,collection,path);
    }),method_getTypeEncoding(m));
    Class container=NSClassFromString(@"KakaoTalk.NowTabContainerViewController");
    for(NSString *name in @[@"viewDidAppear:",@"viewDidLayoutSubviews"]) {
        SEL lifecycle=NSSelectorFromString(name);Method method=class_getInstanceMethod(container,lifecycle);
        BOOL hasArgument=[name hasSuffix:@":"];
        if(hasArgument?!KCNALBoolLifecycle(method):!KCNALSignature(method,"v",0))continue;
        IMP original=method_getImplementation(method);
        void (^normalize)(UIViewController *)=^(UIViewController *self){
            if(!self.isViewLoaded || !self.view.window || objc_getAssociatedObject(self,&KCNowNormalizePendingKey))return;
            Class bridge=NSClassFromString(@"KCChannelNavigationBridge");
            NSInteger role=((NSInteger(*)(id,SEL,id))objc_msgSend)(bridge,NSSelectorFromString(@"nowCurrentRole:"),self);
            if(role!=0)return;
            objc_setAssociatedObject(self,&KCNowNormalizePendingKey,@YES,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
            __weak UIViewController *weak=self;
            dispatch_async(dispatch_get_main_queue(),^{UIViewController *current=weak;if(!current)return;
                objc_setAssociatedObject(current,&KCNowNormalizePendingKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
                if(!current.isViewLoaded || !current.view.window)return;
                UIView *header=KCFindNamedView(current.view,@"TalkAppBase.NowTabHeaderChipSelectionView",0);
                if(header){selectOpenChat(header);hideBrandChip(header);event(@"navigation.now.native-open-requested");}
            });
        };
        IMP replacement=hasArgument?imp_implementationWithBlock(^(UIViewController *self,BOOL animated){((void(*)(id,SEL,BOOL))original)(self,lifecycle,animated);normalize(self);}):imp_implementationWithBlock(^(UIViewController *self){((void(*)(id,SEL))original)(self,lifecycle);normalize(self);});
        replace(container,lifecycle,replacement,method_getTypeEncoding(method));
    }
}
static void KCInstallChannelNavigation(void) {
    if(!kcAppearanceOnlyBuild)return;
    KCInstallNormalPartition();KCInstallBadgePartition();KCInstallChannelPushRouting();KCInstallChannelTabs();KCInstallMoreHomeOnly();
}
