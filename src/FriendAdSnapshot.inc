// Filter the complete advertising sections, including compositional-layout
// spacing. Cells alone cannot override an absolute section layout height.
static id ginppaiFriendPromotionSnapshot(id snapshot) {
    if(!option(@"hideAds") || !ginppaiNativeBuildVerified || ![snapshot respondsToSelector:@selector(sectionIdentifiers)] ||
       ![snapshot respondsToSelector:@selector(copyWithZone:)] || ![snapshot respondsToSelector:@selector(deleteSectionsWithIdentifiers:)])return snapshot;
    int64_t count=ginppaiInt(snapshot,@"numberOfSections");if(count<1 || count>32)return snapshot;
    NSArray *sections=((id(*)(id,SEL))objc_msgSend)(snapshot,@selector(sectionIdentifiers));
    if(![sections isKindOfClass:NSArray.class] || sections.count!=(NSUInteger)count)return snapshot;
    NSArray *indices=((id(*)(id,SEL,id))objc_msgSend)(NSClassFromString(@"KCMessageBridge"),NSSelectorFromString(@"friendAdSectionIndexes:"),sections);
    if(![indices isKindOfClass:NSArray.class] || !indices.count)return snapshot;
    NSMutableIndexSet *remove=[NSMutableIndexSet new];
    for(id value in indices) {
        if(![value isKindOfClass:NSNumber.class])return snapshot;
        NSInteger index=[value integerValue];
        if(index<0 || (NSUInteger)index>=sections.count || [remove containsIndex:(NSUInteger)index])return snapshot;
        [remove addIndex:(NSUInteger)index];
    }
    NSMutableArray *identifiers=[NSMutableArray new];
    [remove enumerateIndexesUsingBlock:^(NSUInteger index,BOOL *stop){[identifiers addObject:sections[index]];}];
    id copy=[snapshot copy];if(!copy || copy==snapshot)return snapshot;
    ((void(*)(id,SEL,id))objc_msgSend)(copy,@selector(deleteSectionsWithIdentifiers:),identifiers);
    NSMutableArray *expected=[sections mutableCopy];[expected removeObjectsAtIndexes:remove];
    NSArray *remaining=((id(*)(id,SEL))objc_msgSend)(copy,@selector(sectionIdentifiers));
    if(![remaining isEqualToArray:expected])return snapshot;
    event(@"ginppai.friendPromotion.filtered");
    event(@"ads.spacing.friendSectionsFiltered");return copy;
}
