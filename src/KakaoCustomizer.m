#import <UIKit/UIKit.h>
#import <QuartzCore/QuartzCore.h>
#import <objc/runtime.h>
#import <objc/message.h>
#ifndef KC_BUILD_ID
#define KC_BUILD_ID "local-untracked"
#endif

// KakaoTalk 26.7.3 only. Fixed telemetry endpoints can be blocked locally.
static NSDictionary *configurationReport(void);
static NSDictionary *ginppaiReport(void);
static NSMutableDictionary *counts;
static NSMutableArray *installed;
static NSMutableArray *viewSamples;
static BOOL pendingWrite;
static const char observedKey;

static void saveReport(void) {
    if (pendingWrite) return;
    pendingWrite = YES;
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, NSEC_PER_SEC), dispatch_get_main_queue(), ^{
        pendingWrite = NO;
        NSDictionary *report = @{ @"version": @"3.0.0-dev.2", @"build": @KC_BUILD_ID, @"target": @"26.7.3",
            @"configuration": configurationReport(), @"ginppai": ginppaiReport(), @"installedHooks": installed, @"events": counts, @"adViewSamples": viewSamples };
        NSData *data = [NSJSONSerialization dataWithJSONObject:report options:NSJSONWritingPrettyPrinted error:nil];
        NSString *path = [NSHomeDirectory() stringByAppendingPathComponent:@"Documents/KakaoAdBlock-status.json"];
        [data writeToFile:path atomically:YES];
    });
}

static void event(NSString *key) {
    if (![NSThread isMainThread]) {
        dispatch_async(dispatch_get_main_queue(), ^{ event(key); });
        return;
    }
    counts[key] = @([counts[key] unsignedIntegerValue] + 1);
    saveReport();
}

static void replace(Class cls, SEL sel, IMP imp, const char *types) {
    // Adding an inherited method to this exact class never modifies UIKit itself.
    if (!class_addMethod(cls, sel, imp, types)) class_replaceMethod(cls, sel, imp, types);
    [installed addObject:[NSString stringWithFormat:@"%@ %@", NSStringFromClass(cls), NSStringFromSelector(sel)]];
}

#import <mach-o/dyld.h>

static NSDictionary *activeOptions;
static NSMutableDictionary *savedOptions;
static NSDictionary *navigationReport;
static BOOL countrySupport;
static NSString *optionsPath(void) { return [NSHomeDirectory() stringByAppendingPathComponent:@"Library/Preferences/KakaoCustomizer.plist"]; }
static NSArray *toggleKeys(void) { return @[@"hideAds",@"hideShortForm",@"preferOpenChat",@"hideShopping",@"hideCallTab",@"hideTabBadges",@"scrollTopOnRetap",@"quickSettings",@"profilePhotoSave",@"hideTyping",@"messageDetails",@"readReceipts",@"messageHistory",@"uncapChatUnread",@"uncapMessageUnread",@"sendMarkdown",@"forwardLeverage",@"showMobileMessages",@"hideMoreGame",@"hideFriendFeed",@"stripPhotoMetadata",@"externalBrowser",@"disableSentry",@"blockSDKTracking",@"blockTalkShareLog"]; }
static NSDictionary *defaultOptions(void) {
    return @{@"hideAds":@YES,@"hideShortForm":@YES,@"preferOpenChat":@YES,@"hideShopping":@YES,
             @"hideCallTab":@NO,@"hideTabBadges":@NO,@"scrollTopOnRetap":@NO,@"quickSettings":@NO,@"profilePhotoSave":@YES,@"hideTyping":@YES,@"messageDetails":@YES,@"readReceipts":@YES,@"messageHistory":@YES,@"uncapChatUnread":@YES,@"uncapMessageUnread":@YES,@"sendMarkdown":@NO,@"forwardLeverage":@NO,@"showMobileMessages":@YES,@"hideMoreGame":@YES,@"hideFriendFeed":@YES,@"stripPhotoMetadata":@NO,@"externalBrowser":@YES,@"disableSentry":@YES,@"blockSDKTracking":@YES,@"blockTalkShareLog":@YES,@"countryISO":@"",@"startupTab":@"remember"};
}
static BOOL option(NSString *key) { return [activeOptions[key] boolValue]; }
static BOOL saveOptions(void) { return [savedOptions writeToFile:optionsPath() atomically:YES]; }
static BOOL hasChanges(void) { return ![savedOptions isEqualToDictionary:activeOptions]; }
static BOOL overseas(NSDictionary *options) { NSString *iso=options[@"countryISO"];return iso.length && ![iso isEqualToString:@"KR"]; }
static NSString *countryName(NSString *iso) {
    if(!iso.length)return @"계정 기본값";
    NSString *name=[[NSLocale localeWithLocaleIdentifier:@"ko_KR"] displayNameForKey:NSLocaleCountryCode value:iso];
    return [NSString stringWithFormat:@"%@  /  %@",name?:iso,iso];
}
static NSArray *startupKeys(void) { return @[@"remember",@"friends",@"chats",@"now",@"calls",@"more"]; }
static NSArray *startupNames(void) { return @[@"카카오톡 기본 동작",@"친구",@"채팅",@"오픈채팅 / 지금",@"통화",@"더보기"]; }
static NSString *startupName(NSString *key) { NSUInteger i=[startupKeys() indexOfObject:key];return startupNames()[i==NSNotFound?0:i]; }
static void loadOptions(void) {
    savedOptions=[defaultOptions() mutableCopy];
    NSDictionary *disk=[NSDictionary dictionaryWithContentsOfFile:optionsPath()];
    for(NSString *key in toggleKeys())if([disk[key] isKindOfClass:NSNumber.class])savedOptions[key]=@([disk[key] boolValue]);
    NSString *iso=[disk[@"countryISO"] isKindOfClass:NSString.class]?[disk[@"countryISO"] uppercaseString]:nil;
    if(!iso && [disk[@"country"] isKindOfClass:NSNumber.class]) {
        NSInteger old=[disk[@"country"] integerValue];if(old>=0 && old<=3)iso=@[@"",@"KR",@"JP",@"US"][old];
    }
    if(iso && (!iso.length || [NSLocale.ISOCountryCodes containsObject:iso]))savedOptions[@"countryISO"]=iso;
    if([disk[@"startupTab"] isKindOfClass:NSString.class] && [startupKeys() containsObject:disk[@"startupTab"]])savedOptions[@"startupTab"]=disk[@"startupTab"];
    activeOptions=[savedOptions copy];
    for(uint32_t i=0;i<_dyld_image_count();i++) {
        const char *path=_dyld_get_image_name(i);
        if(!path || ![[NSString stringWithUTF8String:path].lastPathComponent isEqualToString:@"KakaoTalk"])continue;
        const uint8_t *base=(const uint8_t*)_dyld_get_image_header(i);
        if(memcmp(base+0x73f0,"KCUICFG3",8)==0) {
            NSString *code=activeOptions[@"countryISO"];
            uint32_t packed=code.length==2?([code characterAtIndex:0]|([code characterAtIndex:1]<<8)):0;
            *(volatile uint32_t*)(base+0x83e7f00)=packed;
            countrySupport=YES;
        }
        break;
    }
    // Persist the migration only after validation. No account properties are touched.
    saveOptions();
}
static void showSaveFailure(UIViewController *host) {
    UIAlertController *alert=[UIAlertController alertControllerWithTitle:@"저장하지 못했습니다" message:@"다시 시도해 주세요. 기존 설정은 유지됩니다." preferredStyle:UIAlertControllerStyleAlert];
    [alert addAction:[UIAlertAction actionWithTitle:@"확인" style:UIAlertActionStyleDefault handler:nil]];
    [host presentViewController:alert animated:YES completion:nil];
}
static BOOL storeOption(NSString *key,id value,UIViewController *host) {
    id old=savedOptions[key];savedOptions[key]=value;
    if(saveOptions())return YES;
    savedOptions[key]=old;showSaveFailure(host);return NO;
}

static NSMutableDictionary *ginppaiCapabilities;
#include "CustomizationSettings.inc"

@implementation KCCountryController
- (instancetype)init { return [super initWithStyle:UITableViewStyleInsetGrouped]; }
- (void)viewDidLoad {
    [super viewDidLoad];self.title=@"화면 국가  /  지역";
    NSMutableArray *all=[NSMutableArray array];NSLocale *english=[NSLocale localeWithLocaleIdentifier:@"en_US"];
    for(NSString *iso in NSLocale.ISOCountryCodes) {
        if(iso.length!=2)continue;
        [all addObject:@{@"iso":iso,@"name":countryName(iso),@"english":[english displayNameForKey:NSLocaleCountryCode value:iso]?:iso}];
    }
    [all sortUsingComparator:^NSComparisonResult(NSDictionary *a,NSDictionary *b){return [a[@"name"] localizedStandardCompare:b[@"name"]];}];
    self.countries=all;self.filtered=all;
    self.countrySearch=[[UISearchController alloc] initWithSearchResultsController:nil];
    self.countrySearch.searchResultsUpdater=self;self.countrySearch.obscuresBackgroundDuringPresentation=NO;
    self.countrySearch.searchBar.placeholder=@"국가 이름 또는 코드 검색";
    self.navigationItem.searchController=self.countrySearch;self.navigationItem.hidesSearchBarWhenScrolling=NO;
    self.definesPresentationContext=YES;
}
- (void)updateSearchResultsForSearchController:(UISearchController *)search {
    NSString *query=[search.searchBar.text stringByTrimmingCharactersInSet:NSCharacterSet.whitespaceAndNewlineCharacterSet];
    self.filtered=query.length?[self.countries filteredArrayUsingPredicate:[NSPredicate predicateWithBlock:^BOOL(NSDictionary *row,NSDictionary *bindings){
        return [row[@"name"] localizedStandardContainsString:query] || [row[@"english"] localizedStandardContainsString:query];
    }]]:self.countries;
    if(query.length==2)self.filtered=[self.filtered sortedArrayUsingComparator:^NSComparisonResult(NSDictionary *a,NSDictionary *b){
        BOOL first=[a[@"iso"] caseInsensitiveCompare:query]==NSOrderedSame,second=[b[@"iso"] caseInsensitiveCompare:query]==NSOrderedSame;
        return first!=second?(first?NSOrderedAscending:NSOrderedDescending):[a[@"name"] localizedStandardCompare:b[@"name"]];
    }];
    [self.tableView reloadData];
}
- (NSInteger)numberOfSectionsInTableView:(UITableView *)table { return 2; }
- (NSInteger)tableView:(UITableView *)table numberOfRowsInSection:(NSInteger)section { return section==0?1:self.filtered.count; }
- (NSString *)tableView:(UITableView *)table titleForHeaderInSection:(NSInteger)section { return section==1?[NSString stringWithFormat:@"국가  /  지역 %lu개",(unsigned long)self.filtered.count]:nil; }
- (NSString *)tableView:(UITableView *)table titleForFooterInSection:(NSInteger)section { return section==0?@"계정 기본값은 원래 국가 판정을 사용합니다. 계정과 전화번호의 국가는 유지됩니다. 여러 국가는 같은 해외 화면을 사용합니다.":nil; }
- (UITableViewCell *)tableView:(UITableView *)table cellForRowAtIndexPath:(NSIndexPath *)index {
    UITableViewCell *cell=[[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:nil];
    NSString *iso=index.section==0?@"":self.filtered[index.row][@"iso"];
    cell.textLabel.text=countryName(iso);cell.textLabel.numberOfLines=0;
    if([iso isEqualToString:savedOptions[@"countryISO"]])cell.accessoryType=UITableViewCellAccessoryCheckmark;
    return cell;
}
- (void)tableView:(UITableView *)table didSelectRowAtIndexPath:(NSIndexPath *)index {
    NSString *iso=index.section==0?@"":self.filtered[index.row][@"iso"];
    storeOption(@"countryISO",iso,self);[table deselectRowAtIndexPath:index animated:YES];[self refresh];
}
@end

@implementation KCStartupController
- (instancetype)init { return [super initWithStyle:UITableViewStyleInsetGrouped]; }
- (void)viewDidLoad { [super viewDidLoad];self.title=@"시작 탭"; }
- (NSInteger)tableView:(UITableView *)table numberOfRowsInSection:(NSInteger)section { return startupKeys().count; }
- (BOOL)available:(NSString *)key {
    if([key isEqualToString:@"now"])return !overseas(savedOptions);
    if([key isEqualToString:@"calls"])return overseas(savedOptions) && ![savedOptions[@"hideCallTab"] boolValue];
    return YES;
}
- (UITableViewCell *)tableView:(UITableView *)table cellForRowAtIndexPath:(NSIndexPath *)index {
    UITableViewCell *cell=[[UITableViewCell alloc] initWithStyle:UITableViewCellStyleSubtitle reuseIdentifier:nil];
    NSString *key=startupKeys()[index.row];cell.textLabel.text=startupNames()[index.row];
    if(![self available:key]){cell.textLabel.textColor=UIColor.secondaryLabelColor;cell.detailTextLabel.text=@"현재 선택한 화면에서는 사용할 수 없습니다";}
    if([key isEqualToString:savedOptions[@"startupTab"]])cell.accessoryType=UITableViewCellAccessoryCheckmark;
    return cell;
}
- (NSString *)tableView:(UITableView *)table titleForFooterInSection:(NSInteger)section { return @"카카오톡을 새로 실행할 때 선택한 탭을 엽니다. 이미 채팅방이나 상세 화면이 열렸으면 시작 탭 이동을 생략합니다."; }
- (void)tableView:(UITableView *)table didSelectRowAtIndexPath:(NSIndexPath *)index {
    NSString *key=startupKeys()[index.row];if([self available:key])storeOption(@"startupTab",key,self);
    [table deselectRowAtIndexPath:index animated:YES];[self refresh];
}
@end

static UITableView *findSettingsTable(UIView *view) {
    if([view isKindOfClass:UITableView.class])return (UITableView *)view;
    for(UIView *child in view.subviews){UITableView *table=findSettingsTable(child);if(table)return table;}
    return nil;
}
static const char settingsFooterKey;
static void addSettingsEntry(UIViewController *vc) {
    UITableView *table=findSettingsTable(vc.view);
    if(!table || objc_getAssociatedObject(table,&settingsFooterKey))return;
    UIView *previous=table.tableFooterView;CGFloat previousHeight=previous?CGRectGetHeight(previous.frame):0;
    UIView *wrapper=[[UIView alloc] initWithFrame:CGRectMake(0,0,CGRectGetWidth(table.bounds),previousHeight+112)];
    wrapper.autoresizingMask=UIViewAutoresizingFlexibleWidth;
    UIButtonConfiguration *config=[UIButtonConfiguration filledButtonConfiguration];
    config.title=@"Ginppai";config.subtitle=@"내 카카오톡 맞춤 설정";config.image=[UIImage systemImageNamed:@"slider.horizontal.3"];
    config.imagePadding=12;config.titleAlignment=UIButtonConfigurationTitleAlignmentLeading;
    config.baseBackgroundColor=UIColor.secondarySystemGroupedBackgroundColor;config.baseForegroundColor=UIColor.labelColor;config.cornerStyle=UIButtonConfigurationCornerStyleMedium;
    __weak UIViewController *weakVC=vc;
    UIButton *button=[UIButton buttonWithConfiguration:config primaryAction:[UIAction actionWithHandler:^(UIAction *action){
        UIViewController *host=weakVC;if(!host || host.presentedViewController)return;
        UINavigationController *nav=[[UINavigationController alloc] initWithRootViewController:[[KCSettingsController alloc] init]];
        [host presentViewController:nav animated:YES completion:nil];
    }]];
    button.contentHorizontalAlignment=UIControlContentHorizontalAlignmentLeading;
    button.frame=CGRectMake(16,previousHeight+8,MAX(100,CGRectGetWidth(wrapper.bounds)-32),64);button.autoresizingMask=UIViewAutoresizingFlexibleWidth;
    [wrapper addSubview:button];
    UILabel *credit=[[UILabel alloc] initWithFrame:CGRectMake(16,previousHeight+78,MAX(100,CGRectGetWidth(wrapper.bounds)-32),20)];
    credit.text=@"만든이  /  nogadamachine";credit.textAlignment=NSTextAlignmentCenter;
    credit.font=[UIFont preferredFontForTextStyle:UIFontTextStyleFootnote];credit.textColor=UIColor.secondaryLabelColor;
    credit.adjustsFontForContentSizeCategory=YES;credit.autoresizingMask=UIViewAutoresizingFlexibleWidth;
    [wrapper addSubview:credit];
    if(previous){previous.frame=CGRectMake(0,0,CGRectGetWidth(wrapper.bounds),previousHeight);previous.autoresizingMask=UIViewAutoresizingFlexibleWidth;[wrapper addSubview:previous];}
    table.tableFooterView=wrapper;objc_setAssociatedObject(table,&settingsFooterKey,@YES,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
}
static void installSettingsEntry(void) {
    Class cls=NSClassFromString(@"KakaoTalk.MainSettingsViewController");SEL sel=@selector(viewDidAppear:);Method method=class_getInstanceMethod(cls,sel);if(!method)return;
    IMP original=method_getImplementation(method);
    replace(cls,sel,imp_implementationWithBlock(^(UIViewController *vc,BOOL animated){((void (*)(id,SEL,BOOL))original)(vc,sel,animated);addSettingsEntry(vc);}),method_getTypeEncoding(method));
}
static NSDictionary *configurationReport(void) {
    return @{@"active":activeOptions?:@{},@"countrySwitchAvailable":@(countrySupport),@"navigation":navigationReport?:@{},@"countryCount":@(NSLocale.ISOCountryCodes.count)};
}


static void failLoad(id loader, NSString *callbackName) {
    if (!callbackName) return;
    dispatch_async(dispatch_get_main_queue(), ^{
        SEL getter = NSSelectorFromString(@"delegate");
        if (![loader respondsToSelector:getter]) return;
        id delegate = ((id (*)(id, SEL))objc_msgSend)(loader, getter);
        SEL callback = NSSelectorFromString(callbackName);
        if (![delegate respondsToSelector:callback]) return;
        if ([callbackName hasSuffix:@"error:"]) {
            NSError *error = [NSError errorWithDomain:NSURLErrorDomain code:NSURLErrorCancelled
                userInfo:@{NSLocalizedDescriptionKey: @"Advertisement disabled locally"}];
            ((void (*)(id, SEL, id, id))objc_msgSend)(delegate, callback, loader, error);
        } else {
            ((void (*)(id, SEL, id))objc_msgSend)(delegate, callback, loader);
        }
    });
}

static void blockLoad(NSString *className, NSString *selectorName, NSString *failureCallback) {
    Class cls = NSClassFromString(className);
    SEL sel = NSSelectorFromString(selectorName);
    Method method = class_getInstanceMethod(cls, sel);
    if (!method) return;
    char result[32]; method_getReturnType(method, result, sizeof(result));
    if (strcmp(result, "v") != 0) return;
    // These verified ad-loading entry points return void. Their arguments are
    // deliberately unused: no user objects, region IDs or keywords are read.
    IMP imp = imp_implementationWithBlock(^(id object) {
        event([NSString stringWithFormat:@"blocked:%@.%@", className, selectorName]);
        failLoad(object, failureCallback);
    });
    replace(cls, sel, imp, method_getTypeEncoding(method));
}

static void hideAd(UIView *view) {
    if (!objc_getAssociatedObject(view, &observedKey)) {
        objc_setAssociatedObject(view, &observedKey, @YES, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        NSMutableArray *parents = [NSMutableArray array];
        for (UIView *v = view; v && parents.count < 6; v = v.superview)
            [parents addObject:NSStringFromClass(v.class)];
        // Diagnostic metadata only; never collect labels, images or chat text.
        if (viewSamples.count < 24) [viewSamples addObject:@{@"classes": parents, @"frame": NSStringFromCGRect(view.frame)}];
        event([@"hidden:" stringByAppendingString:NSStringFromClass(view.class)]);
    }
    view.hidden = YES;
    view.alpha = 0;
    view.userInteractionEnabled = NO;
    view.accessibilityElementsHidden = YES;
}

static void collapseMoreAd(void) {
    Class cls = NSClassFromString(@"MoreTab.NativeADUIView");
    if (!cls || ![cls isSubclassOfClass:UIView.class]) return;
    Method fit = class_getInstanceMethod(cls, @selector(sizeThatFits:));
    if (fit) replace(cls, @selector(sizeThatFits:), imp_implementationWithBlock(^CGSize(UIView *view, CGSize size) {
        event(@"collapsed:MoreTab.NativeADUIView.sizeThatFits");
        return CGSizeMake(size.width, 0);
    }), method_getTypeEncoding(fit));
    Method intrinsic = class_getInstanceMethod(cls, @selector(intrinsicContentSize));
    if (intrinsic) replace(cls, @selector(intrinsicContentSize), imp_implementationWithBlock(^CGSize(UIView *view) {
        return CGSizeMake(UIViewNoIntrinsicMetric, 0);
    }), method_getTypeEncoding(intrinsic));
}

static void hideClass(NSString *name) {
    Class cls = NSClassFromString(name);
    if (!cls || ![cls isSubclassOfClass:UIView.class]) return;
    for (NSString *selector in @[@"didMoveToWindow", @"layoutSubviews"]) {
        SEL sel = NSSelectorFromString(selector);
        Method m = class_getInstanceMethod(cls, sel);
        IMP original = method_getImplementation(m);
        IMP replacement = imp_implementationWithBlock(^(UIView *view) {
            ((void (*)(id, SEL))original)(view, sel);
            hideAd(view);
        });
        replace(cls, sel, replacement, method_getTypeEncoding(m));
    }
    Method hidden = class_getInstanceMethod(cls, @selector(setHidden:));
    IMP originalHidden = method_getImplementation(hidden);
    replace(cls, @selector(setHidden:), imp_implementationWithBlock(^(UIView *view, BOOL ignored) {
        ((void (*)(id, SEL, BOOL))originalHidden)(view, @selector(setHidden:), YES);
    }), method_getTypeEncoding(hidden));
}

// Only KakaoTalk's verified 26.7.3 navigation views are modified.
static const char openSelectionPendingKey;
static BOOL containsNavigationText(UIView *view, NSString *text) {
    if ([view isKindOfClass:UILabel.class] && [((UILabel *)view).text isEqualToString:text]) return YES;
    for (UIView *child in view.subviews) if (containsNavigationText(child, text)) return YES;
    return NO;
}
static UIControl *findChip(UIView *view, NSString *text) {
    if ([view isKindOfClass:NSClassFromString(@"TalkDesignSystemUIKit.ChipTab")] &&
        containsNavigationText(view,text)) return (UIControl *)view;
    for (UIView *child in view.subviews) {
        UIControl *found=findChip(child,text); if(found)return found;
    }
    return nil;
}
static void hideBrandChip(UIView *view) {
    if ([NSStringFromClass(view.class) isEqualToString:@"TalkAppBase.BrandTabChip"]) {
        view.hidden=YES; view.userInteractionEnabled=NO; view.accessibilityElementsHidden=YES;
    }
    for (UIView *child in view.subviews) hideBrandChip(child);
}
static void selectOpenChat(UIView *header) {
    UIControl *control=findChip(header,@"오픈채팅");
    if(!header.window || !control || control.selected || objc_getAssociatedObject(header,&openSelectionPendingKey))return;
    objc_setAssociatedObject(header,&openSelectionPendingKey,@YES,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    __weak UIView *weakHeader=header;
    dispatch_async(dispatch_get_main_queue(),^{
        UIView *current=weakHeader;if(!current)return;
        UIControl *open=findChip(current,@"오픈채팅");
        if(current.window && open && !open.selected){[open sendActionsForControlEvents:UIControlEventTouchUpInside];event(@"tabs:openchat-auto-selected");}
        objc_setAssociatedObject(current,&openSelectionPendingKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    });
}
static void customizeNowHeader(UIView *header) {
    if(!option(@"hideShortForm"))return;
    UIControl *shortForm=findChip(header,@"숏폼"),*openChat=findChip(header,@"오픈채팅");
    if(!shortForm || !openChat)return;
    if(!shortForm.hidden)event(@"tabs:shortform-hidden");
    shortForm.hidden=YES;shortForm.userInteractionEnabled=NO;shortForm.accessibilityElementsHidden=YES;
    hideBrandChip(header);selectOpenChat(header);
}
static UIViewController *tabRoot(UIViewController *vc) {
    return [vc isKindOfClass:UINavigationController.class]?((UINavigationController *)vc).viewControllers.firstObject:vc;
}
static NSString *tabKind(UIViewController *vc) {
    NSString *nav=NSStringFromClass(vc.class),*root=NSStringFromClass(tabRoot(vc).class);
    if([nav isEqualToString:@"KakaoTalk.NowTabNavigationController"])return @"now";
    if([root isEqualToString:@"KakaoTalk.CallHistoryViewController"])return @"calls";
    if([root isEqualToString:@"KakaoTalk.ChatsViewController"])return @"chats";
    if([root isEqualToString:@"KakaoTalk.FriendsViewController"] || [root containsString:@"FriendFeed"] || [root containsString:@"FriendsFeed"])return @"friends";
    if([root isEqualToString:@"KakaoTalk.GlobalMoreViewController"] || [root containsString:@"MoreTab"] || [root containsString:@"MoreViewController"])return @"more";
    if([root containsString:@"Shopper"] || [root containsString:@"Shopping"])return @"shopping";
    return @"other";
}
static UITabBarController *mainController(UIView *view) {
    for(UIResponder *r=view.nextResponder;r;r=r.nextResponder)
        if([NSStringFromClass(r.class) isEqualToString:@"KakaoTalk.MainTabBarController"])return (UITabBarController *)r;
    return nil;
}
static void collectTabButtons(UIView *view,NSMutableArray<UIControl *> *buttons) {
    if([NSStringFromClass(view.class) isEqualToString:@"KakaoTalk.MainTabBarButton"]){[buttons addObject:(UIControl *)view];return;}
    for(UIView *child in view.subviews)collectTabButtons(child,buttons);
}
static const char hiddenTabKey,hiddenBadgeKey,redirectPendingKey,startupAppliedKey,quickGestureKey;
@interface KCQuickSettingsTarget : NSObject
- (void)open:(UIGestureRecognizer *)gesture;
@end
@implementation KCQuickSettingsTarget
- (void)open:(UIGestureRecognizer *)gesture {
    BOOL longPress=[gesture isKindOfClass:UILongPressGestureRecognizer.class];
    if(gesture.state!=(longPress?UIGestureRecognizerStateBegan:UIGestureRecognizerStateRecognized))return;
    UITabBarController *main=mainController(gesture.view);UIViewController *host=main.selectedViewController;
    if([host isKindOfClass:UINavigationController.class])host=((UINavigationController *)host).topViewController;
    if(!host || host.presentedViewController || main.presentedViewController)return;
    UINavigationController *nav=[[UINavigationController alloc] initWithRootViewController:[[KCSettingsController alloc] init]];
    [host presentViewController:nav animated:YES completion:nil];event(longPress?@"settings:opened-with-long-press":@"settings:opened-with-double-tap");
}
@end
static UIScrollView *mainScrollView(UIView *view) {
    if(view.hidden || view.alpha<0.01)return nil;
    UIScrollView *best=nil;
    if([view isKindOfClass:UIScrollView.class] && ((UIScrollView *)view).scrollEnabled && view.bounds.size.height>100)best=(UIScrollView *)view;
    for(UIView *child in view.subviews){UIScrollView *candidate=mainScrollView(child);
        if(candidate && (!best || candidate.bounds.size.width*candidate.bounds.size.height>best.bounds.size.width*best.bounds.size.height))best=candidate;
    }
    return best;
}
static void scrollSelectedTabToTop(UIView *bar) {
    UITabBarController *main=mainController(bar);UIViewController *vc=main.selectedViewController;
    if([vc isKindOfClass:UINavigationController.class])vc=((UINavigationController *)vc).topViewController;
    UIScrollView *scroll=mainScrollView(vc.view);
    if(!scroll)return;
    id<UIScrollViewDelegate> delegate=scroll.delegate;
    if([delegate respondsToSelector:@selector(scrollViewShouldScrollToTop:)] && ![delegate scrollViewShouldScrollToTop:scroll])return;
    [scroll setContentOffset:CGPointMake(scroll.contentOffset.x,-scroll.adjustedContentInset.top) animated:NO];
    // KakaoTalk restores its collapsing header in the scroll-to-top delegate callback.
    if([delegate respondsToSelector:@selector(scrollViewDidScrollToTop:)])[delegate scrollViewDidScrollToTop:scroll];
    event(@"tabs:retap-scrolled-to-top");
}
static void hideTabBadge(UIView *view) {
    if([NSStringFromClass(view.class) isEqualToString:@"TalkDesignSystemUIKit.UnreadBadge"]) {
        if(!objc_getAssociatedObject(view,&hiddenBadgeKey))event(@"tabs:badge-hidden");
        objc_setAssociatedObject(view,&hiddenBadgeKey,@YES,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        view.hidden=YES;view.accessibilityElementsHidden=YES;
    }
    for(UIView *child in view.subviews)hideTabBadge(child);
}
static void customizeMainBar(UIView *bar) {
    NSMutableArray<UIControl *> *buttons=[NSMutableArray array];collectTabButtons(bar,buttons);
    UITabBarController *controller=mainController(bar);NSArray<UIViewController *> *tabs=controller.viewControllers;
    BOOL mapped=tabs.count==buttons.count && tabs.count>0;
    NSUInteger firstCall=NSNotFound;NSMutableArray *report=[NSMutableArray array];
    for(NSUInteger i=0;i<buttons.count;i++) {
        UIControl *button=buttons[i];NSString *kind=mapped?tabKind(tabs[i]):@"unknown";
        BOOL duplicate=NO;
        if([kind isEqualToString:@"calls"]){duplicate=firstCall!=NSNotFound;if(!duplicate)firstCall=i;}
        BOOL shopping=[kind isEqualToString:@"shopping"] || [button.accessibilityLabel isEqualToString:@"쇼핑"];
        BOOL hide=duplicate || (option(@"hideCallTab") && [kind isEqualToString:@"calls"]) || (option(@"hideShopping") && shopping);
        if(hide) {
            if(!objc_getAssociatedObject(button,&hiddenTabKey))event(duplicate?@"tabs:duplicate-call-hidden":(shopping?@"tabs:shopping-hidden":@"tabs:call-hidden"));
            objc_setAssociatedObject(button,&hiddenTabKey,@YES,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
            // Preserve original indices and targets: only the arranged button is collapsed.
            button.hidden=YES;button.userInteractionEnabled=NO;button.accessibilityElementsHidden=YES;
        } else if(objc_getAssociatedObject(button,&hiddenTabKey)) {
            button.hidden=NO;button.userInteractionEnabled=YES;button.accessibilityElementsHidden=NO;
            objc_setAssociatedObject(button,&hiddenTabKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        }
        if(option(@"hideTabBadges"))hideTabBadge(button);
        if(option(@"quickSettings") && ([kind isEqualToString:@"more"] || [button.accessibilityLabel isEqualToString:@"더보기"]) && !objc_getAssociatedObject(button,&quickGestureKey)) {
            static KCQuickSettingsTarget *target;static dispatch_once_t once;dispatch_once(&once,^{target=[KCQuickSettingsTarget new];});
            UILongPressGestureRecognizer *gesture=[[UILongPressGestureRecognizer alloc] initWithTarget:target action:@selector(open:)];
            gesture.minimumPressDuration=0.55;gesture.cancelsTouchesInView=YES;[button addGestureRecognizer:gesture];
            UITapGestureRecognizer *doubleTap=[[UITapGestureRecognizer alloc] initWithTarget:target action:@selector(open:)];
            doubleTap.numberOfTapsRequired=2;doubleTap.cancelsTouchesInView=YES;[button addGestureRecognizer:doubleTap];
            objc_setAssociatedObject(button,&quickGestureKey,gesture,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        }
        [report addObject:@{@"index":@(i),@"kind":kind,@"root":mapped?NSStringFromClass(tabRoot(tabs[i]).class):@"",@"hidden":@(button.hidden),@"selected":@(button.selected)}];
    }
    NSDictionary *next=@{@"mapped":@(mapped),@"tabs":report,@"selectedIndex":@(controller.selectedIndex)};
    if(![next isEqualToDictionary:navigationReport]){navigationReport=next;saveReport();}
    // A restored or keyboard-selected hidden duplicate must select its visible twin.
    if(mapped && controller.selectedIndex<buttons.count && objc_getAssociatedObject(buttons[controller.selectedIndex],&hiddenTabKey) &&
       !objc_getAssociatedObject(controller,&redirectPendingKey)) {
        NSUInteger target=NSNotFound;
        if([tabKind(controller.selectedViewController) isEqualToString:@"calls"] && firstCall!=NSNotFound && !buttons[firstCall].hidden)target=firstCall;
        if(target==NSNotFound)for(NSUInteger i=0;i<buttons.count;i++)if(!buttons[i].hidden){target=i;break;}
        if(target!=NSNotFound) {
            objc_setAssociatedObject(controller,&redirectPendingKey,@YES,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
            __weak UITabBarController *weak=controller;
            dispatch_async(dispatch_get_main_queue(),^{UITabBarController *vc=weak;if(!vc)return;vc.selectedIndex=target;objc_setAssociatedObject(vc,&redirectPendingKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);event(@"tabs:hidden-selection-repaired");});
        }
    }
}
static void customizeBarsInView(UIView *view) {
    if([NSStringFromClass(view.class) isEqualToString:@"KakaoTalk.MainTabBarView"]){customizeMainBar(view);return;}
    for(UIView *child in view.subviews)customizeBarsInView(child);
}
static void installNavigationConvenience(void) {
    Class cls=NSClassFromString(@"KakaoTalk.MainTabBarController");
    SEL didSelect=@selector(tabBarController:didSelectViewController:);Method selection=class_getInstanceMethod(cls,didSelect);
    if(selection){IMP original=method_getImplementation(selection);replace(cls,didSelect,imp_implementationWithBlock(^(UITabBarController *vc,UITabBarController *bar,UIViewController *selected){
        ((void (*)(id,SEL,id,id))original)(vc,didSelect,bar,selected);customizeBarsInView(vc.view);
    }),method_getTypeEncoding(selection));}
    SEL appeared=@selector(viewDidAppear:);Method m=class_getInstanceMethod(cls,appeared);
    if(m) {
        IMP original=method_getImplementation(m);
        replace(cls,appeared,imp_implementationWithBlock(^(UITabBarController *vc,BOOL animated){
            ((void (*)(id,SEL,BOOL))original)(vc,appeared,animated);customizeBarsInView(vc.view);
            if(objc_getAssociatedObject(vc,&startupAppliedKey))return;
            objc_setAssociatedObject(vc,&startupAppliedKey,@YES,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
            NSString *desired=activeOptions[@"startupTab"];
            if([desired isEqualToString:@"remember"])return;
            UIViewController *selected=vc.selectedViewController;
            if(vc.presentedViewController || ([selected isKindOfClass:UINavigationController.class] && ((UINavigationController *)selected).viewControllers.count>1))return;
            NSUInteger target=NSNotFound;
            for(NSUInteger i=0;i<vc.viewControllers.count;i++)if([tabKind(vc.viewControllers[i]) isEqualToString:desired]){target=i;break;}
            if(target==NSNotFound || ([desired isEqualToString:@"calls"] && option(@"hideCallTab")))target=0;
            __weak UITabBarController *weak=vc;
            dispatch_async(dispatch_get_main_queue(),^{UITabBarController *current=weak;if(!current || current.presentedViewController)return;
                UIViewController *currentTab=current.selectedViewController;
                if([currentTab isKindOfClass:UINavigationController.class] && ((UINavigationController *)currentTab).viewControllers.count>1)return;
                if(target<current.viewControllers.count){current.selectedIndex=target;customizeBarsInView(current.view);event(@"tabs:startup-selected");}
            });
        }),method_getTypeEncoding(m));
    }
    // Keep VoiceOver's custom element list consistent with the visible tab buttons.
    Class bar=NSClassFromString(@"KakaoTalk.MainTabBarView");SEL ax=@selector(accessibilityElements);Method axMethod=class_getInstanceMethod(bar,ax);
    if(axMethod) {
        IMP original=method_getImplementation(axMethod);
        replace(bar,ax,imp_implementationWithBlock(^NSArray *(UIView *view){
            NSArray *items=((id (*)(id,SEL))original)(view,ax);if(![items isKindOfClass:NSArray.class])return items;
            return [items filteredArrayUsingPredicate:[NSPredicate predicateWithBlock:^BOOL(id item,NSDictionary *bindings){return !([item isKindOfClass:UIView.class] && objc_getAssociatedObject(item,&hiddenTabKey));}]];
        }),method_getTypeEncoding(axMethod));
    }
    // Badge updates do not always trigger a tab-bar layout.
    if(option(@"hideTabBadges")) {
        Class badge=NSClassFromString(@"TalkDesignSystemUIKit.UnreadBadge");SEL sel=@selector(layoutSubviews);Method method=class_getInstanceMethod(badge,sel);
        if(method){IMP original=method_getImplementation(method);replace(badge,sel,imp_implementationWithBlock(^(UIView *view){
            ((void (*)(id,SEL))original)(view,sel);
            for(UIView *parent=view.superview;parent;parent=parent.superview)if([NSStringFromClass(parent.class) isEqualToString:@"KakaoTalk.MainTabBarView"]){hideTabBadge(view);break;}
        }),method_getTypeEncoding(method));}
    }
}


static void repairVisibleNowHeader(UIView *view) {
    if ([NSStringFromClass(view.class) isEqualToString:@"TalkAppBase.NowTabHeaderChipSelectionView"])
        selectOpenChat(view);
    for (UIView *child in view.subviews) repairVisibleNowHeader(child);
}
static BOOL interceptNowReselection(UIView *bar, UIControl *button) {
    if(!option(@"hideShortForm") && !option(@"preferOpenChat") && !option(@"scrollTopOnRetap"))return NO;
    if (![button isKindOfClass:UIControl.class] || !button.selected ||
        ![NSStringFromClass(button.class) isEqualToString:@"KakaoTalk.MainTabBarButton"]) return NO;
    for (UIResponder *r=bar.nextResponder;r;r=r.nextResponder) {
        if (![NSStringFromClass(r.class) isEqualToString:@"KakaoTalk.MainTabBarController"]) continue;
        UITabBarController *controller=(UITabBarController *)r;
        if (![NSStringFromClass(controller.selectedViewController.class) isEqualToString:@"KakaoTalk.NowTabNavigationController"]) return NO;
        if(!option(@"hideShortForm") && !option(@"preferOpenChat")) {
            UIControl *open=findChip(controller.selectedViewController.view,@"오픈채팅");
            if(!open || !open.selected)return NO;
        }
        // The app reselect action resets Now to ShortForm. Consume only this
        // selected-tab gesture; ordinary navigation into Now remains unchanged.
        repairVisibleNowHeader(controller.selectedViewController.view);
        event(@"tabs:now-reselection-kept-openchat");
        return YES;
    }
    return NO;
}
static void installNowReselectionFix(void) {
    Class cls=NSClassFromString(@"KakaoTalk.MainTabBarView");
    for (NSString *name in @[@"handleButtonTouchDown:",@"handleButtonTouchUpInside:"]) {
        SEL sel=NSSelectorFromString(name);Method method=class_getInstanceMethod(cls,sel);
        if(!method)continue;
        IMP original=method_getImplementation(method);
        replace(cls,sel,imp_implementationWithBlock(^(UIView *bar, UIControl *button){
            if(interceptNowReselection(bar,button)) {
                if(option(@"scrollTopOnRetap") && sel==@selector(handleButtonTouchUpInside:))scrollSelectedTabToTop(bar);
                return;
            }
            ((void (*)(id,SEL,id))original)(bar,sel,button);
        }),method_getTypeEncoding(method));
    }
}

static void installTabCustomization(void) {
    for (NSString *name in @[@"KakaoTalk.MainTabBarView", @"TalkAppBase.NowTabHeaderChipSelectionView"]) {
        Class cls=NSClassFromString(name);
        if (!cls || ![cls isSubclassOfClass:UIView.class]) continue;
        for (NSString *methodName in @[@"layoutSubviews",@"didMoveToWindow"]) {
            SEL selector=NSSelectorFromString(methodName);
            Method method=class_getInstanceMethod(cls,selector);
            IMP original=method_getImplementation(method);
            BOOL isHeader=[name isEqualToString:@"TalkAppBase.NowTabHeaderChipSelectionView"];
            replace(cls,selector,imp_implementationWithBlock(^(UIView *view) {
                ((void (*)(id,SEL))original)(view,selector);
                if(isHeader)customizeNowHeader(view);else customizeMainBar(view);
            }),method_getTypeEncoding(method));
        }
    }
}


static void installInitialOpenChatPreference(void) {
    if(!option(@"preferOpenChat") && !option(@"hideShortForm"))return;
    Class cls=NSClassFromString(@"KakaoTalk.NowTabContainerViewController");SEL sel=@selector(viewDidAppear:);Method method=class_getInstanceMethod(cls,sel);if(!method)return;
    IMP original=method_getImplementation(method);
    replace(cls,sel,imp_implementationWithBlock(^(UIViewController *vc,BOOL animated){
        ((void (*)(id,SEL,BOOL))original)(vc,sel,animated);
        __weak UIViewController *weakVC=vc;
        dispatch_async(dispatch_get_main_queue(),^{UIViewController *current=weakVC;if(current.view.window)repairVisibleNowHeader(current.view);});
    }),method_getTypeEncoding(method));
}

#import <Photos/Photos.h>
#import <AVFoundation/AVFoundation.h>
#import <ImageIO/ImageIO.h>
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>
@interface KCMediaResolver : NSObject
+ (NSDictionary *)resolve:(id)object contexts:(NSArray *)contexts;
@end
static UIViewController *profileHost(UIView *view) {
    for(UIResponder *r=view.nextResponder;r;r=r.nextResponder)if([r isKindOfClass:UIViewController.class])return (UIViewController *)r;
    return nil;
}
static void profileNotice(UIView *view,NSString *title,NSString *message) {
    UIViewController *host=profileHost(view);if(!host || host.presentedViewController)return;
    UIAlertController *alert=[UIAlertController alertControllerWithTitle:title message:message preferredStyle:UIAlertControllerStyleAlert];
    [alert addAction:[UIAlertAction actionWithTitle:@"확인" style:UIAlertActionStyleDefault handler:nil]];
    [host presentViewController:alert animated:YES completion:nil];
}
static void profileToast(UIView *view,NSString *message) {
    if(!view.window)return;
    UILabel *label=[UILabel new];label.text=message;label.textAlignment=NSTextAlignmentCenter;
    label.font=[UIFont preferredFontForTextStyle:UIFontTextStyleSubheadline];label.textColor=UIColor.whiteColor;
    label.backgroundColor=[UIColor.blackColor colorWithAlphaComponent:0.8];label.layer.cornerRadius=12;label.clipsToBounds=YES;
    label.translatesAutoresizingMaskIntoConstraints=NO;[view addSubview:label];
    [NSLayoutConstraint activateConstraints:@[[label.centerXAnchor constraintEqualToAnchor:view.centerXAnchor],
        [label.bottomAnchor constraintEqualToAnchor:view.safeAreaLayoutGuide.bottomAnchor constant:-24],
        [label.widthAnchor constraintEqualToConstant:270],[label.heightAnchor constraintEqualToConstant:44]]];
    UIAccessibilityPostNotification(UIAccessibilityAnnouncementNotification,message);
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW,2*NSEC_PER_SEC),dispatch_get_main_queue(),^{[label removeFromSuperview];});
}
static NSArray *profileContexts(UIView *root) {
    NSMutableArray *queue=[NSMutableArray array],*result=[NSMutableArray array];
    UIViewController *vc=profileHost(root);NSMutableSet *seen=[NSMutableSet set];
    while(vc && queue.count<12){[queue addObject:vc];vc=vc.presentingViewController?:vc.parentViewController;}
    for(NSUInteger i=0;i<queue.count && i<40;i++) {
        UIViewController *candidate=queue[i];NSValue *identity=[NSValue valueWithNonretainedObject:candidate];
        if([seen containsObject:identity])continue;[seen addObject:identity];
        [queue addObjectsFromArray:candidate.childViewControllers];
        if([NSStringFromClass(candidate.class) isEqualToString:@"Profile.ProfileHomeViewController"] && candidate.isViewLoaded && candidate.view.window)[result addObject:candidate];
    }
    return result;
}
static id profileMediaOwner(UIView *root) {
    return [root.superview isKindOfClass:UICollectionViewCell.class]?root.superview:profileHost(root);
}
@interface KCProfileTransfer : NSObject <NSURLSessionDownloadDelegate>
@property(nonatomic,weak) UIView *root;
@property(nonatomic,strong) NSURL *remoteURL,*directory,*file;
@property(nonatomic,strong) NSDictionary *resource,*metadata;
@property(nonatomic,strong) NSURLSession *session;
@property(nonatomic,strong) NSURLSessionDownloadTask *task;
@property(nonatomic,strong) AVAssetExportSession *exporter;
@property(nonatomic,strong) UIAlertController *progress;
@property(nonatomic) BOOL video,finished,handedOff,committing,failed;
@property(nonatomic) UIBackgroundTaskIdentifier backgroundTask;
@property(nonatomic) NSTimeInterval lastProgress;
@property(nonatomic) double minimumVideoPixels;
- (void)start;
- (void)fail:(NSString *)message;
@end
static KCProfileTransfer *profileTransfer;
@implementation KCProfileTransfer
- (void)endBackgroundTask {
    if(self.backgroundTask!=UIBackgroundTaskInvalid){[UIApplication.sharedApplication endBackgroundTask:self.backgroundTask];self.backgroundTask=UIBackgroundTaskInvalid;}
}
- (void)clean {
    if(self.finished)return;self.finished=YES;self.handedOff=YES;
    [self.task cancel];[self.exporter cancelExport];[self.session invalidateAndCancel];self.session=nil;
    [self endBackgroundTask];
    if(self.directory)[NSFileManager.defaultManager removeItemAtURL:self.directory error:nil];
    if(profileTransfer==self)profileTransfer=nil;
}
- (void)dismissProgress:(void (^)(void))completion {
    if(self.progress.presentingViewController)[self.progress dismissViewControllerAnimated:YES completion:completion];else completion();
}
- (void)cancel {
    if(self.finished || self.committing)return;
    event(@"profile:download-cancelled");[self clean];[self dismissProgress:^{}];
}
- (void)fail:(NSString *)message {
    if(self.finished || self.failed)return;self.failed=YES;
    self.handedOff=YES;[self.session invalidateAndCancel];[self endBackgroundTask];
    event(@"profile:original-save-failed");
    [self dismissProgress:^{
        UIViewController *host=profileHost(self.root);
        if(!host || host.presentedViewController){[self clean];return;}
        UIAlertController *alert=[UIAlertController alertControllerWithTitle:@"저장하지 못했습니다" message:message preferredStyle:UIAlertControllerStyleAlert];
        [alert addAction:[UIAlertAction actionWithTitle:@"확인" style:UIAlertActionStyleCancel handler:^(UIAlertAction *a){[self clean];}]];
        [host presentViewController:alert animated:YES completion:nil];
    }];
}
- (void)start {
    self.backgroundTask=UIBackgroundTaskInvalid;
    UIViewController *host=profileHost(self.root);
    if(!host || host.presentedViewController){[self clean];return;}
    self.directory=[[NSURL fileURLWithPath:NSTemporaryDirectory() isDirectory:YES] URLByAppendingPathComponent:[@"KakaoOriginal-" stringByAppendingString:NSUUID.UUID.UUIDString] isDirectory:YES];
    NSError *error=nil;[NSFileManager.defaultManager createDirectoryAtURL:self.directory withIntermediateDirectories:YES attributes:nil error:&error];
    if(error){[self fail:@"임시 파일을 만들 수 없습니다. 저장 공간을 확인해 주세요."];return;}
    self.progress=[UIAlertController alertControllerWithTitle:self.video?@"원본 영상 다운로드":@"원본 사진 다운로드" message:@"파일을 준비하고 있습니다…" preferredStyle:UIAlertControllerStyleAlert];
    __weak KCProfileTransfer *weakSelf=self;
    [self.progress addAction:[UIAlertAction actionWithTitle:@"취소" style:UIAlertActionStyleCancel handler:^(UIAlertAction *a){[weakSelf cancel];}]];
    [host presentViewController:self.progress animated:YES completion:^{
        if(self.finished)return;
        self.backgroundTask=[UIApplication.sharedApplication beginBackgroundTaskWithName:@"프로필 원본 저장" expirationHandler:^{
            if(weakSelf.committing)[weakSelf endBackgroundTask];else [weakSelf fail:@"앱을 열어 둔 상태에서 다시 저장해 주세요."];
        }];
        if(self.remoteURL.isFileURL) {
            NSURL *local=[self.directory URLByAppendingPathComponent:@"download"];NSError *copyError=nil;
            if(![NSFileManager.defaultManager copyItemAtURL:self.remoteURL toURL:local error:&copyError]){[self fail:@"원본 파일에 접근할 수 없습니다. 미디어를 다시 열어 주세요."];return;}
            self.file=local;[self validateFile:nil];return;
        }
        NSURLSessionConfiguration *config=NSURLSessionConfiguration.ephemeralSessionConfiguration;
        config.timeoutIntervalForRequest=45;config.timeoutIntervalForResource=600;
        self.session=[NSURLSession sessionWithConfiguration:config delegate:self delegateQueue:NSOperationQueue.mainQueue];
        self.task=[self.session downloadTaskWithURL:self.remoteURL];[self.task resume];
    }];
}
- (void)URLSession:(NSURLSession *)session downloadTask:(NSURLSessionDownloadTask *)task didWriteData:(int64_t)bytesWritten totalBytesWritten:(int64_t)written totalBytesExpectedToWrite:(int64_t)expected {
    if(self.finished || self.handedOff)return;
    if(written>1024LL*1024*1024){[self fail:@"파일이 1GB를 초과하여 다운로드를 중단했습니다."];return;}
    NSTimeInterval now=NSDate.timeIntervalSinceReferenceDate;if(now-self.lastProgress<0.3)return;self.lastProgress=now;
    NSString *amount=[NSByteCountFormatter stringFromByteCount:written countStyle:NSByteCountFormatterCountStyleFile];
    self.progress.message=expected>0?[NSString stringWithFormat:@"%.0f%%  /  %@",100.0*written/expected,amount]:[amount stringByAppendingString:@" 다운로드됨"];
}
- (void)URLSession:(NSURLSession *)session task:(NSURLSessionTask *)task didCompleteWithError:(NSError *)error {
    if(error && !self.finished && !self.handedOff && error.code!=NSURLErrorCancelled)[self fail:@"원본 파일을 받지 못했습니다. 연결을 확인하고 프로필을 다시 연 뒤 시도해 주세요."];
}
- (void)URLSession:(NSURLSession *)session downloadTask:(NSURLSessionDownloadTask *)task didFinishDownloadingToURL:(NSURL *)location {
    if(self.finished || self.handedOff)return;self.handedOff=YES;
    NSHTTPURLResponse *response=(NSHTTPURLResponse *)task.response;
    if([response isKindOfClass:NSHTTPURLResponse.class] && (response.statusCode<200 || response.statusCode>=300)) {
        [self fail:[NSString stringWithFormat:@"서버가 원본 요청을 처리하지 못했습니다(HTTP %ld). 프로필을 다시 열고 시도해 주세요.",(long)response.statusCode]];return;
    }
    self.file=[self.directory URLByAppendingPathComponent:@"download"];NSError *error=nil;
    if(![NSFileManager.defaultManager moveItemAtURL:location toURL:self.file error:&error]){[self fail:@"받은 파일을 보관할 공간이 부족합니다."];return;}
    [self.session finishTasksAndInvalidate];self.session=nil;[self validateFile:response.MIMEType];
}
- (void)validateFile:(NSString *)mime {
    if(self.finished || self.failed)return;self.progress.message=@"원본 파일을 확인하고 있습니다…";
    NSURL *file=self.file;
    dispatch_async(dispatch_get_global_queue(QOS_CLASS_USER_INITIATED,0),^{
        CGImageSourceRef image=CGImageSourceCreateWithURL((__bridge CFURLRef)file,NULL);
        if(image && !self.video) {
            NSDictionary *props=CFBridgingRelease(CGImageSourceCopyPropertiesAtIndex(image,0,NULL));
            NSString *uti=(__bridge NSString *)CGImageSourceGetType(image);
            NSString *ext=(uti?[UTType typeWithIdentifier:uti].preferredFilenameExtension:nil)?:@"jpg";
            NSDictionary *metadata=@{@"kind":@"photo",@"width":props[(NSString *)kCGImagePropertyPixelWidth]?:@0,@"height":props[(NSString *)kCGImagePropertyPixelHeight]?:@0,@"frames":@(CGImageSourceGetCount(image))};
            CFRelease(image);
            dispatch_async(dispatch_get_main_queue(),^{[self fileReady:metadata extension:ext];});return;
        }
        if(image)CFRelease(image);
        NSFileHandle *handle=[NSFileHandle fileHandleForReadingFromURL:file error:nil];NSData *prefix=[handle readDataOfLength:7];[handle closeFile];
        BOOL playlist=prefix.length==7 && memcmp(prefix.bytes,"#EXTM3U",7)==0;
        dispatch_async(dispatch_get_main_queue(),^{
            if(self.finished || self.failed)return;if(playlist){[self exportStream];return;}
            NSString *ext=([mime isEqualToString:@"video/quicktime"] || [self.remoteURL.pathExtension.lowercaseString isEqualToString:@"mov"])?@"mov":@"mp4";
            // AVFoundation needs a typed local URL for reliably loading a
            // downloaded movie, especially inside the container environment.
            if(!self.file.pathExtension.length) {
                NSURL *typed=[self.directory URLByAppendingPathComponent:[@"download." stringByAppendingString:ext]];NSError *renameError=nil;
                if(![NSFileManager.defaultManager moveItemAtURL:self.file toURL:typed error:&renameError]){[self fail:@"영상 파일을 준비하지 못했습니다."];return;}
                self.file=typed;
            }
            AVURLAsset *asset=[AVURLAsset URLAssetWithURL:self.file options:nil];
            [asset loadValuesAsynchronouslyForKeys:@[@"tracks",@"duration"] completionHandler:^{
                NSError *trackError=nil,*durationError=nil;
                AVKeyValueStatus trackStatus=[asset statusOfValueForKey:@"tracks" error:&trackError];
                AVKeyValueStatus durationStatus=[asset statusOfValueForKey:@"duration" error:&durationError];
                AVAssetTrack *track=[asset tracksWithMediaType:AVMediaTypeVideo].firstObject;
                CGSize size=track?CGSizeApplyAffineTransform(track.naturalSize,track.preferredTransform):CGSizeZero;
                Float64 seconds=CMTimeGetSeconds(asset.duration);
                dispatch_async(dispatch_get_main_queue(),^{
                    if(self.finished || self.failed)return;
                    if(!track || !isfinite(seconds) || seconds<=0){
                        NSError *error=trackError?:durationError;
                        event([NSString stringWithFormat:@"profile:video-validation:%ld:%ld:%@:%ld",(long)trackStatus,(long)durationStatus,error.domain?:@"none",(long)error.code]);
                        [self fail:@"내려받은 영상 파일을 읽지 못했습니다. 사진 / 영상을 다시 열어 주세요."];return;
                    }
                    self.video=YES;
                    [self fileReady:@{@"kind":@"video",@"width":@(llround(fabs(size.width))),@"height":@(llround(fabs(size.height))),@"duration":@(seconds)} extension:ext];
                });
            }];
        });
    });
}
- (void)exportStream {
    if(self.finished || self.failed)return;
    self.progress.message=@"제공되는 최고 해상도의 영상을 확인하고 있습니다…";
    AVURLAsset *master=[AVURLAsset URLAssetWithURL:self.remoteURL options:nil];
    [master loadValuesAsynchronouslyForKeys:@[@"variants",@"tracks",@"duration",@"hasProtectedContent"] completionHandler:^{
        AVAssetVariant *best=nil;double bestArea=0,bestRate=-1;
        for(AVAssetVariant *variant in master.variants) {
            CGSize size=variant.videoAttributes.presentationSize;double area=size.width*size.height;
            if(area>bestArea || (area==bestArea && variant.peakBitRate>bestRate)){best=variant;bestArea=area;bestRate=variant.peakBitRate;}
        }
        dispatch_async(dispatch_get_main_queue(),^{
            if(self.finished || self.failed)return;
            if(master.hasProtectedContent){[self fail:@"보호된 스트리밍 영상은 원본 파일로 저장할 수 없습니다."];return;}
            self.minimumVideoPixels=bestArea;
            if(master.variants.count<=1){[self exportStreamAsset:master];return;}
            if(@available(iOS 26.0,*)) {
                if(!best.URL){[self fail:@"최고 화질 영상의 주소를 확인하지 못했습니다."];return;}
                AVURLAsset *highest=[AVURLAsset URLAssetWithURL:best.URL options:nil];
                [highest loadValuesAsynchronouslyForKeys:@[@"tracks",@"duration"] completionHandler:^{
                    NSArray *videoTracks=[highest tracksWithMediaType:AVMediaTypeVideo];
                    NSArray *audioTracks=[highest tracksWithMediaType:AVMediaTypeAudio];
                    if(!audioTracks.count)audioTracks=[master tracksWithMediaType:AVMediaTypeAudio];
                    dispatch_async(dispatch_get_main_queue(),^{
                        if(self.finished || self.failed)return;
                        if(!videoTracks.count){[self fail:@"최고 화질 영상을 읽지 못했습니다."];return;}
                        // Separate audio renditions must not be lost when choosing
                        // a particular high-resolution video playlist.
                        AVMutableComposition *composition=[AVMutableComposition composition];NSError *error=nil;
                        CMTime duration=highest.duration;
                        if(!CMTIME_IS_NUMERIC(duration) || CMTimeCompare(duration,kCMTimeZero)<=0){[self fail:@"끝나지 않은 실시간 영상은 저장할 수 없습니다."];return;}
                        for(AVAssetTrack *track in [videoTracks arrayByAddingObjectsFromArray:audioTracks]) {
                            AVMutableCompositionTrack *target=[composition addMutableTrackWithMediaType:track.mediaType preferredTrackID:kCMPersistentTrackID_Invalid];
                            if(![target insertTimeRange:CMTimeRangeMake(kCMTimeZero,duration) ofTrack:track atTime:kCMTimeZero error:&error])break;
                            target.preferredTransform=track.preferredTransform;
                        }
                        if(error){[self fail:@"최고 화질 영상과 소리를 원본 그대로 준비하지 못했습니다."];return;}
                        [self exportStreamAsset:composition];
                    });
                }];
            } else [self fail:@"이 iOS 버전에서는 스트리밍 영상의 최고 화질을 지정할 수 없습니다."];
        });
    }];
}
- (void)exportStreamAsset:(AVAsset *)asset {
    if(self.finished || self.failed)return;
    // Passthrough preserves encoded video/audio; never re-encode or fall back
    // to a lower rendition or the poster when the platform cannot export.
    self.progress.message=@"최고 화질 영상을 재압축 없이 저장하고 있습니다…";
    self.exporter=[[AVAssetExportSession alloc] initWithAsset:asset presetName:AVAssetExportPresetPassthrough];
    if(!self.exporter){[self fail:@"이 스트리밍 영상은 iOS에서 원본 파일로 내보낼 수 없습니다."];return;}
    BOOL mp4=[self.exporter.supportedFileTypes containsObject:AVFileTypeMPEG4];
    self.exporter.outputURL=[self.directory URLByAppendingPathComponent:mp4?@"stream.mp4":@"stream.mov"];self.exporter.outputFileType=mp4?AVFileTypeMPEG4:AVFileTypeQuickTimeMovie;
    [self.exporter exportAsynchronouslyWithCompletionHandler:^{dispatch_async(dispatch_get_main_queue(),^{
        if(self.finished || self.failed)return;
        if(self.exporter.status!=AVAssetExportSessionStatusCompleted){[self fail:@"이 스트리밍 영상은 원본 화질로 내보내지 못했습니다."];return;}
        self.file=self.exporter.outputURL;self.video=YES;[self validateFile:mp4?@"video/mp4":@"video/quicktime"];
    });}];
}
- (void)fileReady:(NSDictionary *)metadata extension:(NSString *)extension {
    if(self.finished || self.failed)return;
    if([metadata[@"width"] integerValue]<1 || [metadata[@"height"] integerValue]<1){[self fail:@"유효한 원본 파일이 아닙니다."];return;}
    if(self.video && self.minimumVideoPixels>0 && [metadata[@"width"] doubleValue]*[metadata[@"height"] doubleValue]<self.minimumVideoPixels){[self fail:@"최고 해상도보다 작은 결과여서 저장하지 않았습니다."];return;}
    NSDateFormatter *formatter=[NSDateFormatter new];formatter.locale=[NSLocale localeWithLocaleIdentifier:@"en_US_POSIX"];formatter.dateFormat=@"yyyyMMdd-HHmmss";
    NSString *name=[NSString stringWithFormat:@"프로필 %@-%@-%@.%@",self.video?@"영상":@"사진",[formatter stringFromDate:NSDate.date],[NSUUID.UUID.UUIDString substringToIndex:4],extension];
    NSURL *destination=[self.directory URLByAppendingPathComponent:name];NSError *error=nil;
    if(![NSFileManager.defaultManager moveItemAtURL:self.file toURL:destination error:&error]){[self fail:@"저장 파일을 준비하지 못했습니다."];return;}
    self.file=destination;NSNumber *bytes=nil;[destination getResourceValue:&bytes forKey:NSURLFileSizeKey error:nil];
    NSMutableDictionary *info=[metadata mutableCopy];info[@"bytes"]=bytes?:@0;info[@"originalField"]=self.resource[@"originalField"]?:@NO;info[@"reencoded"]=@NO;self.metadata=info;
    [self savePhotos];
}
- (void)recordSuccess:(NSString *)destination {
    NSMutableDictionary *info=[self.metadata mutableCopy];info[@"destination"]=destination;
    // Technical metadata only. Never log the media, URL, contact or account.
    [[NSJSONSerialization dataWithJSONObject:info options:NSJSONWritingPrettyPrinted error:nil] writeToFile:[NSHomeDirectory() stringByAppendingPathComponent:@"Documents/KakaoProfile-last-save.json"] atomically:YES];
    event([NSString stringWithFormat:@"profile:original-%@-saved:%@",self.video?@"video":@"photo",destination]);
}
- (void)savePhotos {
    self.progress.message=@"사진 앱에 저장하고 있습니다…";
    void (^save)(PHAuthorizationStatus)=^(PHAuthorizationStatus status){dispatch_async(dispatch_get_main_queue(),^{
        if(self.finished)return;
        if(status!=PHAuthorizationStatusAuthorized && status!=PHAuthorizationStatusLimited){[self fail:@"사진 앱에 추가할 권한이 없습니다. iOS 설정에서 사진 추가 권한을 허용한 뒤 다시 시도해 주세요."];return;}
        self.committing=YES;self.progress.actions.firstObject.enabled=NO;
        [PHPhotoLibrary.sharedPhotoLibrary performChanges:^{
            PHAssetCreationRequest *request=[PHAssetCreationRequest creationRequestForAsset];
            PHAssetResourceCreationOptions *options=[PHAssetResourceCreationOptions new];options.originalFilename=self.file.lastPathComponent;
            [request addResourceWithType:self.video?PHAssetResourceTypeVideo:PHAssetResourceTypePhoto fileURL:self.file options:options];
        } completionHandler:^(BOOL success,NSError *error){dispatch_async(dispatch_get_main_queue(),^{
            self.committing=NO;if(self.finished)return;
            if(!success){[self fail:@"사진 앱에서 저장하지 못했습니다. 사진 추가 권한과 저장 공간을 확인한 뒤 다시 시도해 주세요."];return;}
            [self recordSuccess:@"photos"];
            [self dismissProgress:^{profileToast(self.root,self.video?@"원본 영상을 사진 앱에 저장했습니다":@"원본 사진을 사진 앱에 저장했습니다");[self clean];}];
        });}];
    });};
    PHAuthorizationStatus status=[PHPhotoLibrary authorizationStatusForAccessLevel:PHAccessLevelAddOnly];
    if(status==PHAuthorizationStatusNotDetermined)[PHPhotoLibrary requestAuthorizationForAccessLevel:PHAccessLevelAddOnly handler:save];else save(status);
}
@end
static void saveProfileOriginal(UIView *root) {
    if(!root.window)return;
    if(profileTransfer && !profileTransfer.committing && (!profileTransfer.root.window || (profileTransfer.task.state!=NSURLSessionTaskStateRunning && !profileTransfer.progress.presentingViewController))) [profileTransfer cancel];
    if(profileTransfer){profileToast(root,@"진행 중인 원본 저장을 완료해 주세요");return;}
    NSDictionary *resource=[KCMediaResolver resolve:profileMediaOwner(root) contexts:profileContexts(root)];NSURL *url=resource[@"url"];
    if(!url){profileNotice(root,@"원본 주소를 찾지 못했습니다",@"사진 / 영상이 완전히 열린 뒤 다시 눌러 주세요. 원본이 없는 경우 화면용 이미지로 대신 저장하지 않습니다.");return;}
    KCProfileTransfer *transfer=[KCProfileTransfer new];transfer.root=root;transfer.resource=resource;transfer.remoteURL=url;
    transfer.video=[resource[@"video"] boolValue];profileTransfer=transfer;[transfer start];
}

static UIViewController *profileFrontController(UIWindow *window) {
    UIViewController *controller=window.rootViewController;
    for(NSUInteger depth=0;controller && depth<24;depth++) {
        if(controller.presentedViewController && !controller.presentedViewController.isBeingDismissed)controller=controller.presentedViewController;
        else if([controller isKindOfClass:UINavigationController.class])controller=((UINavigationController *)controller).visibleViewController;
        else if([controller isKindOfClass:UITabBarController.class])controller=((UITabBarController *)controller).selectedViewController;
        else break;
    }
    return controller;
}
static BOOL profileHostIsFront(UIViewController *host,UIViewController *front) {
    if(!host || !front || host.isBeingDismissed || host.isMovingFromParentViewController)return NO;
    for(UIViewController *current=host;current;current=current.parentViewController)if(current==front)return YES;
    return NO;
}

// Keep one small control in the app window, outside the moving media cells.
// Weak candidates are rescored while paging so a reused/offscreen cell cannot
// remain the target. No full-screen overlay intercepts the viewer's gestures.
@interface KCProfileSaveOverlay : NSObject
@property(nonatomic,weak) UIWindow *window;
@property(nonatomic,weak) UIView *selectedRoot;
@property(nonatomic,strong) NSHashTable<UIView *> *mediaRoots;
@property(nonatomic,strong) UIButton *button;
@property(nonatomic,strong) CADisplayLink *displayLink;
- (void)track:(UIView *)root;
- (void)refresh;
@end
@implementation KCProfileSaveOverlay
- (instancetype)init {
    if((self=[super init])) {
        _mediaRoots=[NSHashTable weakObjectsHashTable];
        [NSNotificationCenter.defaultCenter addObserver:self selector:@selector(suspend) name:UIApplicationWillResignActiveNotification object:nil];
        [NSNotificationCenter.defaultCenter addObserver:self selector:@selector(resume) name:UIApplicationDidBecomeActiveNotification object:nil];
    }
    return self;
}
- (void)dealloc { [self.displayLink invalidate];[NSNotificationCenter.defaultCenter removeObserver:self]; }
- (void)suspend { self.displayLink.paused=YES;self.button.hidden=YES; }
- (void)resume { self.displayLink.paused=NO;[self refresh]; }
- (void)track:(UIView *)root {
    [self.mediaRoots addObject:root];
    if(!self.displayLink) {
        self.displayLink=[CADisplayLink displayLinkWithTarget:self selector:@selector(tick:)];
        self.displayLink.preferredFramesPerSecond=15;
        [self.displayLink addToRunLoop:NSRunLoop.mainRunLoop forMode:NSRunLoopCommonModes];
    }
    [self refresh];
}
- (void)tick:(CADisplayLink *)link { [self refresh]; }
- (void)save {
    [self refresh];UIView *root=self.selectedRoot;
    if(root && !self.button.hidden)saveProfileOriginal(root);
}
- (void)refresh {
    UIWindow *window=self.window;
    if(!window){[self.button removeFromSuperview];[self.displayLink invalidate];self.displayLink=nil;return;}
    UIViewController *front=profileFrontController(window);
    UIView *selected=nil;CGFloat bestScore=-CGFLOAT_MAX;BOOL attached=NO;
    CGRect viewport=window.bounds;
    for(UIView *root in self.mediaRoots.allObjects) {
        if(root.window!=window)continue;attached=YES;
        if(!profileHostIsFront(profileHost(root),front))continue;
        CGRect visible=[root convertRect:root.bounds toView:window];
        BOOL hidden=NO;
        for(UIView *ancestor=root;ancestor && ancestor!=window;ancestor=ancestor.superview) {
            if(ancestor.hidden || ancestor.alpha<0.01){hidden=YES;break;}
            if(ancestor.clipsToBounds)visible=CGRectIntersection(visible,[ancestor convertRect:ancestor.bounds toView:window]);
        }
        if(hidden)continue;
        visible=CGRectIntersection(visible,viewport);
        if(CGRectIsNull(visible) || CGRectIsEmpty(visible))continue;
        CGFloat area=visible.size.width*visible.size.height;
        if(area<viewport.size.width*viewport.size.height*0.2)continue;
        CGFloat distance=fabs(CGRectGetMidY(visible)-CGRectGetMidY(viewport))+fabs(CGRectGetMidX(visible)-CGRectGetMidX(viewport));
        CGFloat score=area-distance;
        if(score>bestScore){selected=root;bestScore=score;}
    }
    self.selectedRoot=selected;
    if(!attached) {
        [self.button removeFromSuperview];[self.displayLink invalidate];self.displayLink=nil;return;
    }
    if(!selected || UIApplication.sharedApplication.applicationState!=UIApplicationStateActive || !option(@"profilePhotoSave")) {
        self.button.hidden=YES;return;
    }
    if(!self.button) {
        UIButtonConfiguration *configuration=[UIButtonConfiguration filledButtonConfiguration];
        configuration.image=[UIImage systemImageNamed:@"square.and.arrow.down"];
        configuration.baseBackgroundColor=[UIColor.blackColor colorWithAlphaComponent:0.65];configuration.baseForegroundColor=UIColor.whiteColor;
        configuration.cornerStyle=UIButtonConfigurationCornerStyleCapsule;
        self.button=[UIButton buttonWithConfiguration:configuration primaryAction:nil];
        self.button.accessibilityLabel=@"프로필 원본을 사진 앱에 저장";
        self.button.accessibilityHint=@"현재 보고 있는 사진 또는 영상을 바로 저장합니다";
        [self.button addTarget:self action:@selector(save) forControlEvents:UIControlEventTouchUpInside];
        event(@"profile:save-button-added");
    }
    if(self.button.superview!=window)[window addSubview:self.button];
    // Below the native close/sound controls; independent of content offset.
    self.button.frame=CGRectMake(CGRectGetMaxX(viewport)-window.safeAreaInsets.right-60,CGRectGetMinY(viewport)+window.safeAreaInsets.top+60,44,44);
    self.button.hidden=NO;self.button.alpha=1;
    if(window.subviews.lastObject!=self.button)[window bringSubviewToFront:self.button];
}
@end
static void placeProfileSaveButton(UIView *root,BOOL cell) {
    if(!option(@"profilePhotoSave") || !root.window)return;
    static NSMapTable<UIWindow *,KCProfileSaveOverlay *> *overlays;
    if(!overlays)overlays=[NSMapTable weakToStrongObjectsMapTable];
    KCProfileSaveOverlay *overlay=[overlays objectForKey:root.window];
    if(!overlay){overlay=[KCProfileSaveOverlay new];overlay.window=root.window;[overlays setObject:overlay forKey:root.window];}
    [overlay track:root];
}
static void installProfileDownload(void) {
    if(!option(@"profilePhotoSave"))return;
    for(NSString *name in @[@"Profile.ProfileHomeMediaDetailViewController",@"KakaoTalk.ProfilePictureViewController",@"KakaoTalk.ProfileImageViewController"]) {
        Class cls=NSClassFromString(name);if(!cls || ![cls isSubclassOfClass:UIViewController.class])continue;
        for(NSString *methodName in @[@"viewDidAppear:",@"viewDidLayoutSubviews"]) {
            SEL sel=NSSelectorFromString(methodName);Method method=class_getInstanceMethod(cls,sel);if(!method)continue;IMP original=method_getImplementation(method);
            if([methodName hasSuffix:@":"])replace(cls,sel,imp_implementationWithBlock(^(UIViewController *vc,BOOL animated){((void (*)(id,SEL,BOOL))original)(vc,sel,animated);placeProfileSaveButton(vc.view,NO);}),method_getTypeEncoding(method));
            else replace(cls,sel,imp_implementationWithBlock(^(UIViewController *vc){((void (*)(id,SEL))original)(vc,sel);placeProfileSaveButton(vc.view,NO);}),method_getTypeEncoding(method));
        }
    }
    for(NSString *name in @[@"Profile.ProfilePostDetailImageCell",@"Profile.ProfilePostDetailVideoCell"]) {
        Class cell=NSClassFromString(name);if(!cell || ![cell isSubclassOfClass:UICollectionViewCell.class])continue;
        for(NSString *methodName in @[@"layoutSubviews",@"didMoveToWindow"]) {
            SEL sel=NSSelectorFromString(methodName);Method method=class_getInstanceMethod(cell,sel);if(!method)continue;IMP original=method_getImplementation(method);
            replace(cell,sel,imp_implementationWithBlock(^(UICollectionViewCell *view){((void (*)(id,SEL))original)(view,sel);placeProfileSaveButton(view.contentView,YES);}),method_getTypeEncoding(method));
        }
    }
}

#include "CustomizationFeatures.inc"
static NSDictionary *ginppaiReport(void) {return @{@"upstream":@"v1.5.0-dev.9",@"commit":@"70f43985acdb0f9fe49d5cda0b88676583aca2d1",@"capabilities":ginppaiCapabilities?:@{},@"bubbleLayout":ginppaiLayoutDiagnostics?:@{},@"unreadCalculation":ginppaiUnreadProbe?:@{},@"universalCover":ginppaiCoverProbe?:@{}};}

__attribute__((constructor)) static void initializeKakaoAdBlock(void) {
    @autoreleasepool {
        NSBundle *bundle = NSBundle.mainBundle;
        NSString *bundleID=bundle.bundleIdentifier;
        if (!([bundleID isEqualToString:@"com.iwilab.KakaoTalk"] || [bundleID hasPrefix:@"com.iwilab.KakaoTalk."]) ||
            ![[bundle objectForInfoDictionaryKey:@"CFBundleShortVersionString"] isEqualToString:@"26.7.3"]) return;
        loadOptions();
        counts = [NSMutableDictionary dictionary];
        installed = [NSMutableArray array];
        viewSamples = [NSMutableArray array];
        if(option(@"hideAds")) {
        blockLoad(@"AdFitPM.AdFitNativeAdLoader", @"loadAdWithKeyword:regionId:duplicateKey:", @"nativeAdLoaderDidFailToReceiveAd:error:");
        blockLoad(@"AdFitPM.FriendFeedAdLoader", @"loadAdWithPosition:", @"friendFeedAdLoaderDidFailToReceiveAd:error:");
        blockLoad(@"AdFitPM.BizBoardAdView", @"loadAd", @"bizBoardAdViewDidFailToReceiveAd:");
        blockLoad(@"AdFitPM.AdFitGlobalBannerAdView", @"loadAd", nil);
        blockLoad(@"AdFitPM.BizBoardProvider", @"firstLoadWithDuplicateKey:", nil);
        blockLoad(@"TalkAppBase.BizboardManager", @"firstLoad", nil);
        for (NSString *name in @[
            @"MoreTab.NativeADUIView", @"MoreTab.LocalBizboardUIView", @"TalkAppBase.BizboardCell",
            @"TalkAppBase.BizboardCollectionCell", @"TalkAppBase.BizboardCollectionCellWithoutSoundManager",
            @"TalkAppBase.ChatsGlobalBizboardCell", @"TalkAppBase.GlobalBizboardCollectionCell",
            @"AdFitPM.AdFitAdView", @"AdFitPM.BizBoardAdView", @"AdFitPM.AdFitGlobalBannerAdView",
            @"AdFitPM.BizBoardTemplate", @"AdFitPM.BizBoardCell", @"AdFitPM.MultiNativeAdView"
        ]) hideClass(name);
        }
        installTabCustomization();
        installNowReselectionFix();
        installInitialOpenChatPreference();
        installSettingsEntry();
        installCustomizationFeatures();
        installProfileDownload();
        installNavigationConvenience();
        dispatch_async(dispatch_get_main_queue(), ^{ saveReport(); });
        NSLog(@"[Ginppai-Kakao-Customizer] 3.0.0-dev.2 loaded for KakaoTalk 26.7.3 (%lu hooks)", (unsigned long)installed.count);
    }
}
