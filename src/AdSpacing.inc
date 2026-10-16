// Apply layout changes only to exact advertisement classes observed in the app.
// Parent lists and delegate classes are hooked only after hosting a known ad cell.
static void hideClass(NSString *name);
static const char kcAdOwnHeightKey;
static const char kcAdSizingInstalledKey,kcAdHeightDelegateKey,kcAdSizeDelegateKey;
static const char kcAdCellsKey,kcAdUpdatePendingKey,kcAdLastListKey,kcAdLastPathKey;
static NSArray<NSString *> *kcAdSpacingClasses(void) {
    return @[@"MoreTab.NativeADUIView",@"MoreTab.LocalBizboardUIView",@"TalkAppBase.BizboardCell",
        @"TalkAppBase.BizboardCollectionCell",@"TalkAppBase.BizboardCollectionCellWithoutSoundManager",
        @"TalkAppBase.ChatsGlobalBizboardCell",@"TalkAppBase.GlobalBizboardCollectionCell",
        @"KakaoTalk.GlobalBizBoardCell",@"OpenLink.OpenChatBizBoardView",@"OpenLink.OpenChatsFeedADTableViewCell",
        @"OpenLink.OpenChatFeedADMediaContainerView",@"OpenLink.OpenChatFeedADSkeletonView",@"OpenLink.OpenChatAdCell",
        @"Friends.PlusFriendsAdSectionCell",@"FriendsFeedPresentation.FriendFeedADCell",
        @"FriendsFeedPresentation.FeedADContentsCell",@"FriendsFeedPresentation.FeedADHeaderView"];
}
static BOOL kcIsAdSpacingCell(id cell) {
    return cell && [kcAdSpacingClasses() containsObject:NSStringFromClass([cell class])];
}
static BOOL kcAdLayoutDelegateAllowed(Class cls) {
#ifdef KC_AD_SPACING_TESTING
    return cls!=Nil;
#else
    const char *path=cls?class_getImageName(cls):NULL;
    return path && [[NSString stringWithUTF8String:path] hasPrefix:[NSBundle.mainBundle.bundlePath stringByAppendingString:@"/"]];
#endif
}
static NSMapTable *kcAdCells(UIView *list) {
    NSMapTable *map=objc_getAssociatedObject(list,&kcAdCellsKey);
    if(!map){map=[NSMapTable strongToWeakObjectsMapTable];objc_setAssociatedObject(list,&kcAdCellsKey,map,OBJC_ASSOCIATION_RETAIN_NONATOMIC);}
    return map;
}
static BOOL kcAdCellAtPath(UIView *list,NSIndexPath *path) {
    NSMapTable *map=objc_getAssociatedObject(list,&kcAdCellsKey);
    id cell=[map objectForKey:path];
    if(!kcIsAdSpacingCell(cell))return NO;
    NSIndexPath *current=nil;
    if([list isKindOfClass:UITableView.class])current=[(UITableView *)list indexPathForCell:cell];
    else if([list isKindOfClass:UICollectionView.class])current=[(UICollectionView *)list indexPathForCell:cell];
    // A reused cell or reordered snapshot must never collapse a different row.
    return current && [current isEqual:path];
}
static BOOL kcAdLayoutSignature(Method method,const char *result,NSUInteger arguments) {
    if(!method || method_getNumberOfArguments(method)!=arguments+2)return NO;
    char type[128];method_getReturnType(method,type,sizeof(type));
    if(strcmp(type,result)!=0)return NO;
    for(NSUInteger i=0;i<arguments;i++) {
        method_getArgumentType(method,(unsigned)i+2,type,sizeof(type));
        if(strcmp(type,@encode(id))!=0)return NO;
    }
    return YES;
}
static BOOL kcAdInstallListSizing(UIView *list) {
    if([list isKindOfClass:UITableView.class]) {
        id delegate=((UITableView *)list).delegate;Class cls=[delegate class];
        if(!kcAdLayoutDelegateAllowed(cls))return NO;
        if(objc_getAssociatedObject(cls,&kcAdHeightDelegateKey))return YES;
        SEL sel=@selector(tableView:heightForRowAtIndexPath:);Method method=class_getInstanceMethod(cls,sel);
        // Do not synthesize optional delegate methods or change UIKit defaults.
        if(!kcAdLayoutSignature(method,@encode(CGFloat),2))return NO;
        IMP original=method_getImplementation(method);
        replace(cls,sel,imp_implementationWithBlock(^CGFloat(id owner,UITableView *table,NSIndexPath *path){
            if(kcAdCellAtPath(table,path))return 0.01;
            return ((CGFloat(*)(id,SEL,id,id))original)(owner,sel,table,path);
        }),method_getTypeEncoding(method));
        objc_setAssociatedObject(cls,&kcAdHeightDelegateKey,@YES,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        event(@"ads.spacing.tableDelegateInstalled");return YES;
    }
    if([list isKindOfClass:UICollectionView.class]) {
        id delegate=((UICollectionView *)list).delegate;Class cls=[delegate class];
        if(!kcAdLayoutDelegateAllowed(cls))return NO;
        if(objc_getAssociatedObject(cls,&kcAdSizeDelegateKey))return YES;
        SEL sel=@selector(collectionView:layout:sizeForItemAtIndexPath:);Method method=class_getInstanceMethod(cls,sel);
        if(!kcAdLayoutSignature(method,@encode(CGSize),3))return NO;
        IMP original=method_getImplementation(method);
        replace(cls,sel,imp_implementationWithBlock(^CGSize(id owner,UICollectionView *collection,id layout,NSIndexPath *path){
            CGSize size=((CGSize(*)(id,SEL,id,id,id))original)(owner,sel,collection,layout,path);
            if(kcAdCellAtPath(collection,path))size.height=0.01;
            return size;
        }),method_getTypeEncoding(method));
        objc_setAssociatedObject(cls,&kcAdSizeDelegateKey,@YES,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        event(@"ads.spacing.collectionDelegateInstalled");return YES;
    }
    return NO;
}
static void kcAdRegisterCell(UIView *view) {
    BOOL tableCell=[view isKindOfClass:UITableViewCell.class],collectionCell=[view isKindOfClass:UICollectionViewCell.class];
    if(!tableCell && !collectionCell)return;
    UIView *list=view.superview;
    while(list && !(tableCell?[list isKindOfClass:UITableView.class]:[list isKindOfClass:UICollectionView.class]))list=list.superview;
    if(!list)return;
    NSIndexPath *path=tableCell?[(UITableView *)list indexPathForCell:(id)view]:[(UICollectionView *)list indexPathForCell:(id)view];
    if(!path)return;
    NSMapTable *map=kcAdCells(list);
    // Remove stale keys for this reused ad cell; never retain cells or lists.
    for(NSIndexPath *key in map.keyEnumerator.allObjects) {
        id tracked=[map objectForKey:key];
        if(!tracked || (tracked==view && ![key isEqual:path]))[map removeObjectForKey:key];
    }
    [map setObject:view forKey:path];
    BOOL delegateInstalled=kcAdInstallListSizing(list);
    if(!delegateInstalled && tableCell)return; // self-sizing hooks can still apply
    if(!list.window)return;
    NSHashTable *lastList=objc_getAssociatedObject(view,&kcAdLastListKey);
    NSIndexPath *lastPath=objc_getAssociatedObject(view,&kcAdLastPathKey);
    if(lastList.anyObject==list && [lastPath isEqual:path])return;
    NSHashTable *reference=[NSHashTable weakObjectsHashTable];[reference addObject:list];
    objc_setAssociatedObject(view,&kcAdLastListKey,reference,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    objc_setAssociatedObject(view,&kcAdLastPathKey,path,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    event([@"ads.spacing.tracked:" stringByAppendingString:NSStringFromClass(view.class)]);
    if(objc_getAssociatedObject(list,&kcAdUpdatePendingKey))return;
    objc_setAssociatedObject(list,&kcAdUpdatePendingKey,@YES,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    __weak UIView *weakList=list;
    dispatch_async(dispatch_get_main_queue(),^{
        UIView *current=weakList;if(!current)return;
        objc_setAssociatedObject(current,&kcAdUpdatePendingKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        BOOL busy=[current isKindOfClass:UITableView.class] && ((UITableView *)current).hasUncommittedUpdates;
        if(!current.window || busy) {
            for(UIView *cell in kcAdCells(current).objectEnumerator.allObjects) {
                objc_setAssociatedObject(cell,&kcAdLastListKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
                objc_setAssociatedObject(cell,&kcAdLastPathKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
            }
            return; // Retry on the next cell layout, outside the active update.
        }
        if([current isKindOfClass:UITableView.class]) {
            UITableView *table=(id)current;
            [UIView performWithoutAnimation:^{[table beginUpdates];[table endUpdates];}];
        } else [((UICollectionView *)current).collectionViewLayout invalidateLayout];
        event(@"ads.spacing.listInvalidated");
    });
}
static BOOL kcAdSizingSignature(Method method,NSArray<NSString *> *arguments) {
    if(!method || method_getNumberOfArguments(method)!=arguments.count+2)return NO;
    char type[128];method_getReturnType(method,type,sizeof(type));
    if(strcmp(type,@encode(CGSize))!=0)return NO;
    for(NSUInteger i=0;i<arguments.count;i++) {
        method_getArgumentType(method,(unsigned)i+2,type,sizeof(type));
        if(strcmp(type,arguments[i].UTF8String)!=0)return NO;
    }
    return YES;
}
static void kcAdInstallViewSizing(NSString *name) {
    Class cls=NSClassFromString(name);
    if(!cls || ![cls isSubclassOfClass:UIView.class] || objc_getAssociatedObject(cls,&kcAdSizingInstalledKey))return;
    // Check UIKit's struct signatures before installing ABI-specific blocks.
    SEL fit=@selector(sizeThatFits:);Method method=class_getInstanceMethod(cls,fit);
    if(kcAdSizingSignature(method,@[@(@encode(CGSize))]))replace(cls,fit,imp_implementationWithBlock(^CGSize(UIView *view,CGSize size){return CGSizeMake(size.width,0);}),method_getTypeEncoding(method));
    SEL intrinsic=@selector(intrinsicContentSize);method=class_getInstanceMethod(cls,intrinsic);
    if(kcAdSizingSignature(method,@[]))replace(cls,intrinsic,imp_implementationWithBlock(^CGSize(UIView *view){return CGSizeMake(UIViewNoIntrinsicMetric,0);}),method_getTypeEncoding(method));
    SEL fitting=@selector(systemLayoutSizeFittingSize:);method=class_getInstanceMethod(cls,fitting);
    if(kcAdSizingSignature(method,@[@(@encode(CGSize))]))replace(cls,fitting,imp_implementationWithBlock(^CGSize(UIView *view,CGSize size){return CGSizeMake(size.width,0);}),method_getTypeEncoding(method));
    fitting=@selector(systemLayoutSizeFittingSize:withHorizontalFittingPriority:verticalFittingPriority:);method=class_getInstanceMethod(cls,fitting);
    if(kcAdSizingSignature(method,@[@(@encode(CGSize)),@(@encode(UILayoutPriority)),@(@encode(UILayoutPriority))]))replace(cls,fitting,imp_implementationWithBlock(^CGSize(UIView *view,CGSize size,UILayoutPriority horizontal,UILayoutPriority vertical){return CGSizeMake(size.width,0);}),method_getTypeEncoding(method));
    if([cls isSubclassOfClass:UICollectionViewCell.class]) {
        SEL preferred=@selector(preferredLayoutAttributesFittingAttributes:);method=class_getInstanceMethod(cls,preferred);
        if(kcAdLayoutSignature(method,@encode(id),1))replace(cls,preferred,imp_implementationWithBlock(^UICollectionViewLayoutAttributes *(UICollectionViewCell *view,UICollectionViewLayoutAttributes *input){
            UICollectionViewLayoutAttributes *result=[input copy];CGSize size=result.size;size.height=0.01;result.size=size;return result;
        }),method_getTypeEncoding(method));
    }
    objc_setAssociatedObject(cls,&kcAdSizingInstalledKey,@YES,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    event([@"ads.spacing.sizing:" stringByAppendingString:name]);
}
static void kcAdCollapseOwnHeight(UIView *view) {
    // Edit only constant-height constraints on this exact ad view, never a
    // parent container, shared layout guide, ordinary message or profile row.
    BOOL changed=NO;
    for(NSLayoutConstraint *constraint in view.constraints)
        if(constraint.active && constraint.firstItem==view && constraint.firstAttribute==NSLayoutAttributeHeight && !constraint.secondItem && constraint.constant>0)
            {constraint.constant=0;changed=YES;}
    if(changed || !objc_getAssociatedObject(view,&kcAdOwnHeightKey)) {
        objc_setAssociatedObject(view,&kcAdOwnHeightKey,@YES,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        [view invalidateIntrinsicContentSize];
    }
    kcAdRegisterCell(view);
}
static void installAdSpacing(void) {
    if(!option(@"hideAds"))return;
    for(NSString *name in kcAdSpacingClasses()){kcAdInstallViewSizing(name);hideClass(name);}
}
