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
  입력 검증 실패 시 출력은 그대로다. IO 오류의 출력·부분 변경은 아래 backend 계약을 따른다.
  getter는 성공한 open으로 얻은 descriptor만 받는 upstream
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
  Release에서도 upstream assert를 유지한다. 아래 target runtime이 watchdog 진행/시간을
  제한한다. 기존 host boot_go 모형의 progress count는 실제 watchdog 검증이 아니다.
- `image_hooks.c`는 기존 T-007 metadata 대조 함수를 재사용한다. header512/padding,
  단일 protected TLV0xA0/168B, 일반 SHA256/keyhash/ECDSA TLV 순서·길이와 전체180KiB
  경계를 검사한다. 별도 identity 공급 계약이 board/role/layout/제조 epoch/STM ABI의
  기대값을 제공하며 이미지 값으로 덮어쓰지 않는다. 아래 BSP 공급자는 명시적 빌드
  입력만 사용한다. 기존 MCUboot 모형의 합성 identity는 제품에 link하지 않는다.
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
변조, 미신뢰 key, test-swap, 미확정 revert, 확정을 검사한다. 이제 제품용 `flash_io.c`를
그대로 link하고 bounded read와 한8B program/한2KiB erase primitive만 모형으로 대체한다.
FF 입력은 IO에서 program하지 않고, 실제 program된 doubleword의 중복은 모형이 거절한다.
cut sweep는 작은 image의 모든8B program/2KiB erase 전후를 대상으로 한다. 매 중단 뒤
RAM CFI 상태를 reset하고 Flash/프로그램 이력만 보존해 이전 정상 image 복귀를 확인한다.
한 명령 내부의 torn word/page나 실제 ECC/시간은 이 모형이 증명하지 않는다.

정상 키로 다시 서명한 잘못된 board/role/layout/epoch/ABI와 metadata 길이·reserved,
identity 공급 실패·잘못된 로컬 role·Flash read 실패도 거절한다. 일반 TLV 길이만
바꿔도 native verifier가 허용하는 경우를 회귀시험으로 추가하고 고정 profile 검사에서
거절한다. 이는 MCUboot 서명 검증을 자체 암호 코드로 대체하는 것이 아니다.

Guard 단위시험은 option16개·size65536개·WRP65536개 조합과 register 불변을 확인한다.
MCUboot 모형에도 같은 guard C를 연결해 실패 시 backend write/erase 미호출을 검사한다.
모형의 추가 register 상수19개는 실제 Arm 빌드에서 vendor CMSIS와 compile-time 대조한다.

미구현/미검증은 BSP identity의 최종 boot 연결·제조 입력 승인, 생산 보호 profile 승인/실측, G474
Flash IO의 최종 boot 연결·SRAM 배치·ECC 복구·erase stall·watchdog, 쓰기 도중 torn word/page,
boot handoff, T-205 CONFIRM_INTENT/floor 연결이다. 모형의 직접 confirm은 제품 정책
API가 아니다. Physical/HIL·Flash·option-byte/provisioning은 NOT_RUN, 차량 TX는 NO-GO다.

## BSP identity와 공개키 빌드 입력

`bsp/boot_identity.c`는 불변 role/board/layout와 제조 epoch, 별도 manifest root ID,
지원 STM ABI를 복사해서 제공한다. heap·가변 전역·SDK 호출은 없고 null 실패는 출력을
바꾸지 않는다. MCUboot의 기존 `bootutil_keys` ABI에 const P-256 SPKI DER 한 개를
제공한다. 개인키를 firmware·생성 header·저장소에 넣지 않는다.

OTA §6의 전체 보드 ID `comm-r2-n16r8`와 역할 결합 layout
`communicator-ota-layout-v1`을 사용한다. STM32 pin profile의
`comm-r2-stm32g474ceu6`는 하위 MCU 핀 계약이므로 OTA board ID로 대체하지 않는다.
새 container나 키 저장 형식을 만들지 않고 표준 DER와 기존 identity를 재사용한다.

Pinned MCUboot를 지정한 configure에 다음 네 입력을 **모두 명시**한다.

- `CANVIEW_BOOT_PUBLIC_DER`: 신뢰된 P-256 공개 SPKI DER의 기존 절대 파일 경로
- `CANVIEW_BOOT_SECURITY_EPOCH`: 제조 epoch의 unsigned decimal u32
- `CANVIEW_BOOT_MANIFEST_KEY_ID`: 별도 manifest trust root ID의 unsigned decimal u32
- `CANVIEW_BOOT_STM_ABI`: 지원 STM image ABI의 unsigned decimal u32

`0`도 명시한 값과 누락을 구별한다. 모두 비어 있으면 BSP target은 생성하지 않고
`NOT_CONFIGURED`를 표시한다. 일부만 입력하면 configure가 실패하고, 잘못된 DER나
범위를 넘는 정수는 build가 실패한다. 승인된 실제 값 대신 시험 값을 제품 기본값으로
저장하지 않는다. manifest key ID는 MCUboot 서명키 선택/검증을 대체하지 않는다.
파일 경로의 진위·제조 승인·불변 Flash 보호는 빌드가 증명하지 않으며 T-507 gate다.

`generate_stm32_boot_trust.py`는 최대92B만 읽고 canonical uncompressed91B DER,
P-256 curve와 u32를 검사한다. 출력은 build 폴더에만 생성하며 key/상수 변경 시
CMake가 다시 생성·컴파일한다. 생성 header의 공개 DER digest는 추적용이다.
Host `stm32-boot-trust`는 실제 C BSP/생성기를 link해 null·복사 격리·경계·728bit 변이,
누락·잘못된 path/DER/u32와 기존 build의 입력 교체를 시험한다. 시험 개인키는 메모리
전용이며 `--public-output`은 추가 Arm compile용 공개 시험 artifact만 보존한다.
아직 부트로더 실행 파일 연결·최종 map/WRP 검증·실기 boot를 완료한 것은 아니다.

## G474 단일 Flash 명령 — backend 연결 전

`platform/stm32g474/flash_command.c`는 새 framework 없이 한 page erase와 한8B program만
구현한다. [C 계약](../interface/canview_stm_flash_command.h)의 boot 전용 동기 호출이며
MCUboot IO primitive 연결은 아래 `flash_io.c`이며 최종 boot 실행은 아직이다. 상위 owner는 DMA·주변장치를
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

## G474 guarded Flash read — backend 연결 전

이 절의 read primitive는 이제 아래 IO adapter에서 호출한다. 최종 boot ELF 연결과
실기 수용을 완료했다는 뜻은 아니다.

`platform/stm32g474/flash_read.c`와 [C 계약](../interface/canview_stm_flash_read.h)은
primary/secondary에서1..256B를 읽는다. 임시 buffer에만 읽고 ECC가 없을 때만
호출자 buffer를 갱신한다. 단일 boot privileged MSP owner가 DMA와 다른 Flash 사용자를
멈춘 상태에서 호출하며 ISR/RTOS/reentry는 금지한다. RDP0와 기존 BSP guard, lock,
status/cache/ECC 상태를 먼저 확인하고 기존 오류를 임의로 지우지 않는다.

- `.canview_flash_read_ram`의 read/NMI/reset 세 함수와 literal을 SRAM1/2에 복사해야 한다.
  stack의512B 정렬 임시 vector를 VTOR에 설치하고 PRIMASK/cache를 보존·복원한다.
  출력도 SRAM1/2여야 한다. 최종 linker 배치·복사와 전체 call-chain stack 검사는 아직이다.
- RM0440 Rev9 p137의 DBANK1 ECCC/ECCD W1C 계약을 사용한다. 고정 CMSIS의 interrupt
  enable 이름은 `FLASH_ECCR_ECCIE`다. ECCC도 보수적으로 INCOMPLETE 처리한다.
  각 load 이후 RDERR 등 SR 오류도 확인해 INCOMPLETE로 반환하고 SR은 보존한다.
  예상 밖 BSY는 Flash 호출자로 돌아가지 않고 SRAM reset/fail-stop으로 처리한다.
  ECCD NMI는 현재 read의 실패 flag만 세우고 ECC flag를 clear한다. parsing, erase,
  logging, queue, allocation과 watchdog feed는 하지 않는다.
- NMI가 공유하는 armed/failed flag만 volatile이며 전역 mutable context는 없다.
  NMI는 임시 VTOR에서 context를 얻는다. load는 유한하고 ECCD 전달 대기는 최대31회다.
  다른 NMI/HardFault, clock/parity 동시 오류, flag clear 실패·전달 timeout은 reset 요청
  후 fail-stop이다. 실행 시간과 실제 NMI latency는 미측정으로 HIL 대상이다.
- ECCR 주소/은행 정보를 복구 erase 주소로 사용하지 않는다. 읽기 실패를 안전한 이미지
  선택·recovery로 연결하는 책임은 상위 backend/MCUboot에 남는다. 현재 단일 Flash 명령의
  program 사전 read를 이 함수가 자동 보호하지 않는다. IO adapter는 명령 전 guarded read를
  수행하지만 그 이후 새로 생기는 하드웨어 fault의 reset/fail-stop 경계는 여전히 남는다.

`ctest --preset host-debug -R 'stm32-flash-(read|ram)' --output-on-failure`는 전체 슬롯
98816개32bit word, byte alignment4개×length256개, per-load ECC195개, NMI 지연31개,
RDP256개, 범위/overflow·동시 fault·clear 실패·출력 불변을 검사한다. 실제 Arm post-build는
세 SRAM 함수/외부 relocation0/branch 범위를 검사한다. 모형은 실제 ECC 주입이나
예외 복귀 timing을 증명하지 않으며 physical ECC/reset/HIL은 NOT_RUN이다.

## MCUboot IO primitive 연결 — boot executable 연결 전

`platform/stm32g474/flash_io.c`는 기존 세 IO 함수만 구현한다. BSP 범위 검사를 재사용해
primary 또는 secondary 한 슬롯 안의 요청만 받고, boot/policy/config 쓰기를 열지 않는다.
read는256B 이하, write는8B, erase는2KiB 단위로 기존 primitive를 호출한다.
별도 journal·swap 알고리즘·heap·가변 전역 상태를 만들지 않는다.

- write는 guarded read로 erased 값을 확인하고 all-FF input은 program하지 않는다.
  명령 성공 뒤 guarded read-back을 대조한다. 실패하면 자동 retry/erase 없이 반환한다.
  이전 boot에서 torn word가 FF/ECC처럼 보일 때의 재개 결정은 상위 MCUboot/복구 책임이다.
- erase는 한 page 명령 뒤256B씩 전체 page read-back이 FF인지 확인한다.
  검증된 program/page 완료에서만 기존 progress hook을 호출한다. 이 hook 호출은
  무조건 watchdog feed가 아니며 아래 runtime의 시간·상태 조건을 함께 만족해야 한다.
- 큰 read가 실패하면 앞선 chunk가 destination에 남을 수 있으므로 호출자는 전체 출력을
  버려야 한다. 실패한 write/erase도 이전 단위의 효과가 남을 수 있다. API 단위 원자성을
  약속하지 않으며, 기존 MCUboot swap/trailer와 상위 재개 정책이 이를 소유한다.
- `stm32-flash-io`는 fake primitive로 chunk/단위 순서, 중복·FF skip, 모든 read-back
  위치의 오류, 잘못된 성공 데이터, 보호 영역 불변을 검사한다. `stm32-mcuboot-model`도
  같은 IO를 실제 `boot_go`에 link한다. 이전 별도 backend 결과를 소급 재분류하지 않고
  이 연결 뒤의 시험 결과로만 통합 여부를 판단한다.

Arm archive에서 실제 read/명령/IO를 컴파일하고 아래 SRAM link 시험으로 배치를 확인한다.
전체 stack·boot executable과 T-205 정책은 미완료다. SRAM 검사는 매 target build에 실행하며,
실제 Arm assembler로 만든 조건부 간접 분기·`blx lr` negative object도 거절한다.

## Boot SRAM 배치와 SDK 초기화 복사

`ld/STM32G474CEUx_BOOT.ld`는 boot64KiB, SRAM96KiB와 stack8KiB를 제한한다.
첫 dummy4B 뒤의 command/read SRAM code와 `.data`를 VMA/LMA 양쪽에서 연속 배치해
기존 Cube SDK Reset_Handler의 `_sidata` → `_sdata.._edata` word-copy를 재사용한다.
별도 복사 framework나 heap은 없다. `.bss`는 복사 뒤 초기화하며 preinit의
`startup_flash_ram.c`가 DSB/ISB를 실행한 다음 main으로 간다. 이 startup 전 SRAM
함수 호출이나 main 직접 진입은 허용하지 않는다. CCM 사용은 linker에서 거절한다.
RX/RW ELF segment는 분리하지만 MPU 보호를 설정했다는 뜻은 아니다.

`canview-boot-ram-link-test.elf/.map/.bin`은 실제 제품 Flash C와 SDK startup을
이 배치에 link하는 **비배포 시험 image**다. safe output과 watchdog은 연결했지만 boot_go, 정책, handoff가 없으므로
장치에 flash하지 않으며 최종 bootloader binary나 runtime 수용으로 계산하지 않는다.
App VTOR macro는 source-global이 아닌 target-local로 바꿔 boot0x08000000과
primary0x08010200의 SystemInit 컴파일을 분리했다. SDK 원본은 변경하지 않았다.

primary-debug/release build마다 `tests/ota/test_stm32_boot_ram.py`가 ELF/BIN load byte,
SRAM 함수/branch, copy/bss/stack symbol, 실제 SDK loop·VTOR·preinit을 검사한다.
startup/VTOR592bit·preinit32bit 변이와 비연속/정렬/누락 입력을 거절하고, 실제 Arm
linker의64KiB 초과·정렬·SRAM 크기·VMA gap·CCM 오류도 시험한다.
CI는 시험 산출물6개를 기존 target evidence manifest에 별도 이름으로 보존한다.
전체 MCUboot call-chain stack과 실제 SRAM 실행·ECC/reset/HIL은 아직 NOT_RUN이다.

## Boot 시간원과 watchdog — 최종 loader 연결 전

`platform/stm32g474/boot_runtime.c`는 SDK startup 이후의 privileged MSP main 단일
owner다. 기존 BSP `enter_safe_state` 성공 뒤 한 번만 시작하며, CAN/UART/DMA/IRQ를
시작하지 않는다. 앱의 PLL·SysTick·TIM2·worker watchdog을 가져오지 않고 reset HSI16,
DWT cycle counter, IWDG만 사용한다. API·실패 계약은
[`canview_boot_runtime.h`](include/canview_boot_runtime.h)에 있다.

- HSI16/분주 없음, boot VTOR, IPSR/CONTROL/PRIMASK/BASEPRI/FAULTMASK를 확인한다.
  DWT 실제 증가를 최대1024회 확인하고 LSI/IWDG 동기화는 각100만회로 제한한다.
  counter는 reset하지 않는다. 단일 wrap은 unsigned 차이로 처리하며, 정지나 역행,
  clock/config/context 오류·시간 만료는 실패로 latch한다. 초기화 실패도 재시도하지 않는다.
- IWDG prescaler256/reload4095를 설정하고 reset window4095는 쓰지 않는다.
  WINR 쓰기는 암묵 reload를 일으키므로 정상 reset값이 아니면 시작부터 거절한다.
  고정 CubeG4 v1.6.3 `stm32g4xx_hal_iwdg.c`의 PR/RLR→SR 대기→reload 순서를 따른다.
  LSI/동기화·전체 상태 확인 후 최초 한 번, 이후 MCUboot hash/검증된 Flash progress와
  nominal100ms 간격 조건을 만족할 때만 reload한다. ISR이나 준비 상태 조회는 feed하지 않는다.
- nominal30초 boot budget을 넘으면 progress가 계속 와도 feed를 중단한다. 이는 초기
  보수적 구현값이지 측정된 swap 시간이나 독립 wall-clock 증거가 아니다. DWT full-wrap
  전에 IWDG가 reset한다는 전제, debug halt/freeze, HSI/LSI 오차, Flash stall과 전체
  MCUboot 실행 시간은 실제 보드에서 확인해야 한다. 이 조건을 만족할 때까지 배포하지 않는다.
- 성공한 `boot_go`만으로 앱에 진입하지 않는다. 최종 caller가 정책/서명/vector와
  runtime readiness를 확인해야 한다. 아래 handoff primitive만 구현됐고 최종 연결은 남았다. 실패 시 safe output을
  유지하고 무조건 feed하지 않는다. runtime 자체가 recovery 정책이나 TX gate가 아니다.

`stm32-boot-runtime`은 같은 C의 초기화·중복 호출·partial init·ISR/context·clock 설정
변이·timeout·wrap·counter 정지·deadline·feed rate·fault latch를 register 모형에서 검사한다.
Arm link 시험은 기존 BSP safe output을 먼저 호출하고 runtime을 실제 SDK와 링크한다.
Host 모형은 IWDG 전기 동작·실제 reset/전원/HIL을 증명하지 않는다. 모두 NOT_RUN이다.

## 승인 뒤 primary 진입

`canview_boot_handoff()`는 기존 ECC read로 고정 primary의 첫8B를 읽고,
authenticated payload 길이·정확한 MSP(0x20018000)·Thumb reset PC의 payload 내부
범위를 검사한다. runtime readiness는 read 전후 확인하고 MPU/lazy FPU 활성은 거절한다.
추가 image format이나 승인 flag를 만들지 않는다. 서명과 T-205 영속 정책 판정은
caller 책임이며, 현재 link 시험은 길이0의 거절만 호출한다. 최종 bootloader가 아니다.

성공 경로는 IRQ를 막고 SysTick/NVIC enable·pending과 PendSV를 정리한 후 VTOR를
바꾼다. BSP가 HSI16/IWDG/safe output을 유지하며 caller가 DMA/Flash 변경을 금지한다.
마지막16B만 assembly로 LR=-1, MSP 교체, ISB, IRQ mask 해제, reset PC branch를 수행한다.
MSP 교체 이후 C stack 접근이 없음을 실제 Arm BIN 명령열과128개 bit 변이로 검사한다.
앱은 NVIC와 clock을 다시 초기화한다. NMI는 mask할 수 없으므로 VTOR/MSP 전환 도중
NMI·reset timing의 실기 확인이 남는다. 이 source 검사가 해당 physical gate를 닫지 않는다.

`stm32-boot-handoff`는 실제 handoff C에 runtime/read 오류를 주입하여 길이·vector
경계, MSP32bit 변이, read 실패, 전후 readiness/MPU/FPU, 실패 시 register 보존,
성공 시 cleanup 순서와 branch 인수를 확인한다. 대역은 서명·floor 승인이나
실제 interrupt controller를 모사하지 않는다. physical handoff/HIL은 NOT_RUN이다.

## SRAM reset 전제와 ECC errata

[ES0430 Rev9, 2024-06](https://www.st.com/resource/en/errata_sheet/es0430-stm32g471xx473xx474xx483xx484xx-device-errata-stmicroelectronics.pdf)
§2.2.7의 첫 SRAM write 손실에 대비해 앱 startup의 첫 `SystemInit` 호출을 linker
`--wrap`으로 받는다. 고정 CubeG4 startup 원본은 수정하지 않는다.
`platform/stm32g474/startup_ram.c`의44B naked wrapper가 stack/data 쓰기 전에
첫 SRAM1 cut의 전용 dummy(0x20000000,4B)를 두 번 초기화한다. 첫 쓰기 손실은
허용하며 각 DSB 뒤 두 번째 쓰기가 data/parity를 설정한다. Linker는 dummy를
앱 data와 분리한다. 나머지0x20008000/0x20010000/0x20014000은 읽은 뒤 SDK로 tail branch한다.
C prologue도 stack을 쓸 수 있어 이 진입부만 assembly를 사용한다.

고정 HAL의 `OB_SRAM_PARITY_ENABLE` 설명처럼 parity는 CCM뿐 아니라 SRAM1 첫32KiB에도
적용된다. 따라서 parity 설정을 바꾸거나 미초기화 parity 메모리를 먼저 읽지 않는다.
ecbce7d의 read-only wrapper는 이 조건을 놓쳤으며 현재 구현으로 수정했다.

`check_stm32_core.py`는 bench/primary 실제 BIN의 vector, MSP literal, Reset_Handler
첫 세 명령, dummy 두 write/DSB와 세 read의 명령열, SDK tail branch를 검사한다. CCM 초기화는
구현하지 않았으므로 linker의 CCM section은0B여야 한다. CCM 사용을 추가하려면
parity/reset 초기화를 별도로 구현해야 한다. Toolchain/SDK가 명령열을 변경하면
검사를 느슨하게 하지 않고 다시 검토한다. 실제 reset/전원 및 SRAM 실측은 NOT_RUN이다.
Host의 작은 접근 모형은 parity on/off와 네 cut의 첫 write 손실32조합을 시험하며,
이전 read-only 명령열의 parity 오류도 재현한다. 이는 CPU/버스 emulator나 실기 시험은 아니다.

같은 문서 §2.2.3은 중단된 Flash 작업 뒤 read 시 ECCR 정보 손상 가능성을 명시한다.
따라서 ECCR 주소만으로 지울 page를 선택하거나 유일한 정상 이미지를 자동 erase하는
복구는 허용하지 않는다. 실제 ECC-safe read와 소프트웨어 작업 범위에 근거한 복구는
아직 구현 중이며, 이번 startup 보완이 그 수용을 대신하지 않는다.
