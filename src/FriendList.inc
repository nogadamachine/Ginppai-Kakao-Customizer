// The native header uses a Swift diffable source with private enum identifiers.
// Match those exact enum roles, never translated labels or arbitrary cell order.
static const char ginppaiFriendSelectionPending;
static NSInteger ginppaiFriendListIndex(NSArray *items) {
    return ((NSInteger(*)(id,SEL,id))objc_msgSend)(NSClassFromString(@"KCMessageBridge"),NSSelectorFromString(@"friendListIndex:"),items);
}
static NSArray *ginppaiFriendItems(id snapshot) {
    if(![snapshot respondsToSelector:@selector(itemIdentifiers)])return nil;
    int64_t count=ginppaiInt(snapshot,@"numberOfItems");if(count<1 || count>2)return nil;
    id items=((id(*)(id,SEL))objc_msgSend)(snapshot,@selector(itemIdentifiers));
    if(![items isKindOfClass:NSArray.class] || [items count]<1 || [items count]>2 || ginppaiFriendListIndex(items)<0)return nil;
    return items;
}
static id ginppaiFriendPromotionSnapshot(id snapshot) {
    if(!option(@"hideAds") || !ginppaiNativeBuildVerified || ![snapshot respondsToSelector:@selector(sectionIdentifiers)] ||
       ![snapshot respondsToSelector:@selector(copyWithZone:)] || ![snapshot respondsToSelector:@selector(deleteSectionsWithIdentifiers:)])return snapshot;
    int64_t count=ginppaiInt(snapshot,@"numberOfSections");if(count<1 || count>32)return snapshot;
    NSArray *sections=((id(*)(id,SEL))objc_msgSend)(snapshot,@selector(sectionIdentifiers));
    if(![sections isKindOfClass:NSArray.class] || sections.count!=(NSUInteger)count)return snapshot;
    NSInteger index=((NSInteger(*)(id,SEL,id))objc_msgSend)(NSClassFromString(@"KCMessageBridge"),NSSelectorFromString(@"specialFriendSectionIndex:"),sections);
    if(index<0 || (NSUInteger)index>=sections.count)return snapshot;
    id copy=[snapshot copy];if(copy==snapshot)return snapshot;
    ((void(*)(id,SEL,id))objc_msgSend)(copy,@selector(deleteSectionsWithIdentifiers:),@[sections[index]]);
    NSMutableArray *expected=[sections mutableCopy];[expected removeObjectAtIndex:index];
    NSArray *remaining=((id(*)(id,SEL))objc_msgSend)(copy,@selector(sectionIdentifiers));
    if(![remaining isEqualToArray:expected])return snapshot;
    event(@"ginppai.friendPromotion.filtered");return copy;
}
static id ginppaiFriendSnapshot(id snapshot) {
    if(!option(@"hideFriendFeed") || !ginppaiNativeBuildVerified)return snapshot;
    NSArray *items=ginppaiFriendItems(snapshot);
    if(items.count!=2 || ![snapshot respondsToSelector:@selector(copyWithZone:)] ||
       ![snapshot respondsToSelector:@selector(deleteItemsWithIdentifiers:)])return snapshot;
    NSInteger list=ginppaiFriendListIndex(items);
    id copy=[snapshot copy];
    if(copy==snapshot)return snapshot;
    ((void(*)(id,SEL,id))objc_msgSend)(copy,@selector(deleteItemsWithIdentifiers:),@[items[1-list]]);
    NSArray *remaining=ginppaiFriendItems(copy);
    if(remaining.count!=1)return snapshot;
    event(@"ginppai.friendFeed.filtered");return copy;
}
static UICollectionView *ginppaiFriendCollection(UIView *view,UIView *header,NSUInteger depth) {
    if(depth>5)return nil;
    if([view isKindOfClass:UICollectionView.class] && ((UICollectionView *)view).delegate==(id)header)return (id)view;
    for(UIView *child in view.subviews){UICollectionView *found=ginppaiFriendCollection(child,header,depth+1);if(found)return found;}
    return nil;
}
static id ginppaiFriendSource(UICollectionView *collection) {
    id source=collection.dataSource;
    NSString *name=NSStringFromClass([source class]);
    if(![name hasPrefix:@"_TtGC5UIKit34UICollectionViewDiffableDataSource"] ||
       ![name containsString:@"FriendsFeedPresentation20FriendsTabHeaderView"])return nil;
    Ivar ivar=class_getInstanceVariable([source class],"_impl");
    if(!ivar)return nil;
    id implementation=object_getIvar(source,ivar);
    return [NSStringFromClass([implementation class]) isEqualToString:@"__UIDiffableDataSource"]?implementation:nil;
}
static BOOL ginppaiFriendFeedIsVisible(UIView *header) {
    for(UIResponder *responder=header.nextResponder;responder;responder=responder.nextResponder) {
        if(![NSStringFromClass(responder.class) isEqualToString:@"FriendsFeedPresentation.FriendsTabContainerViewController"])continue;
        UIViewController *host=(id)responder;
        UIViewController *active=host.childViewControllerForStatusBarStyle;
        return [NSStringFromClass(active.class) isEqualToString:@"FriendsFeedPresentation.FriendsFeedViewController"];
    }
    return NO;
}
static void ginppaiRefreshFriendHeader(UIView *header) {
    if(!option(@"hideFriendFeed") || !header.window || objc_getAssociatedObject(header,&ginppaiFriendSelectionPending))return;
    UICollectionView *collection=ginppaiFriendCollection(header,header,0);id source=ginppaiFriendSource(collection);
    if(![source respondsToSelector:@selector(snapshot)])return;
    id snapshot=((id(*)(id,SEL))objc_msgSend)(source,@selector(snapshot));NSArray *items=ginppaiFriendItems(snapshot);
    if(!items)return;
    id filtered=ginppaiFriendSnapshot(snapshot);
    SEL apply=NSSelectorFromString(@"applyDifferencesFromSnapshot:animatingDifferences:completion:");
    if(filtered!=snapshot && [source respondsToSelector:apply]) {
        ((void(*)(id,SEL,id,BOOL,id))objc_msgSend)(source,apply,filtered,NO,nil);
        snapshot=filtered;items=ginppaiFriendItems(snapshot);
    }
    NSInteger list=ginppaiFriendListIndex(items);
    SEL indexSelector=NSSelectorFromString(@"indexPathForItemIdentifier:");
    if(list<0 || ![snapshot respondsToSelector:indexSelector])return;
    NSIndexPath *target=((id(*)(id,SEL,id))objc_msgSend)(snapshot,indexSelector,items[list]);
    if(![target isKindOfClass:NSIndexPath.class])return;
    NSIndexPath *selected=((id(*)(id,SEL,id))objc_msgSend)(NSClassFromString(@"KCMessageBridge"),NSSelectorFromString(@"friendSelectedIndexPath:"),header);
    // A persisted feed can still be the active child even after removing its
    // chip selects the remaining index. Check the content controller as well.
    if([selected isEqual:target] && !ginppaiFriendFeedIsVisible(header))return;
    objc_setAssociatedObject(header,&ginppaiFriendSelectionPending,@YES,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    __weak UIView *weakHeader=header;
    dispatch_async(dispatch_get_main_queue(),^{
        UIView *current=weakHeader;if(!current)return;
        UICollectionView *visible=ginppaiFriendCollection(current,current,0);
        if(current.window && target.section<visible.numberOfSections && target.item<[visible numberOfItemsInSection:target.section]) {
            // Use KakaoTalk's own selection handler, which updates the content
            // controller and accessibility state together.
            [visible.delegate collectionView:visible didSelectItemAtIndexPath:target];
            event(@"ginppai.friendFeed.listSelected");
        }
        objc_setAssociatedObject(current,&ginppaiFriendSelectionPending,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    });
}
static void ginppaiRefreshFriendTree(UIView *view,NSUInteger depth) {
    if(depth>14)return;
    if([NSStringFromClass(view.class) isEqualToString:@"FriendsFeedPresentation.FriendsTabHeaderView"]){ginppaiRefreshFriendHeader(view);return;}
    for(UIView *child in view.subviews)ginppaiRefreshFriendTree(child,depth+1);
}
static BOOL ginppaiFriendApplySignature(Method method) {
    if(!method || method_getNumberOfArguments(method)!=6)return NO;
    char type[64];method_getReturnType(method,type,sizeof(type));if(type[0]!='v')return NO;
    for(unsigned int i=2;i<6;i++){method_getArgumentType(method,i,type,sizeof(type));if(i==2 || i==4){if(type[0]!='@')return NO;}else if(type[0]!='B' && type[0]!='c')return NO;}
    return YES;
}
static void ginppaiInstallFriendList(void) {
    ginppaiCapabilities[@"hideFriendFeed"]=@NO;
    ginppaiCapabilities[@"hideSpecialFriendPromo"]=@NO;
    Class header=NSClassFromString(@"FriendsFeedPresentation.FriendsTabHeaderView");
    Class source=NSClassFromString(@"__UIDiffableDataSource");
    SEL apply=NSSelectorFromString(@"_applyDifferencesFromSnapshot:animatingDifferences:completion:isOnApplyQueue:");
    Method method=class_getInstanceMethod(source,apply);
    if(!ginppaiNativeBuildVerified || !header || !ginppaiFriendApplySignature(method))return;
    ginppaiCapabilities[@"hideFriendFeed"]=@YES;
    ginppaiCapabilities[@"hideSpecialFriendPromo"]=@YES;
    if(!option(@"hideFriendFeed") && !option(@"hideAds"))return;
    IMP original=method_getImplementation(method);
    replace(source,apply,imp_implementationWithBlock(^(id self,id snapshot,BOOL animated,id completion,BOOL onQueue){
        ((void(*)(id,SEL,id,BOOL,id,BOOL))original)(self,apply,ginppaiFriendSnapshot(ginppaiFriendPromotionSnapshot(snapshot)),animated,completion,onQueue);
    }),method_getTypeEncoding(method));
    if(!option(@"hideFriendFeed"))return;
    for(NSString *name in @[@"didMoveToWindow",@"layoutSubviews"]) {
        SEL sel=NSSelectorFromString(name);Method m=class_getInstanceMethod(header,sel);if(!m)continue;IMP original=method_getImplementation(m);
        replace(header,sel,imp_implementationWithBlock(^(UIView *self){((void(*)(id,SEL))original)(self,sel);ginppaiRefreshFriendHeader(self);}),method_getTypeEncoding(m));
    }
    SEL selection=@selector(collectionView:didSelectItemAtIndexPath:);Method selected=class_getInstanceMethod(header,selection);
    if(selected){IMP original=method_getImplementation(selected);replace(header,selection,imp_implementationWithBlock(^(UIView *self,UICollectionView *collection,NSIndexPath *index){
        id source=ginppaiFriendSource(collection);id snapshot=ginppaiCall(source,@"snapshot");NSArray *items=ginppaiFriendItems(snapshot);
        NSInteger list=items?ginppaiFriendListIndex(items):-1;
        if(list>=0 && [snapshot respondsToSelector:NSSelectorFromString(@"indexPathForItemIdentifier:")])index=((id(*)(id,SEL,id))objc_msgSend)(snapshot,NSSelectorFromString(@"indexPathForItemIdentifier:"),items[list]);
        ((void(*)(id,SEL,id,id))original)(self,selection,collection,index);
    }),method_getTypeEncoding(selected));}
    Class container=NSClassFromString(@"FriendsFeedPresentation.FriendsTabContainerViewController");
    for(NSString *name in @[@"viewDidLayoutSubviews",@"viewDidAppear:"]) {
        SEL sel=NSSelectorFromString(name);Method m=class_getInstanceMethod(container,sel);if(!m)continue;IMP original=method_getImplementation(m);
        IMP replacement=[name hasSuffix:@":"]?imp_implementationWithBlock(^(UIViewController *self,BOOL animated){((void(*)(id,SEL,BOOL))original)(self,sel,animated);ginppaiRefreshFriendTree(self.view,0);}):imp_implementationWithBlock(^(UIViewController *self){((void(*)(id,SEL))original)(self,sel);ginppaiRefreshFriendTree(self.view,0);});
        replace(container,sel,replacement,method_getTypeEncoding(m));
    }
}
