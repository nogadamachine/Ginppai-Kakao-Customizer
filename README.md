# Ginppai-Kakao-Customizer

카카오톡 화면 정리와 메시지 편의 기능.

**3.1.0** / KakaoTalk **26.7.3, 26.8.0** / iOS 17 이상

[다운로드](https://github.com/nogadamachine/Ginppai-Kakao-Customizer/releases/latest) / [설치 안내](docs/INSTALL.md) / [긴빠이 Repo](https://nogadamachine.github.io/Ginppai-Repo/)

## 기능

- 광고, 특별한 친구, 숏폼, 쇼핑 탭 숨김
- 삭제 전에 보관한 원문 표시, 메시지 정보와 읽음 목록, 변경 기록 검색
- 프로필 사진과 영상 원본을 사진 앱에 저장
- 더보기 메뉴, 시작 화면, 국가 화면과 개인정보 옵션

**카카오톡 설정 맨 아래 > Ginppai**에서 설정합니다. 변경 후 **적용 후 종료**를 누르고 다시 실행하세요.

3.1.0은 카카오톡 26.8.0에 대응하며 숏폼 숨김을 수정했습니다. **iPad 판정과 기본 키보드 수정은 별도 [Ginppai-Kakao-iPad](https://github.com/nogadamachine/Ginppai-Kakao-iPad)의 기능입니다.**

탈옥용 **DEB 3.1.0-1**은 후킹 라이브러리 연결을 수정했습니다. LiveContainer용 파일은 기존 3.1.0과 같습니다.

## 설치

| 환경 | 파일 |
|---|---|
| 탈옥 | Repo 또는 환경에 맞는 루트리스 / 루트풀 DEB |
| LiveContainer | 카카오톡 전용 트윅 폴더에 DYLIB |
| SideStore 직접 설치 | 본인의 IPA에 DYLIB를 넣고 서명 |

메시지 기능과 국가 선택에는 실행 파일 추가 패치가 필요할 수 있습니다. [설치 안내](docs/INSTALL.md)를 확인하세요. 구버전 DYLIB와 중복 설치하지 마세요.

확인 환경은 iOS 26.1 / LiveContainer입니다. 탈옥 실기와 SideStore 직접 설치는 미검증입니다. [기능별 범위](docs/FEATURES.md) / [26.8.0 검증 기록](docs/kakao-26.8-support.md)

만든이 [@nogadamachine](https://www.instagram.com/nogadamachine/). Kakao와 관계없는 비공식 프로젝트입니다. [GPL-3.0 및 저작권 고지](NOTICE.md).
