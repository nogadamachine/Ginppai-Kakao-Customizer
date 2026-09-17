# 설치

KakaoTalk **26.7.3 또는 26.8.0**, iOS 17 이상이 필요합니다. 앱을 바꾸기 전에 대화를 백업하세요. IPA는 제공하지 않습니다.

## 탈옥

Sileo / Zebra / Cydia에 [긴빠이 Repo](https://nogadamachine.github.io/Ginppai-Repo/)를 추가하고 **Ginppai-Kakao-Customizer**를 설치합니다.

- 루트리스: `iphoneos-arm64.deb`
- 루트풀: `iphoneos-arm.deb`

## LiveContainer

1. Releases에서 `GinppaiKakaoCustomizer.dylib`를 받습니다.
2. 카카오톡 전용 트윅 폴더에 넣고 앱의 Tweak Folder로 지정합니다.
3. 카카오톡을 다시 실행하고 **설정 맨 아래 > Ginppai**를 엽니다.

기존 `KakaoAdBlock.dylib`와 중복 설치하지 말고 교체하세요. iPad 트윅은 함께 사용할 수 있습니다. JIT은 필요하지 않습니다.

## 추가 기능과 국가 선택

비탈옥에서 메시지 기록과 일부 대화 기능은 `--features`, 국가 화면 선택은 `--country-ui` 연결이 필요합니다. 본인이 준비한 비암호화 IPA에 적용합니다.

```bash
python3 tools/prepare_ipa.py original.ipa Ginppai-Kakao.ipa \
  --dylib GinppaiKakaoCustomizer.dylib --features --country-ui
```

`prepare_ipa.py`, `native_patch.py`, `native_layouts.py`를 같은 폴더에 두세요. 확인한 코드 지문과 다르거나 이미 패치된 파일은 중단합니다. iPad 트윅도 포함하려면 `--dylib GinppaiKakaoIPad.dylib`를 추가합니다.

완성한 IPA는 LiveContainer에 가져오거나 SideStore로 서명합니다. IPA에 포함한 DYLIB를 LiveContainer 트윅 폴더에 중복으로 넣지 마세요. **SideStore는 DEB나 DYLIB 자체를 설치하지 않습니다.**

국가 선택은 화면만 바꿉니다. 계정과 전화번호 국가는 유지됩니다. DEB만 설치하면 국가 전환이 비활성화될 수 있습니다.

## 업데이트와 제거

앱을 종료한 상태에서 트윅만 교체하거나 제거합니다. IPA에 직접 넣었다면 새 DYLIB로 원래 IPA를 다시 준비해 서명합니다. 국가 패치까지 제거하려면 원래 실행 파일을 사용합니다.

## 문제가 생기면

- 메뉴가 없음: 앱 버전, 트윅 폴더와 로더 설정 확인
- 국가 선택이 회색: 추가 UI 패치 필요
- 사진 저장 실패: 사진 추가 권한, 저장 공간과 네트워크 확인

탈옥 실기와 SideStore 직접 설치는 미검증입니다. [기능별 범위](FEATURES.md) / [26.8.0 검증](kakao-26.8-support.md)
