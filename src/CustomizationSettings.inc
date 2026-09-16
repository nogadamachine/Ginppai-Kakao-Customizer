// Ginppai settings. Stable preference keys preserve existing installations.
static NSArray<NSDictionary *> *customizationGroups(void) {
    return @[
      @{@"title":@"화면",@"icon":@"rectangle.3.group",@"summary":@"광고와 하단 탭 정리",@"rows":@[
        @[@"hideAds",@"광고 자리 비우기",@"지원되는 광고를 차단하고 친구 목록의 특별한 친구 프로모션 영역을 숨깁니다."],
        @[@"hideShortForm",@"숏폼 없이 사용",@"숏폼을 숨기고 오픈채팅 목록을 바로 엽니다."],
        @[@"preferOpenChat",@"오픈채팅 먼저 보기",@"지금 탭을 열 때 오픈채팅 목록부터 표시합니다."],
        @[@"hideShopping",@"쇼핑 탭 숨기기",@"하단 메뉴에서 쇼핑을 뺍니다."],
        @[@"hideCallTab",@"통화 탭 숨기기",@"해외 화면의 하단 통화 메뉴를 뺍니다."],
        @[@"hideFriendFeed",@"친구 목록만 보기",@"친구 탭에서 소식을 빼고, 다시 열 때도 친구 목록을 유지합니다."],
        @[@"hideTabBadges",@"하단 숫자와 점 숨기기",@"하단 탭의 알림 표시만 정리합니다."]]},
      @{@"title":@"더보기",@"icon":@"square.grid.2x2",@"summary":@"필요한 서비스와 정보만 표시",@"rows":@[
        @[@"hideMoreGame",@"게임 탭 숨기기",@"더보기 상단에서 게임 항목을 뺍니다."],
        @[@"hideMorePay",@"페이 영역 숨기기",@"더보기 안의 페이 영역을 숨깁니다. 결제나 계정 설정은 바뀌지 않습니다."],
        @[@"hideMoreNow",@"카카오나우 숨기기",@"추천 콘텐츠 카드 영역을 숨깁니다."],
        @[@"hideMoreWeather",@"날씨 숨기기",@"날씨와 지역 정보 카드를 숨깁니다."],
        @[@"hideMoreServices",@"서비스 모음 숨기기",@"아이콘 메뉴와 세로형 서비스 목록을 함께 숨깁니다."],
        @[@"hideMoreLinks",@"세로형 서비스 목록 숨기기",@"목록 형태로 배치된 서비스만 숨깁니다. 아이콘 메뉴는 유지합니다."]]},
      @{@"title":@"대화",@"icon":@"bubble.left.and.bubble.right",@"summary":@"입력 표시, 읽지 않은 수와 메시지 전송",@"rows":@[
        @[@"hideTyping",@"입력 중 표시 감추기",@"글을 쓰는 중이라는 표시를 보내지 않습니다. 읽음 표시는 유지됩니다."],
        @[@"uncapChatUnread",@"채팅 목록의 숫자 끝까지 표시",@"300+ 대신 기기가 가진 실제 안 읽은 메시지 수를 사용합니다."],
        @[@"uncapMessageUnread",@"안 읽은 사람 수 끝까지 표시",@"메시지 옆 사람 수를 99명에서 잘라내지 않습니다."],
        @[@"showMobileMessages",@"보조 기기에서도 메시지 보기",@"모바일 전용 안내로 가려진 메시지 표시를 확장합니다. 잠금과 연령 조건은 유지됩니다."],
        @[@"sendMarkdown",@"텍스트 서식 보내기",@"**굵게** 같은 마크다운 서식을 붙여 보냅니다. 현재 메시지를 수정하면 서식이 풀릴 수 있습니다."],
        @[@"forwardLeverage",@"공유 카드 전달 범위 넓히기",@"전달 가능한 링크 카드의 범위를 확장합니다. 메시지 종류에 따라 동작이 다를 수 있습니다."]]},
      @{@"title":@"사진과 영상",@"icon":@"photo.on.rectangle",@"summary":@"프로필 원본 저장과 촬영 정보 정리",@"rows":@[
        @[@"profilePhotoSave",@"프로필에 저장 버튼 표시",@"현재와 이전 프로필, 배경의 원본 사진과 영상을 사진 앱에 저장합니다."],
        @[@"stripPhotoMetadata",@"보낼 사진의 촬영 정보 지우기",@"JPEG, HEIC, PNG의 위치와 촬영 정보를 정리합니다. 사진 앱 원본은 유지하며, 처리할 수 없는 형식은 원본 그대로 보냅니다."]]},
      @{@"title":@"기록",@"icon":@"clock.arrow.circlepath",@"summary":@"메시지 정보와 기기에 보관한 변경 내역",@"rows":@[
        @[@"showDeletedMessages",@"삭제된 메시지 원문 표시",@"삭제 전에 기기에 보관된 텍스트를 말풍선에 표시하고 시간 옆에 삭제됨을 붙입니다. 켜기 전에 지워져 원문이 없는 메시지는 복원하지 못합니다."],
        @[@"messageDetails",@"메시지 정보 보기",@"메시지를 길게 누른 뒤 메시지 도구에서 내용과 보낸 시각을 확인합니다."],
        @[@"readReceipts",@"읽은 사람 목록 보기",@"기기에 있는 읽음 위치를 기준으로 표시합니다. 자료가 없으면 확인 불가로 남깁니다."],
        @[@"messageHistory",@"확인한 메시지 기록 남기기",@"원문과 변경 내역을 이 기기에 보관합니다. 최대 5,000개 메시지, 메시지당 20개 기록을 유지합니다."]]},
      @{@"title":@"개인정보",@"icon":@"hand.raised",@"summary":@"오류 보고와 사용 분석 전송 설정",@"rows":@[
        @[@"disableSentry",@"오류 보고 보내지 않기",@"지원되는 앱 내부 오류 수집기의 시작을 막습니다."],
        @[@"blockSDKTracking",@"사용 분석 요청 줄이기",@"확인된 광고 및 사용 분석 전송 주소를 차단합니다. 모든 분석 요청을 막는 기능은 아닙니다."],
        @[@"blockTalkShareLog",@"공유 분석 기록 보내지 않기",@"공유 기록을 수집하는 전용 주소를 차단합니다. 메시지 공유와 링크 열기는 유지합니다."]]},
      @{@"title":@"사용 방식",@"icon":@"slider.horizontal.3",@"summary":@"시작 화면, 국가와 빠른 동작",@"rows":@[
        @[@"startupTab",@"처음 열 화면",@"카카오톡을 새로 실행할 때 사용할 탭을 고릅니다."],
        @[@"countryISO",@"화면 국가와 지역",@"화면 구성만 선택합니다. 계정과 전화번호의 국가는 바뀌지 않습니다."],
        @[@"externalBrowser",@"링크를 기본 브라우저에서 열기",@"채팅의 일반 웹 링크를 iPhone에 설정된 브라우저로 엽니다."],
        @[@"scrollTopOnRetap",@"오픈채팅 다시 누르면 맨 위로",@"하단 오픈채팅 탭을 한 번 더 눌러 목록 처음으로 돌아갑니다."],
        @[@"quickSettings",@"더보기에서 설정 바로 열기",@"더보기 탭을 두 번 누르거나 길게 눌러 Ginppai 설정을 엽니다."]]}
    ];
}

@interface KCPreferencesController : UITableViewController
- (void)refresh;
@end
@interface KCSettingsController : KCPreferencesController
@end
@interface KCOptionsController : KCPreferencesController
@property(nonatomic,strong) NSDictionary *group;
@end
@interface KCCountryController : KCPreferencesController <UISearchResultsUpdating>
@property(nonatomic,strong) NSArray<NSDictionary *> *countries;
@property(nonatomic,strong) NSArray<NSDictionary *> *filtered;
@property(nonatomic,strong) UISearchController *countrySearch;
@end
@interface KCStartupController : KCPreferencesController
@end

static UITableViewCell *customizationCell(void) {
    UITableViewCell *cell=[[UITableViewCell alloc] initWithStyle:UITableViewCellStyleSubtitle reuseIdentifier:nil];
    cell.textLabel.font=[UIFont preferredFontForTextStyle:UIFontTextStyleBody];
    cell.detailTextLabel.font=[UIFont preferredFontForTextStyle:UIFontTextStyleFootnote];
    cell.textLabel.adjustsFontForContentSizeCategory=YES;cell.detailTextLabel.adjustsFontForContentSizeCategory=YES;
    cell.textLabel.numberOfLines=0;cell.detailTextLabel.numberOfLines=0;
    cell.detailTextLabel.textColor=UIColor.secondaryLabelColor;
    return cell;
}
static void openCustomizationURL(NSString *value,UIViewController *host) {
    NSURL *url=[NSURL URLWithString:value];
    [UIApplication.sharedApplication openURL:url options:@{} completionHandler:^(BOOL success){
        if(success)return;
        dispatch_async(dispatch_get_main_queue(),^{
            UIAlertController *alert=[UIAlertController alertControllerWithTitle:@"링크를 열지 못했습니다" message:@"연결 상태를 확인하고 다시 눌러 주세요." preferredStyle:UIAlertControllerStyleAlert];
            [alert addAction:[UIAlertAction actionWithTitle:@"확인" style:UIAlertActionStyleDefault handler:nil]];
            [host presentViewController:alert animated:YES completion:nil];
        });
    }];
}
@implementation KCPreferencesController
- (instancetype)init {return [super initWithStyle:UITableViewStyleInsetGrouped];}
- (void)viewDidLoad {
    [super viewDidLoad];self.tableView.rowHeight=UITableViewAutomaticDimension;self.tableView.estimatedRowHeight=78;
    self.navigationItem.backButtonTitle=@"뒤로";
}
- (void)viewWillAppear:(BOOL)animated {[super viewWillAppear:animated];[self refresh];}
- (void)refresh {
    self.navigationItem.rightBarButtonItem=[[UIBarButtonItem alloc] initWithTitle:@"적용 후 종료" style:UIBarButtonItemStyleDone target:self action:@selector(apply)];
    self.navigationItem.rightBarButtonItem.enabled=hasChanges();[self.tableView reloadData];
}
- (void)apply {
    if(!saveOptions()){showSaveFailure(self);return;}
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW,300*NSEC_PER_MSEC),dispatch_get_main_queue(),^{exit(0);});
}
@end

@implementation KCSettingsController
- (void)viewDidLoad {
    [super viewDidLoad];self.title=@"Ginppai";
    self.navigationItem.leftBarButtonItem=[[UIBarButtonItem alloc] initWithTitle:@"닫기" style:UIBarButtonItemStylePlain target:self action:@selector(close)];
}
- (void)close {[self dismissViewControllerAnimated:YES completion:nil];}
- (NSInteger)numberOfSectionsInTableView:(UITableView *)table {return 4;}
- (NSInteger)tableView:(UITableView *)table numberOfRowsInSection:(NSInteger)section {return section==0?customizationGroups().count:(section==1?1:2);}
- (NSString *)tableView:(UITableView *)table titleForHeaderInSection:(NSInteger)section {return @[@"내 카카오톡 맞춤 설정",@"내 기기의 기록",@"만든 사람과 저장소",@"설정 관리"][section];}
- (NSString *)tableView:(UITableView *)table titleForFooterInSection:(NSInteger)section {
    if(section==0)return @"필요한 기능만 골라 사용하세요. 변경값은 자동 저장되고, 적용 후 카카오톡을 다시 실행하면 반영됩니다.";
    if(section==1)return @"기능을 켠 뒤 기기에서 확인한 내용만 남습니다. 받기 전에 지워진 메시지는 복원할 수 없습니다.";
    if(section==3)return @"Ginppai-Kakao-Customizer 3.0.0-dev.5\nKakaoTalk 26.7.3\nMade by nogadamachine\n개발판에는 아직 기기 검증 중인 기능이 포함되어 있습니다.";
    return nil;
}
- (UITableViewCell *)tableView:(UITableView *)table cellForRowAtIndexPath:(NSIndexPath *)index {
    UITableViewCell *cell=customizationCell();cell.accessoryType=UITableViewCellAccessoryDisclosureIndicator;
    if(index.section==0) {
        NSDictionary *group=customizationGroups()[index.row];cell.textLabel.text=group[@"title"];cell.detailTextLabel.text=group[@"summary"];
        cell.imageView.image=[UIImage systemImageNamed:group[@"icon"]];cell.imageView.tintColor=UIColor.labelColor;
    } else if(index.section==1) {
        cell.textLabel.text=@"메시지 보관함";cell.detailTextLabel.text=@"기록한 원문과 변경 내역 찾아보기";cell.imageView.image=[UIImage systemImageNamed:@"archivebox"];
    } else if(index.section==2) {
        cell.textLabel.text=index.row==0?@"@nogadamachine":@"긴빠이 Repo";
        cell.detailTextLabel.text=index.row==0?@"개발자 인스타그램":@"트윅 다운로드와 설치 안내";
        cell.imageView.image=[UIImage systemImageNamed:index.row==0?@"person.crop.circle":@"shippingbox"];
    } else if(index.row==0) {
        cell.textLabel.text=hasChanges()?@"저장됨, 다음 실행부터 적용":@"저장한 설정 사용 중";
        cell.detailTextLabel.text=[NSString stringWithFormat:@"현재 화면: %@",countryName(activeOptions[@"countryISO"])];
        cell.accessoryType=UITableViewCellAccessoryNone;cell.selectionStyle=UITableViewCellSelectionStyleNone;
    } else {cell.textLabel.text=@"설정 초기화";cell.detailTextLabel.text=@"Ginppai 옵션을 기본값으로 되돌리기";cell.textLabel.textColor=UIColor.systemRedColor;}
    return cell;
}
- (void)tableView:(UITableView *)table didSelectRowAtIndexPath:(NSIndexPath *)index {
    [table deselectRowAtIndexPath:index animated:YES];
    if(index.section==0) {KCOptionsController *page=[KCOptionsController new];page.group=customizationGroups()[index.row];[self.navigationController pushViewController:page animated:YES];}
    else if(index.section==1)[self.navigationController pushViewController:[NSClassFromString(@"KCMessageHistoryListController") new] animated:YES];
    else if(index.section==2)openCustomizationURL(index.row==0?@"https://www.instagram.com/nogadamachine/":@"https://nogadamachine.github.io/Ginppai-Repo/",self);
    else if(index.section==3 && index.row==1) {
        UIAlertController *alert=[UIAlertController alertControllerWithTitle:@"설정을 초기화할까요?" message:@"Ginppai 옵션만 기본값으로 바뀝니다. 메시지 보관함과 카카오톡 데이터는 유지됩니다." preferredStyle:UIAlertControllerStyleAlert];
        [alert addAction:[UIAlertAction actionWithTitle:@"취소" style:UIAlertActionStyleCancel handler:nil]];
        [alert addAction:[UIAlertAction actionWithTitle:@"초기화" style:UIAlertActionStyleDestructive handler:^(UIAlertAction *action){
            NSDictionary *old=[savedOptions copy];[savedOptions setDictionary:defaultOptions()];
            if(!saveOptions()){[savedOptions setDictionary:old];showSaveFailure(self);}[self refresh];
        }]];[self presentViewController:alert animated:YES completion:nil];
    }
}
@end

@implementation KCOptionsController
- (void)viewDidLoad {[super viewDidLoad];self.title=self.group[@"title"];}
- (NSInteger)tableView:(UITableView *)table numberOfRowsInSection:(NSInteger)section {return [self.group[@"rows"] count];}
- (NSString *)tableView:(UITableView *)table titleForFooterInSection:(NSInteger)section {return @"변경 후 위쪽의 '적용 후 종료'를 누르고 카카오톡을 다시 실행해 주세요. 현재 화면이나 설치 환경에서 사용할 수 없는 옵션은 회색으로 표시됩니다.";}
- (UITableViewCell *)tableView:(UITableView *)table cellForRowAtIndexPath:(NSIndexPath *)index {
    UITableViewCell *cell=customizationCell();NSArray *row=self.group[@"rows"][index.row];NSString *key=row[0];
    cell.textLabel.text=row[1];cell.detailTextLabel.text=row[2];
    if([key isEqualToString:@"startupTab"] || [key isEqualToString:@"countryISO"]) {
        cell.detailTextLabel.text=[key isEqualToString:@"startupTab"]?startupName(savedOptions[key]):countryName(savedOptions[key]);
        cell.accessoryType=UITableViewCellAccessoryDisclosureIndicator;
        if([key isEqualToString:@"countryISO"] && !countrySupport){cell.textLabel.textColor=UIColor.secondaryLabelColor;cell.detailTextLabel.text=@"현재 설치에서는 국가 화면을 바꿀 수 없습니다.";}
        return cell;
    }
    UISwitch *toggle=[UISwitch new];toggle.on=[savedOptions[key] boolValue];toggle.tag=index.row;toggle.accessibilityLabel=row[1];
    NSString *unavailable=nil;
    if(overseas(savedOptions) && [@[@"hideShortForm",@"preferOpenChat",@"hideShopping",@"scrollTopOnRetap"] containsObject:key])unavailable=@"해외 화면에는 이 탭이 없습니다. 한국 화면으로 돌아오면 저장한 값을 다시 사용합니다.";
    if(!overseas(savedOptions) && [key isEqualToString:@"hideCallTab"])unavailable=@"별도 통화 탭이 있는 해외 화면에서 사용할 수 있습니다.";
    if([key isEqualToString:@"preferOpenChat"] && [savedOptions[@"hideShortForm"] boolValue] && !overseas(savedOptions)){unavailable=@"숏폼 없이 사용 중에는 오픈채팅이 항상 먼저 열립니다.";toggle.on=YES;}
    if([@[@"uncapMessageUnread",@"sendMarkdown",@"forwardLeverage",@"showMobileMessages",@"hideMoreGame",@"hideFriendFeed",@"hideMorePay",@"hideMoreNow",@"hideMoreWeather",@"hideMoreServices",@"hideMoreLinks",@"blockTalkShareLog"] containsObject:key] && ![ginppaiCapabilities[key] boolValue])unavailable=@"현재 설치에서는 이 기능을 사용할 수 없습니다. 설치 안내에서 지원 조건을 확인해 주세요.";
    if(overseas(savedOptions) && [@[@"hideMorePay",@"hideMoreNow",@"hideMoreWeather",@"hideMoreServices",@"hideMoreLinks"] containsObject:key])unavailable=@"한국 더보기 화면에서 적용됩니다. 해외 화면에서는 저장한 값을 유지합니다.";
    if([key isEqualToString:@"hideMoreLinks"] && [savedOptions[@"hideMoreServices"] boolValue]){unavailable=@"서비스 모음 숨기기에 포함되어 있습니다.";toggle.on=YES;}
    if(unavailable){toggle.enabled=NO;cell.detailTextLabel.text=unavailable;cell.textLabel.textColor=UIColor.secondaryLabelColor;}
    [toggle addTarget:self action:@selector(changed:) forControlEvents:UIControlEventValueChanged];cell.accessoryView=toggle;cell.selectionStyle=UITableViewCellSelectionStyleNone;
    return cell;
}
- (void)changed:(UISwitch *)sender {
    NSString *key=self.group[@"rows"][sender.tag][0];
    if(!storeOption(key,@(sender.on),self))sender.on=[savedOptions[key] boolValue];[self refresh];
}
- (void)tableView:(UITableView *)table didSelectRowAtIndexPath:(NSIndexPath *)index {
    [table deselectRowAtIndexPath:index animated:YES];NSString *key=self.group[@"rows"][index.row][0];
    if([key isEqualToString:@"startupTab"])[self.navigationController pushViewController:[KCStartupController new] animated:YES];
    else if([key isEqualToString:@"countryISO"] && countrySupport)[self.navigationController pushViewController:[KCCountryController new] animated:YES];
}
@end
