# ESP native verifier SDK 빌드 fixture

[T-007](../../../docs/tasks/T-007-ota-container.md)의 read-only adapter를 실제
ESP-IDF6.0.3/RSA 서명 검증 설정으로 compile/link한다. 정상 장치 firmware나
서명된 CANView OTA 배포 이미지가 아니다. 이 fixture는 Flash 쓰기·키 생성·
provisioning을 하지 않는다. `app_main()`은 adapter NULL negative 이후 공개 합성
golden을 기존 C prefix/manifest/body와 실제 SDK PSA provider로 검증하도록 연결한다.
이 경로의 장치 실행은 `NOT_RUN`이며 compile/link와 구분한다.

## 실행

저장소 루트 PowerShell에서 고정 SDK를 활성화하고 실행한다.

```powershell
. C:/cv/esp-idf-6.0.3/export.ps1
$otaImageBuild = Join-Path (Get-Location) 'build/idf-ota-image'
idf.py -C tests/fixtures/idf-ota-image -B $otaImageBuild build
```

`-B`에는 현재 checkout 아래의 절대 build 경로를 준다. `-B` 상대
경로는 `-C` 기준으로 해석되지 않는다. CI는 fixture 자체의 기본 build 디렉터리를 쓴다.
실제 ELF/MAP/BIN과 compiler/linker/CMake warning0를 확인한다.

`CONFIG_SECURE_SIGNED_ON_UPDATE=y`와 `CONFIG_SECURE_SIGNED_APPS_RSA_SCHEME=y`가
필수다. 자동 signing은 끄므로 이미지 자체의 서명 성공을 주장하지 않는다.
이 설정은 production Secure Boot/eFuse 구성이나 board sdkconfig의 대체가 아니다.

## 합성 native descriptor

`main/metadata.c`는 SDK의 `.rodata_custom_desc`에168B `CVIMG001`을 넣고 `-u`로
linker 제거를 막는다. C 구조 크기·문자열 offset·little-endian을 compile-time에 확인한다.
고정 합성 값은 Communicator/COMM_ESP, epoch7, ABI2, sequence `UINT64_MAX`,
`synthetic-board`/`synthetic-layout`이며 app version은 `1.2.3+4`다. 제품 identity나
배포 가능한 release가 아니다. 실제 signing/golden을 준비하는 시험 입력이다.

```powershell
python -B tests/ota/check_sdk_metadata.py build/idf-ota-image/canview_ota_image_sdk_probe.bin
```

SDK image header24B·첫 segment header8B·app descriptor256B 뒤, offset288의 실제
BIN byte열을 검사한다. metadata168개 전 byte 변이와4개 절단을 거절하는지도 검사하며
CI target build 뒤에도 실행한다. header/app/custom 위치 근거는 고정 SDK의
[custom descriptor 문서](https://github.com/espressif/esp-idf/blob/76f5dedd9950a3012fee8fb7d5586df21fc67802/docs/en/api-reference/system/app_image_format.rst)다.
이 검사는 metadata 위치/값만 대조한다. 전체 native 서명, MCU 실행 또는 Flash 설치의
검증이 아니다. production app descriptor 삽입과 신뢰 identity provider 연결은 남아 있다.

## 구현 경계

[ota_image.c](../../../firmware/platform/esp32s3/ota_image.c)는 BSP 전용 SDK adapter다.
실제 partition 객체·내부 Flash·범위·64KiB 정렬·signature sector 길이를 검증하고,
암호화 Flash의 staging도 암호화되어 있어야 한다. 전체 native file hash에는
signature sector가 포함되므로 SDK hash 함수의 DATA 모드를 사용한다. 별도 SHA나
ESP image parser를 만들지 않는다. SDK 전체 verifier 뒤에 app/custom descriptor를
읽고 실패하면 출력을 지운다. bootloader alias와 서명 비활성/0값/다른 scheme/FPGA
설정에서는 성공을 반환하지 않는다.

caller는 모든 bootloader_mmap/OTA 검증을 단일 task에서 직렬화하고 검증 도중
Flash 내용을 불변으로 유지해야 한다. adapter 자체는 mutex나 writer를 만들지 않는다.
반환하는 custom168B는 서명된 원본 byte열이며 CANView role/board/layout/ABI/u64
sequence 대조는 [공통 portable 검사기](../../../shared/ota/src/native_metadata.h)가 맡는다.
이 fixture는 SDK version/custom 배열 크기의 drift도 compile-time에 검사하고 공통
검사기와 Communicator BSP 연결을 실제 target에 compile/link한다. Native Flash adapter는
NULL negative만 호출하므로 SDK→정책의 장치 실행이나 정상 app integration/영속 floor 완료 증거는 아니다.

Communicator BSP는 generated board ID·고정 bundle_stage 주소/크기를 검사하고 SDK 성공
뒤 공통 metadata를 대조한다. [BSP 모형 시험](../../ota/test_comm_ota.c)은 SDK를 모형으로
대체하고 실제 BSP→metadata를 실행한다. 이미지 hash/RSA의 장치 실행 시험은 아니다.
generated bundle_stage CSV에는 SDK 암호화 reader와의 일치를 위해 `encrypted`를 넣었다.
OTA template의 복구 app label은 `recovery_app`이다. 기존 `recovery`는 SDK의
bootloader subtype 이름과 충돌해 경고를 내므로 label만 구분했다. app/test subtype과
각 보드 주소/크기는 그대로이며 실제 파티션 migration을 실행하지 않는다.
실제 writer·원자성·설치 상태 provider는 후속 T-007/T-205 연결에서 대조해야 한다.
현재 factory-only board 설치를 변경하지 않았다.
그 전에는 이 compile fixture를 설치·복구 완료로 표시하지 않는다.

Host의 [SDK 모형 시험](../../ota/test_esp_image_sdk.c)은 호출 순서·오류·범위·출력 정리
검증이지 실제 RSA 또는 장치 실행 증거가 아니다. physical/HIL은 `NOT_RUN`, 차량 TX는
`NO-GO`다. 전체 T-007 독립2인 리뷰·target 통합 gate도 남아 있다.

SDK 근거는 고정 commit `76f5dedd9950a3012fee8fb7d5586df21fc67802`의
[전체 file hash](https://github.com/espressif/esp-idf/blob/76f5dedd9950a3012fee8fb7d5586df21fc67802/components/bootloader_support/src/bootloader_common.c)와
[native verifier](https://github.com/espressif/esp-idf/blob/76f5dedd9950a3012fee8fb7d5586df21fc67802/components/bootloader_support/src/esp_image_format.c)다.

## 전체 컨테이너 수신 fixture

[공용 IDF component](../../../firmware/components/canview_ota_parser/CMakeLists.txt)는
기존 portable C99 parser를 그대로 사용한다. [receiver.c](main/receiver.c)는
31B prefix 부분 수신 뒤 manifest 서명/identity/floor 검사, 최대16KiB body chunk의
실제 SHA256을 수행한다. 정상·manifest 서명 변조·body 마지막 byte 변조·local identity
불일치의4개 기대 결과를 비교한다. 동일 C 흐름은 Windows `ota-idf-receiver`에서
실제 CNG로 실행한다. CNG 성공을 SDK PSA 장치 실행 성공으로 바꾸지 않는다.

읽기 전용 linker 영역에 보존 golden394310B를 넣는다. 공개 합성 root와 합성 floor0는
시험 전용이며 제품 신뢰 root/영속 policy가 아니다. Native 서명 검증이나 설치 승인을
반환하지 않고 Flash·PREPARED·boot selector를 호출하지 않는다. 기존 signed golden의
입력 SDK BIN은 provenance에 고정되어 있으며 이 fixture 변경 때 자동 재서명하지 않는다.

`app_main`은 단일 service/self-test 실행이며 추가 task/ISR/queue를 만들지 않는다.
SDK 기본 priority1, fixture 전용 stack16384B를 사용한다. 실제 ELF DWARF에서
prefix16488B/body856B/PSA context108B를 확인했으며 모두 함수 static storage다.
`-fstack-usage`는 자체 함수의 정적 frame 근거일 뿐 SDK 암호 호출 체인의 총 stack이나
WCET 증거가 아니다. SDK 내부 key/hash 할당은 실패 가능하며 provider가 오류와 cleanup을
처리한다. Main stack high-water, heap 최저 여유,4개 검증 시간, watchdog 지연은
장치가 없어 `NOT_RUN`이다. 기본 watchdog 설정은 끄거나 완화하지 않는다.
16384B는 시험용 예약량이며 측정으로 충분함을 입증한 값은 아니다.
