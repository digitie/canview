# T-306 OTA-05 세 장치 독립 AP와 브라우저 세션

- 상태: `BLOCKED`
- 우선순위: `P0`
- Gate: `G2 / OTA-05`
- 선행: `T-007`, `T-204`, `T-108`

## 목표

세 장치 각각에서 기본 휴대폰 브라우저로 offline bundle 선택·검증·명시 설치·결과 확인을 완수한다. Controller UI 작업군에 두지만 Communicator/Bridge 자체 AP 경로도 이 task가 소유한다.

## 고정 결정

[OTA §3·7–9](../architecture/ota.md)의 `/api/ota/v1`을 Diagnostic API와 구분한다. fixed IP·기본 파일 선택이 필수이고 captive portal/mDNS/QR은 보조다. 빌드 선행은 런타임 다른 장치 의존성이 아니다.

## 구현 범위

- OpenAPI/route 및 generated client, bounded streaming upload·status polling·단일 관리 세션
- 세 역할 offline AP·고유 WPA2 자격·10분 물리 세션·CSRF/Origin/Host·request 제한
- PREPARED_WAIT와 activate/cancel UI, 실제 phase/digest 기반 재접속과 error/recovery 안내
- Controller lease 반납·Comm J31 확인·Bridge observer 종료, radio raw/capture 중지

## 범위 밖

STA HTTPS 자동 다운로드·ESP-NOW firmware 중계·BLE 필수 경로·App Store 앱. Diagnostic Bridge 제어 권한 확대.

## 예상 변경 파일

아래는 이 task가 생성·확정할 미래 산출물이다. 경로가 아직 없다는 사실을 검증 통과로 해석하지 않는다.

```text
api/ota-v1.openapi.yaml
shared/ota/
firmware/*/components/canview_ota_web/
firmware/communicator/esp32/components/canview_ota_web/
tests/ota/test_api.py
tests/ota/web/
```

## 수용 기준

- [ ] Android Chrome/iOS Safari 실기기의 파일 선택·인터넷 없는 AP·휴대폰 잠금·뒤로가기·재연결을 각각 검증한다. 브라우저 simulator 결과는 구분한다.
- [ ] Controller와 Bridge를 끈 상태의 Comm ESP+STM OTA, Comm 없이 Controller OTA, 다른 장치 없이 Bridge OTA가 가능하다.
- [ ] 전송 100%·HTTP200·UART ACK·LOCAL_HEALTH_OK·최종 CONFIRMED를 구분하고 server boot/transaction/실제 digest로 상태를 복원한다.
- [ ] PREPARED_WAIT·승인 commit·reset·cancel/activate 경합의 service contract fixture에서 승인 전 설치0·승인 후 TOO_LATE·duplicate 단일 결과를 표시한다. 실제 영속 writer의 이 경계 검증은 T-205/T-508에서 완료한다.
- [ ] 인증 실패·CSRF·cross-origin·다중 client·초과 body·slow upload·session expiry·cache된 token을 거절하고 secret/VIN/위치/capture를 로그로 내보내지 않는다.
- [ ] RTC/DNS/외부 CDN/PSRAM/LCD 없는 recovery에서도 고정 주소와 label 안내가 유효하며 STA 수신 포트/CORS는 기본 닫힌다.
- [ ] Flash writer single owner·요청 buffer lifetime·disconnect 취소와 이미 commit된 작업의 지속을 분리한다. frontend abort를 device rollback으로 오표시하지 않는다.

## 검증 계획

API negative fixture·Playwright mock/server integration은 이 task에서 구현한다. T-205가 소비할 service interface와 합성 phase/digest provider를 먼저 제공하고, 후속 영속 정책의 완료를 이 task의 선행으로 요구하지 않는다. fixture만으로 실제 Flash activation/confirmation이 통과했다고 표시하지 않는다. 별도로 실제 세 장치/두 모바일 브라우저의 offline AP·기본 upload/상태 표시를 시험하고 기종·OS·브라우저 version·재연결 결과를 기록한다. 실제 updater end-to-end 완료는 T-205/T-508이다.

## evidence와 rollback

OpenAPI/asset digest·상태 전이 로그·민감정보 없는 화면 기록을 남긴다. 불완전 upload는 재업로드 요청, 승인 이후는 동일 plan만 계속하며 기존 앱을 임의 삭제하지 않는다.
