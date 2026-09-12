# Ginppai-Kakao-Customizer

카카오톡의 광고 자리를 긴빠이치고, 필요한 화면만 남깁니다. 프로필 사진·영상은 서버가 제공하는 원본 파일로 가져옵니다.

**만든이 · nogadamachine** · **2.3.1** · **KakaoTalk 26.7.3 전용**

[다운로드](https://github.com/nogadamachine/Ginppai-Kakao-Customizer/releases/latest) · [탈옥용 Ginppai Repo](https://nogadamachine.github.io/Ginppai-Repo/) · [설치·삭제 안내](docs/INSTALL.md) · [검증 범위](docs/COMPATIBILITY.md)

## 주요 기능

| 기능 | 설명 |
|---|---|
| 광고 숨김 | 확인된 배너·비즈보드 광고 로딩과 표시 영역을 숨깁니다. |
| 숏폼·쇼핑 정리 | 숏폼 숨김, 오픈채팅 우선 열기, 쇼핑 탭 숨김을 각각 설정합니다. |
| 오픈채팅 재선택 | 하단 탭을 다시 눌러도 숏폼으로 전환되지 않으며 목록 맨 위로 이동합니다. |
| 국가 선택 | 국가·지역을 이름이나 코드로 검색합니다. **추가 UI 패치가 적용된 실행 파일에서만 활성화됩니다.** |
| 중복 통화 탭 정리 | 해외 화면에서 중복으로 나타나는 통화 탭을 하나로 정리합니다. |
| 시작 화면 | 친구·채팅·오픈채팅/지금·통화·더보기 중 시작 탭을 선택합니다. 없는 탭은 친구로 돌아갑니다. |
| 편의 옵션 | 통화 탭 숨김, 하단 알림 배지 숨김, 더보기 두 번 누르기/길게 누르기로 빠른 설정을 지원합니다. |
| 원본 사진·영상 저장 | 현재·이전 프로필 사진, 배경사진, 프로필 영상을 사진 앱 또는 파일에 저장합니다. |

설정 맨 아래에 트윅 메뉴가 추가됩니다. 옵션은 자동 저장되며 **적용 후 종료**를 누른 뒤 카카오톡을 다시 실행하면 반영됩니다. 해외 화면에 없는 기능은 설명과 함께 비활성화됩니다.

## 탈옥·비탈옥 설치

| 환경 | 사용할 파일 | 설치 방식 |
|---|---|---|
| 탈옥 루트리스 | `…_iphoneos-arm64.deb` | Sileo/Zebra에서 Ginppai Repo를 추가하거나 DEB를 직접 설치 |
| 탈옥 루트풀 | `…_iphoneos-arm.deb` | Cydia/Sileo/Zebra에서 저장소를 추가하거나 DEB를 직접 설치 |
| 비탈옥, SideStore + LiveContainer | `GinppaiKakaoCustomizer.dylib` | SideStore로 LiveContainer를 설치하고, 카카오톡 전용 트윅 폴더에 DYLIB 추가 |
| 비탈옥, SideStore 직접 설치 | 직접 준비한 IPA | 동봉한 도구로 본인의 IPA에 DYLIB를 넣은 뒤 SideStore에서 서명·설치 |

**SideStore는 DEB나 DYLIB를 단독 앱으로 설치하지 않습니다.** LiveContainer가 불러오게 하거나, IPA에 먼저 넣어야 합니다. JIT 활성화 앱은 이 트윅 자체의 필수 조건이 아닙니다.

탈옥용 repo 주소:

```text
https://nogadamachine.github.io/Ginppai-Repo/
```

카카오톡 26.7.3 자체가 **iOS 17 이상**을 요구합니다. 탈옥 여부와 별개로 이 조건을 충족해야 합니다. 이 저장소가 iOS 26의 탈옥을 제공하는 것은 아닙니다. DEB는 일반적인 루트풀·루트리스 트윅 설치 구조로 제작했으며 **탈옥 기기 실사용과 SideStore 직접 설치는 아직 검증하지 않았습니다.** 확인한 환경은 iOS 26.1의 SideStore + LiveContainer입니다.

## 원본 저장의 의미

사진·영상을 크게 열고 오른쪽 위 저장 버튼에서 **원본을 사진 앱에 저장** 또는 **원본을 파일에 저장**을 선택합니다. 화면 캡처나 현재 표시된 축소 이미지를 저장하지 않습니다.

- 현재 프로필과 배경은 앱 모델의 원본 URL을 우선합니다.
- 이전 기록은 선택한 사진·영상의 파일 URL을 사용합니다. 영상의 썸네일을 대신 저장하지 않습니다.
- JPEG·MP4 등의 파일을 직접 내려받아 재압축 없이 보존합니다.
- 주소가 없거나 형식을 처리할 수 없으면 오류를 표시합니다.

서버가 앱에 제공하지 않는 업로드 전 고해상도 원본을 복구하지는 못합니다. HLS 최고 화질 선택·재압축 없는 내보내기는 구현했지만 실제 프로필 HLS 영상은 미검증입니다.

## 국가 설정의 범위

국가 설정은 **화면 구성**만 선택합니다. 전화번호, 계정 국가, 결제 지역을 바꾸지 않습니다. 여러 국가가 같은 해외 화면을 사용하며 모든 국가를 하나씩 실행해 본 것은 아닙니다.

DEB/DYLIB만 원래 앱에 설치하면 국가 선택이 비활성화됩니다. `tools/prepare_ipa.py --country-ui`는 코드 지문이 일치하는 26.7.3 실행 파일에 검증된 UI 패치를 적용합니다. 다른 버전이나 이미 변형된 코드에는 적용하지 않습니다. [자세한 방법](docs/INSTALL.md#국가-선택-추가-패치)을 확인하세요.

## 공개 파일과 빌드

- `src/KakaoCustomizer.m`: 설정·광고·탭·다운로드 기능.
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

결과는 `dist/`에 생성됩니다. 개인 개발자 인증서 없이 임시 서명한 배포본입니다. 비탈옥 설치 시 SideStore/LiveContainer가 사용자의 계정으로 다시 서명합니다. 카카오톡 IPA·실행 파일, 개인 설정·대화·프로필 사진·미디어 주소·인증서는 공개 파일에 포함하지 않습니다.

## 변경 이력

### 2.3.1 — Ginppai 시리즈 공개 배포

- 기존 KakaoCustomizer 2.3.0의 기능을 Ginppai 이름으로 공개.
- SideStore가 팀 식별자를 덧붙인 번들 ID와 일반 IPA의 앱 폴더 이름 대응.
- 동일 DYLIB를 사용하는 루트풀·루트리스 DEB와 공개 빌드/IPA 준비 도구 추가.

### 2.3.0 — 원본 사진·영상

- 화면용 이미지 대신 서버 원본 파일 저장.
- 프로필 영상 MP4 저장과 실제 사진 앱 재생 검증.

## Ginppai 시리즈

- [Ginppai-Kakao-iPad](https://github.com/nogadamachine/Ginppai-Kakao-iPad): 카카오톡 안에서 iPad 기기 판정을 가져옵니다. 별도 트윅이며 함께 쓸 수 있습니다.
- [Ginppai-Kakao-Profile-Photo](https://github.com/nogadamachine/Ginppai-Kakao-Profile-Photo): macOS 카카오톡 캐시 기반 프로필 이미지 도구.
- [Ginppai-GUID-for-Bypass-A12](https://github.com/nogadamachine/Ginppai-GUID-for-Bypass-A12)

이 프로젝트는 Kakao와 관계없는 비공식 개인 프로젝트입니다. 광고 숨김은 확인된 광고 영역에 적용되며 모든 추천·프로모션 콘텐츠를 제거한다고 보장하지 않습니다. 소스와 새로 빌드한 트윅은 MIT 라이선스입니다.
