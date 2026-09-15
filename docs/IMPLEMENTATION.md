# 구현 대조 기록

참조 버전 v1.5.0-dev.9, 커밋 `70f43985acdb0f9fe49d5cda0b88676583aca2d1`. 출처와 기여자 고지는 [NOTICE.md](../NOTICE.md)에 있습니다. 원래 Android 대상은 26.7.2이며 iOS 대상은 26.7.3입니다. 아래 영문 이름과 번호는 소스 대조용 식별자로, 설정 화면의 메뉴 이름과 다릅니다.

전체 72개 항목의 대응이 끝난 상태가 아닙니다. 구현 코드, 계산 시험, 실제 기기 결과는 구분해서 기록합니다. Android 전용 항목은 iOS에서 동일하게 구현했다는 뜻이 아닙니다.

| 번호 | 참조 항목 | 상태 | iOS 대응과 남은 작업 |
|---|---|---|---|
| 1 | Add Packet Handler | 미구현 | 외부 앱에서 LOCO 패킷을 처리하는 선택 기능. iOS IPC와 권한 모델에 맞는 별도 설계가 필요하며 현재 외부 패킷 API는 없다. |
| 2 | Add Pine Lib | Android 전용 방식 | Android ART용 libpine.so를 넣는 구성 요소. iOS는 Objective-C/Swift 연결 방식과 탈옥 런타임을 사용한다. |
| 3 | Add settings resources | 기존 기능 | Android XML 대신 기존 UIKit 설정 화면을 사용한다. 기존 옵션 화면의 기기 표시와 저장 및 재시작 후 적용을 확인했다. |
| 4 | Add settings tab | 기존 기능 | 카카오톡 설정 맨 아래 Ginppai 메뉴와 그 안의 기존 추가 설정 화면을 기기에서 확인했다. |
| 5 | Allow Hide on Any Chat | 미구현 | 관리 권한이 있을 때 자기 메시지를 포함해 메시지를 가리는 기능. 채팅방 숨김 기능과 다르며 미구현이다. |
| 6 | Allow Open Chat Managers To Block Members | 미구현 | 오픈채팅 방장과 부방장의 일반 참여자 차단 동작. 서버 관리 권한을 유지하는 iOS 경로 확인이 필요하다. |
| 7 | Allow direct thread reply editing | 미구현 | 자신의 스레드 답글을 메인 대화에서 길게 눌러 편집. iOS 편집 메뉴와 스레드 대상을 연결해야 한다. |
| 8 | Allow invisible characters | 미구현 | 입력과 표시 과정의 보이지 않는 문자 제거를 중지한다. 전체 입력 검증을 무력화하는 방식으로 대체하지 않는다. |
| 9 | Allow open chat media bundle | 미구현 | 오픈채팅에서 GIF/WebP 등을 묶음 사진으로 전송하는 제한을 푼다. iOS 첨부 생성과 전송 경로가 미확인이다. |
| 10 | Allow profile media download | 기존 기능 | 현재와 과거 프로필, 배경, 영상의 원본 파일을 사진 앱에 저장하는 기존 기능으로 대응한다. |
| 11 | Allow reply to feed | 미구현 | 피드 메시지에 스와이프 답장 또는 댓글을 허용한다. carouselLeverage 예외와 삭제 메시지 방어를 함께 구현해야 한다. |
| 12 | Always Show Kick Button | 미구현 | 참여자 관리에서 내보내기 버튼을 표시한다. 서버 권한이 생기는 기능은 아니며 iOS 관리 동작은 미구현이다. |
| 13 | Block reactions on deleted or hidden messages | 미구현 | 원본을 보존해 보여주는 서버 삭제/가림 메시지의 반응과 두 번 탭 전송을 막는다. 현재 보관함은 읽기 전용이며 채팅 본문 보존 기능은 미구현이다. |
| 14 | Block replies on deleted or hidden messages | 미구현 | 보존된 서버 삭제/가림 메시지를 답장/댓글 대상으로 보내지 못하게 한다. 채팅 본문 보존 기능과 함께 구현해야 한다. |
| 15 | Bypass Moat check | Android 전용 방식 | KakaoPay Android Moat 네이티브 스캔을 생략하는 기능. iOS 결제 무결성 검사에 동일 패치를 적용하지 않았다. |
| 16 | Bypass input mention limit in non-multichat | 미구현 | 다중대화가 아닌 대화의 멘션 입력 허용 판단을 확장하는 패치다. upstream은 MentionComponent의 Bool 판정을 바꾼다. 단순한 최대 멘션 개수 변경으로 대체하지 않으며 iOS 입력 경로는 분석 중이다. |
| 17 | Change model | 별도 트윅 | Android는 Samsung 태블릿 Build 정보를 사용한다. iOS 기기 판정은 별도 Ginppai-Kakao-iPad가 담당하며 기존 설치 파일을 보존한다. |
| 18 | Change package name | 미구현 | Android 패키지 이름 변경 기능. iOS 번들 ID 변경 UI는 미구현이며 SideStore의 서명 ID 변경과 동일 기능으로 계산하지 않는다. |
| 19 | Custom branding | 개발 중 | IPA 준비 도구의 --display-name으로 메인 앱의 모든 포함 언어 표시 이름을 변경한다. 앱 식별자와 실행 파일 이름 보존 및 잘못된 언어 파일 처리 시험을 통과했다. 홈 화면 표시 기기 시험은 남았다. |
| 20 | Default external browser | 개발 중 | 채팅방 processWebUrlLink:appName:userID:inWeb:에서 일반 웹 링크를 기본 브라우저로 연다. 다른 링크 경로와 실패 시 복귀는 미검증이다. |
| 21 | Disable 300+ unread limit | 개발 중 | Chat.displayUnreadCount에서 실제 양수 unreadCount를 표시한다. 기기 호출 기록은 확인했지만 300개 초과 화면은 미검증이다. |
| 22 | Disable 99 unread limit | 개발 중 | 메시지의 읽지 않은 사람 수를 계산하는 실제 루프의 99 중단 조건을 선택적으로 해제한다. 원래 조건 보존을 포함한 ARM64 코드 시험과 아이폰에서 98, 99, 100, 350명 계산을 확인했다. 100명 이상 실제 대화의 화면 표시는 미검증이다. |
| 23 | Disable AdFit environment detection | Android 전용 방식 | Android AdFit의 root/emulator 판정 보고를 억제한다. 기존 iOS 광고 차단이 같은 탐지 억제라는 뜻은 아니다. |
| 24 | Disable ChatRoomAdController | 일부 대응 | 기존 BizboardManager/AdFit 로딩 차단이 일부 대응한다. 오픈링크 채팅방 광고 컨트롤러 전체 동등성은 미확인이다. |
| 25 | Disable Collapse Button | 미구현 | 오픈채팅 목록의 접기/더보기용 잘림을 없앤다. 기존 재선택 후 맨 위 이동과 별도 기능이다. |
| 26 | Disable Community Tab | 미구현 | 오픈채팅 목록의 커뮤니티 영역을 제외한다. 숏폼 탭 숨김과 별도 기능이며 미구현이다. |
| 27 | Disable Friend Feed tab | 개발 중 | 친구 헤더의 소식 항목을 모델에서 제외하고 앱의 기본 선택 처리로 친구 목록을 유지한다. 아이폰에서 옵션 켜기와 끄기, 저장된 소식 상태에서 재실행, 하단 탭 반복 선택을 확인했다. 다른 iOS 버전 시험은 남았다. |
| 28 | Disable Friend Lists ad | 일부 대응 | 기존 FriendFeedAdLoader와 BizBoard/GlobalBanner 차단이 대응한다. 친구 영역의 모든 지역별 광고 경로를 이번 개발판에서 다시 검증하지 않았다. |
| 29 | Disable OpenChat feed ad | 일부 대응 | 공통 AdFit 차단이 일부 대응한다. 오픈채팅 피드의 요청과 렌더링 경로 전체는 미확인이다. |
| 30 | Disable Pay banner ad | 미구현 | KakaoPay 배너 광고 로딩과 표시 차단. iOS Pay 배너만을 대상으로 한 연결 지점이 미확인이다. |
| 31 | Disable S2Event | 미구현 | S2 이벤트의 보관과 전송을 중단한다. 일부 Tiara 주소 차단과 동일한 기능으로 계산하지 않는다. |
| 32 | Disable SDK Tracker | 개발 중 | 확인된 ad.daum.net 및 Tiara 두 호스트를 로컬 응답으로 차단한다. 기기에서 요청 한 건의 차단 호출을 확인했다. 전체 SDK 경로의 동등성 검증은 남았다. |
| 33 | Disable Sentry | 개발 중 | Sentry SDK 초기화 호출을 중단한다. 초기화 차단 호출 기록은 확인했지만 전체 전송 여부 검증은 남아 있다. |
| 34 | Disable ShortForm ad | 일부 대응 | 숏폼을 숨기면 해당 화면은 사라진다. 숏폼을 켠 상태의 개별 광고 로더 차단은 아직 동등하게 구현하지 않았다. |
| 35 | Disable Talk Share Log | 개발 중 | 실제 API 주소를 요청 시점에 확인하도록 수정했다. 아이폰에서 외부 전송을 막는 시험용 보호 장치를 둔 상태로 정확한 공유 분석 주소의 로컬 204 응답을 확인했다. 일반 URLSession 전송 대상이며 로컬 기록 구성과 백그라운드 URLSession까지 중단하지는 않는다. |
| 36 | Disable abuse detection report | Android 전용 방식 | Android Play Integrity 응답의 토큰/오류 값을 변경한다. 신고 전송 자체를 모두 중단하는 기능이 아니며 iOS App Attest 패치로 계산하지 않는다. |
| 37 | Disable chat room list ad | 일부 대응 | 기존 채팅 목록 Bizboard/GlobalBizboard 표시 차단과 공통 광고 로더 차단이 일부 대응한다. 전체 경로 동등성은 미확인이다. |
| 38 | Disable open chat room comments | 미구현 | 오픈채팅 댓글 기능을 끄는 옵션. iOS 댓글 정책/화면 연결은 미구현이다. |
| 39 | Disable verifying signature | Android 전용 방식 | Android 앱의 자체 서명 검사를 바꾼다. iOS 코드 서명을 생략하거나 유효하게 만드는 기능이 아니다. |
| 40 | Enable Markdown | 개발 중 | 새 텍스트 전송에 markdown=true를 추가한다. 나와의 채팅에서 굵은 글씨와 전송 첨부를 확인했다. 기본 수정 기능을 거치면 서식이 풀리는 경로가 확인되어 아직 전체 동등 구현은 아니다. |
| 41 | Enable send big text | 미구현 | 짧은 입력에서 보내기 버튼을 길게 누를 때 특수 문자를 추가해 큰 글씨로 보낸다. 장문 길이 제한 해제와 다르다. iOS native shout 경로는 분석만 했다. |
| 42 | Force enable debug mode | 미구현 | 앱 내부 debug 플래그를 켠다. 트윅 상태 보고 파일이 생기는 것을 동일한 카카오톡 debug 기능으로 계산하지 않는다. |
| 43 | Force enable emoticon plus feature | 미구현 | 클라이언트 이모티콘 플러스 판정을 활성화한다. 서버 사용 제한은 유지된다. iOS의 일일 체험 전송 경로는 미구현이다. |
| 44 | Ghost Mode | 개발 중 | 정확한 iOS 실행 파일에서 입력 중 표시 판단 Swift getter를 변경한다. 미전송 입력 시험에서 차단 경로 6회 실행과 초안 정리를 확인했다. 상대 기기 표시 결과는 미검증이다. 읽음 표시 숨김이 아니다. |
| 45 | Hide More tab Game tab | 개발 중 | 더보기 게임 항목의 표시 정책을 연결했다. 한국 화면에서 옵션을 켜면 홈만 남고 끄면 게임 탭이 다시 나타나는 것을 확인했다. 게임 선택 시 기본 화면으로 복귀하는 기존 처리와 지갑 항목은 유지한다. 시험 후 일본 화면과 게임 숨김 설정을 복원했다. |
| 46 | Hide More tab components | 미구현 | 더보기 Pay/지금/날씨/서비스 그룹/라인 서비스를 각각 숨긴다. iOS에서 독립 옵션과 모델 필터 구현이 남았다. |
| 47 | Hook Package Manager | Android 전용 방식 | Android appComponentFactory/PackageManager 연결 변경. iOS 설치 및 런타임 구성 요소와 직접 호환되지 않는다. |
| 48 | Ignore forward restriction | 개발 중 | iOS의 leverageMessageForwardable 판정에 선택 옵션을 연결했다. 준비된 실행 파일에 설치했고 기본값은 꺼짐이다. 다양한 메시지의 전달 선택 화면과 전송 결과는 미검증이다. |
| 49 | Open profile from open chat feed | 미구현 | 오픈채팅 입장/퇴장 피드의 사용자 프로필을 연다. 여러 대상 처리, 권한, 퇴장한 사용자 처리까지 iOS 대응이 필요하다. |
| 50 | Override feature flag | 미구현 | 이름=값 형태로 기능 플래그를 덮어쓴다. iOS 플래그 저장/판정 경로가 미확인이라 작동하지 않는 입력창은 추가하지 않았다. |
| 51 | Register settings activity | 기존 기능 | Android Activity 대신 UIKit 화면을 앱 안에서 연다. 아이폰에서 설정 화면 진입, 옵션 저장, 재시작 후 적용을 확인했다. |
| 52 | Remove BizBoard ads | 기존 기능 | 기존 BizBoard 로딩과 표시 영역 숨김으로 대응한다. 모든 광고 제거를 보장하는 항목은 아니다. |
| 53 | Remove More tab ad | 일부 대응 | 기존 MoreTab.NativeADUIView/LocalBizboardUIView 숨김으로 일부 대응한다. 모든 더보기 프로모션을 제거하는 기능은 아니다. |
| 54 | Remove OpenLink chat room list ad | 일부 대응 | 공통 광고 차단과 오픈채팅 우선 화면이 일부 대응한다. OpenLink 전용 전체 로딩 경로 검증은 남았다. |
| 55 | Remove Short-form Tab | 기존 기능 | 기존 숏폼 숨김과 오픈채팅 우선 열기로 대응한다. 반복 탭 시 숏폼으로 바뀌지 않게 하는 기존 처리도 유지한다. |
| 56 | Remove feed ad | 일부 대응 | 기존 FriendFeedAdLoader와 공통 광고 영역 숨김이 일부 대응한다. 모든 피드 광고의 동등한 차단은 미검증이다. |
| 57 | Remove focus ad | 미구현 | Android Focus 광고 로딩 완료 경로를 바꾼다. iOS 대응 로더와 적용 범위는 미확인이다. |
| 58 | Remove native ad | 일부 대응 | 기존 AdFitNativeAdLoader 차단이 대응한다. 요청 실패 처리와 모든 화면의 재시도 동작은 이번 개발판에서 검증하지 않았다. |
| 59 | Remove shop tab | 기존 기능 | 기존 쇼핑 탭 숨김 옵션으로 대응한다. |
| 60 | Restore keyword notification log | 미구현 | 키워드 알림 기록 수집, 목록 진입, 원래 대화 위치 이동을 복원한다. 단순 보관함 검색은 이 기능을 대신하지 않는다. |
| 61 | Show chatroom channel ID | 개발 중 | 선택 메시지 도구에서 채팅방 ID를 복사한다. upstream의 채팅 설정 및 사이드 제목 진입 위치까지 동일하지는 않다. |
| 62 | Show deleted, hidden, or edited messages | 개발 중 | 관측한 메시지의 텍스트와 유형 변경을 별도 읽기 전용 보관함에 저장한다. 실제 수정 전후 두 버전 표시와 재실행 후 기록 보존을 확인했다. 채팅 본문 원문 유지, 첨부와 서식 보존, 모든 수신 경로는 미구현이다. |
| 63 | Show message details | 개발 중 | 선택한 iOS MessageRecord와 해석한 첨부를 JSON으로 표시한다. 나와의 채팅 사진과 3인 대화의 보낸 텍스트에서 표시 및 선택 본문 일치를 확인했다. 임의 계정 객체는 탐색하지 않는다. Android 전체 ChatLog와 필드 및 진입 UI는 다르다. |
| 64 | Show message read receipts | 개발 중 | 네이티브 코드에서 참여자 ID와 읽음 위치의 두 Int64 배열이 같은 인덱스로 대응함을 확인했다. 위치 대응 및 누락/잘못된 값 처리를 테스트했다. 기기에서 나와의 채팅 1명, 3인 대화 3명의 읽음 목록과 이름 표시를 확인했다. |
| 65 | Show messages restricted to mobile | 개발 중 | Universal 표시 정책과 알림톡 및 Leverage의 보조 기기 표시 경로를 변경했다. 아이폰에서 모바일 및 보조 기기 조건 해제와 유효성, 버전, 잠금, 연령 제한 유지 계산을 확인했다. 실제 제한 메시지 화면 비교는 남았다. |
| 66 | Spoof App ID | Android 전용 방식 | Android APK key hash를 쓰는 App ID/생체 인증 식별자 패치. iOS 생체 인증을 우회하는 기능으로 이식하지 않았다. |
| 67 | Spoof apk checksums | Android 전용 방식 | Android APK 체크섬 보고를 바꾼다. iOS Mach-O 파일에 APK 체크섬을 넣지 않는다. |
| 68 | Spoof attestation package name | Android 전용 방식 | Android attestation 보고의 패키지 이름만 바꾼다. iOS 계정이나 번들 ID 변경과 동일하지 않다. |
| 69 | Spoof installer package name | Android 전용 방식 | Android 무결성 보고의 설치 출처를 Google Play로 바꾼다. iOS App Store 설치 출처 패치가 아니다. |
| 70 | Spoof signature | Android 전용 방식 | Android 앱 서명 해시 응답을 바꾼다. SideStore/LiveContainer 재서명 절차를 대체하지 않는다. |
| 71 | Strip image metadata | 개발 중 | JPEG, HEIC, PNG의 방향과 픽셀을 유지하며 촬영 정보를 제거한다. 아이폰에서 GPS, EXIF, IPTC가 들어 있는 합성 사진을 원본 화질로 전송했고 서버 파일이 처리 결과와 바이트 단위로 같음을 확인했다. 처리할 수 없는 형식은 원래 정책처럼 원본 그대로 전송한다. 전체 업로드 경로 검증은 남았다. |
| 72 | Version info patch | 기존 기능 | 기존 Ginppai 설정에 작성자와 트윅 버전을 표시한다. 카카오톡 앱 버전 자체는 바꾸지 않는다. |

기계 판독용 대조 자료는 [feature-matrix.json](feature-matrix.json)에 있습니다.
