# ADR-007: N16R8과 장치별 독립 복구 OTA

- 상태: accepted — 설계 결정이며 OTA 구현·실물 단전 시험 완료가 아님
- 날짜: 2026-09-06
- 배경: 사용자 N16R8 변경·Bridge 없는 독립 OTA·회로 변경 요청을 기존 설계와 ADR 정본에 동기화
- 대체 범위: ADR-006 결정 1의 Communicator MINI-1-N4R2 고정과 공용 reset 해석만 대체. 차량/USB 전원 분리·센서·Bridge 권한 제한 등 나머지는 유지

## 결정

1. Communicator는 ESP32-S3-WROOM-1-N16R8로 변경한다. 회로 board ID는 `comm-r2-n16r8`이다. 기존 `docs/hardware/r1/` 경로를 board revision으로 오인하지 않는다.
2. Controller·Communicator·Diagnostic Bridge 각각 자체 Wi-Fi AP와 휴대폰 브라우저 업데이트를 제공한다. Bridge나 정상 Controller가 없어도 Communicator ESP/STM 복구가 가능해야 한다.
3. ESP 정상 앱 A/B와 별도 복구 앱, STM 내부 primary/secondary와 보호된 부트로더를 사용한다. 현재안에서 외장 SPI NOR은 필수가 아니다. PSRAM은 휘발성 작업 메모리이므로 복구 원본을 보존하는 장치로 계산하지 않는다.
4. ESP/STM reset을 분리하고 ESP가 STM reset·custom recovery를 제어한다. 서비스 인터록은 MCU 출력만으로 우회되지 않게 하드웨어로 차단한다. ROM BOOT0는 물리 서비스용이며 일반 웹에 raw ROM/erase/option-byte API를 제공하지 않는다.
5. 초기 partition/bootloader provisioning은 안정된 유선 작업대에서 수행한다. 일반 OTA는 보호 영역을 바꾸지 않는다. 성공은 업로드 완료가 아니라 이미지 검증·시험 부팅·실제 호환성 확인·영속 commit 뒤 판정한다.

## 정본과 수용 조건

Flash 주소·복구 protocol·회로 pin·전원 차단 transaction·공격 모델·시험 행렬은 [OTA 설계](../architecture/ota.md), 회로 입력은 [OTA 회로 생성기](../../tools/hardware/ota_circuits.py), 구현 단계는 [작업 요약](../tasks.md)에 둔다. 이 ADR에 수치와 wire 구조를 복제하지 않는다.

WROOM 면적·안테나·ECC 조건·실제 image 크기와 STM MCUboot port는 실측·빌드 gate다. 기존 단일 factory partition 설치본이 무선으로 자동 이행 가능하다고 표시하지 않는다. 회로 ERC나 host 회귀 통과는 단전 복구·실차 TX 승인 근거를 대신하지 않는다.
