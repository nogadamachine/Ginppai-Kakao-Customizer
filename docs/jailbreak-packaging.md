# 탈옥 패키지 3.1.0-1

일부 메시지 기능이 사용하는 `MSHookFunction`을 다른 트윅의 라이브러리 로드에 의존해 찾던 문제를 수정했습니다. DEB 안의 DYLIB만 Cydia Substrate 호환 프레임워크를 명시적으로 연결합니다. 앱 코드와 LiveContainer용 DYLIB는 3.1.0 그대로입니다.

- 루트풀: `/Library/Frameworks`
- 루트리스: `/var/jb/Library/Frameworks`와 `@loader_path/.jbroot/Library/Frameworks`

[Theos의 루트리스 경로 규칙](https://theos.dev/docs/rootless)과 [ElleKit의 호환 프레임워크 경로](https://github.com/tealbathingsuit/ellekit/blob/main/Makefile)를 기준으로 구성했습니다.

## 검증

`python3 tools/test_jailbreak_link.py`는 macOS 동적 로더와 별도 시험 프레임워크로 수정 전의 함수 탐색 실패와 수정 후 성공을 확인합니다. iOS 탈옥 환경을 대체하는 시험은 아닙니다.

`python3 tools/verify_release.py`는 두 DEB의 의존 경로, 원래 실행 코드 보존, 서명, 필터와 파일 해시를 검사합니다. 탈옥 실기 설치와 기능 동작은 미검증입니다.

## 설치 조건

Customizer는 iOS 17 이상, KakaoTalk 26.7.3 또는 26.8.0의 지원 실행 파일과 활성화된 트윅 주입이 필요합니다. RootHide는 별도 변환과 검증이 필요해 지원 대상으로 표시하지 않습니다. 국가 화면은 실행 파일 추가 패치가 필요할 수 있습니다.

Repo 추가 오류의 구체적인 기기 조건은 확인되지 않았습니다. 기본 주소의 HTTPS 응답과 패키지 파일에는 이상이 없었으며, 목록을 최신 버전으로 정리하고 루트풀과 루트리스 전용 주소를 추가했습니다.
