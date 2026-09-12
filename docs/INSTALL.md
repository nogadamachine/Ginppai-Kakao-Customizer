# 설치와 삭제

카카오톡 26.7.3 및 iOS 17 이상이 필요합니다. 처음 설치하거나 앱을 업데이트하기 전에 카카오톡의 대화 백업을 확인하세요. 앱을 삭제·재설치하면 기존 로그인과 대화가 유지된다고 보장할 수 없습니다.

## 탈옥 — DEB 또는 repo

1. Sileo/Zebra/Cydia에 `https://nogadamachine.github.io/Ginppai-Repo/`를 추가합니다.
2. **Ginppai-Kakao-Customizer**를 설치합니다. 사용하는 부트스트랩이 지원하는 패키지만 선택합니다.
3. 카카오톡을 완전히 종료한 뒤 다시 실행합니다.
4. 카카오톡 설정 맨 아래의 트윅 메뉴를 엽니다.

DEB 직접 설치도 가능합니다. `iphoneos-arm64`는 루트리스(`/var/jb`), `iphoneos-arm`은 루트풀입니다. 다른 종류의 DEB를 강제로 설치하지 마세요. 이 구분은 아이폰 CPU가 64비트인지 여부가 아니라 패키지의 설치 구조를 뜻합니다.

패키지는 다음 위치에 DYLIB와 번들 필터를 설치합니다.

| 방식 | 경로 |
|---|---|
| 루트풀 | `/Library/MobileSubstrate/DynamicLibraries/` |
| 루트리스 | `/var/jb/Library/MobileSubstrate/DynamicLibraries/` |

필터 대상은 `com.iwilab.KakaoTalk`뿐입니다. 수동 DYLIB 설치를 선호한다면 DEB에서 DYLIB와 동명 PLIST를 **함께** 꺼내 해당 경로에 넣을 수 있지만, 패키지 관리자가 제거·업데이트를 관리하는 DEB 방식을 권장합니다.

## 비탈옥 — SideStore + LiveContainer

1. [SideStore 공식 설치 안내](https://docs.sidestore.io/docs/installation/install)를 따라 SideStore를 설치합니다.
2. [LiveContainer 공식 설치 안내](https://livecontainer.github.io/docs/installation/lc_sidestore)를 따라 LiveContainer를 설치하고 서명을 설정합니다.
3. 준비한 카카오톡 IPA를 LiveContainer에 가져옵니다.
4. Releases에서 `GinppaiKakaoCustomizer.dylib`를 받습니다.
5. LiveContainer의 **Tweaks → 새 폴더**에서 카카오톡 전용 폴더를 만들고 DYLIB를 가져옵니다.
6. 카카오톡 앱 설정의 **Tweak Folder**로 그 폴더를 지정합니다. TweakLoader를 끄는 옵션은 해제합니다.
7. 카카오톡을 실행합니다. LiveContainer가 트윅을 서명해 불러옵니다.

[LiveContainer의 트윅 공식 안내](https://livecontainer.github.io/docs/guides/tweaks)에 폴더 지정과 재서명 방법이 있습니다. 기존 `KakaoAdBlock.dylib`을 사용 중이면 새 파일과 동시에 두지 말고 교체합니다. 설정 파일 이름을 유지하므로 기존 옵션을 계속 읽습니다. iPad 트윅은 별도 파일로 함께 둘 수 있습니다.

## 비탈옥 — SideStore에 IPA 직접 설치

카카오톡 26.7.3의 **본인이 준비한 비암호화 IPA**와 배포 DYLIB가 필요합니다. 이 프로젝트는 IPA를 배포하거나 암호화를 해제하지 않습니다.

```bash
python3 tools/prepare_ipa.py original.ipa Ginppai-Kakao.ipa \
  --dylib GinppaiKakaoCustomizer.dylib
```

두 트윅을 함께 넣으려면:

```bash
python3 tools/prepare_ipa.py original.ipa Ginppai-Kakao.ipa \
  --dylib GinppaiKakaoCustomizer.dylib \
  --dylib GinppaiKakaoIPad.dylib
```

완성된 IPA를 아이폰으로 옮겨 SideStore에서 가져와 서명·설치합니다. 앱 ID, 알림, 키체인과 확장 기능은 재서명 환경에 따라 원래 App Store 앱과 다르게 동작할 수 있습니다. **직접 SideStore 설치는 구조·패키지 검증까지 했으며 실제 로그인·푸시·전체 기능은 미검증입니다.** LiveContainer 경로와 구분해서 판단하세요.

## 국가 선택 추가 패치

위 명령에 `--country-ui`를 추가합니다. 코드 지문이 일치하는 26.7.3에만 적용됩니다.

```bash
python3 tools/prepare_ipa.py original.ipa Ginppai-Kakao-Country.ipa \
  --dylib GinppaiKakaoCustomizer.dylib --country-ui
```

UI 국가 진입점만 바꾸며, 원래 계정 국가 판정 함수는 보존합니다. 기본값은 계정 기본 화면입니다. 이미 설치된 App Store 앱이나 탈옥 환경의 원래 앱 실행 파일을 DEB 설치 과정에서 덮어쓰지 않습니다. 따라서 **DEB 단독 설치에서는 국가 전환이 비활성화**될 수 있습니다.

## 제거·업데이트

- 탈옥: 패키지 관리자에서 제거/업데이트 후 카카오톡을 다시 실행합니다.
- LiveContainer: 카카오톡을 종료하고 해당 앱의 전용 트윅 폴더에서 이 DYLIB만 삭제/교체합니다.
- 직접 주입 IPA: 트윅이 없는 본인의 IPA를 다시 준비하고 서명해야 합니다. 앱 재설치 전 대화 백업을 확인합니다.
- 국가 패치 제거: 패치를 적용하지 않은 원래 IPA/실행 파일로 되돌립니다. DYLIB만 지워도 UI 선택값은 다시 적용되지 않지만, 파일에 넣은 패치 자체가 삭제되지는 않습니다.

## 문제가 생겼을 때

- 기능이 안 보임: 카카오톡 버전이 **정확히 26.7.3**인지, 트윅 로더와 폴더가 설정됐는지 확인합니다.
- 국가 선택이 회색: 위의 추가 UI 패치가 없는 실행 파일입니다.
- 저장 실패: 미디어를 완전히 연 뒤 다시 시도하고, 사진 추가 권한·저장 공간·네트워크를 확인합니다. 사진 앱 실패 시 파일 저장을 시도할 수 있습니다.
- 다른 트윅과 충돌: 구버전/중복 트윅을 제거하고 하나씩 켜서 확인합니다. 광고·탭 관련 기능이 겹치는 트윅은 충돌할 수 있습니다.

이슈에는 앱/iOS 버전, 설치 경로, 오류 문구만 적어 주세요. 계정 비밀번호·인증서·대화·개인 사진·개인 미디어 URL은 올리지 마세요.
