// Customization feature adapters for KakaoTalk 26.7.3. Included in KakaoCustomizer.m.
#import <CoreData/CoreData.h>
#import <mach/mach.h>
#import <dlfcn.h>

static NSMutableDictionary *ginppaiLayoutDiagnostics;
static NSMutableSet *ginppaiCellLayouts;
static NSDictionary *ginppaiUnreadProbe;
static NSDictionary *ginppaiCoverProbe;
static __weak id ginppaiChatController;
static __weak id ginppaiChat;
static const char ginppaiBubbleKey;
static const char ginppaiToolsTargetKey;
static intptr_t ginppaiImageSlide;
static const uint8_t *ginppaiImageBase;
static BOOL ginppaiNativeBuildVerified;
static PHImageRequestID (*ginppaiOriginalPhotoRequest)(id,SEL,PHAsset *,PHImageRequestOptions *,void(^)(NSData *,NSString *,CGImagePropertyOrientation,NSDictionary *));

static PHImageRequestID ginppaiPhotoRequest(id self,SEL selector,PHAsset *asset,PHImageRequestOptions *options,
                                         void(^completion)(NSData *,NSString *,CGImagePropertyOrientation,NSDictionary *)) {
    uintptr_t caller=(uintptr_t)__builtin_return_address(0)-ginppaiImageSlide;
    // Exact Photos data call sites in the verified 26.7.3 MediaKit wrappers.
    // The unrelated reporting and video editing call sites are excluded.
    BOOL mediaCaller=caller==0x104de6b68 || caller==0x104e12ed8 || caller==0x104e13388;
    if(!completion || !option(@"stripPhotoMetadata") || !ginppaiNativeBuildVerified || !mediaCaller)
        return ginppaiOriginalPhotoRequest(self,selector,asset,options,completion);
    event(@"ginppai.photoPrivacy.request");
    return ginppaiOriginalPhotoRequest(self,selector,asset,options,^(NSData *data,NSString *type,CGImagePropertyOrientation orientation,NSDictionary *info){
        if(!data.length){completion(data,type,orientation,info);return;}
        NSData *clean=((id(*)(id,SEL,id))objc_msgSend)(NSClassFromString(@"KCPhotoPrivacyBridge"),NSSelectorFromString(@"sanitizedCopy:"),data);
        if(clean) {event(@"ginppai.photoPrivacy.cleaned");completion(clean,type,orientation,info);}
        else {
            // Match Customization's fail-open contract: unsupported containers must
            // not break media sending. Settings disclose the fallback.
            event(@"ginppai.photoPrivacy.unchanged");completion(data,type,orientation,info);
        }
    });
}
static void ginppaiInstallPhotoPrivacy(void) {
    Class cls=PHImageManager.class;SEL sel=@selector(requestImageDataAndOrientationForAsset:options:resultHandler:);
    Method method=class_getInstanceMethod(cls,sel);ginppaiCapabilities[@"stripPhotoMetadata"]=@NO;
    if(!method || !ginppaiNativeBuildVerified)return;
    ginppaiOriginalPhotoRequest=(void *)method_getImplementation(method);
    replace(cls,sel,(IMP)ginppaiPhotoRequest,method_getTypeEncoding(method));ginppaiCapabilities[@"stripPhotoMetadata"]=@YES;
}

static id ginppaiCall(id obj, NSString *name) {
    SEL sel=NSSelectorFromString(name);Method m=class_getInstanceMethod(object_getClass(obj),sel);
    if(!m || method_getNumberOfArguments(m)!=2)return nil;
    char result[64];method_getReturnType(m,result,sizeof(result));
    return result[0]=='@'?((id(*)(id,SEL))objc_msgSend)(obj,sel):nil;
}
static int64_t ginppaiInt(id obj,NSString *name) {
    SEL sel=NSSelectorFromString(name);Method m=class_getInstanceMethod(object_getClass(obj),sel);
    if(!m || method_getNumberOfArguments(m)!=2)return 0;
    char type[64];method_getReturnType(m,type,sizeof(type));
    if(type[0]=='q' || type[0]=='Q')return ((int64_t(*)(id,SEL))objc_msgSend)(obj,sel);
    if(type[0]=='@'){id n=ginppaiCall(obj,name);return [n isKindOfClass:NSNumber.class]?[n longLongValue]:0;}
    return 0;
}
static id ginppaiField(id obj,NSString *name) {
    if(!obj)return nil;
    return ((id(*)(id,SEL,id,id))objc_msgSend)(NSClassFromString(@"KCMessageBridge"),NSSelectorFromString(@"fieldObject:name:"),obj,name);
}
static NSDictionary *ginppaiSnapshot(id message) {
    if(!message)return nil;
    NSDictionary *value=((id(*)(id,SEL,id))objc_msgSend)(NSClassFromString(@"KCMessageBridge"),NSSelectorFromString(@"snapshot:"),message);
    if(!value)return nil;
    NSMutableDictionary *result=[value mutableCopy];
    if(![result[@"text"] isKindOfClass:NSString.class]) {
        id text=ginppaiCall(message,@"messageTextForTransmission");
        // The fallback returns an empty string before native decryption has
        // finished. Do not record that placeholder as a received original.
        if([text isKindOfClass:NSString.class] && [text length])result[@"text"]=text;
    }
    return result;
}
static void ginppaiObserve(id message) {
    if(!option(@"messageHistory"))return;
    NSDictionary *snapshot=ginppaiSnapshot(message);
    if(snapshot){((void(*)(id,SEL,id))objc_msgSend)(NSClassFromString(@"KCMessageHistory"),NSSelectorFromString(@"observe:"),snapshot);event(@"ginppai.history.observed");}
}
static NSArray *ginppaiHistory(NSDictionary *snapshot) {
    if(!snapshot)return @[];
    return ((id(*)(id,SEL,id,id))objc_msgSend)(NSClassFromString(@"KCMessageHistory"),NSSelectorFromString(@"historyWithChatID:logID:"),snapshot[@"chatID"],snapshot[@"logID"])?:@[];
}
static UIViewController *ginppaiHost(UIView *view) {
    UIViewController *host=view.window.rootViewController;
    if(!host)for(UIScene *scene in UIApplication.sharedApplication.connectedScenes)if([scene isKindOfClass:UIWindowScene.class])for(UIWindow *window in ((UIWindowScene *)scene).windows)if(window.isKeyWindow){host=window.rootViewController;break;}
    for(NSUInteger i=0;host && i<20;i++) {
        if(host.presentedViewController && !host.presentedViewController.isBeingDismissed){host=host.presentedViewController;continue;}
        if([host isKindOfClass:UINavigationController.class]){host=((UINavigationController *)host).visibleViewController;continue;}
        if([host isKindOfClass:UITabBarController.class]){host=((UITabBarController *)host).selectedViewController;continue;}
        break;
    }
    return host;
}
static NSString *ginppaiJSON(id value) {
    if(![NSJSONSerialization isValidJSONObject:value])return @"표시할 자료가 없습니다.";
    NSData *data=[NSJSONSerialization dataWithJSONObject:value options:NSJSONWritingPrettyPrinted|NSJSONWritingSortedKeys error:nil];
    return [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding]?:@"표시할 자료가 없습니다.";
}

@interface KCGinppaiTrackerProtocol : NSURLProtocol
@end
@implementation KCGinppaiTrackerProtocol
+ (BOOL)canInitWithRequest:(NSURLRequest *)request {
    if(!request.URL)return NO;
    Class bridge=NSClassFromString(@"KCMessageBridge");
    if(option(@"blockSDKTracking") && ((BOOL(*)(id,SEL,id))objc_msgSend)(bridge,NSSelectorFromString(@"isTrackerURL:"),request.URL))return YES;
    return option(@"blockTalkShareLog") && ginppaiNativeBuildVerified &&
        ((BOOL(*)(id,SEL,id))objc_msgSend)(bridge,NSSelectorFromString(@"isShareLogURL:"),request.URL);
}
+ (NSURLRequest *)canonicalRequestForRequest:(NSURLRequest *)request {return request;}
- (void)startLoading {
    // Terminate the measurement locally. No payload is read, copied or sent to
    // a replacement server. The SDK receives a local HTTP no-content response.
    NSHTTPURLResponse *response=[[NSHTTPURLResponse alloc] initWithURL:self.request.URL statusCode:204 HTTPVersion:@"HTTP/1.1" headerFields:@{@"Cache-Control":@"no-store"}];
    [self.client URLProtocol:self didReceiveResponse:response cacheStoragePolicy:NSURLCacheStorageNotAllowed];
    [self.client URLProtocolDidFinishLoading:self];
    BOOL share=((BOOL(*)(id,SEL,id))objc_msgSend)(NSClassFromString(@"KCMessageBridge"),NSSelectorFromString(@"isShareLogURL:"),self.request.URL);
    event(share?@"ginppai.shareLog.blocked":@"ginppai.sdkTracking.blocked");
}
- (void)stopLoading {}
@end

@interface KCMessageTextController : UIViewController
@property(nonatomic,copy) NSString *content;
@end
@implementation KCMessageTextController
- (void)viewDidLoad {
    [super viewDidLoad];self.view.backgroundColor=UIColor.systemBackgroundColor;
    UITextView *text=[[UITextView alloc] initWithFrame:CGRectZero];text.editable=NO;text.selectable=YES;text.text=self.content;
    text.font=[UIFont monospacedSystemFontOfSize:13 weight:UIFontWeightRegular];text.backgroundColor=UIColor.systemBackgroundColor;
    text.textColor=UIColor.labelColor;text.textContainerInset=UIEdgeInsetsMake(16,12,24,12);text.translatesAutoresizingMaskIntoConstraints=NO;
    [self.view addSubview:text];[NSLayoutConstraint activateConstraints:@[[text.topAnchor constraintEqualToAnchor:self.view.safeAreaLayoutGuide.topAnchor],[text.leadingAnchor constraintEqualToAnchor:self.view.leadingAnchor],[text.trailingAnchor constraintEqualToAnchor:self.view.trailingAnchor],[text.bottomAnchor constraintEqualToAnchor:self.view.safeAreaLayoutGuide.bottomAnchor]]];
    self.navigationItem.leftBarButtonItem=[[UIBarButtonItem alloc] initWithTitle:@"닫기" style:UIBarButtonItemStylePlain target:self action:@selector(close)];
    self.navigationItem.rightBarButtonItem=[[UIBarButtonItem alloc] initWithTitle:@"복사" style:UIBarButtonItemStylePlain target:self action:@selector(copyText)];
}
- (void)close {[self dismissViewControllerAnimated:YES completion:nil];}
- (void)copyText {UIPasteboard.generalPasteboard.string=self.content;self.navigationItem.rightBarButtonItem.title=@"복사됨";}
@end
static void ginppaiShowText(UIView *anchor,NSString *title,NSString *text) {
    KCMessageTextController *vc=[KCMessageTextController new];vc.title=title;vc.content=text;
    UIViewController *host=ginppaiHost(anchor);UINavigationController *navigation=[[UINavigationController alloc] initWithRootViewController:vc];
    if([host isKindOfClass:UIAlertController.class]) {
        UIViewController *presenter=host.presentingViewController;
        [host dismissViewControllerAnimated:YES completion:^{[presenter presentViewController:navigation animated:YES completion:nil];}];
    } else [host presentViewController:navigation animated:YES completion:nil];
}
static NSDictionary *ginppaiReceipts(NSDictionary *snapshot) {
    if([snapshot[@"logID"] longLongValue]<=0)return @{@"error":@"아직 전송되지 않은 메시지에는 읽음 정보가 없습니다."};
    id chat=ginppaiChat;if(!chat)return @{@"error":@"현재 채팅방의 읽음 정보를 가져오지 못했습니다."};
    if(ginppaiInt(chat,@"id")!=[snapshot[@"chatID"] longLongValue])return @{@"error":@"선택한 메시지와 채팅방이 일치하지 않습니다."};
    id marks=ginppaiCall(chat,@"readMarks"),members=ginppaiCall(chat,@"activeMemberIDs");
    if(!marks)marks=@[];
    if([marks isKindOfClass:NSArray.class] && (!ginppaiNativeBuildVerified || [members isKindOfClass:NSSet.class]))return @{@"error":@"이 설치의 읽음 정보 형식을 확인하지 못했습니다."};
    if(![marks isKindOfClass:NSDictionary.class] && ![marks isKindOfClass:NSArray.class])return @{@"error":@"이 채팅방에서 읽음 위치 자료를 사용할 수 없습니다."};
    if([members isKindOfClass:NSSet.class])members=[members allObjects];
    if(![members isKindOfClass:NSArray.class])members=@[];
    NSDictionary *input=@{@"watermarks":marks,@"members":members,@"logID":@([snapshot[@"logID"] longLongValue]),@"senderID":snapshot[@"senderID"]?:@"",@"myID":[NSString stringWithFormat:@"%lld",(long long)ginppaiInt(ginppaiChatController,@"myUserID")],@"activeCount":@(ginppaiInt(chat,@"activeMemberCount"))};
    NSMutableDictionary *result=[((id(*)(id,SEL,id))objc_msgSend)(NSClassFromString(@"KCMessageBridge"),NSSelectorFromString(@"readReceipts:"),input) mutableCopy];
    NSMutableDictionary *names=[NSMutableDictionary new];id users=ginppaiCall(chat,@"displayUsers");
    if([users isKindOfClass:NSArray.class] || [users isKindOfClass:NSSet.class])for(id user in users) {
        id name=ginppaiCall(user,@"talkName")?:ginppaiCall(user,@"name");
        if([name isKindOfClass:NSString.class])names[[NSString stringWithFormat:@"%lld",(long long)ginppaiInt(user,@"id")]]=name;
    }
    if([input[@"myID"] longLongValue]>0)names[input[@"myID"]]=@"나";
    result[@"names"]=names;return result;
}
static NSString *ginppaiReceiptText(NSDictionary *result) {
    if(result[@"error"])return result[@"error"];
    NSMutableString *text=[NSMutableString stringWithString:@"기기가 현재 가진 읽음 위치를 기준으로 합니다. 자료가 없는 사용자는 확인 불가로 표시합니다.\n\n"];
    NSArray *keys=@[@"read",@"unread",@"unknown"],*titles=@[@"읽음",@"읽지 않음",@"확인 불가"];
    for(NSUInteger i=0;i<keys.count;i++) {
        NSArray *ids=result[keys[i]];[text appendFormat:@"%@ (%lu)\n",titles[i],(unsigned long)ids.count];
        for(NSString *uid in ids)[text appendFormat:@"%@\n",result[@"names"][uid]?:[NSString stringWithFormat:@"사용자 ID %@",uid]];
        [text appendString:@"\n"];
    }
    if([result[@"missingMembers"] integerValue]>0)[text appendFormat:@"목록에 없는 참여자: %@명\n",result[@"missingMembers"]];
    return text;
}
static void ginppaiShowHistory(NSDictionary *snapshot,UIView *anchor) {
    NSArray *history=ginppaiHistory(snapshot);NSMutableString *text=[NSMutableString new];
    NSDateFormatter *format=[NSDateFormatter new];format.dateFormat=@"yyyy-MM-dd HH:mm:ss";
    for(NSDictionary *revision in history) {
        [text appendFormat:@"%@\n유형 %@\n%@\n\n",[format stringFromDate:[NSDate dateWithTimeIntervalSince1970:[revision[@"observedAt"] doubleValue]]],revision[@"type"],revision[@"text"]?:@""];
        if([revision[@"attachment"] length])[text appendFormat:@"첨부 정보\n%@\n\n",revision[@"attachment"]];
    }
    if(!history.count)[text appendString:@"보관된 기록이 없습니다. 기능을 켠 뒤 기기에서 확인한 메시지부터 기록합니다. 받기 전에 삭제된 내용은 복원할 수 없습니다."];
    ginppaiShowText(anchor,@"메시지 기록",text);event(@"ginppai.history.open");
}

@interface KCMessageHistoryListController : UITableViewController <UISearchResultsUpdating>
@property(nonatomic,strong) NSArray *entries;
@property(nonatomic,strong) NSArray *filtered;
@property(nonatomic,strong) UISearchController *search;
@end
@implementation KCMessageHistoryListController
- (void)viewDidLoad {
    [super viewDidLoad];self.title=@"메시지 보관함";
    self.entries=((id(*)(id,SEL))objc_msgSend)(NSClassFromString(@"KCMessageHistory"),NSSelectorFromString(@"recentEntries"))?:@[];self.filtered=self.entries;
    self.search=[[UISearchController alloc] initWithSearchResultsController:nil];self.search.searchResultsUpdater=self;self.search.obscuresBackgroundDuringPresentation=NO;
    self.search.searchBar.placeholder=@"보관한 내용 검색";self.navigationItem.searchController=self.search;self.definesPresentationContext=YES;
}
- (void)updateSearchResultsForSearchController:(UISearchController *)search {
    NSString *query=search.searchBar.text?:@"";
    self.filtered=query.length?[self.entries filteredArrayUsingPredicate:[NSPredicate predicateWithBlock:^BOOL(NSDictionary *entry,NSDictionary *bindings){return [entry[@"preview"] localizedCaseInsensitiveContainsString:query];}]]:self.entries;
    [self.tableView reloadData];
}
- (NSInteger)tableView:(UITableView *)table numberOfRowsInSection:(NSInteger)section {return self.filtered.count;}
- (NSString *)tableView:(UITableView *)table titleForFooterInSection:(NSInteger)section {return @"최근 보관한 메시지 최대 500개를 표시합니다. 기기에서 기록한 원문과 변경 내용을 확인하는 읽기 전용 보관함입니다.";}
- (UITableViewCell *)tableView:(UITableView *)table cellForRowAtIndexPath:(NSIndexPath *)index {
    NSDictionary *entry=self.filtered[index.row];UITableViewCell *cell=[[UITableViewCell alloc] initWithStyle:UITableViewCellStyleSubtitle reuseIdentifier:nil];
    cell.textLabel.text=[entry[@"preview"] length]?entry[@"preview"]:@"텍스트 없는 메시지";cell.textLabel.numberOfLines=2;
    cell.detailTextLabel.text=[NSString stringWithFormat:@"기록 %@개",entry[@"revisions"]];cell.accessoryType=UITableViewCellAccessoryDisclosureIndicator;return cell;
}
- (void)tableView:(UITableView *)table didSelectRowAtIndexPath:(NSIndexPath *)index {
    [table deselectRowAtIndexPath:index animated:YES];ginppaiShowHistory(self.filtered[index.row],self.view);
}
@end
static void ginppaiAfterDismiss(UIView *anchor,void(^action)(void)) {
    UIViewController *host=ginppaiHost(anchor);
    if([host isKindOfClass:UIAlertController.class]) [host dismissViewControllerAnimated:YES completion:action];
    else dispatch_async(dispatch_get_main_queue(),action);
}
static NSArray<UIAccessibilityCustomAction *> *ginppaiNativeMenuActions(id handler) {
    id values=ginppaiCall(handler,@"subMenuActionsForAccessibility");
    if(![values isKindOfClass:NSArray.class])return @[];
    NSMutableArray *actions=[NSMutableArray new];
    for(id value in values)if([value isKindOfClass:UIAccessibilityCustomAction.class] && [value name].length && actions.count<32)[actions addObject:value];
    return actions;
}
static void ginppaiNativeActionsMenu(NSArray<UIAccessibilityCustomAction *> *actions,UIView *anchor) {
    ginppaiAfterDismiss(anchor,^{
        UIViewController *host=ginppaiHost(anchor);
        UIAlertController *menu=[UIAlertController alertControllerWithTitle:@"카카오톡 메시지 기능" message:nil preferredStyle:UIAlertControllerStyleActionSheet];
        for(UIAccessibilityCustomAction *action in actions) {
            [menu addAction:[UIAlertAction actionWithTitle:action.name style:UIAlertActionStyleDefault handler:^(UIAlertAction *selected){
                ginppaiAfterDismiss(anchor,^{
                    BOOL handled=NO;
                    if(action.actionHandler)handled=action.actionHandler(action);
                    else {
                        id target=action.target;SEL selector=action.selector;Method method=class_getInstanceMethod(object_getClass(target),selector);
                        if(method) {
                            char type[16];method_getReturnType(method,type,sizeof(type));
                            if(type[0]=='B' || type[0]=='c') {
                                if(method_getNumberOfArguments(method)==3)handled=((BOOL(*)(id,SEL,id))objc_msgSend)(target,selector,action);
                                else if(method_getNumberOfArguments(method)==2)handled=((BOOL(*)(id,SEL))objc_msgSend)(target,selector);
                            }
                        }
                    }
                    event(handled?@"ginppai.nativeAction.handled":@"ginppai.nativeAction.unavailable");
                    if(!handled)ginppaiShowText(anchor,@"카카오톡 메시지 기능",@"현재 메시지에서 이 기능을 실행하지 못했습니다.");
                });
            }]];
        }
        [menu addAction:[UIAlertAction actionWithTitle:@"닫기" style:UIAlertActionStyleCancel handler:nil]];
        menu.popoverPresentationController.sourceView=anchor?:host.view;menu.popoverPresentationController.sourceRect=anchor?anchor.bounds:CGRectMake(1,1,1,1);
        [host presentViewController:menu animated:YES completion:nil];event(@"ginppai.nativeActions.open");
    });
}
static void ginppaiMessageMenu(id message,UIView *anchor,id nativeHandler) {
    NSDictionary *snapshot=ginppaiSnapshot(message);UIViewController *host=ginppaiHost(anchor);
    if(!snapshot || !host || [host isKindOfClass:UIAlertController.class])return;
    ginppaiObserve(message);
    UIAlertController *menu=[UIAlertController alertControllerWithTitle:@"Ginppai 메시지 도구" message:nil preferredStyle:UIAlertControllerStyleActionSheet];
    NSArray *nativeActions=ginppaiNativeMenuActions(nativeHandler);
    if(nativeActions.count)[menu addAction:[UIAlertAction actionWithTitle:@"카카오톡 기본 메뉴" style:UIAlertActionStyleDefault handler:^(UIAlertAction *a){ginppaiNativeActionsMenu(nativeActions,anchor);}]];
    if(option(@"messageDetails"))[menu addAction:[UIAlertAction actionWithTitle:@"메시지 상세 정보" style:UIAlertActionStyleDefault handler:^(UIAlertAction *a){ginppaiShowText(anchor,@"메시지 상세 정보",ginppaiJSON(snapshot));event(@"ginppai.details.open");}]];
    if(option(@"readReceipts"))[menu addAction:[UIAlertAction actionWithTitle:@"읽은 사람 확인" style:UIAlertActionStyleDefault handler:^(UIAlertAction *a){ginppaiShowText(anchor,@"읽은 사람 확인",ginppaiReceiptText(ginppaiReceipts(snapshot)));event(@"ginppai.receipts.open");}]];
    if(option(@"messageHistory"))[menu addAction:[UIAlertAction actionWithTitle:@"삭제 및 수정 기록" style:UIAlertActionStyleDefault handler:^(UIAlertAction *a){ginppaiShowHistory(snapshot,anchor);}]];
    [menu addAction:[UIAlertAction actionWithTitle:@"채팅방 ID 복사" style:UIAlertActionStyleDefault handler:^(UIAlertAction *a){UIPasteboard.generalPasteboard.string=snapshot[@"chatID"];}]];
    [menu addAction:[UIAlertAction actionWithTitle:@"닫기" style:UIAlertActionStyleCancel handler:nil]];
    menu.popoverPresentationController.sourceView=anchor?:host.view;menu.popoverPresentationController.sourceRect=anchor?anchor.bounds:CGRectMake(CGRectGetMidX(host.view.bounds),CGRectGetMidY(host.view.bounds),1,1);
    [host presentViewController:menu animated:YES completion:nil];event(@"ginppai.menu.open");
}

@interface KCMessageToolsTarget : NSObject
@property(nonatomic,weak) UIView *bubble;
@end
@implementation KCMessageToolsTarget
- (void)tap:(UIButton *)button {
    id message=((id(*)(id,SEL,id))objc_msgSend)(NSClassFromString(@"KCMessageBridge"),NSSelectorFromString(@"bubbleMessage:"),self.bubble);
    UIView *cell=self.bubble;
    while(cell && ![cell isKindOfClass:UITableViewCell.class])cell=cell.superview;
    id handler=cell?((id(*)(id,SEL,id))objc_msgSend)(NSClassFromString(@"KCMessageBridge"),NSSelectorFromString(@"nativeMenuHandler:"),cell):nil;
    if(message)ginppaiMessageMenu(message,button,handler);
}
@end
static void ginppaiUpdateBubble(UIView *view,BOOL observe) {
    BOOL modern=[view isKindOfClass:UITableViewCell.class] && [NSStringFromClass(view.class) hasPrefix:@"_TtGC10ChatBubble"];
    if(modern || [view isKindOfClass:NSClassFromString(@"KakaoTalk.UserMessageCellView")]) {
        id message=((id(*)(id,SEL,id))objc_msgSend)(NSClassFromString(@"KCMessageBridge"),NSSelectorFromString(@"bubbleMessage:"),view);
        if(observe)ginppaiObserve(message);
        if(option(@"messageDetails") || option(@"readReceipts") || option(@"messageHistory")) {
            // Native photo/movie wrappers deliberately have zero bounds. The
            // enclosing table cell is the visible, reusable layout container.
            UIView *ancestor=modern?view:view.superview;
            while(ancestor && ![ancestor isKindOfClass:UITableViewCell.class])ancestor=ancestor.superview;
            UIView *host=((UITableViewCell *)ancestor).contentView;
            if(!host)return;
            UIButton *button=objc_getAssociatedObject(host,&ginppaiBubbleKey);
            if(!button){
                KCMessageToolsTarget *target=[KCMessageToolsTarget new];
                button=[UIButton buttonWithType:UIButtonTypeSystem];[button setImage:[UIImage systemImageNamed:@"info.circle"] forState:UIControlStateNormal];button.accessibilityLabel=@"Ginppai 메시지 도구";
                button.backgroundColor=[UIColor.secondarySystemBackgroundColor colorWithAlphaComponent:0.94];button.layer.cornerRadius=13;
                [button addTarget:target action:@selector(tap:) forControlEvents:UIControlEventTouchUpInside];[host addSubview:button];objc_setAssociatedObject(host,&ginppaiBubbleKey,button,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
                objc_setAssociatedObject(button,&ginppaiToolsTargetKey,target,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
            }
            ((KCMessageToolsTarget *)objc_getAssociatedObject(button,&ginppaiToolsTargetKey)).bubble=view;
            button.hidden=!message || host.bounds.size.width<35 || host.bounds.size.height<24;
            BOOL sent=[NSStringFromClass(view.class) containsString:@"SentBubbleCell"];
            button.frame=CGRectMake(sent?4:MAX(0,host.bounds.size.width-30),2,26,26);[host bringSubviewToFront:button];
            if(ginppaiLayoutDiagnostics.count<12 || ginppaiLayoutDiagnostics[NSStringFromClass(view.class)])
                ginppaiLayoutDiagnostics[NSStringFromClass(view.class)]=@{@"bounds":NSStringFromCGRect(view.bounds),@"hostBounds":NSStringFromCGRect(host.bounds),@"buttonFrame":NSStringFromCGRect(button.frame),@"hidden":@(button.hidden),@"messageFound":@(message!=nil),@"attached":@(button.superview==host)};
        }
    }
}
static void ginppaiDecorateBubble(UIView *view) {
    ginppaiUpdateBubble(view,YES);
    for(UIView *child in [view.subviews copy])if(![child isKindOfClass:UIButton.class])ginppaiDecorateBubble(child);
}

static BOOL __attribute__((swiftcall)) ginppaiTypingBlocked(__attribute__((swift_context)) void *object) {
    // The native input pipeline branches around SendTypingActionUseCase at
    // 0x100dbcb94 when this property is true. Text editing continues normally.
    event(@"ginppai.typing.blocked");return YES;
}
static BOOL ginppaiWritableRegion(void *pointer,size_t length) {
    vm_address_t address=(vm_address_t)pointer;vm_size_t size=0;
    vm_region_basic_info_data_64_t info;mach_msg_type_number_t count=VM_REGION_BASIC_INFO_COUNT_64;mach_port_t object=0;
    kern_return_t result=vm_region_64(mach_task_self(),&address,&size,VM_REGION_BASIC_INFO_64,(vm_region_info_t)&info,&count,&object);
    if(object)mach_port_deallocate(mach_task_self(),object);
    return result==KERN_SUCCESS && (info.protection&VM_PROT_WRITE) && (uintptr_t)pointer>=address && (uintptr_t)pointer+length<=address+size;
}
static BOOL ginppaiWritablePointer(void **slot,void *expected,void *replacement) {
    if(!ginppaiWritableRegion(slot,sizeof(*slot)) || *slot!=expected)return NO;
    *slot=replacement;return YES;
}
static BOOL __attribute__((swiftcall)) ginppaiNativeTrue(__attribute__((swift_context)) void *object) {return YES;}
typedef BOOL __attribute__((swiftcall)) (*KCMoreTabPolicy)(uint8_t);
static KCMoreTabPolicy ginppaiOriginalMoreTab;
static BOOL __attribute__((swiftcall)) ginppaiNativeMoreTab(uint8_t type) {
    return type==2?NO:ginppaiOriginalMoreTab(type);
}
typedef uint8_t __attribute__((swiftcall)) (*KCCoverFunction)(BOOL,BOOL,BOOL,BOOL,BOOL,BOOL);
static KCCoverFunction ginppaiOriginalCover;
static uint8_t __attribute__((swiftcall)) ginppaiNativeCover(BOOL valid,BOOL version,BOOL consent,BOOL mobile,BOOL locked,BOOL adult) {
    return ginppaiOriginalCover(valid,version,NO,NO,locked,adult);
}
static BOOL __attribute__((swiftcall)) ginppaiNativeAlimtalk(__attribute__((swift_context)) void *object);
static __typeof__(ginppaiNativeAlimtalk) *ginppaiOriginalAlimtalk;
static BOOL __attribute__((swiftcall)) ginppaiNativeAlimtalk(__attribute__((swift_context)) void *object) {
    uintptr_t caller=(uintptr_t)__builtin_return_address(0)-ginppaiImageSlide;
    if(caller==0x10078c17c || caller==0x1007b7530)return YES;
    return ginppaiOriginalAlimtalk(object);
}
extern int64_t __attribute__((swiftcall)) KCGinppaiUnreadCalculator(int64_t,int64_t,void *,void *,BOOL,int64_t);
static void ginppaiInstallNativeOptions(void) {
    NSArray *keys=@[@"uncapMessageUnread",@"sendMarkdown",@"forwardLeverage",@"showMobileMessages",@"hideMoreGame"];
    for(NSString *key in keys)ginppaiCapabilities[key]=@NO;
    ginppaiCapabilities[@"subdeviceAlimtalk"]=@NO;
    if(!ginppaiNativeBuildVerified)return;
    NSUInteger supported=memcmp(ginppaiImageBase+0x7ff0,"GCOPT003",8)==0?5:(memcmp(ginppaiImageBase+0x7ff0,"GCOPT002",8)==0?4:(memcmp(ginppaiImageBase+0x7ff0,"GCOPT001",8)==0?3:0));
    if(supported) {
        uint64_t *flags=(uint64_t *)(0x1083e7f10+ginppaiImageSlide);
        if(!ginppaiWritableRegion(flags,sizeof(*flags)) || *flags!=0)return;
        uint64_t value=0;for(NSUInteger bit=0;bit<supported;bit++)if(option(keys[bit]))value|=1ull<<bit;
        *flags=value;for(NSUInteger bit=0;bit<supported;bit++)ginppaiCapabilities[keys[bit]]=@YES;
        ginppaiCapabilities[@"subdeviceAlimtalk"]=@(supported>=4);
        [installed addObject:@"Verified native feature switches"];
    } else {
        void(*hook)(void *,void *,void **)=dlsym(RTLD_DEFAULT,"MSHookFunction");
        if(!hook)return;
        const uintptr_t addresses[]={0x1017bec68,0x1046ade0c,0x1046acee0,0x100b153b4,0x1008133d0};
        const uint32_t prologues[]={0xd2800009,0xd10203ff,0xd10203ff,0x0a010008,0x72001c08};
        for(NSUInteger i=0;i<keys.count;i++) {
            void *address=(void *)(addresses[i]+ginppaiImageSlide);
            if(*(const uint32_t *)address!=prologues[i])continue;
            ginppaiCapabilities[keys[i]]=@YES;
            if(option(keys[i])) {
                void *original=NULL;
                if(i==3) {hook(address,(void *)&ginppaiNativeCover,(void **)&ginppaiOriginalCover);original=(void *)ginppaiOriginalCover;}
                else if(i==4) {hook(address,(void *)&ginppaiNativeMoreTab,(void **)&ginppaiOriginalMoreTab);original=(void *)ginppaiOriginalMoreTab;}
                else hook(address,i==0?(void *)&KCGinppaiUnreadCalculator:(void *)&ginppaiNativeTrue,&original);
                ginppaiCapabilities[keys[i]]=@(original!=NULL);
            }
        }
        if(*(const uint32_t *)(0x104679d4c+ginppaiImageSlide)==0x90000000) {
            ginppaiCapabilities[@"subdeviceAlimtalk"]=@YES;
            if(option(@"showMobileMessages")) {
                hook((void *)(0x104679d4c+ginppaiImageSlide),(void *)&ginppaiNativeAlimtalk,(void **)&ginppaiOriginalAlimtalk);
                ginppaiCapabilities[@"subdeviceAlimtalk"]=@(ginppaiOriginalAlimtalk!=NULL);
            }
        }
    }
    // Exercise the installed native calculator with synthetic data, including
    // counts on either side of 99. No private message or network is involved.
    ginppaiUnreadProbe=((id(*)(id,SEL,uintptr_t))objc_msgSend)(NSClassFromString(@"KCMessageBridge"),NSSelectorFromString(@"nativeUnreadProbe:"),(uintptr_t)(0x1017bec68+ginppaiImageSlide));
    BOOL enabled=option(@"uncapMessageUnread") && [ginppaiCapabilities[@"uncapMessageUnread"] boolValue];
    BOOL valid=[ginppaiUnreadProbe[@"98"] integerValue]==98 && [ginppaiUnreadProbe[@"99"] integerValue]==99 &&
      [ginppaiUnreadProbe[@"100"] integerValue]==(enabled?100:99) && [ginppaiUnreadProbe[@"350"] integerValue]==(enabled?350:99) && [ginppaiUnreadProbe[@"mixed"] integerValue]==2;
    if(!valid)ginppaiCapabilities[@"uncapMessageUnread"]=@NO;
    event(valid?@"ginppai.unread.nativeProbePassed":@"ginppai.unread.nativeProbeFailed");
    ginppaiCoverProbe=((id(*)(id,SEL,uintptr_t))objc_msgSend)(NSClassFromString(@"KCMessageBridge"),NSSelectorFromString(@"nativeCoverProbe:"),(uintptr_t)(0x100b153b4+ginppaiImageSlide));
    BOOL show=option(@"showMobileMessages") && [ginppaiCapabilities[@"showMobileMessages"] boolValue];
    BOOL coverValid=[ginppaiCoverProbe[@"mobileOnly"] integerValue]==(show?6:3) && [ginppaiCoverProbe[@"subdeviceConsent"] integerValue]==(show?6:2) && [ginppaiCoverProbe[@"locked"] integerValue]==4 && [ginppaiCoverProbe[@"adultLocked"] integerValue]==5 && [ginppaiCoverProbe[@"invalid"] integerValue]==0 && [ginppaiCoverProbe[@"invalidVersion"] integerValue]==1;
    if(!coverValid)ginppaiCapabilities[@"showMobileMessages"]=@NO;
    event(coverValid?@"ginppai.cover.nativeProbePassed":@"ginppai.cover.nativeProbeFailed");
}
static void ginppaiNativeBeforeSave(void *context) {
    static _Thread_local BOOL observing;
    if(observing || !option(@"messageHistory"))return;
    observing=YES;
    @try {ginppaiObserve((__bridge id)context);event(@"ginppai.history.nativeSave");}
    @catch(NSException *exception){event(@"ginppai.history.captureFailed");}
    observing=NO;
}
static BOOL __attribute__((swiftcall)) ginppaiNativeSave(__attribute__((swift_context)) void *context);
static __typeof__(ginppaiNativeSave) *ginppaiOriginalSave;
static BOOL __attribute__((swiftcall)) ginppaiNativeSave(__attribute__((swift_context)) void *context) {
    ginppaiNativeBeforeSave(context);return ginppaiOriginalSave(context);
}
static void ginppaiInstallNativeHistory(void) {
    if(!option(@"messageHistory") || !ginppaiNativeBuildVerified)return;
    if(memcmp(ginppaiImageBase+0x77f0,"GCAMPLE1",8)==0) {
        void **slot=(void **)(0x1083e7f08+ginppaiImageSlide);
        if(ginppaiWritablePointer(slot,NULL,(void *)&ginppaiNativeBeforeSave)) {
            ginppaiCapabilities[@"nativeMessageHistory"]=@YES;
            [installed addObject:@"ChatMessage.save prepared native callback"];
        }
        return;
    }
    // Jailbreak packages already depend on Substrate or ElleKit. The optional
    // function adapter is used only when that runtime is present.
    void(*hook)(void *,void *,void **)=dlsym(RTLD_DEFAULT,"MSHookFunction");
    if(hook && *(const uint32_t *)(0x104631934+ginppaiImageSlide)==0xaa1e03e0) {
        hook((void *)(0x104631934+ginppaiImageSlide),(void *)&ginppaiNativeSave,(void **)&ginppaiOriginalSave);
        if(ginppaiOriginalSave){ginppaiCapabilities[@"nativeMessageHistory"]=@YES;[installed addObject:@"ChatMessage.save jailbreak native hook"];}
    }
}
static void ginppaiInstallTyping(void) {
    if(!option(@"hideTyping") || !ginppaiNativeBuildVerified)return;
    intptr_t slide=0;BOOL found=NO;
    for(uint32_t i=0;i<_dyld_image_count();i++)if([[[NSString stringWithUTF8String:_dyld_get_image_name(i)] lastPathComponent] isEqualToString:@"KakaoTalk"]){slide=_dyld_get_image_vmaddr_slide(i);found=YES;break;}
    Class base=NSClassFromString(@"KakaoTalk.ChatInputController");if(!found || !base)return;
    unsigned int count=0;Class *classes=objc_copyClassList(&count);NSUInteger patched=0;
    for(unsigned int i=0;i<count;i++) {
        Class cls=classes[i];BOOL match=NO;
        for(Class current=cls;current;current=class_getSuperclass(current))if(current==base){match=YES;break;}
        if(!match)continue;
        // Verified Swift class metadata vtable offset for this exact app build.
        void **slot=(void **)((uint8_t *)(__bridge void *)cls+0xc90);
        if(ginppaiWritablePointer(slot,(void *)(0x100d91268+slide),(void *)&ginppaiTypingBlocked))patched++;
    }
    free(classes);ginppaiCapabilities[@"hideTyping"]=@(patched>0);
    if(patched)[installed addObject:@"ChatInputController isBlockTypingAction Swift getter"];
}

#include "FriendList.inc"
static void installCustomizationFeatures(void) {
    ginppaiCapabilities=[NSMutableDictionary new];
    ginppaiLayoutDiagnostics=[NSMutableDictionary new];
    ginppaiCellLayouts=[NSMutableSet new];
    for(uint32_t i=0;i<_dyld_image_count();i++)if([[[NSString stringWithUTF8String:_dyld_get_image_name(i)] lastPathComponent] isEqualToString:@"KakaoTalk"]){ginppaiImageSlide=_dyld_get_image_vmaddr_slide(i);ginppaiImageBase=(const uint8_t *)_dyld_get_image_header(i);break;}
    if(ginppaiImageBase) {
        const struct mach_header_64 *header=(const struct mach_header_64 *)ginppaiImageBase;
        const uint8_t *cursor=ginppaiImageBase+sizeof(*header),*end=cursor+header->sizeofcmds;
        const uint8_t expectedUUID[16]={0xfa,0xab,0x71,0xb6,0x29,0x25,0x3d,0x3e,0x81,0x7f,0xc6,0x61,0x42,0xb3,0x77,0x78};
        for(uint32_t i=0;i<header->ncmds && cursor+sizeof(struct load_command)<=end;i++) {
            const struct load_command *cmd=(const struct load_command *)cursor;
            if(cmd->cmdsize<sizeof(*cmd) || cursor+cmd->cmdsize>end)break;
            if(cmd->cmd==LC_UUID && cmd->cmdsize>=sizeof(struct uuid_command))ginppaiNativeBuildVerified=memcmp(((const struct uuid_command *)cmd)->uuid,expectedUUID,16)==0;
            cursor+=cmd->cmdsize;
        }
    }
    ginppaiInstallTyping();
    ginppaiInstallNativeOptions();
    ginppaiInstallPhotoPrivacy();
    ginppaiInstallNativeHistory();
    ginppaiInstallFriendList();
    if(ginppaiNativeBuildVerified) {
        NSDictionary *render=((id(*)(id,SEL,id))objc_msgSend)(NSClassFromString(@"KCMoreTabRendering"),NSSelectorFromString(@"install:"),activeOptions);
        NSDictionary *caps=render[@"capabilities"];
        for(NSString *key in caps)if(![key isEqualToString:@"hideAds"])ginppaiCapabilities[key]=caps[key];
        ginppaiCapabilities[@"moreAdLayout"]=caps[@"hideAds"]?:@NO;
        for(NSString *name in render[@"installed"])[installed addObject:[@"More rendering: " stringByAppendingString:name]];
    }
    for(NSString *name in @[@"KakaoTalk.UserMessageCellView",@"KakaoTalk.PhotoMessageCellView",@"KakaoTalk.MovieMessageCellView",@"KakaoTalk.LongTextMessageCellView"]) {
        Class bubble=NSClassFromString(name);SEL layout=@selector(layoutSubviews);Method layoutMethod=class_getInstanceMethod(bubble,layout);
        if(layoutMethod) {
            IMP original=method_getImplementation(layoutMethod);
            replace(bubble,layout,imp_implementationWithBlock(^(UIView *self){
                ((void(*)(id,SEL))original)(self,layout);ginppaiUpdateBubble(self,NO);
            }),method_getTypeEncoding(layoutMethod));
        }
    }
    Class controller=NSClassFromString(@"KakaoTalk.ChattingTableViewController");
    SEL display=NSSelectorFromString(@"tableView:willDisplayCell:forRowAtIndexPath:");Method method=class_getInstanceMethod(controller,display);
    if(method){
        IMP original=method_getImplementation(method);
        replace(controller,display,imp_implementationWithBlock(^(id self,id table,UIView *cell,id index){
            ((void(*)(id,SEL,id,id,id))original)(self,display,table,cell,index);
            if([cell isKindOfClass:UITableViewCell.class]) {
                UIButton *old=objc_getAssociatedObject(((UITableViewCell *)cell).contentView,&ginppaiBubbleKey);
                old.hidden=YES;((KCMessageToolsTarget *)objc_getAssociatedObject(old,&ginppaiToolsTargetKey)).bubble=nil;
            }
            ginppaiChatController=self;ginppaiChat=ginppaiField(ginppaiField(self,@"messageProvider"),@"chat");
            NSString *cellName=NSStringFromClass(cell.class);
            if([cell isKindOfClass:UITableViewCell.class] && [cellName hasPrefix:@"_TtGC10ChatBubble"] && ![ginppaiCellLayouts containsObject:cellName]) {
                SEL layout=@selector(layoutSubviews);Method layoutMethod=class_getInstanceMethod(cell.class,layout);
                if(layoutMethod) {
                    IMP layoutOriginal=method_getImplementation(layoutMethod);
                    replace(cell.class,layout,imp_implementationWithBlock(^(UIView *view){
                        ((void(*)(id,SEL))layoutOriginal)(view,layout);ginppaiUpdateBubble(view,NO);
                    }),method_getTypeEncoding(layoutMethod));
                    [ginppaiCellLayouts addObject:cellName];
                }
            }
            ginppaiDecorateBubble(cell);
            __weak UIView *weakCell=cell;
            dispatch_async(dispatch_get_main_queue(),^{if(weakCell)ginppaiDecorateBubble(weakCell);});
        }),method_getTypeEncoding(method));
        for(NSString *key in @[@"messageDetails",@"readReceipts",@"messageHistory"])ginppaiCapabilities[key]=@YES;
    }
    if(option(@"uncapChatUnread")) {
        Class cls=NSClassFromString(@"Chat");SEL sel=NSSelectorFromString(@"displayUnreadCount");Method m=class_getInstanceMethod(cls,sel);
        if(m){IMP original=method_getImplementation(m);replace(cls,sel,imp_implementationWithBlock(^id(id self){int64_t count=ginppaiInt(self,@"unreadCount");if(count>0){event(@"ginppai.unread.full");return [NSString stringWithFormat:@"%lld",(long long)count];}return ((id(*)(id,SEL))original)(self,sel);}),method_getTypeEncoding(m));ginppaiCapabilities[@"uncapChatUnread"]=@YES;}
    }
    if(option(@"externalBrowser")) {
        SEL sel=NSSelectorFromString(@"processWebUrlLink:appName:userID:inWeb:");Method m=class_getInstanceMethod(controller,sel);
        if(m){IMP original=method_getImplementation(m);replace(controller,sel,imp_implementationWithBlock(^(id self,id value,id app,id user,BOOL inWeb){
            NSURL *url=[value isKindOfClass:NSURL.class]?value:([value isKindOfClass:NSString.class]?[NSURL URLWithString:value]:nil);
            if(url.host.length && [@[@"http",@"https"] containsObject:url.scheme.lowercaseString]){[UIApplication.sharedApplication openURL:url options:@{} completionHandler:nil];event(@"ginppai.browser.open");}
            else ((void(*)(id,SEL,id,id,id,BOOL))original)(self,sel,value,app,user,inWeb);
        }),method_getTypeEncoding(m));ginppaiCapabilities[@"externalBrowser"]=@YES;}
    }
    if(option(@"disableSentry"))for(NSString *name in @[@"SentrySDKInternal",@"Sentry.SentrySDK"]) {
        Class sdk=NSClassFromString(name);Class meta=object_getClass(sdk);
        for(NSString *methodName in @[@"startWithOptions:",@"startWithConfigureOptions:"]) {
            SEL sel=NSSelectorFromString(methodName);Method m=class_getInstanceMethod(meta,sel);if(!m)continue;
            replace(meta,sel,imp_implementationWithBlock(^(id self,id options){event(@"ginppai.sentry.blocked");}),method_getTypeEncoding(m));ginppaiCapabilities[@"disableSentry"]=@YES;
        }
    }
    ginppaiCapabilities[@"blockTalkShareLog"]=@(ginppaiNativeBuildVerified && ((BOOL(*)(id,SEL))objc_msgSend)(NSClassFromString(@"KCMessageBridge"),NSSelectorFromString(@"shareLogAvailable")));
    if(option(@"blockSDKTracking") || option(@"blockTalkShareLog")) {
        [NSURLProtocol registerClass:KCGinppaiTrackerProtocol.class];
        Class cls=NSURLSessionConfiguration.class;SEL sel=@selector(protocolClasses);Method m=class_getInstanceMethod(cls,sel);
        if(m){IMP original=method_getImplementation(m);replace(cls,sel,imp_implementationWithBlock(^id(id self){
            NSArray *classes=((id(*)(id,SEL))original)(self,sel);
            if([classes containsObject:KCGinppaiTrackerProtocol.class])return classes;
            return [@[KCGinppaiTrackerProtocol.class] arrayByAddingObjectsFromArray:classes?:@[]];
        }),method_getTypeEncoding(m));ginppaiCapabilities[@"blockSDKTracking"]=@YES;}
    }
}
