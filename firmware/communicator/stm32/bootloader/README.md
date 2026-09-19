# MCUboot C 포트 — T-107 구현 중

[STM32 안내](../README.md), [T-107](../../../../docs/tasks/T-107-stm32-mcuboot.md),
[OTA 정본 §5](../../../../docs/architecture/ota.md#5-stm32-부트로더와-esp-제어)를 따른다.
현재는 실제 MCUboot C library와 host Flash 모형, Arm archive compile 단계다.
부트로더 ELF/BIN, 실기 부팅, production loader 완료를 의미하지 않는다.

## 책임과 경계

- BSP `flash_layout`을 재사용한다. MCUboot에는 primary/secondary의 불변 descriptor만
  제공하며 boot/policy/config/reserved 영역은 open할 수 없다.
- `flash_map.c`는 descriptor 주소·범위·정렬을 검사하고 동기식 IO 세 함수에 전달한다.
  가짜 descriptor/NULL/zero/overflow/잘못된 image·slot·sector capacity를 거절한다.
  실패 시 출력은 그대로다. getter는 성공한 open으로 얻은 descriptor만 받는 upstream
  내부 API이며 외부 요청 API가 아니다. close는 해제할 자원이 없는 no-op이다.
- IO는 boot 단일 owner 전용이며 ISR/task/callback 재진입을 허용하지 않는다.
  buffer는 호출 반환까지 유효하다. 기존 앱·UART·CAN에는 연결하지 않는다.
- write/erase 전달 직전 `canview_boot_flash_check()`를 호출한다. G474의
  `platform/stm32g474/flash_guard.c`가 SYSCFG clock, Flash512KiB, busy/option 오류,
  DBANK=1, BFB2=0, bank swap 없음, NRST 양방향 및 WRP1A page0..31을 매번 확인한다.
  다른 WRP 영역은 start>end인 비활성 상태만 허용한다. option byte·상태 register를
  쓰거나 오류를 지우지 않는다. 이 검사는 고정 배치의 필수 조건이며 T-507의 생산용
  RDP/PCROP·유선 복구 정책 승인이나 실제 ECC/SRAM/Flash IO 검증을 대신하지 않는다.
  [RM0440 Rev9 §3.7](https://www.st.com/resource/en/reference_manual/dm00355726-stm32g4-series-advanced-armbased-32bit-mcus-stmicroelectronics.pdf)와
  고정 CubeG4 v1.6.3 CMSIS register 정의를 따른다.
- SHA-256/P-256은 고정 MCUboot의 TinyCrypt와 ASN1 parser를 사용한다. 직접 암호를
  작성하지 않는다. ASN1 allocator와 키 생성용 RNG는 실패를 반환한다. 검증에 heap은
  필요하지 않으며 사용하지 않는 upstream split-image allocator도 거절한다.
- header512B/offset-swap/primary validation/FIH MEDIUM/97 sectors를 고정한다.
  Release에서도 upstream assert를 유지한다. 실제 watchdog 진행/시간 판정은 미구현이며
  host의 progress count는 watchdog 검증이 아니다.
- `image_hooks.c`는 기존 T-007 metadata 대조 함수를 재사용한다. header512/padding,
  단일 protected TLV0xA0/168B, 일반 SHA256/keyhash/ECDSA TLV 순서·길이와 전체180KiB
  경계를 검사한다. 별도 identity 공급 계약이 board/role/layout/제조 epoch/STM ABI의
  기대값을 제공하며 이미지 값으로 덮어쓰지 않는다. 실제 BSP 공급자는 아직 미구현이고
  합성 identity는 host 시험 파일에만 있다. 제품 loader에 기본 허용값은 없다.
- Hook은 일치 시에도 FIH_SUCCESS가 아닌 FIH_BOOT_HOOK_REGULAR를 반환한다.
  따라서 MCUboot 원래 hash/P-256 검증은 필수다. swap/revert의 읽기 시작 위치는
  공식 loader 상태 API에서 얻으며 offset2048을 무조건 가정하지 않는다.
  release_sequence는 여기서 floor 승인하지 않는다. T-205의 manifest/activation/
  confirmation 대조는 별도 미완료 gate다.

## upstream과 경고 처리

[MCUboot v2.4.0](https://github.com/mcu-tools/mcuboot/tree/6d3b3d2c38ab20c242e5b9abb04d050086383eb2)
pin/clean 검사 후 사용한다. SDK 원본·license는 그대로 보존한다. Upstream `_Static_assert`
때문에 해당 경계는 C11이고 BSP/Flash adapter의 portable C99는 유지한다.

`mcuboot_compat.cmake`는 build 폴더 사본에 두 가지 제한된 변환만 한다.

1. GCC가 무시하는 함수 반환형의 top-level `volatile`을 제거한다. scalar 반환35곳,
   struct 반환4곳만 바꾸며 FIH 전역/지역 객체·멤버·매크로의 volatile은 보존한다.
   FIH struct에는 동일 layout의 무수식 반환 alias를 추가한다. 변경 수가 달라지면 실패한다.
2. TinyCrypt의 `t5` 임시 배열3개를 명시 zero-initialize한다. 고정 P-256의 word count를
   GCC가 증명하지 못해 발생한 maybe-uninitialized 경고를 해결하며 산술/판정은 유지한다.

경고 억제나 FIH profile 하향은 하지 않는다. Host 모델은 Arm panic branch loop만
abort로 바꾸고 MEDIUM의 double-variable/CFI 검사를 유지한다. Arm build는 원래 panic
loop를 컴파일한다. Windows Clang에서는 C volatile 의미를 명시하며 MS 확장의
acquire/release struct 연산을 요구하지 않는다. 실제 glitch 내성 증거는 아니다.

## 재현과 시험 범위

Windows host configure에 `-DCANVIEW_MCUBOOT_ROOT=C:/cv/mcuboot-2.4.0`을 전달하고
`cmake --build --preset host-debug` / `ctest --preset host-debug -R stm32-mcuboot-model -V`를
실행한다. Python은 기존 OTA lock 의존성이 설치된 환경을 사용한다. root가 없으면
이 시험은 명시적으로 NOT_RUN이며 CI Windows job은 root를 필수로 전달한다.
Arm primary-debug/primary-release도 같은 인자로 library compile을 검사한다.

`tests/ota/test_mcuboot_model.py`는 메모리 전용 임시 개인키로 공식 imgtool image를
생성하고 `mcuboot_model.c`의 실제 `boot_go`를 실행한다. 공개 DER와 합성 image만
임시 파일로 전달하며 종료 시 제거한다. 정상/최대 크기, 잘린 image, header/TLV/서명
변조, 미신뢰 key, test-swap, 미확정 revert, 확정을 검사한다. 모델은 8B write/2KiB erase,
`0xff` doubleword까지 포함한 중복 program 거절과 보호 영역 불변을 검사한다.
현재 cut sweep는 작은 image의 swap120/revert138 API 전후 경계이며 매 중단 뒤
RAM CFI 상태를 reset하고 Flash만 보존해 이전 정상 image 복귀를 확인한다.

정상 키로 다시 서명한 잘못된 board/role/layout/epoch/ABI와 metadata 길이·reserved,
identity 공급 실패·잘못된 로컬 role·Flash read 실패도 거절한다. 일반 TLV 길이만
바꿔도 native verifier가 허용하는 경우를 회귀시험으로 추가하고 고정 profile 검사에서
거절한다. 이는 MCUboot 서명 검증을 자체 암호 코드로 대체하는 것이 아니다.

Guard 단위시험은 option16개·size65536개·WRP65536개 조합과 register 불변을 확인한다.
MCUboot 모형에도 같은 guard C를 연결해 실패 시 backend write/erase 미호출을 검사한다.
모형의 추가 register 상수19개는 실제 Arm 빌드에서 vendor CMSIS와 compile-time 대조한다.

미구현/미검증은 실제 BSP identity 공급, 생산 보호 profile 승인/실측, 실제 G474
Flash driver·SRAM critical path·ECC/NMI·erase stall·watchdog, 쓰기 도중 torn word/page,
boot handoff, T-205 CONFIRM_INTENT/floor 연결이다. 모형의 직접 confirm은 제품 정책
API가 아니다. Physical/HIL·Flash·option-byte/provisioning은 NOT_RUN, 차량 TX는 NO-GO다.
