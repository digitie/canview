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
Flash backend 연결·최종 SRAM 배치·ECC 복구·erase stall·watchdog, 쓰기 도중 torn word/page,
boot handoff, T-205 CONFIRM_INTENT/floor 연결이다. 모형의 직접 confirm은 제품 정책
API가 아니다. Physical/HIL·Flash·option-byte/provisioning은 NOT_RUN, 차량 TX는 NO-GO다.

## G474 단일 Flash 명령 — backend 연결 전

`platform/stm32g474/flash_command.c`는 새 framework 없이 한 page erase와 한8B program만
구현한다. [C 계약](../interface/canview_stm_flash_command.h)의 boot 전용 동기 호출이며
MCUboot read/write/erase backend에는 아직 연결하지 않았다. 상위 owner는 DMA·주변장치를
정지하고 IWDG/DWT를 먼저 준비해야 한다. ISR·RTOS·callback 재진입은 허용하지 않는다.

- BSP guard와 RDP0(0xAA), lock/option lock, 오류/진행 중 명령을 확인한다. 다른 RDP는
  자동 해제하지 않고 거절한다. SRAM 실행과 생산 보호 조합은 T-507 승인 대상이다.
- primary/secondary만 다룬다. address/정렬/enum을 검사하고 PROGRAM의 all-FF는 거절한다.
  상위 IO가 all-FF를 skip해야 하며, 이미 프로그램한 FF/ECC를 판독만으로 판별할 수 있다는
  가정을 하지 않는다. read-back 비FF는 DUPLICATE지만 fresh-erase 소유와 torn-word/ECC
  복구는 별도 backend 책임이다. 성공은 controller EOP이며 read-back 보증이 아니다.
- `flash_execute`와 예외용 `flash_fault_reset`, literal을 `.canview_flash_ram`에 둔다.
  code와 임시 vector가 SRAM1/2 밖이면 거절한다. 512B 정렬128-entry vector를 stack에
  마련하고 PRIMASK를 보존한다. 최대 개별 stack frame은 Arm Debug1176B/Release1144B다.
  최종 boot linker/startup의 전체 section 배치·복사·stack budget 검사는 아직이다.
- busy 동안 Flash code/helper/상수를 읽지 않는다. SRAM NMI/HardFault는 reset 요청만
  하고 복귀하지 않는다. 이는 persistent ECC에 대한 복구 정책이 아니며, 그대로 연결하면
  reset loop가 될 수 있으므로 ECC-safe read와 recovery 선택이 연결 gate로 남는다.
- HAL의 doubleword/erase/cache 순서를 따르되 interrupt tick 대신 DWT 차이와 유한 poll
  상한을 사용한다. 8500000cycle(170MHz에서50ms), counter 정지 시17000000회 상한이다.
  clock별 실제 실행 시간/IWDG 여유는 미측정이며 watchdog feed/disable은 하지 않는다.
  BSY timeout 때는 SRAM reset 요청 후 fail-stop, 정상/오류 반환 때는 cache를 폐기하고
  Flash lock·VTOR·PRIMASK를 복원한다. Flash가 busy인 채 Flash 호출자에게 반환하지 않는다.

근거는 고정 CubeG4 v1.6.3의 `stm32g4xx_hal_flash.c`/`stm32g4xx_hal_flash_ex.c`,
위 RM0440과 [ST G474 datasheet](https://www.st.com/resource/en/datasheet/stm32g474re.pdf)의
page erase 최대24.47ms/64bit program83.35µs 표다. 해당 수치를 board 실측으로 표시하지 않는다.

`stm32-flash-command`는 동일 C를 register 모형에 연결해 슬롯 전체49408word/193page,
정렬/보호/RDP256값, duplicate, unlock 실패, status 오류, DWT wrap/정지, timeout,
NMI와 정상/오류 cleanup을 검사한다. Arm post-build의 `check_stm32_flash_ram.py`는
실제 object의 두 함수·section 크기·외부 relocation0·직접 branch 범위를 확인한다.
Debug480B/Release364B다. 이 검사는 최종 ELF의 SRAM 주소나 실제 Flash 실행을 증명하지 않는다.

## SRAM reset 전제와 ECC errata

[ES0430 Rev9, 2024-06](https://www.st.com/resource/en/errata_sheet/es0430-stm32g471xx473xx474xx483xx484xx-device-errata-stmicroelectronics.pdf)
§2.2.7의 첫 SRAM write 손실을 피하려고 앱 startup의 첫 `SystemInit` 호출을 linker
`--wrap`으로 받는다. 고정 CubeG4 startup 원본은 수정하지 않는다.
`platform/stm32g474/startup_ram.c`의 최소 naked wrapper가 stack/data 쓰기 전에
0x20000000/0x20008000/0x20010000/0x20014000을 읽고 SDK 함수로 tail branch한다.
C prologue도 stack을 쓸 수 있어 이 진입부만 assembly를 사용한다.

`check_stm32_core.py`는 bench/primary 실제 BIN의 vector, MSP literal, Reset_Handler
첫 세 명령, 네 read의 정확한 명령열과 SDK tail branch를 검사한다. CCM 초기화는
구현하지 않았으므로 linker의 CCM section은0B여야 한다. CCM 사용을 추가하려면
parity/reset 초기화를 별도로 구현해야 한다. Toolchain/SDK가 명령열을 변경하면
검사를 느슨하게 하지 않고 다시 검토한다. 실제 reset/전원 및 SRAM 실측은 NOT_RUN이다.

같은 문서 §2.2.3은 중단된 Flash 작업 뒤 read 시 ECCR 정보 손상 가능성을 명시한다.
따라서 ECCR 주소만으로 지울 page를 선택하거나 유일한 정상 이미지를 자동 erase하는
복구는 허용하지 않는다. 실제 ECC-safe read와 소프트웨어 작업 범위에 근거한 복구는
아직 구현 중이며, 이번 startup 보완이 그 수용을 대신하지 않는다.
