# T-107 OTA-03 G474 MCUboot와 보호 Flash map

- 상태: `IN_PROGRESS`
- 우선순위: `P0`
- Gate: `G1/G2 / OTA-03`
- 선행: `T-007`, `T-102`
- 외부 선행: STM32G474CEU6 샘플·SWD와 전원 fault 장비

## 2026-09-19 source-only 시작

T-007 PR36은 `c60641f`로 merge됐다. T-102 source/review PR29의 merge
`50410ba`도 확인했다. T-102 전체는 물리 계측과 이 task의 보호 Flash map 연결이
남아 IN_PROGRESS이며, 완료됐다고 간주하지 않는다.
사용자의 G1 이전 firmware 구현·하드웨어 없이 가능한 작업 진행 승인에 따라
C source와 host/실제 Arm target 검증부터 진행한다. 외부 선행과 아래 수용 기준은
삭제하거나 완료 체크하지 않는다.

Branch는 `codex/t107-stm32-mcuboot`다. SDK와 evidence 보존을 위해 기존
`F:/dev/canview-wt/t007-ota-container` 경로를 새 branch로 재사용한다.
첫 구현은 정본의 고정 Flash map·공식 imgtool 경계와 잘못된 profile/주소/정렬의
거절 시험이다. 이어 실제 MCUboot CMake port·보호 Flash API·swap/revert로 연결한다.
기존 전체 Flash linker를 OTA 지원 완료로 표시하거나 host 모형을 실제 ECC/전원
복구 근거로 대체하지 않는다. Physical/HIL은 NOT_RUN, 차량 TX는 NO-GO다.

## 2026-09-19 첫 C 구현 단위

[PR37](https://github.com/digitie/canview/pull/37)의 첫 source 단위는 BSP 고정 Flash
배치 조회·범위 검사다. null/zero/invalid enum/정렬/overflow/영역 끝과 보호 영역
쓰기 거절을 C로 구현했으며, heap·callback·가변 전역 상태를 추가하지 않았다.
Host와 Arm은 같은 source를 컴파일한다. 아직 Flash IO나 앱에 연결되지 않았으며
성공 반환은 쓰기 권한·서명·유일한 정상본 보존을 의미하지 않는다.

최종 로컬 검증은 Host Debug/Release151/151, 새 `flash_layout.c` ASan/UBSan과
line/region/function/branch coverage100%, 기존 STM32 Debug/Release target
ELF/MAP/BIN 생성 및 compiler/linker/CMake warning0이다. 최초 Arm 검증에서
새 library의 CAPTURE_ONLY forced include 누락을 발견해 CMake 계약을 연결했고
검증기를 변경하지 않았다. 합성 capture fixture source digest도 함께 갱신했다.
독립 2인 task 리뷰와 새 CI artifact 감사는 아직이며 아래 수용 기준은 체크하지 않는다.

다음은 공식 MCUboot port·별도 linker·실제 imgtool 크기 경계 연결이다.
DBANK/WRP/NRST profile 검사, 중복 doubleword, ECC/NMI·stall·watchdog,
swap/revert/confirmation은 남아 있다. 실제 Flash/HIL은 NOT_RUN이다.

## 2026-09-19 primary 앱 target 연결

`primary-debug`/`primary-release` CMake preset과 primary linker wrapper를 추가했다.
기본 bench와 section/RAM/stack script는 공유하되 primary vector0x08010200과
payload179KiB를 별도로 제한한다. 고정 SDK SystemInit의 지원 macro를 사용해
VTOR를 맞추며 SDK 원본은 수정하지 않는다. 실제 Arm ELF의 SystemInit disassembly도
SCB VTOR(0xE000ED08)에0x08010200을 쓰는 것을 확인했다.

`tools/ota/validate_stm32_map.py`는 실제 ELF/map/BIN/vector/Reset_Handler/load byte,
gap·중첩·영역 밖을 검사하며 bench-as-primary를 거절한다. 기존 build ID gate도
bench/primary 두 base만 허용한다. `check_stm32_primary_image.py`는 공식 imgtool로
실제 앱 BIN과179KiB 경계 copy를 메모리 전용 임시 P256 key로 서명·검증한다.
signed180KiB·공식 trailer2376B/page reserve4096B·secondary 추가2048B를 확인했다.
개인키/시험 서명은 배포하지 않으며 actual bootloader 실행 증거로 사용하지 않는다.

CI target matrix에 primary2개 빌드와 서명 교차 검사를 넣고 ELF/MAP/BIN6개를
artifact/hash manifest에 추가했다. 이 변경의 CI·독립 task 리뷰는 아직이며 AC는
체크하지 않는다. Bootloader64KiB image·Flash IO/profile·ECC·swap/revert·confirmation이
남아 있고 actual Flash/HIL은 NOT_RUN이다.

## 2026-09-19 MCUboot C 포트 첫 실행

[포트 설명](../../firmware/communicator/stm32/bootloader/README.md)에 source/host/Arm
경계와 재현을 기록했다. 직접 swap 알고리즘을 만들지 않고 고정 upstream boot_go를
사용하며 BSP map 뒤에 동기식 IO 세 함수만 둔다. Host 모형에서 실제 P-256 검증,
test-swap/revert/confirm, malformed/미신뢰 image와 API 경계 cut258곳을 시험한다.
FIH MEDIUM 객체/검사는 보존하고 GCC 반환형 qualifier 및 TinyCrypt 임시 배열의
경고 원인만 build 사본에 수정한다. SDK 원본은 clean으로 유지한다.

Arm archive compile은 bootloader final binary가 아니다. 아래 AC는 계속 열어 둔다.
다음은 board/role/ABI protected TLV와 G474 profile/실제 Flash·ECC driver 및
bootloader executable 연결이다. 물리 검증과 production provisioning은 NOT_RUN이다.

## 2026-09-19 signed metadata 부팅 검사

MCUboot image hook에 기존 T-007 metadata 대조를 연결했다. 신뢰된 BSP identity 공급
계약과 입력 이미지의 값을 분리하고, protected TLV board/role/layout/epoch/ABI를
검사한 뒤에도 원래 P-256/hash 검증을 반드시 수행한다. 일반 TLV 길이 변이에 대한
고정 profile 검사도 추가했다. 실제 BSP 공급·floor/activation 정책은 아직 연결하지
않았으며 합성 identity와 직접 confirm은 host 시험에만 있다.

전체 task 완료나 물리 gate 통과가 아니며 아래 AC는 계속 열어 둔다. 다음은 실제
G474 profile/Flash/ECC·SRAM/watchdog와 bootloader 실행 파일 연결이다.

## 2026-09-19 Flash 보호 guard

write/erase 직전 고정 G474 배치 guard를 연결했다. DBANK1/WRP1A
page0..31/양방향 NRST, 다른 WRP 비활성,512KiB와 bank remap·busy/option 오류를
읽기 전용으로 확인한다. 실제 register/option-byte 쓰기는 하지 않는다. Host와 Arm이
동일 C를 사용하며 자세한 계약은 [포트 설명](../../firmware/communicator/stm32/bootloader/README.md)에 둔다.
이것만으로 생산 profile 승인이나 실제 보호·Flash/ECC 수용 기준을 체크하지 않는다.

## 목표

2026-09-19 단일 page erase/8B program C와 SRAM 전용 code/literal, 임시 vector,
유한 busy 대기와 cleanup 시험을 추가했다. [포트 설명](../../firmware/communicator/stm32/bootloader/README.md)의
제한대로 아직 IO backend·ECC 복구·최종 SRAM map/boot executable에 연결하지 않았으며
RDP0 개발 profile 이외를 승인하지 않는다. AC는 계속 열어 둔다.

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
