# ESP native verifier SDK 빌드 fixture

[T-007](../../../docs/tasks/T-007-ota-container.md)의 read-only adapter를 실제
ESP-IDF6.0.3/RSA 서명 검증 설정으로 compile/link한다. 정상 장치 firmware나
서명된 CANView OTA 배포 이미지가 아니다. 이 fixture는 Flash 쓰기·키 생성·
provisioning을 하지 않으며 `app_main()`도 NULL negative만 호출한다.

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
검사기와 Communicator BSP 연결을 실제 target에 compile/link한다. NULL negative만
호출하므로 SDK→정책의 장치 실행이나 정상 app integration/영속 floor 완료 증거는 아니다.

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
