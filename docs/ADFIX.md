# 추가 광고 요청 차단

기존 ‘광고와 프로모션 숨기기’ 옵션에 다음 광고 진입점을 보완합니다.

- Thread / Focus: 검증된 AdFitPM의 Swift Void 진입점을 서명 전에 준비하고, 런타임 옵션이 켜졌을 때만 요청을 생략합니다. 옵션이 꺼지거나 커스터마이저가 연결되지 않으면 원래 실행 경로를 유지합니다.
- BizBoard: `loadWithUserObject:contentObject:duplicateKey:`에 동기 Bool 거절 어댑터를 적용합니다.
- Quick View / Brand Chip / NAS: Objective-C 메서드의 반환·인자형을 확인한 뒤 각 실패 콜백 형태에 맞춰 차단합니다. 광고가 성공한 것처럼 알리거나 nil 광고를 성공 콜백에 전달하지 않습니다.
- 확인된 오픈채팅·친구 피드·글로벌 광고 전용 뷰를 숨깁니다.

새 SDK 준비 경로는 KakaoTalk **26.8.0 / 2026.9.14.1** 및 정확한 원본 AdFitPM의 SHA-256·UUID·함수 첫 명령·예약 공간이 맞을 때만 적용합니다. 그 외 바이너리를 추측해서 패치하지 않습니다.

원본 SDK SHA-256:

`3f4cfaffcf29328567476741adfeecf70120a4b10629d2c9e3bc4ef6dd2b285b`

```bash
python3 build.py
python3 tools/prepare_ipa.py original.ipa prepared.ipa \
  --dylib build/GinppaiKakaoCustomizer.dylib \
  --features --country-ui --ad-features
```

`ad_native_patch.py`를 `prepare_ipa.py`와 같은 폴더에 포함해야 합니다. 변경된 앱·SDK·dylib는 설치 도구로 다시 서명합니다. Thread / Focus 연결은 dylib만 교체하는 방식으로 생기지 않습니다. 광고 SDK 준비가 없는 환경에서도 기존 차단 기능과 지원되는 Objective-C 경로는 별도로 동작합니다.

로컬 사용자에게서 추가 광고 차단이 동작한다는 보고를 받았습니다. 탈옥·LiveContainer 및 모든 화면의 동작을 새로 검증한 결과는 아닙니다. Native 명령어·예약 공간 검사 7개와 Objective-C 실패 콜백·Bool 반환 ABI 검사를 포함합니다.
