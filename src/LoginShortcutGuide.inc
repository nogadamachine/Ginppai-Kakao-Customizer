// The shared shortcut is installed by the user in Apple's Shortcuts app.
// This screen only opens/copies its fixed distribution URL; it never reads login URLs.
static NSString *kcLoginShortcutURL(void) {
    return @"https://www.icloud.com/shortcuts/800cb2bb124246459f6d2e82d0566074";
}
static NSArray<NSDictionary *> *kcLoginShortcutGuide(void) {
    return @[
      @{@"title":@"단축어 추가",@"rows":@[
        @[@"카카오톡 로그인 연결 단축어 추가",@"단축어 추가 화면을 엽니다. 내용을 확인한 뒤 ‘단축어 추가’를 눌러 주세요. 이 버튼만으로 자동 설치되지는 않습니다.",@"install"],
        @[@"추가 링크 복사",@"추가 화면이 열리지 않으면 링크를 복사해 Safari 주소창에 붙여 넣으세요. 다른 사용자에게 이 배포 링크를 공유할 수도 있습니다.",@"copy"]]},
      @{@"title":@"왜 필요한가요?",@"rows":@[
        @[@"설치돼 있는데 미설치 안내가 뜰 때",@"외부 앱의 ‘카카오톡으로 로그인’을 눌렀는데 브라우저가 열리고 카카오톡이 없다는 안내가 나오는 경우에 사용합니다. 직접 서명해 설치한 환경에서는 웹 주소와 앱을 연결하는 정보가 달라져 이런 현상이 생길 수 있습니다."],
        @[@"로그인 주소를 앱 연결 주소로 변환",@"이 단축어는 브라우저가 받은 카카오 로그인 주소를 카카오톡이 직접 열 수 있는 주소로 변환합니다. 현재 요청의 인증 정보를 유지해 카카오톡 로그인 화면으로 연결합니다. 이미 로그인이 잘 된다면 사용할 필요가 없습니다."]]},
      @{@"title":@"처음 한 번 적용하기",@"rows":@[
        @[@"1. 단축어 추가",@"위의 추가 버튼을 누르고 단축어 앱에서 ‘카카오톡 로그인 연결’을 추가하세요. 단축어 앱이 없다면 먼저 App Store에서 설치하세요. Ginppai의 ‘적용 후 종료’나 카카오톡 재실행은 필요하지 않습니다."],
        @[@"2. 공유 메뉴에서 찾기",@"Safari나 사용 중인 브라우저의 공유 메뉴에서 ‘카카오톡 로그인 연결’을 찾으세요. 보이지 않으면 단축어 앱에서 해당 단축어의 세부사항을 열어 ‘공유 시트에서 보기’를 켜세요. 입력 유형에 Safari 웹페이지, URL, 텍스트가 포함되어 있는지도 확인하세요."]]},
      @{@"title":@"로그인할 때 사용하는 방법",@"rows":@[
        @[@"1. 원래 앱에서 로그인 시작",@"로그인하려는 앱의 ‘카카오톡으로 로그인’을 누르세요. 설치되지 않았다는 안내가 나오는 브라우저 화면을 닫거나 원래 앱의 로그인을 취소하지 마세요."],
        @[@"2. Safari에서는 페이지 공유",@"주소창 옆이나 브라우저 메뉴의 공유 버튼을 누르고 ‘카카오톡 로그인 연결’을 선택하세요. 단축어가 현재 페이지의 URL을 가져오므로 주소를 따로 복사할 필요가 없습니다."],
        @[@"다른 브라우저나 앱에서는 링크 공유",@"현재 페이지 또는 로그인 링크의 공유 메뉴에서 ‘카카오톡 로그인 연결’을 선택하세요. 일반 URL이나 링크가 포함된 텍스트를 받으면 그 안의 주소를 추출합니다. 로그인 링크 하나만 공유하세요. 복사만 해서는 실행되지 않으며, 현재 배포본은 클립보드를 자동으로 읽지 않습니다."],
        @[@"3. 카카오톡에서 로그인 완료",@"처음 실행할 때 앱 열기나 웹페이지 접근에 대한 허용을 요청하면 내용을 확인하고 허용하세요. 카카오톡의 로그인·동의 화면을 완료한 뒤 원래 앱으로 돌아가 로그인됐는지 확인하세요. 로그인할 때마다 새로 열린 페이지나 링크에서 실행하세요."]]},
      @{@"title":@"잘 안될 때",@"rows":@[
        @[@"공유 메뉴에 단축어가 없어요",@"공유 메뉴 아래쪽까지 내려 확인하세요. 그래도 없으면 단축어의 ‘공유 시트에서 보기’와 입력 유형을 확인하세요. 일부 앱은 링크를 공유하지 않을 수 있으니 로그인 페이지를 Safari에서 열어 공유해 보세요."],
        @[@"잘못된 요청·SDK 오류가 떠요",@"원래 앱에서 로그인을 다시 시작한 뒤 새 로그인 링크를 공유하세요. 단축어를 편집했다면 ‘접두사 제거 → URL 디코딩 한 번 → URL → URL 열기’ 순서와 변수 연결을 확인하세요. SDK 버전이나 인증 파라미터를 임의로 바꾸지 마세요."],
        @[@"카카오톡은 열리지만 로그인이 안 끝나요",@"원래 앱에서 요청이 취소됐거나 만료됐을 수 있습니다. 원래 앱에서 다시 로그인을 시작해 주세요. 완료 여부는 해당 앱의 로그인 처리에도 영향을 받습니다."]]},
      @{@"title":@"지원 범위와 기록",@"rows":@[
        @[@"공유 메뉴에서 직접 실행",@"이 기능은 외부 앱의 로그인 버튼을 자동으로 가로채거나 iOS의 웹 연결 설정을 복구하지 않습니다. 브라우저가 열렸을 때 공유 메뉴에서 단축어를 실행해야 합니다. 브라우저와 앱의 공유 방식에 따라 동작이 달라질 수 있습니다."],
        @[@"로그인 주소를 저장하지 않아요",@"Ginppai의 이 안내 화면은 고정된 단축어 배포 링크만 열거나 복사합니다. 개인 로그인 주소나 클립보드를 읽지 않습니다. 현재 공유된 단축어는 받은 주소를 기기에서 변환해 카카오톡으로 전달합니다."]]}
    ];
}

@interface KCLoginShortcutController : UITableViewController
@end
@implementation KCLoginShortcutController
- (instancetype)init {return [super initWithStyle:UITableViewStyleInsetGrouped];}
- (void)viewDidLoad {
    [super viewDidLoad];self.title=@"카카오 로그인 연결";
    self.navigationItem.backButtonTitle=@"뒤로";
    self.tableView.rowHeight=UITableViewAutomaticDimension;
    self.tableView.estimatedRowHeight=140;
}
- (NSInteger)numberOfSectionsInTableView:(UITableView *)table {return kcLoginShortcutGuide().count;}
- (NSInteger)tableView:(UITableView *)table numberOfRowsInSection:(NSInteger)section {return [kcLoginShortcutGuide()[section][@"rows"] count];}
- (NSString *)tableView:(UITableView *)table titleForHeaderInSection:(NSInteger)section {return kcLoginShortcutGuide()[section][@"title"];}
- (UITableViewCell *)tableView:(UITableView *)table cellForRowAtIndexPath:(NSIndexPath *)index {
    NSArray *row=kcLoginShortcutGuide()[index.section][@"rows"][index.row];
    UITableViewCell *cell=customizationCell();cell.textLabel.text=row[0];cell.detailTextLabel.text=row[1];
    if(row.count>2) {
        cell.textLabel.textColor=UIColor.systemBlueColor;
        cell.imageView.image=[UIImage systemImageNamed:[row[2] isEqualToString:@"install"]?@"arrow.down.app":@"doc.on.doc"];
        cell.imageView.tintColor=UIColor.systemBlueColor;
        cell.accessoryType=UITableViewCellAccessoryDisclosureIndicator;
    } else cell.selectionStyle=UITableViewCellSelectionStyleNone;
    return cell;
}
- (void)tableView:(UITableView *)table didSelectRowAtIndexPath:(NSIndexPath *)index {
    [table deselectRowAtIndexPath:index animated:YES];
    NSArray *row=kcLoginShortcutGuide()[index.section][@"rows"][index.row];
    if(row.count<3)return;
    if([row[2] isEqualToString:@"install"])openCustomizationURL(kcLoginShortcutURL(),self);
    else if([row[2] isEqualToString:@"copy"]) {
        UIPasteboard.generalPasteboard.string=kcLoginShortcutURL();
        UIAlertController *alert=[UIAlertController alertControllerWithTitle:@"추가 링크를 복사했습니다" message:@"Safari 주소창에 붙여 넣어 단축어 추가 화면을 열 수 있습니다." preferredStyle:UIAlertControllerStyleAlert];
        [alert addAction:[UIAlertAction actionWithTitle:@"확인" style:UIAlertActionStyleDefault handler:nil]];
        [self presentViewController:alert animated:YES completion:nil];
    }
}
@end
