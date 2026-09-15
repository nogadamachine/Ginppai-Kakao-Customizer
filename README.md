# Ginppai-Kakao-Customizer

카카오톡을 내가 쓰는 방식에 맞춥니다. 광고와 탭을 정리하고, 메시지를 살펴보고, 프로필의 원본 사진과 영상을 저장할 수 있습니다.

**nogadamachine 제작** / **3.0.0-dev.2 개발판** / **KakaoTalk 26.7.3 전용**

[개발자 인스타그램](https://www.instagram.com/nogadamachine/) / [긴빠이 Repo](https://nogadamachine.github.io/Ginppai-Repo/) / [다운로드](https://github.com/nogadamachine/Ginppai-Kakao-Customizer/releases) / [설치 안내](docs/INSTALL.md)

## 설정 메뉴

카카오톡 설정 맨 아래에서 **Ginppai**를 엽니다. 기존 옵션과 국가 선택은 업데이트 후에도 유지됩니다.

| 메뉴 | 할 수 있는 일 |
|---|---|
| 화면 | 광고, 숏폼, 쇼핑, 통화, 게임과 하단 알림 표시 정리, 친구 목록만 보기 |
| 대화 | 입력 중 표시 숨김, 안 읽은 수 확장, 텍스트 서식, 공유 카드 전달 설정 |
| 사진과 영상 | 현재와 과거 프로필 및 배경 원본 저장, 전송 사진의 촬영 정보 제거 |
| 기록 | 메시지 상세 정보, 읽은 사람 목록, 관측한 원문과 변경 내역 보관 |
| 개인정보 | 확인된 오류 수집기와 사용 및 공유 분석 요청 제한 |
| 사용 방식 | 시작 탭, 화면 국가, 기본 브라우저, 다시 누르기와 빠른 설정 |

옵션은 자동 저장됩니다. 각 화면 위의 **적용 후 종료**를 누른 뒤 카카오톡을 다시 실행해 주세요. 지원되지 않는 옵션은 이유와 함께 회색으로 표시합니다. 설정 첫 화면에는 메시지 보관함과 개발자 인스타그램, 긴빠이 Repo 바로가기가 있습니다.

3.0.0-dev.2에서는 **친구 목록만 보기**를 추가하고, 앱 시작 순서 때문에 공유 분석 차단이 적용되지 않던 문제를 수정했습니다.

## 개발판 확인 범위

3.0 개발판은 기능별 검증을 진행 중입니다. 사진 촬영 정보 제거는 합성 사진을 실제로 전송해 서버 파일까지 확인했습니다. 메시지 정보와 읽음 목록, 수정 기록, 게임 탭 표시 전환도 기기에서 확인했습니다.

메시지 수정 후 서식 유지, 일부 전달과 링크 경로, 모든 수신 경로의 기록 수집은 아직 확인이 끝나지 않았습니다. 큰 대화방의 숫자는 기기의 실제 계산 함수까지 시험했으며 대규모 대화 화면 시험은 남았습니다. [기능별 범위](docs/FEATURES.md)와 [검증 기록](docs/validation.json)을 확인하세요. 현재 공개 안정판은 2.3.2입니다.

## 탈옥 / 비탈옥 설치

| 환경 | 사용할 파일 | 설치 방식 |
|---|---|---|
| 탈옥 루트리스 | `…_iphoneos-arm64.deb` | Sileo/Zebra에서 Ginppai Repo를 추가하거나 DEB를 직접 설치 |
| 탈옥 루트풀 | `…_iphoneos-arm.deb` | Cydia/Sileo/Zebra에서 저장소를 추가하거나 DEB를 직접 설치 |
| 비탈옥, SideStore + LiveContainer | `GinppaiKakaoCustomizer.dylib` | SideStore로 LiveContainer를 설치하고, 카카오톡 전용 트윅 폴더에 DYLIB 추가 |
| 비탈옥, SideStore 직접 설치 | 직접 준비한 IPA | 동봉한 도구로 본인의 IPA에 DYLIB를 넣은 뒤 SideStore에서 서명 / 설치 |

**SideStore는 DEB나 DYLIB를 단독 앱으로 설치하지 않습니다.** LiveContainer가 불러오게 하거나, IPA에 먼저 넣어야 합니다. JIT 활성화 앱은 이 트윅 자체의 필수 조건이 아닙니다.

탈옥용 repo 주소:

```text
https://nogadamachine.github.io/Ginppai-Repo/
```

카카오톡 26.7.3 자체가 **iOS 17 이상**을 요구합니다. 탈옥 여부와 별개로 이 조건을 충족해야 합니다. 이 저장소가 iOS 26의 탈옥을 제공하는 것은 아닙니다. DEB는 일반적인 루트풀 / 루트리스 트윅 설치 구조로 제작했으며 **탈옥 기기 실사용과 SideStore 직접 설치는 아직 검증하지 않았습니다.** 확인한 환경은 iOS 26.1의 SideStore + LiveContainer입니다.

## 원본 저장의 의미

사진 / 영상을 크게 열고 오른쪽 위 저장 버튼을 한 번 누르면 **사진 앱에 바로 저장**합니다. 버튼은 위아래로 스크롤해도 화면의 같은 위치에 남으며, 현재 보이는 사진 / 영상을 저장합니다. 파일 앱으로 내보내는 메뉴는 제공하지 않습니다. 화면 캡처나 현재 표시된 축소 이미지를 저장하지 않습니다.

- 현재 프로필과 배경은 앱 모델의 원본 URL을 우선합니다.
- 이전 기록은 선택한 사진 / 영상의 파일 URL을 사용합니다. 영상의 썸네일을 대신 저장하지 않습니다.
- JPEG / MP4 등의 파일을 직접 내려받아 재압축 없이 보존합니다.
- 주소가 없거나 형식을 처리할 수 없으면 오류를 표시합니다.

서버가 앱에 제공하지 않는 업로드 전 고해상도 원본을 복구하지는 못합니다. HLS 최고 화질 선택 / 재압축 없는 내보내기는 구현했지만 실제 프로필 HLS 영상은 미검증입니다.

## 국가 설정의 범위

국가 설정은 **화면 구성**만 선택합니다. 전화번호, 계정 국가, 결제 지역을 바꾸지 않습니다. 여러 국가가 같은 해외 화면을 사용하며 모든 국가를 하나씩 실행해 본 것은 아닙니다.

DEB/DYLIB만 원래 앱에 설치하면 국가 선택이 비활성화됩니다. `tools/prepare_ipa.py --country-ui`는 코드 지문이 일치하는 26.7.3 실행 파일에 검증된 UI 패치를 적용합니다. 다른 버전이나 이미 변형된 코드에는 적용하지 않습니다. [자세한 방법](docs/INSTALL.md#국가-선택-추가-패치)을 확인하세요.

## 공개 파일과 빌드

- `src/KakaoCustomizer.m`: 광고 / 탭 / 다운로드 및 런타임 연결.
- `src/CustomizationSettings.inc`: 기능별 설정 화면과 개발자 링크.
- `src/FriendList.inc`: 친구 화면의 소식 항목 제거와 목록 선택 유지.
- `src/MessageCore.swift`: 메시지 정보 / 읽음 위치 / 기록과 전송 정책.
- `src/MediaResolver.swift`: 현재 선택한 미디어의 원본 주소 판별.
- `src/ResolverTests.swift`: 잘못된 사진 선택, 영상/정지사진 혼동, URL 쿼리 변경 방지 검증.
- `build.py`, `tools/package.py`: DYLIB 및 루트풀/루트리스 DEB 생성.
- `tools/prepare_ipa.py`: 사용자가 준비한 비암호화 IPA에 DYLIB를 넣는 도구.

Xcode 26 이상과 Python 3.9 이상이 있는 Mac에서:

```bash
python3 build.py
brew install dpkg
python3 tools/package.py
python3 test.py
```

결과는 `dist/`에 생성됩니다. 개인 개발자 인증서 없이 임시 서명한 배포본입니다. 비탈옥 설치 시 SideStore/LiveContainer가 사용자의 계정으로 다시 서명합니다. 카카오톡 IPA / 실행 파일, 개인 설정 / 대화 / 프로필 사진 / 미디어 주소 / 인증서는 공개 파일에 포함하지 않습니다.

## 변경 이력

### 2.3.2 - 스크롤 중 저장 버튼 유지

- 프로필 확대 화면의 저장 버튼을 사진 스크롤 영역 밖에 고정.
- 사진 / 영상을 넘긴 뒤 현재 보이는 항목을 다시 판별해 저장.
- 저장 버튼 한 번으로 사진 앱에 직접 저장. 파일 내보내기 메뉴와 오류 시 파일 저장 선택 제거.
- 프로필 화면을 닫거나 다른 화면으로 이동하면 저장 버튼 숨김.

### 2.3.1 - Ginppai 시리즈 공개 배포

- 기존 KakaoCustomizer 2.3.0의 기능을 Ginppai 이름으로 공개.
- SideStore가 팀 식별자를 덧붙인 번들 ID와 일반 IPA의 앱 폴더 이름 대응.
- 동일 DYLIB를 사용하는 루트풀 / 루트리스 DEB와 공개 빌드/IPA 준비 도구 추가.

### 2.3.0 - 원본 사진 / 영상

- 화면용 이미지 대신 서버 원본 파일 저장.
- 프로필 영상 MP4 저장과 실제 사진 앱 재생 검증.

## Ginppai 시리즈

- [Ginppai-Kakao-iPad](https://github.com/nogadamachine/Ginppai-Kakao-iPad): 카카오톡 안에서 iPad 기기 판정을 가져옵니다. 별도 트윅이며 함께 쓸 수 있습니다.
- [Ginppai-Kakao-Profile-Photo](https://github.com/nogadamachine/Ginppai-Kakao-Profile-Photo): macOS 카카오톡 캐시 기반 프로필 이미지 도구.
- [Ginppai-GUID-for-Bypass-A12](https://github.com/nogadamachine/Ginppai-GUID-for-Bypass-A12)

이 프로젝트는 Kakao와 관계없는 비공식 개인 프로젝트입니다. 광고 숨김은 확인된 광고 영역에 적용되며 모든 추천 / 프로모션 콘텐츠를 제거한다고 보장하지 않습니다. 3.0 개발판의 전체 배포 라이선스는 GPL-3.0입니다. 기존 MIT 저작권 고지는 보존했습니다. [저작권 및 참조 코드](NOTICE.md)를 확인하세요.
