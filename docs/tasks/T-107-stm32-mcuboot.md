# T-107 OTA-03 G474 MCUboot와 보호 Flash map

- 상태: `BLOCKED`
- 우선순위: `P0`
- Gate: `G1/G2 / OTA-03`
- 선행: `T-007`, `T-102`
- 외부 선행: STM32G474CEU6 샘플·SWD와 전원 fault 장비

## 목표

STM32 전체 Flash scaffold에서 독립 부트로더·정상 앱·offset-swap 슬롯으로 옮긴다. MCUboot는 G474 완제품이 아니므로 port와 실패 복구 근거를 직접 만든다.

## 고정 결정

[OTA §5·7·9](../architecture/ota.md)의 DBANK=1, 2 KiB page, 8B write/ECC, swap using offset, 별도 MCUboot trailer를 유지한다. MCUboot 정확한 version/commit과 라이선스를 고정한다. BFB2 bank 전환·scratch 재설계는 하지 않는다.

## 구현 범위

- bootloader/application 분리 CMake/linker·imgtool·공개키 검증·protected TLV board/role/ABI 검사
- primary 0x08010000/vector 0x08010200, secondary 추가 page와 policy/config A/B 고정 map
- bank stall·SRAM critical code/vector·ECC/NMI·watchdog progress와 fault recovery
- bootloader는 CAN·ARM을 초기화하지 않는 local health/confirmation interface

## 범위 밖

UART transport(T-108), production 보호 설정 실행(T-507), 전체 회로 variant fault qualification(T-508).

## 예상 변경 파일

아래는 이 task가 생성·확정할 미래 산출물이다. 경로가 아직 없다는 사실을 검증 통과로 해석하지 않는다.

```text
firmware/communicator/stm32/bootloader/
firmware/communicator/stm32/linker/
tools/ota/validate_stm32_map.py
tests/ota/test_stm32_flash.py
```

## 수용 기준

- [ ] DBANK/WRP/NRST가 승인 profile과 다르면 erase 전에 거절하며 option-byte를 자동 변경하지 않는다.
- [ ] bootloader 64 KiB·signed app 180 KiB·header/TLV/trailer/추가 page 경계를 실제 imgtool 산식과 map으로 검증한다.
- [ ] 8B alignment·중복 partial write·잘못된 enum address·서명/TLV/board 불일치를 거절하며 bootloader와 유일한 정상본을 덮어쓰지 않는다.
- [ ] ECC 오류·NMI·bank1 fetch stall·erase worst case에 안전 복구하고 무조건 watchdog feed/무기한 disable이 없다.
- [ ] swap/revert 중 deterministic cut point와 후보 hang/reset 시험에서 previous-known-good 보존 또는 명시 recovery로 수렴한다.
- [ ] 정상 앱 CONFIRM_INTENT·floor reconciliation interface는 T-205 정책과 연결하며 부트로더가 전체 journal로 MCUboot trailer를 대체하지 않는다.

## 검증 계획

이 task의 map/Flash negative tests를 먼저 구현하고 host simulator·Arm Debug/Release build·map·target swap/revert를 실행한다. host simulator만으로 실제 Flash ECC 복구를 합격 처리하지 않는다.

## evidence와 rollback

upstream commit·linker/map·서명된 합성 image·SWD readback·cut-point 로그를 기록한다. 최초 layout 변경은 유선 재설치이며 웹 mass erase/보호 해제로 rollback하지 않는다.
