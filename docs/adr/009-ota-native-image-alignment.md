# ADR-009: SDK 재사용을 위한 OTA image 정렬

- 상태: accepted — T-007 설계 결정이며 최종 리뷰·target 통합·실물 검증 완료가 아님
- 날짜: 2026-09-15
- 보완: ADR-007의 독립 복구·권한·서명 정책은 유지한다. 미배포 compact 컨테이너 후보만 revision2로 바꾼다.

## 이유

ESP-IDF6.0.3의 전체 image verifier는 mapped segment의 Flash 주소와 실행 주소가
MMU page 안에서 일치하는지 검사한다. 현재 보드의 page는64KiB이며 native image를
임의 위치에 붙이면 이 검사가 실패한다. Secure Boot V2의 서명 sector도4KiB 정렬을
요구하므로4KiB 조건만 맞추는 것으로는 부족하다.

별도 ESP image parser를 만들거나 Flash 주소를 가상으로 바꾸는 계층 대신,
native image 앞에 필요한0 byte만 두어 SDK가 원본 그대로 읽게 한다.
압축·archive·파일 경로·플러그인·새 암호 구현은 추가하지 않는다.

## 결정

- `CVOTA002`, header version2, signed `format_version=2`로 구분한다.
  기존 `CVOTA001`/version1은 거절한다. 조용한 해석 변경이나 자동 fallback은 없다.
- 작은 header+manifest+서명 prefix는 그대로 유지한다. 각 image 시작만64KiB로
  올리고 사이 공간은 반드시0이다. 마지막 image 뒤에는 padding이 없다.
- image length/hash는 padding을 제외한 원래 native image byte열이다.
  offset은 signed length와 고정 정렬 규칙으로 계산하고 입력 주소는 받지 않는다.
- padding은 수신 chunk에서 검사한다.64KiB RAM buffer를 만들지 않는다.
  실제 staging 시작도64KiB 정렬을 만족해야 한다. factory-only bench 설치를
  OTA partition으로 자동 변경하지 않는다.
- native metadata의 `CVIMG001`/version1과 partition `layout_id`는 바꾸지 않는다.
  컨테이너 revision과 native image·Flash layout version은 별도 계약이다.

## 비용과 검증

image마다 추가 공간은64KiB 미만이다. 현재 Communicator의4.5MiB staging은
ESP4MiB+STM180KiB를 두 순서 모두 수용한다. 작은 prefix가 첫 경계 안에 있으므로
전체 길이는 ESP→STM4,444,160B, STM→ESP4,456,448B이며 각각274,432B/262,144B가 남는다.

C/Python offset 대조, legacy/future revision 거절, padding 변이·절단·chunk 경계,
길이/hash와 cleanup 회귀를 요구한다. native SDK provider·최종 target binary·
독립2인 리뷰와 실제 Flash/HIL 검증은 별도 gate로 남긴다.

근거는 고정 SDK commit `76f5dedd9950a3012fee8fb7d5586df21fc67802`의
[image verifier](https://github.com/espressif/esp-idf/blob/76f5dedd9950a3012fee8fb7d5586df21fc67802/components/bootloader_support/src/esp_image_format.c)와
[MMU 설정](https://github.com/espressif/esp-idf/blob/76f5dedd9950a3012fee8fb7d5586df21fc67802/components/soc/Kconfig)다.
실제 CI BIN의 주소 조건 재현은 [journal의 SDK 정렬 조사](../journal.md),
현재 byte 계약은 [OTA 설계 §7](../architecture/ota.md#7-패키지인증보안-계약)에 둔다.
