// Presentation-only navigation cleanup for the exact appearance build. No
// model, database, unread counter or fixed-address patch is used here.
static BOOL KCNALSignature(Method method, const char *result, NSUInteger objectArguments) {
    if(!method || method_getNumberOfArguments(method)!=objectArguments+2)return NO;
    char type[128];method_getReturnType(method,type,sizeof(type));
    if(strcmp(type,result))return NO;
    for(NSUInteger index=2;index<objectArguments+2;index++) {
        method_getArgumentType(method,(unsigned int)index,type,sizeof(type));
        if(type[0]!='@')return NO;
    }
    return YES;
}
static id KCNALObjectGetter(id object, NSString *name) {
    SEL selector=NSSelectorFromString(name);
    if(!object || !KCNALSignature(class_getInstanceMethod([object class],selector),"@",0))return nil;
    return ((id(*)(id,SEL))objc_msgSend)(object,selector);
}
static NSIndexSet *KCNALIndexes(id values, NSUInteger count) {
    if(![values isKindOfClass:NSArray.class] || [values count]>count)return nil;
    NSMutableIndexSet *indexes=[NSMutableIndexSet new];
    for(id value in values) {
        if(![value isKindOfClass:NSNumber.class])return nil;
        NSInteger index=[value integerValue];
        if(index<0 || (NSUInteger)index>=count || ![value isEqualToNumber:@(index)] || [indexes containsIndex:(NSUInteger)index])return nil;
        [indexes addIndex:(NSUInteger)index];
    }
    return indexes;
}
static id KCNavigationFilterAdvertisingSnapshot(id snapshot) {
    if(!kcAppearanceOnlyBuild || !snapshot)return snapshot;
    @try {
        NSArray *sections=KCNALObjectGetter(snapshot,@"sectionIdentifiers");
        NSArray *items=KCNALObjectGetter(snapshot,@"itemIdentifiers");
        if(![sections isKindOfClass:NSArray.class] || ![items isKindOfClass:NSArray.class] || sections.count<1 || sections.count>256 || items.count>50000)return snapshot;
        Class bridge=NSClassFromString(@"KCChannelNavigationBridge");SEL planSelector=NSSelectorFromString(@"advertisingRemovalPlan:items:");
        if(!KCNALSignature(class_getClassMethod(bridge,planSelector),"@",2))return snapshot;
        id plan=((id(*)(id,SEL,id,id))objc_msgSend)(bridge,planSelector,sections,items);
        if(![plan isKindOfClass:NSDictionary.class])return snapshot;
        NSIndexSet *sectionIndexes=KCNALIndexes(plan[@"sectionIndexes"],sections.count);
        NSIndexSet *itemIndexes=KCNALIndexes(plan[@"itemIndexes"],items.count);
        if(!sectionIndexes || !itemIndexes || (!sectionIndexes.count && !itemIndexes.count))return snapshot;
        SEL deleteSections=NSSelectorFromString(@"deleteSectionsWithIdentifiers:");
        SEL deleteItems=NSSelectorFromString(@"deleteItemsWithIdentifiers:");
        SEL sectionItems=NSSelectorFromString(@"itemIdentifiersInSectionWithIdentifier:");
        if(![snapshot respondsToSelector:@selector(copyWithZone:)] ||
           !KCNALSignature(class_getInstanceMethod([snapshot class],deleteSections),"v",1) ||
           !KCNALSignature(class_getInstanceMethod([snapshot class],deleteItems),"v",1) ||
           (sectionIndexes.count && !KCNALSignature(class_getInstanceMethod([snapshot class],sectionItems),"@",1)))return snapshot;
        NSArray *removedSections=[sections objectsAtIndexes:sectionIndexes];
        NSArray *removedItems=[items objectsAtIndexes:itemIndexes];
        NSSet *allItems=[NSSet setWithArray:items];
        NSMutableSet *expectedRemoved=[NSMutableSet setWithArray:removedItems];
        for(id section in removedSections) {
            id members=((id(*)(id,SEL,id))objc_msgSend)(snapshot,sectionItems,section);
            if(![members isKindOfClass:NSArray.class] || [members count]>items.count)return snapshot;
            for(id member in members) {
                if(![allItems containsObject:member])return snapshot;
                [expectedRemoved addObject:member];
            }
        }
        NSMutableArray *expectedSections=[sections mutableCopy];[expectedSections removeObjectsAtIndexes:sectionIndexes];
        NSMutableArray *expectedItems=[NSMutableArray arrayWithCapacity:items.count];
        for(id item in items)if(![expectedRemoved containsObject:item])[expectedItems addObject:item];
        id copy=[snapshot copy];
        if(copy==snapshot || !KCNALSignature(class_getInstanceMethod([copy class],deleteSections),"v",1) ||
           !KCNALSignature(class_getInstanceMethod([copy class],deleteItems),"v",1) ||
           ![KCNALObjectGetter(copy,@"sectionIdentifiers") isEqualToArray:sections] ||
           ![KCNALObjectGetter(copy,@"itemIdentifiers") isEqualToArray:items])return snapshot;
        if(removedSections.count)((void(*)(id,SEL,id))objc_msgSend)(copy,deleteSections,removedSections);
        NSArray *afterSections=KCNALObjectGetter(copy,@"itemIdentifiers");
        if(![afterSections isKindOfClass:NSArray.class])return snapshot;
        NSSet *remainingSet=[NSSet setWithArray:afterSections];NSMutableArray *stillPresent=[NSMutableArray new];
        for(id item in removedItems)if([remainingSet containsObject:item])[stillPresent addObject:item];
        if(stillPresent.count)((void(*)(id,SEL,id))objc_msgSend)(copy,deleteItems,stillPresent);
        if(![KCNALObjectGetter(copy,@"sectionIdentifiers") isEqualToArray:expectedSections] ||
           ![KCNALObjectGetter(copy,@"itemIdentifiers") isEqualToArray:expectedItems])return snapshot;
        event(@"navigation.advertising.snapshot-filtered");
        return copy;
    } @catch(NSException *exception) {
        // Unsupported copies are discarded; the original snapshot was not changed.
        return snapshot;
    }
}

static const char KCNALMoreChromeStateKey, KCNALMorePendingKey, KCNALMoreAttemptKey, KCNALMorePaddingStateKey, KCNALMorePaddingUpdatingKey, KCNALMorePaddingFailuresKey;
static id KCNALField(id object, NSString *field) {
    if(!object || !field)return nil;
    Class bridge=NSClassFromString(@"KCChannelNavigationBridge");SEL selector=NSSelectorFromString(@"field:name:");
    if(!KCNALSignature(class_getClassMethod(bridge,selector),"@",2))return nil;
    return ((id(*)(id,SEL,id,id))objc_msgSend)(bridge,selector,object,field);
}
static NSInteger KCNALMoreRole(UIViewController *controller) {
    Class bridge=NSClassFromString(@"KCChannelNavigationBridge");SEL selector=NSSelectorFromString(@"moreCurrentRole:");
    if(!KCNALSignature(class_getClassMethod(bridge,selector),@encode(NSInteger),1))return -1;
    return ((NSInteger(*)(id,SEL,id))objc_msgSend)(bridge,selector,controller);
}
static NSInteger KCNALMoreHomeIndex(NSArray *identifiers) {
    Class bridge=NSClassFromString(@"KCChannelNavigationBridge");SEL selector=NSSelectorFromString(@"moreHomeItemIndex:");
    if(!KCNALSignature(class_getClassMethod(bridge,selector),@encode(NSInteger),1))return -1;
    return ((NSInteger(*)(id,SEL,id))objc_msgSend)(bridge,selector,identifiers);
}
static UIView *KCNALMoreChip(UIViewController *controller) {
    id chip=KCNALField(controller,@"chipTabView");
    return [chip isKindOfClass:UIView.class] && [NSStringFromClass([chip class]) isEqualToString:@"KakaoTalk.MoreTabChipTabView"]?chip:nil;
}
static BOOL KCNALVisibleMore(UIViewController *controller) {
    if(![NSStringFromClass(controller.class) isEqualToString:@"KakaoTalk.MoreTabHomeViewController"] ||
       !controller.isViewLoaded || !controller.view.window || controller.presentedViewController ||
       controller.navigationController.presentedViewController || controller.tabBarController.presentedViewController)return NO;
    if(controller.navigationController && controller.navigationController.topViewController!=controller)return NO;
    for(UIView *view=controller.view;view;view=view.superview)if(view.hidden || view.alpha<0.01)return NO;
    return YES;
}
static void KCNALFindCollections(UIView *view, NSMutableArray<UICollectionView *> *result, NSUInteger depth) {
    if(depth>8 || result.count>1)return;
    if([view isKindOfClass:UICollectionView.class]){[result addObject:(id)view];return;}
    for(UIView *child in view.subviews)KCNALFindCollections(child,result,depth+1);
}
static NSArray *KCNALMoreIdentifiers(UICollectionView *collection) {
    id source=collection.dataSource;
    Class bridge=NSClassFromString(@"KCChannelNavigationBridge");SEL implementationSelector=NSSelectorFromString(@"diffableImplementation:");
    if(!KCNALSignature(class_getClassMethod(bridge,implementationSelector),"@",1))return nil;
    // UIKit's Swift-stored _impl has an empty Objective-C ivar encoding on the
    // observed runtime. The bridge reflects only whitelisted native sources.
    id implementation=((id(*)(id,SEL,id))objc_msgSend)(bridge,implementationSelector,source);
    if(![NSStringFromClass([implementation class]) isEqualToString:@"__UIDiffableDataSource"])return nil;
    id snapshot=KCNALObjectGetter(implementation,@"snapshot");
    NSArray *sections=KCNALObjectGetter(snapshot,@"sectionIdentifiers");
    NSArray *items=KCNALObjectGetter(snapshot,@"itemIdentifiers");
    if(![sections isKindOfClass:NSArray.class] || sections.count!=1 || ![items isKindOfClass:NSArray.class] ||
       items.count<1 || items.count>4 || collection.numberOfSections!=1 ||
       [collection numberOfItemsInSection:0]!=(NSInteger)items.count)return nil;
    return items;
}
static BOOL KCNALConstantChipHeight(NSLayoutConstraint *constraint, UIView *chip) {
    return constraint.firstItem==chip && constraint.firstAttribute==NSLayoutAttributeHeight &&
        constraint.relation==NSLayoutRelationEqual && constraint.secondItem==nil &&
        constraint.secondAttribute==NSLayoutAttributeNotAnAttribute && constraint.multiplier==1 &&
        constraint.constant>=0 && constraint.constant<=256;
}
static void KCNALRestoreMoreChip(UIView *chip) {
    NSDictionary *state=objc_getAssociatedObject(chip,&KCNALMoreChromeStateKey);if(!state)return;
    NSLayoutConstraint *zero=state[@"zero"];
    zero.active=NO;
    for(NSLayoutConstraint *constraint in state[@"heights"])
        if(KCNALConstantChipHeight(constraint,chip) && !constraint.active)constraint.active=YES;
    for(NSDictionary *entry in state[@"insets"]) {
        NSLayoutConstraint *constraint=entry[@"constraint"];
        if(constraint.secondItem==chip && constraint.firstAttribute==NSLayoutAttributeTop &&
           constraint.secondAttribute==NSLayoutAttributeTop && constraint.constant==0)
            constraint.constant=[entry[@"constant"] doubleValue];
    }
    if(chip.hidden)chip.hidden=[state[@"hidden"] boolValue];
    if(chip.alpha==0)chip.alpha=[state[@"alpha"] doubleValue];
    if(!chip.userInteractionEnabled)chip.userInteractionEnabled=[state[@"interaction"] boolValue];
    if(chip.accessibilityElementsHidden)chip.accessibilityElementsHidden=[state[@"accessibility"] boolValue];
    objc_setAssociatedObject(chip,&KCNALMoreChromeStateKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
}
static void KCNALCollapseMoreChip(UIView *chip) {
    if(!chip || !chip.window || objc_getAssociatedObject(chip,&KCNALMoreChromeStateKey))return;
    NSMutableArray<NSLayoutConstraint *> *heights=[NSMutableArray new];
    NSUInteger depth=0;
    for(UIView *owner=chip;owner && depth<8;owner=owner.superview,depth++)
        for(NSLayoutConstraint *constraint in owner.constraints)
            if(constraint.active && KCNALConstantChipHeight(constraint,chip) && ![heights containsObject:constraint])[heights addObject:constraint];
    BOOL arranged=[chip.superview isKindOfClass:UIStackView.class] && [((UIStackView *)chip.superview).arrangedSubviews containsObject:chip];
    // An unmeasured parent/top/bottom constraint is never rewritten. If the
    // native view owns no constant height and is not arranged, leave it intact.
    if(!heights.count && !arranged)return;
    NSMutableArray<UICollectionView *> *collections=[NSMutableArray new];KCNALFindCollections(chip,collections,0);
    NSMutableArray *insets=[NSMutableArray new];
    if(collections.count==1) {
        UICollectionView *collection=collections.firstObject;
        // Observed 26.9.2 geometry is a 50 pt chip with a 48 pt collection
        // inset 2 pt from its top. Remove only that exact measured inset while
        // collapsed, so a zero-height parent does not imply a negative child.
        for(NSLayoutConstraint *constraint in chip.constraints)
            if(constraint.active && constraint.firstItem==collection && constraint.secondItem==chip &&
               constraint.firstAttribute==NSLayoutAttributeTop && constraint.secondAttribute==NSLayoutAttributeTop &&
               constraint.relation==NSLayoutRelationEqual && constraint.multiplier==1 && constraint.constant==2)
                [insets addObject:@{@"constraint":constraint,@"constant":@(constraint.constant)}];
    }
    NSLayoutConstraint *zero=[chip.heightAnchor constraintEqualToConstant:0];
    NSDictionary *state=@{@"heights":[heights copy],@"insets":[insets copy],@"zero":zero,@"hidden":@(chip.hidden),@"alpha":@(chip.alpha),
        @"interaction":@(chip.userInteractionEnabled),@"accessibility":@(chip.accessibilityElementsHidden)};
    objc_setAssociatedObject(chip,&KCNALMoreChromeStateKey,state,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    for(NSDictionary *entry in insets)((NSLayoutConstraint *)entry[@"constraint"]).constant=0;
    [NSLayoutConstraint deactivateConstraints:heights];zero.active=YES;
    chip.hidden=YES;chip.alpha=0;chip.userInteractionEnabled=NO;chip.accessibilityElementsHidden=YES;
    event(@"navigation.more.home-selector-collapsed");
}
static BOOL KCNALNativeTitleHeight(NSLayoutConstraint *constraint,UIView *view) {
    return [constraint isKindOfClass:NSLayoutConstraint.class] && [view isKindOfClass:UIView.class] &&
        [NSStringFromClass(view.class) isEqualToString:@"TalkAppBase.ScrollHidableView"] &&
        constraint.active && constraint.firstItem==view && constraint.firstAttribute==NSLayoutAttributeHeight &&
        constraint.relation==NSLayoutRelationEqual && constraint.secondItem==nil &&
        constraint.secondAttribute==NSLayoutAttributeNotAnAttribute && constraint.multiplier==1 &&
        isfinite(constraint.constant) && constraint.constant>=56 && constraint.constant<=256;
}
static void KCNALRestoreMorePadding(UIViewController *controller) {
    if(!controller || objc_getAssociatedObject(controller,&KCNALMorePaddingUpdatingKey))return;
    NSDictionary *state=objc_getAssociatedObject(controller,&KCNALMorePaddingStateKey);if(!state)return;
    objc_setAssociatedObject(controller,&KCNALMorePaddingUpdatingKey,@YES,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    id context=state[@"context"];NSDictionary *current=KCMHPState(controller);NSNumber *value=current[@"padding"];
    CGFloat target=[state[@"target"] doubleValue];
    // Restore only the same context and only the value owned by this adapter.
    if(current[@"context"]==context && value && fabs(value.doubleValue-target)<0.5)
        KCMoreHomeSetNativePadding(controller,context,target,[state[@"native"] doubleValue]);
    NSLayoutConstraint *constraint=state[@"constraint"];UIView *hiding=state[@"hiding"];
    if(KCNALField(controller,@"hidingTitleViewHeightConstraint")==constraint && KCNALField(controller,@"hidingTitleView")==hiding &&
       KCNALNativeTitleHeight(constraint,hiding) && fabs(constraint.constant-target)<0.5)
        constraint.constant=[state[@"nativeHeight"] doubleValue];
    objc_setAssociatedObject(controller,&KCNALMorePaddingStateKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    objc_setAssociatedObject(controller,&KCNALMorePaddingUpdatingKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
}
static void KCNALUpdateMorePadding(UIViewController *controller) {
    if(objc_getAssociatedObject(controller,&KCNALMorePaddingUpdatingKey) || !KCNALVisibleMore(controller) || KCNALMoreRole(controller)!=1)return;
    CGFloat expected=controller.view.safeAreaInsets.top+106,target=controller.view.safeAreaInsets.top+56;
    if(!isfinite(expected) || !isfinite(target) || expected>256 || target<56)return;
    NSDictionary *current=KCMHPState(controller);id context=current[@"context"];NSNumber *value=current[@"padding"];
    NSLayoutConstraint *constraint=KCNALField(controller,@"hidingTitleViewHeightConstraint");UIView *hiding=KCNALField(controller,@"hidingTitleView");
    if(!context || !value || !isfinite(value.doubleValue) || !KCNALNativeTitleHeight(constraint,hiding) || ![hiding isDescendantOfView:controller.view])return;
    NSDictionary *prior=objc_getAssociatedObject(controller,&KCNALMorePaddingStateKey);
    if(prior && (prior[@"context"]!=context || prior[@"constraint"]!=constraint || prior[@"hiding"]!=hiding || fabs([prior[@"target"] doubleValue]-target)>=0.5)) {
        KCNALRestoreMorePadding(controller);return;
    }
    // A completed change is never subtracted again. Native changes with another
    // formula are left intact; only the verified 106 -> 56 reservation is used.
    BOOL paddingAtTarget=fabs(value.doubleValue-target)<0.5,heightAtTarget=fabs(constraint.constant-target)<0.5;
    if(paddingAtTarget && heightAtTarget)return;
    BOOL paddingAtNative=fabs(value.doubleValue-expected)<0.5,heightAtNative=fabs(constraint.constant-expected)<0.5;
    if(!prior && (!paddingAtNative || !heightAtNative))return;
    if(prior && ((!paddingAtNative && !paddingAtTarget) || (!heightAtNative && !heightAtTarget)))return;
    if(prior && paddingAtTarget && heightAtNative){constraint.constant=target;return;}
    NSUInteger failures=[objc_getAssociatedObject(controller,&KCNALMorePaddingFailuresKey) unsignedIntegerValue];
    if(failures>=2)return;
    NSDictionary *state=@{@"context":context,@"native":prior[@"native"]?:value,@"target":@(target),
        @"constraint":constraint,@"hiding":hiding,@"nativeHeight":prior[@"nativeHeight"]?:@(constraint.constant)};
    objc_setAssociatedObject(controller,&KCNALMorePaddingStateKey,state,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    objc_setAssociatedObject(controller,&KCNALMorePaddingUpdatingKey,@YES,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    BOOL applied=KCMoreHomeSetNativePadding(controller,context,expected,target);
    // Observation can synchronously rebuild UI. Recheck identities and the
    // native constraint after returning from the setter, before adjusting it.
    if(applied && KCNALVisibleMore(controller) && KCNALMoreRole(controller)==1 &&
       KCNALField(controller,@"hidingTitleViewHeightConstraint")==constraint && KCNALField(controller,@"hidingTitleView")==hiding &&
       KCNALNativeTitleHeight(constraint,hiding) && (fabs(constraint.constant-expected)<0.5 || fabs(constraint.constant-target)<0.5))
        constraint.constant=target;
    else applied=NO;
    if(applied) {
        NSNumber *readback=KCMHPPadding(context);
        applied=readback && isfinite(readback.doubleValue) && fabs(readback.doubleValue-target)<0.5 &&
            KCNALField(controller,@"hidingTitleViewHeightConstraint")==constraint && KCNALField(controller,@"hidingTitleView")==hiding &&
            KCNALNativeTitleHeight(constraint,hiding) && fabs(constraint.constant-target)<0.5;
    }
    objc_setAssociatedObject(controller,&KCNALMorePaddingUpdatingKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    // Legitimate native safe-area/configuration resets may be normalized each
    // time. Only failed commits consume the per-appearance retry budget.
    objc_setAssociatedObject(controller,&KCNALMorePaddingFailuresKey,applied?nil:@(failures+1),OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    if(applied)event(@"navigation.more.home-padding-collapsed");
    else KCNALRestoreMorePadding(controller);
}
static void KCNALRestoreMoreHome(UIViewController *controller) {
    KCNALRestoreMorePadding(controller);KCNALRestoreMoreChip(KCNALMoreChip(controller));
}
static void KCNALUpdateMoreHome(UIViewController *controller) {
    if(!kcAppearanceOnlyBuild || !KCNALVisibleMore(controller))return;
    UIView *chip=KCNALMoreChip(controller);if(!chip || !chip.window)return;
    NSInteger role=KCNALMoreRole(controller);
    if(role==1){KCNALCollapseMoreChip(chip);KCNALUpdateMorePadding(controller);return;}
    KCNALRestoreMoreHome(controller);
    if(role!=0 || objc_getAssociatedObject(controller,&KCNALMorePendingKey) || objc_getAssociatedObject(controller,&KCNALMoreAttemptKey))return;
    NSMutableArray<UICollectionView *> *collections=[NSMutableArray new];KCNALFindCollections(chip,collections,0);
    if(collections.count!=1)return;
    UICollectionView *collection=collections.firstObject;id delegate=collection.delegate;
    SEL select=NSSelectorFromString(@"collectionView:didSelectItemAtIndexPath:");
    if(delegate!=chip || !KCNALSignature(class_getInstanceMethod([delegate class],select),"v",2))return;
    NSArray *identifiers=KCNALMoreIdentifiers(collection);NSInteger home=KCNALMoreHomeIndex(identifiers);
    if(home<0 || (NSUInteger)home>=identifiers.count)return;
    id source=collection.dataSource;NSArray *expected=[identifiers copy];NSObject *token=[NSObject new];
    objc_setAssociatedObject(controller,&KCNALMorePendingKey,token,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    __weak UIViewController *weakController=controller;__weak UIView *weakChip=chip;__weak UICollectionView *weakCollection=collection;
    dispatch_async(dispatch_get_main_queue(),^{
        UIViewController *current=weakController;UIView *currentChip=weakChip;UICollectionView *currentCollection=weakCollection;
        if(!current || objc_getAssociatedObject(current,&KCNALMorePendingKey)!=token)return;
        objc_setAssociatedObject(current,&KCNALMorePendingKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        if(!currentChip || !currentCollection || !KCNALVisibleMore(current) || KCNALMoreChip(current)!=currentChip ||
           currentCollection.window!=currentChip.window || !currentChip.window || ![currentCollection isDescendantOfView:currentChip] ||
           currentCollection.dataSource!=source || currentCollection.delegate!=(id)currentChip || KCNALMoreRole(current)!=0 ||
           objc_getAssociatedObject(current,&KCNALMoreAttemptKey))return;
        NSArray *fresh=KCNALMoreIdentifiers(currentCollection);NSInteger index=KCNALMoreHomeIndex(fresh);
        if(![fresh isEqualToArray:expected] || index!=home || index<0 || (NSUInteger)index>=fresh.count ||
           !KCNALSignature(class_getInstanceMethod([currentChip class],select),"v",2))return;
        // One attempt per appearance prevents a failed/unsupported native route
        // from creating a selection/layout retry loop.
        objc_setAssociatedObject(current,&KCNALMoreAttemptKey,@YES,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        NSIndexPath *path=[NSIndexPath indexPathForItem:index inSection:0];
        [currentCollection selectItemAtIndexPath:path animated:NO scrollPosition:UICollectionViewScrollPositionNone];
        ((void(*)(id,SEL,id,id))objc_msgSend)(currentChip,select,currentCollection,path);
        event(@"navigation.more.home-selected");
        if(KCNALMoreRole(current)==1){KCNALCollapseMoreChip(currentChip);KCNALUpdateMorePadding(current);}
    });
}
static BOOL KCNALBoolLifecycle(Method method) {
    if(!method || method_getNumberOfArguments(method)!=3)return NO;
    char type[64];method_getReturnType(method,type,sizeof(type));if(strcmp(type,"v"))return NO;
    method_getArgumentType(method,2,type,sizeof(type));return !strcmp(type,"B") || !strcmp(type,"c");
}
static void KCInstallMoreHomeOnly(void) {
    if(!kcAppearanceOnlyBuild)return;
    Class cls=NSClassFromString(@"KakaoTalk.MoreTabHomeViewController");
    if(!cls || ![cls isSubclassOfClass:UIViewController.class])return;
    SEL layout=@selector(viewDidLayoutSubviews),appear=@selector(viewWillAppear:),didAppear=@selector(viewDidAppear:),disappear=@selector(viewDidDisappear:);
    Method layoutMethod=class_getInstanceMethod(cls,layout),appearMethod=class_getInstanceMethod(cls,appear),didAppearMethod=class_getInstanceMethod(cls,didAppear),disappearMethod=class_getInstanceMethod(cls,disappear);
    if(!KCNALSignature(layoutMethod,"v",0) || !KCNALBoolLifecycle(appearMethod) || !KCNALBoolLifecycle(didAppearMethod) || !KCNALBoolLifecycle(disappearMethod))return;
    IMP oldLayout=method_getImplementation(layoutMethod),oldAppear=method_getImplementation(appearMethod),oldDidAppear=method_getImplementation(didAppearMethod),oldDisappear=method_getImplementation(disappearMethod);
    replace(cls,layout,imp_implementationWithBlock(^(UIViewController *controller){
        ((void(*)(id,SEL))oldLayout)(controller,layout);KCNALUpdateMoreHome(controller);
    }),method_getTypeEncoding(layoutMethod));
    replace(cls,appear,imp_implementationWithBlock(^(UIViewController *controller,BOOL animated){
        KCNALRestoreMoreHome(controller);
        objc_setAssociatedObject(controller,&KCNALMorePaddingFailuresKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        objc_setAssociatedObject(controller,&KCNALMorePendingKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        objc_setAssociatedObject(controller,&KCNALMoreAttemptKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        ((void(*)(id,SEL,BOOL))oldAppear)(controller,appear,animated);KCNALUpdateMoreHome(controller);
    }),method_getTypeEncoding(appearMethod));
    replace(cls,didAppear,imp_implementationWithBlock(^(UIViewController *controller,BOOL animated){
        ((void(*)(id,SEL,BOOL))oldDidAppear)(controller,didAppear,animated);KCNALUpdateMoreHome(controller);
    }),method_getTypeEncoding(didAppearMethod));
    replace(cls,disappear,imp_implementationWithBlock(^(UIViewController *controller,BOOL animated){
        ((void(*)(id,SEL,BOOL))oldDisappear)(controller,disappear,animated);
        objc_setAssociatedObject(controller,&KCNALMorePendingKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        KCNALRestoreMoreHome(controller);
    }),method_getTypeEncoding(disappearMethod));
}
