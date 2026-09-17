// The record and model stay deleted. A separate native TextMessage is used
// only by the chat cell provider. Its associated text changes display getters;
// its shared record is never edited, persisted, or passed to a send operation.
typedef struct { uint64_t first, second; } KCSwiftString;
extern KCSwiftString __attribute__((swiftcall)) KCGinppaiNativeString(NSString *text);
static int32_t __attribute__((swiftcall)) (*ginppaiOriginalMessageType)(__attribute__((swift_context)) void *);
static KCSwiftString __attribute__((swiftcall)) (*ginppaiOriginalMessageText)(__attribute__((swift_context)) void *);
static BOOL __attribute__((swiftcall)) (*ginppaiOriginalMessageDeleted)(__attribute__((swift_context)) void *);
typedef struct { void *object, *witness; } KCNativeMessageRecord;
static KCNativeMessageRecord __attribute__((swiftcall)) (*ginppaiMessageRecord)(__attribute__((swift_context)) void *);
static void __attribute__((swiftcall)) (*ginppaiSetMessageRecord)(KCNativeMessageRecord,__attribute__((swift_context)) void *);
static const char ginppaiDeletedDisplayTextKey;
static const char ginppaiDeletedLabelKey;
static const char ginppaiDeletedUnreadKey;
static const char ginppaiDeletedGesturesKey;

static NSString *ginppaiPreservedMessage(id message) {
    if(!message || !option(@"showDeletedMessages") || !ginppaiNativeBuildVerified)return nil;
    NSString *cached=objc_getAssociatedObject(message,&ginppaiDeletedDisplayTextKey);if(cached)return cached;
    if(ginppaiOriginalMessageType && [message isKindOfClass:NSClassFromString(@"TalkAppBase.ChatMessage")] && ginppaiOriginalMessageType((__bridge void *)message)!=0x4001)return nil;
    NSDictionary *snapshot=((id(*)(id,SEL,id))objc_msgSend)(NSClassFromString(@"KCMessageBridge"),NSSelectorFromString(@"snapshot:"),message);
    if([snapshot[@"record"][@"type"] integerValue]!=0x4001)return nil;
    return ((id(*)(id,SEL,id))objc_msgSend)(NSClassFromString(@"KCMessageHistory"),NSSelectorFromString(@"preservedText:"),snapshot);
}
static NSString *ginppaiRenderText(void *object) {
    return object?objc_getAssociatedObject((__bridge id)object,&ginppaiDeletedDisplayTextKey):nil;
}
static id ginppaiDeletedDisplayMessage(id message) {
    if(ginppaiRenderText((__bridge void *)message))return message;
    NSString *text=ginppaiPreservedMessage(message);if(!text)return message;
    event(@"ginppai.deleted.cloneStart");
    // Native init initializes TextMessage-specific fields. Changing an existing
    // DeletedMessage's class would leave those fields absent and is unsafe.
    id display=[NSClassFromString(@"KakaoTalk.TextMessage") new];if(!display)return message;
    KCNativeMessageRecord record=ginppaiMessageRecord((__bridge void *)message);
    // The getter returns an owned class-bound existential. The setter consumes
    // that ownership. Both objects now reference the same unmodified record.
    ginppaiSetMessageRecord(record,(__bridge void *)display);
    if(ginppaiOriginalMessageType((__bridge void *)display)!=0x4001 || ginppaiOriginalMessageType((__bridge void *)message)!=0x4001){event(@"ginppai.deleted.cloneWrongType");return message;}
    objc_setAssociatedObject(display,&ginppaiDeletedDisplayTextKey,text,OBJC_ASSOCIATION_COPY_NONATOMIC);
    event(@"ginppai.deleted.displayClone");return display;
}
static int32_t __attribute__((swiftcall)) ginppaiDisplayedMessageType(__attribute__((swift_context)) void *object) {
    if(ginppaiRenderText(object)){event(@"ginppai.deleted.renderType");return 1;}
    return ginppaiOriginalMessageType(object);
}
static KCSwiftString __attribute__((swiftcall)) ginppaiDisplayedMessageText(__attribute__((swift_context)) void *object) {
    NSString *text=ginppaiRenderText(object);
    if(text){event(@"ginppai.deleted.renderText");return KCGinppaiNativeString(text);}
    return ginppaiOriginalMessageText(object);
}
static BOOL __attribute__((swiftcall)) ginppaiDisplayedMessageDeleted(__attribute__((swift_context)) void *object) {
    return ginppaiRenderText(object)?NO:ginppaiOriginalMessageDeleted(object);
}

static NSUInteger ginppaiDeletedRenderDepth;
static void * __attribute__((swiftcall)) (*ginppaiOriginalMessageAtIndex)(const void *,__attribute__((swift_context)) void *);
static void * __attribute__((swiftcall)) ginppaiDisplayedMessageAtIndex(const void *index,__attribute__((swift_context)) void *provider) {
    void *native=ginppaiOriginalMessageAtIndex(index,provider);
    if(!native || !NSThread.isMainThread || !ginppaiDeletedRenderDepth)return native;
    // Native Swift returns an owned optional object. Transfer it into ARC and
    // return one owned reference whether it is unchanged or a display clone.
    id message=CFBridgingRelease(native);
    return (__bridge_retained void *)ginppaiDeletedDisplayMessage(message);
}
static void ginppaiBeginDeletedRender(void) { if(NSThread.isMainThread)ginppaiDeletedRenderDepth++; }
static void ginppaiEndDeletedRender(void) { if(NSThread.isMainThread && ginppaiDeletedRenderDepth)ginppaiDeletedRenderDepth--; }

static UILabel *ginppaiTimestampInStatus(UIView *view) {
    if(view.hidden || view.alpha<0.01)return nil;
    if([view isKindOfClass:UILabel.class]) {
        UILabel *label=(UILabel *)view;NSString *text=label.text;
        if(text.length && [text rangeOfString:@"^[0-9]{1,2}:[0-9]{2}$" options:NSRegularExpressionSearch].location!=NSNotFound)return label;
    }
    for(UIView *child in view.subviews){UILabel *label=ginppaiTimestampInStatus(child);if(label)return label;}
    return nil;
}
static UIView *ginppaiStatusInBubble(UIView *view) {
    if(view.hidden || view.alpha<0.01)return nil;
    if([NSStringFromClass(view.class) isEqualToString:@"ChatBubble.BubbleStatusStackView"])return view;
    for(UIView *child in view.subviews){UIView *status=ginppaiStatusInBubble(child);if(status)return status;}
    return nil;
}
static void ginppaiHideDeletedUnread(UIView *view,UILabel *time,NSMutableArray *hidden) {
    if([view isKindOfClass:UILabel.class] && view!=time && !view.hidden) {
        NSString *text=((UILabel *)view).text;
        if(text.length && [text rangeOfString:@"^[0-9]+[+]?$" options:NSRegularExpressionSearch].location!=NSNotFound) {
            view.hidden=YES;[hidden addObject:view];
        }
    }
    for(UIView *child in view.subviews)ginppaiHideDeletedUnread(child,time,hidden);
}
static void ginppaiClearDeletedLabel(UITableViewCell *cell) {
    UILabel *badge=objc_getAssociatedObject(cell,&ginppaiDeletedLabelKey);badge.hidden=YES;
    for(UIView *label in objc_getAssociatedObject(cell,&ginppaiDeletedUnreadKey))label.hidden=NO;
    objc_setAssociatedObject(cell,&ginppaiDeletedUnreadKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    for(UIGestureRecognizer *gesture in objc_getAssociatedObject(cell,&ginppaiDeletedGesturesKey))gesture.enabled=YES;
    objc_setAssociatedObject(cell,&ginppaiDeletedGesturesKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
}
static void ginppaiDisableDeletedActions(UIView *view,NSMutableArray *disabled) {
    // Restrict only this read-only text cell. Ancestor table scrolling and
    // long-press copy remain intact; reuse restores only gestures we disabled.
    for(UIGestureRecognizer *gesture in view.gestureRecognizers) {
        BOOL doubleTap=[gesture isKindOfClass:UITapGestureRecognizer.class] && ((UITapGestureRecognizer *)gesture).numberOfTapsRequired==2;
        BOOL swipe=[gesture isKindOfClass:UIPanGestureRecognizer.class] || [gesture isKindOfClass:UISwipeGestureRecognizer.class];
        if(gesture.enabled && (doubleTap || swipe)){[disabled addObject:gesture];gesture.enabled=NO;}
    }
    for(UIView *child in view.subviews)ginppaiDisableDeletedActions(child,disabled);
}
static void ginppaiUpdateDeletedLabel(UITableViewCell *cell) {
    UILabel *badge=objc_getAssociatedObject(cell,&ginppaiDeletedLabelKey);
    id message=((id(*)(id,SEL,id))objc_msgSend)(NSClassFromString(@"KCMessageBridge"),NSSelectorFromString(@"bubbleMessage:"),cell);
    NSString *text=ginppaiPreservedMessage(message);
    UIView *status=text?ginppaiStatusInBubble(cell):nil;
    UILabel *time=status?ginppaiTimestampInStatus(status):nil;
    if(!status || !cell.window){ginppaiClearDeletedLabel(cell);return;}
    NSMutableArray *disabled=[objc_getAssociatedObject(cell,&ginppaiDeletedGesturesKey) mutableCopy]?:[NSMutableArray new];
    ginppaiDisableDeletedActions(cell,disabled);
    objc_setAssociatedObject(cell,&ginppaiDeletedGesturesKey,disabled,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    NSMutableArray *unread=[objc_getAssociatedObject(cell,&ginppaiDeletedUnreadKey) mutableCopy]?:[NSMutableArray new];ginppaiHideDeletedUnread(status,time,unread);
    objc_setAssociatedObject(cell,&ginppaiDeletedUnreadKey,unread,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    if(!badge){badge=[UILabel new];badge.text=@"삭제됨";badge.textColor=UIColor.systemRedColor;
        badge.isAccessibilityElement=YES;badge.accessibilityLabel=@"삭제된 메시지, 기기에 보관된 원문";
        badge.userInteractionEnabled=NO;[cell.contentView addSubview:badge];
        objc_setAssociatedObject(cell,&ginppaiDeletedLabelKey,badge,OBJC_ASSOCIATION_RETAIN_NONATOMIC);}
    badge.font=[UIFont systemFontOfSize:MAX(time?time.font.pointSize:10,10) weight:UIFontWeightMedium];
    [badge sizeToFit];
    // Grouped messages have an empty status stack because KakaoTalk omits the
    // repeated time. Its native origin still anchors the bubble's status edge.
    CGRect frame=time?[time convertRect:time.bounds toView:cell.contentView]:[status convertRect:status.bounds toView:cell.contentView];
    BOOL sent=[NSStringFromClass(cell.class) containsString:@"Sent"];
    badge.frame=CGRectMake(sent?CGRectGetMaxX(frame)-badge.bounds.size.width:CGRectGetMinX(frame),
                           CGRectGetMinY(frame)-badge.bounds.size.height-(time?2:0),badge.bounds.size.width,badge.bounds.size.height);
    badge.hidden=NO;[cell.contentView bringSubviewToFront:badge];event(@"ginppai.deleted.badge");
}
static void ginppaiInstallDeletedMessages(void) {
    ginppaiCapabilities[@"showDeletedMessages"]=@NO;
    if(!ginppaiNativeBuildVerified)return;
    Class message=NSClassFromString(@"KakaoTalk.TextMessage");if(!message)return;
    const NSUInteger offsets[]={0x1e0,0x1f8,0x408,0x170,0x178};
    const uintptr_t addresses[]={KCAddress(0x10461fea8),KCAddress(0x10461feec),KCAddress(0x104621174),KCAddress(0x10461fc74),KCAddress(0x10461fca8)};
    void *replacements[]={(void *)&ginppaiDisplayedMessageType,(void *)&ginppaiDisplayedMessageText,(void *)&ginppaiDisplayedMessageDeleted};
    for(NSUInteger i=0;i<5;i++) {
        void **slot=(void **)((uint8_t *)(__bridge void *)message+KCMessageSlot(offsets[i]));
        if(!ginppaiWritableRegion(slot,sizeof(*slot)) || *slot!=(void *)(addresses[i]+ginppaiImageSlide))return;
    }
    Class controller=NSClassFromString(@"KakaoTalk.ChattingTableViewController");
    SEL providerSel=NSSelectorFromString(@"updateCellProviderFor:");Method provider=class_getInstanceMethod(controller,providerSel);
    if(!provider || method_getNumberOfArguments(provider)!=3 || strcmp(method_getTypeEncoding(provider),"v24@0:8@16"))return;
    Class source=NSClassFromString(@"KakaoTalk.ChatMessageProvider");if(!source)return;
    void **fetchSlot=(void **)((uint8_t *)(__bridge void *)source+0x6c8);
    void *fetchOriginal=(void *)(KCAddress(0x100297d64)+ginppaiImageSlide);
    if(!ginppaiWritableRegion(fetchSlot,sizeof(*fetchSlot)) || *fetchSlot!=fetchOriginal)return;
    SEL cellSel=NSSelectorFromString(@"tableView:cellForRowAtIndexPath:");Method cell=class_getInstanceMethod(controller,cellSel);
    SEL heightSel=NSSelectorFromString(@"tableView:heightForRowAtIndexPath:");Method height=class_getInstanceMethod(controller,heightSel);
    if(!cell || !height)return;
    ginppaiCapabilities[@"showDeletedMessages"]=@YES;
    if(!option(@"showDeletedMessages"))return;
    ginppaiOriginalMessageAtIndex=fetchOriginal;
    ginppaiOriginalMessageType=(void *)(addresses[0]+ginppaiImageSlide);
    ginppaiOriginalMessageText=(void *)(addresses[1]+ginppaiImageSlide);
    ginppaiOriginalMessageDeleted=(void *)(addresses[2]+ginppaiImageSlide);
    ginppaiMessageRecord=(void *)(addresses[3]+ginppaiImageSlide);
    ginppaiSetMessageRecord=(void *)(addresses[4]+ginppaiImageSlide);
    // Only display clones have the associated text. Ordinary TextMessage
    // objects retain their original getters in every context.
    for(NSUInteger i=0;i<3;i++)ginppaiWritablePointer((void **)((uint8_t *)(__bridge void *)message+KCMessageSlot(offsets[i])),(void *)(addresses[i]+ginppaiImageSlide),replacements[i]);
    ginppaiWritablePointer(fetchSlot,fetchOriginal,(void *)&ginppaiDisplayedMessageAtIndex);
    // The native height and cell factory both fetch by IndexPath through this
    // vtable slot. Only these synchronous UI calls receive the display clone.
    IMP originalCell=method_getImplementation(cell),originalHeight=method_getImplementation(height);
    replace(controller,cellSel,imp_implementationWithBlock(^id(id self,UITableView *table,NSIndexPath *index){
        ginppaiBeginDeletedRender();
        @try{return ((id(*)(id,SEL,id,id))originalCell)(self,cellSel,table,index);}
        @finally{ginppaiEndDeletedRender();}
    }),method_getTypeEncoding(cell));
    replace(controller,heightSel,imp_implementationWithBlock(^CGFloat(id self,UITableView *table,NSIndexPath *index){
        ginppaiBeginDeletedRender();
        @try{return ((CGFloat(*)(id,SEL,id,id))originalHeight)(self,heightSel,table,index);}
        @finally{ginppaiEndDeletedRender();}
    }),method_getTypeEncoding(height));
    IMP originalProvider=method_getImplementation(provider);
    replace(controller,providerSel,imp_implementationWithBlock(^(id self,id value){
        event(@"ginppai.deleted.provider");
        id display=ginppaiDeletedDisplayMessage(value);
        ((void(*)(id,SEL,id))originalProvider)(self,providerSel,display);
    }),method_getTypeEncoding(provider));
    // A reused cell must lose the marker immediately, including offscreen reuse.
    Class status=NSClassFromString(@"ChatBubble.BubbleStatusStackView");SEL layout=@selector(layoutSubviews);Method lm=class_getInstanceMethod(status,layout);
    if(lm){IMP original=method_getImplementation(lm);replace(status,layout,imp_implementationWithBlock(^(UIView *self){
        ((void(*)(id,SEL))original)(self,layout);UIView *parent=self;
        while(parent && ![parent isKindOfClass:UITableViewCell.class])parent=parent.superview;
        if(parent)ginppaiUpdateDeletedLabel((UITableViewCell *)parent);
    }),method_getTypeEncoding(lm));}
    SEL reuse=@selector(prepareForReuse);Method reuseMethod=class_getInstanceMethod(UITableViewCell.class,reuse);
    if(reuseMethod){IMP original=method_getImplementation(reuseMethod);
        replace(UITableViewCell.class,reuse,imp_implementationWithBlock(^(UITableViewCell *self){
            ginppaiClearDeletedLabel(self);((void(*)(id,SEL))original)(self,reuse);
        }),method_getTypeEncoding(reuseMethod));}
    for(NSString *name in @[@"KakaoTalk.ChatMessageCellViewSubmenuHandler",@"KakaoTalk.ChatMessageCellViewMimicSubmenuHandler"]) {
        Class handler=NSClassFromString(name);
        for(NSString *name in @[@"doubleTapReaction:",@"openReactionMenu:",@"openMiniEmoticonReactionPicker:"]) {
            SEL sel=NSSelectorFromString(name);Method m=class_getInstanceMethod(handler,sel);if(!m || method_getNumberOfArguments(m)!=3)continue;
            char type[8];method_getReturnType(m,type,sizeof(type));if(type[0]!='B')continue;IMP original=method_getImplementation(m);
            replace(handler,sel,imp_implementationWithBlock(^BOOL(id self,id value){
                if(ginppaiPreservedMessage(ginppaiCall(self,@"message"))){event(@"ginppai.deleted.actionBlocked");return YES;}
                return ((BOOL(*)(id,SEL,id))original)(self,sel,value);
            }),method_getTypeEncoding(m));
        }
        for(NSString *name in @[@"replyMenuAction:",@"invokeReactionOnlySubmenuAction:",@"invokeReactionOnlyWithTargetView:"]) {
            SEL sel=NSSelectorFromString(name);Method m=class_getInstanceMethod(handler,sel);if(!m || method_getNumberOfArguments(m)!=3)continue;
            char type[8];method_getReturnType(m,type,sizeof(type));if(type[0]!='v')continue;IMP original=method_getImplementation(m);
            replace(handler,sel,imp_implementationWithBlock(^(id self,id value){
                if(!ginppaiPreservedMessage(ginppaiCall(self,@"message")))((void(*)(id,SEL,id))original)(self,sel,value);
            }),method_getTypeEncoding(m));
        }
        SEL reply=NSSelectorFromString(@"performReplyActionWith:triggerSource:");Method rm=class_getInstanceMethod(handler,reply);
        if(rm && method_getNumberOfArguments(rm)==4){IMP original=method_getImplementation(rm);
            replace(handler,reply,imp_implementationWithBlock(^(id self,id value,id source){
                if(!ginppaiPreservedMessage(ginppaiCall(self,@"message")))((void(*)(id,SEL,id,id))original)(self,reply,value,source);
            }),method_getTypeEncoding(rm));}
    }
    ginppaiCapabilities[@"showDeletedMessages"]=@YES;[installed addObject:@"Native deleted message display clones"];
}
