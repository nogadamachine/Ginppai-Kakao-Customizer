// The native long-press menu owns this context, so cell reuse cannot change
// which message an already-open menu inspects. No controls are added to bubbles.
@interface KCMessageMenuContext : NSObject
@property(nonatomic,copy) NSDictionary *snapshot;
@property(nonatomic,weak) UIView *anchor;
@property(nonatomic,weak) UIView *container;
@end
@implementation KCMessageMenuContext
@end
static KCMessageMenuContext *ginppaiPendingMenu;
static const char ginppaiMenuContextKey;
static const char ginppaiMenuAccessoryKey;

static BOOL ginppaiHasMessageTools(void) {
    return option(@"messageDetails") || option(@"readReceipts") || option(@"messageHistory") || option(@"showDeletedMessages");
}

@interface KCMessageInfoController : UITableViewController
@property(nonatomic,copy) NSDictionary *snapshot;
@property(nonatomic,copy) NSArray<NSArray<NSString *> *> *rows;
@end
@implementation KCMessageInfoController
- (void)viewDidLoad {
    [super viewDidLoad];self.title=@"메시지 정보";
    self.tableView.rowHeight=UITableViewAutomaticDimension;self.tableView.estimatedRowHeight=70;
    self.rows=((id(*)(id,SEL,id))objc_msgSend)(NSClassFromString(@"KCMessageBridge"),NSSelectorFromString(@"messageInfoRows:"),self.snapshot)?:@[];
    self.navigationItem.leftBarButtonItem=[[UIBarButtonItem alloc] initWithTitle:@"닫기" style:UIBarButtonItemStylePlain target:self action:@selector(close)];
}
- (void)close {[self dismissViewControllerAnimated:YES completion:nil];}
- (NSInteger)tableView:(UITableView *)table numberOfRowsInSection:(NSInteger)section {return self.rows.count;}
- (NSString *)tableView:(UITableView *)table titleForFooterInSection:(NSInteger)section {return @"내용을 누르면 복사합니다. 기기에 없는 정보는 표시하지 않습니다.";}
- (UITableViewCell *)tableView:(UITableView *)table cellForRowAtIndexPath:(NSIndexPath *)index {
    UITableViewCell *cell=customizationCell();NSArray *row=self.rows[index.row];
    cell.textLabel.text=row[0];cell.detailTextLabel.text=row[1];
    cell.detailTextLabel.font=[UIFont preferredFontForTextStyle:UIFontTextStyleBody];
    cell.detailTextLabel.textColor=UIColor.labelColor;cell.textLabel.textColor=UIColor.secondaryLabelColor;
    cell.textLabel.font=[UIFont preferredFontForTextStyle:UIFontTextStyleFootnote];return cell;
}
- (void)tableView:(UITableView *)table didSelectRowAtIndexPath:(NSIndexPath *)index {
    [table deselectRowAtIndexPath:index animated:YES];UIPasteboard.generalPasteboard.string=self.rows[index.row][1];
    self.navigationItem.rightBarButtonItem=[[UIBarButtonItem alloc] initWithTitle:@"복사됨" style:UIBarButtonItemStylePlain target:nil action:nil];
    self.navigationItem.rightBarButtonItem.enabled=NO;
}
@end

static void ginppaiPresentMessageInfo(NSDictionary *snapshot,UIView *anchor) {
    KCMessageInfoController *vc=[[KCMessageInfoController alloc] initWithStyle:UITableViewStyleInsetGrouped];vc.snapshot=snapshot;
    UINavigationController *nav=[[UINavigationController alloc] initWithRootViewController:vc];
    UIViewController *host=ginppaiHost(anchor);
    if([host isKindOfClass:UIAlertController.class]) {
        UIViewController *presenter=host.presentingViewController;
        [host dismissViewControllerAnimated:YES completion:^{[presenter presentViewController:nav animated:YES completion:nil];}];
    } else [host presentViewController:nav animated:YES completion:nil];
    event(@"ginppai.details.open");
}
static void ginppaiMessageMenu(NSDictionary *snapshot,UIView *anchor) {
    UIViewController *host=ginppaiHost(anchor);if(!host || !snapshot || [host isKindOfClass:UIAlertController.class])return;
    UIAlertController *menu=[UIAlertController alertControllerWithTitle:@"메시지 도구" message:nil preferredStyle:UIAlertControllerStyleActionSheet];
    if([snapshot[@"preservedDeleted"] boolValue]) {
        menu.message=@"이 기기에 보관된 원문입니다.";
        [menu addAction:[UIAlertAction actionWithTitle:@"원문 복사" style:UIAlertActionStyleDefault handler:^(UIAlertAction *a){UIPasteboard.generalPasteboard.string=snapshot[@"text"];}]];
    }
    if(option(@"messageDetails"))[menu addAction:[UIAlertAction actionWithTitle:@"메시지 정보" style:UIAlertActionStyleDefault handler:^(UIAlertAction *a){
        dispatch_async(dispatch_get_main_queue(),^{ginppaiPresentMessageInfo(snapshot,anchor);});
    }]];
    if(option(@"readReceipts"))[menu addAction:[UIAlertAction actionWithTitle:@"읽은 사람" style:UIAlertActionStyleDefault handler:^(UIAlertAction *a){
        ginppaiShowText(anchor,@"읽은 사람",ginppaiReceiptText(ginppaiReceipts(snapshot)));event(@"ginppai.receipts.open");
    }]];
    if(option(@"messageHistory"))[menu addAction:[UIAlertAction actionWithTitle:@"변경 기록" style:UIAlertActionStyleDefault handler:^(UIAlertAction *a){ginppaiShowHistory(snapshot,anchor);}]];
    if(!menu.actions.count)return;
    [menu addAction:[UIAlertAction actionWithTitle:@"취소" style:UIAlertActionStyleCancel handler:nil]];
    menu.popoverPresentationController.sourceView=anchor?:host.view;
    menu.popoverPresentationController.sourceRect=anchor?anchor.bounds:CGRectMake(1,1,1,1);
    [host presentViewController:menu animated:YES completion:nil];event(@"ginppai.menu.open");
}

static KCMessageMenuContext *ginppaiNativeMenuContext(id container) {
    KCMessageMenuContext *context=objc_getAssociatedObject(container,&ginppaiMenuContextKey);
    if(ginppaiPendingMenu && context!=ginppaiPendingMenu) {
        context=ginppaiPendingMenu;context.container=container;
        objc_setAssociatedObject(container,&ginppaiMenuContextKey,context,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    return context;
}
static void ginppaiClearMenuAccessory(UITableViewCell *cell) {
    if(cell.accessoryView==objc_getAssociatedObject(cell,&ginppaiMenuAccessoryKey))cell.accessoryView=nil;
    objc_setAssociatedObject(cell,&ginppaiMenuAccessoryKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
}
static void ginppaiAttachMessageMenu(id container,UITableView *table,UITableViewCell *cell,NSIndexPath *index) {
    ginppaiClearMenuAccessory(cell);
    if(!option(@"messageDetails") && !option(@"readReceipts") && !option(@"messageHistory"))return;
    KCMessageMenuContext *context=ginppaiNativeMenuContext(container);
    if(!context || index.section!=0 || index.row!=0 || table.dataSource!=container || ![container respondsToSelector:@selector(accessibilityPerformEscape)])return;
    // Use an accessory inside the existing first row. An added header changes
    // content height without changing the native popup's fixed height.
    UIButton *button=[UIButton buttonWithType:UIButtonTypeSystem];button.frame=CGRectMake(0,0,120,30);
    [button setTitle:@"메시지 도구" forState:UIControlStateNormal];
    button.titleLabel.font=[UIFont systemFontOfSize:13 weight:UIFontWeightMedium];
    [button setTitleColor:UIColor.labelColor forState:UIControlStateNormal];
    button.backgroundColor=UIColor.tertiarySystemFillColor;button.layer.cornerRadius=8;
    button.accessibilityLabel=@"메시지 도구";button.accessibilityHint=@"선택한 메시지의 정보와 읽음 목록, 변경 기록을 엽니다.";
    __weak KCMessageMenuContext *weakContext=context;
    [button addAction:[UIAction actionWithHandler:^(UIAction *action){
        KCMessageMenuContext *selected=weakContext;UIView *menu=selected.container;UIView *anchor=selected.anchor;
        if(!selected || !menu.window || !anchor.window)return;
        if(!((BOOL(*)(id,SEL))objc_msgSend)(menu,@selector(accessibilityPerformEscape))){event(@"ginppai.menu.dismissFailed");return;}
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW,250*NSEC_PER_MSEC),dispatch_get_main_queue(),^{
            if(anchor.window)ginppaiMessageMenu(selected.snapshot,anchor);
        });
    }] forControlEvents:UIControlEventTouchUpInside];
    objc_setAssociatedObject(cell,&ginppaiMenuAccessoryKey,button,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    cell.accessoryView=button;event(@"ginppai.menu.nativeAttached");
}

// Gesture recognizers send one-argument actions. Their native Swift bodies
// can call the two-argument implementation directly, bypassing ObjC dispatch.
// Capture the selected message at both entry points, keeping nested calls scoped.
static void ginppaiWithNativeMenuContext(id handler,id gesture,UIView *view,dispatch_block_t invokeNative) {
    if(!ginppaiHasMessageTools()){invokeNative();return;}
    event(@"ginppai.menu.nativeEntry");
    KCMessageMenuContext *previous=ginppaiPendingMenu;id message=ginppaiCall(handler,@"message");
    NSDictionary *snapshot=ginppaiSnapshot(message);
    NSString *preserved=ginppaiPreservedMessage(message);
    if(preserved && [view isKindOfClass:UIView.class] && view.window) {
        if([gesture isKindOfClass:UIGestureRecognizer.class]) {
            UIGestureRecognizerState wanted=[gesture isKindOfClass:UILongPressGestureRecognizer.class]?UIGestureRecognizerStateBegan:UIGestureRecognizerStateRecognized;
            if(((UIGestureRecognizer *)gesture).state!=wanted)return;
        }
        NSMutableDictionary *display=[snapshot mutableCopy];display[@"text"]=preserved;display[@"preservedDeleted"]=@YES;
        ginppaiMessageMenu(display,view);event(@"ginppai.deleted.menu");return;
    }
    if(snapshot && [view isKindOfClass:UIView.class] && view.window) {
        KCMessageMenuContext *context=[KCMessageMenuContext new];context.snapshot=snapshot;context.anchor=view;ginppaiPendingMenu=context;
        ginppaiObserve(message);event(@"ginppai.menu.nativeInvoked");
    } else {ginppaiPendingMenu=nil;event(snapshot?@"ginppai.menu.noVisibleAnchor":@"ginppai.menu.noSnapshot");}
    @try {invokeNative();}
    @finally {ginppaiPendingMenu=previous;}
}

static void ginppaiInstallMessageMenu(void) {
    ginppaiCapabilities[@"messageContextMenu"]=@NO;
    if(!ginppaiNativeBuildVerified)return;
    Class menu=NSClassFromString(@"KakaoTalk.TalkContextMenuContainer");
    SEL rows=NSSelectorFromString(@"tableView:numberOfRowsInSection:");Method rowMethod=class_getInstanceMethod(menu,rows);
    if(!rowMethod || method_getNumberOfArguments(rowMethod)!=4)return;
    NSUInteger hooked=0;
    for(NSString *name in @[@"KakaoTalk.ChatMessageCellViewSubmenuHandler",@"KakaoTalk.ChatMessageCellViewMimicSubmenuHandler"]) {
        Class cls=NSClassFromString(name);
        for(NSString *selector in @[@"invokeSubmenuAction:",@"invokeSubmenuActionByTap:"]) {
            SEL sel=NSSelectorFromString(selector);Method m=class_getInstanceMethod(cls,sel);
            if(!m || strcmp(method_getTypeEncoding(m),"v24@0:8@16"))continue;IMP original=method_getImplementation(m);
            replace(cls,sel,imp_implementationWithBlock(^(id self,id gesture){
                UIView *view=[gesture isKindOfClass:UIGestureRecognizer.class]?((UIGestureRecognizer *)gesture).view:nil;
                ginppaiWithNativeMenuContext(self,gesture,view,^{((void(*)(id,SEL,id))original)(self,sel,gesture);});
            }),method_getTypeEncoding(m));hooked++;
        }
        for(NSString *selector in @[@"invokeSubmenuAction:withTargetView:",@"invokeSubmenuActionByTap:withTargetView:"]) {
            SEL sel=NSSelectorFromString(selector);Method m=class_getInstanceMethod(cls,sel);
            if(!m || strcmp(method_getTypeEncoding(m),"v32@0:8@16@24"))continue;IMP original=method_getImplementation(m);
            replace(cls,sel,imp_implementationWithBlock(^(id self,id gesture,UIView *view){
                ginppaiWithNativeMenuContext(self,gesture,view,^{((void(*)(id,SEL,id,id))original)(self,sel,gesture,view);});
            }),method_getTypeEncoding(m));hooked++;
        }
    }
    if(!hooked)return;
    IMP original=method_getImplementation(rowMethod);
    replace(menu,rows,imp_implementationWithBlock(^NSInteger(id self,UITableView *table,NSInteger section){
        NSInteger count=((NSInteger(*)(id,SEL,id,NSInteger))original)(self,rows,table,section);
        ginppaiNativeMenuContext(self);return count;
    }),method_getTypeEncoding(rowMethod));
    SEL cellSel=NSSelectorFromString(@"tableView:cellForRowAtIndexPath:");Method cellMethod=class_getInstanceMethod(menu,cellSel);
    if(!cellMethod)return;IMP originalCell=method_getImplementation(cellMethod);
    replace(menu,cellSel,imp_implementationWithBlock(^id(id self,UITableView *table,NSIndexPath *index){
        id cell=((id(*)(id,SEL,id,id))originalCell)(self,cellSel,table,index);
        if([cell isKindOfClass:UITableViewCell.class])ginppaiAttachMessageMenu(self,table,cell,index);
        return cell;
    }),method_getTypeEncoding(cellMethod));
    SEL reuse=@selector(prepareForReuse);Method reuseMethod=class_getInstanceMethod(UITableViewCell.class,reuse);
    if(reuseMethod){IMP originalReuse=method_getImplementation(reuseMethod);
        replace(UITableViewCell.class,reuse,imp_implementationWithBlock(^(UITableViewCell *self){
            ginppaiClearMenuAccessory(self);((void(*)(id,SEL))originalReuse)(self,reuse);
        }),method_getTypeEncoding(reuseMethod));}
    ginppaiCapabilities[@"messageContextMenu"]=@YES;
}
