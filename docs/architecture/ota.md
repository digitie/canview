# 독립 OTA와 전원 차단 복구 설계

검토일: 2026-09-06. 대상: Controller, Communicator(ESP32-S3 + STM32G474), Diagnostic Bridge.

이 문서는 OTA 설계, 필요한 회로 변경, 구현 계약, 검증 조건과 독립 적대적 리뷰를 한 파일에 모은다. 펌웨어 구현 및 실물 전원 차단 시험은 후속 단계다.

## 1. 채택 방향과 적용 범위

각 장치가 자체 Wi-Fi AP와 업데이트 웹 페이지를 제공한다. 휴대폰의 기본 브라우저에서 서명된 업데이트 파일을 선택하면 된다. Controller가 없거나 Bridge가 꺼져 있어도 Communicator의 ESP32가 자체 업데이트와 STM32 업데이트를 수행한다. Bridge도 자기 AP에서 자신만 업데이트하며 차량 제어 권한을 얻지 않는다.

| 대상 | 정상 업데이트 | 첫 부팅 실패 | 정상 앱 둘 다 사용할 수 없을 때 |
|---|---|---|---|
| Controller ESP32 | 내부 `ota_0/ota_1` 교대 | 이전 앱으로 rollback | 어댑터 RECOVERY 버튼으로 별도 복구 앱 → 휴대폰 재업로드 |
| Communicator ESP32 | 내부 `ota_0/ota_1` 교대 | 이전 앱으로 rollback | RECOVERY 버튼 → 복구 앱이 ESP/STM 업데이트 제공 |
| Communicator STM32 | ESP가 UART로 내부 secondary 기록 → MCUboot 교체 | 시험 앱 미확정이면 revert | 보호된 부트로더가 UART 복구 수신; 필요 시 ESP가 STM만 reset |
| Bridge ESP32 | 내부 `ota_0/ota_1` 교대 | 이전 앱으로 rollback | 기존 PAIR 버튼을 reset 중 길게 눌러 복구 앱 |

권고 구성은 **외장 SPI NOR 없이** ESP 이중 앱 슬롯과 STM 내부 교체 슬롯을 사용한다. 부트로더와 파티션표의 최초 설치는 안정된 작업대 전원과 유선 도구로 수행한다. 이후 일반 OTA에서는 이 영역과 복구 앱을 갱신하지 않는다. ESP-IDF도 앱 OTA와 달리 부트로더·파티션표의 교체는 전원 차단에 취약한 작업으로 구분한다. [ESP-IDF v6.0 OTA](https://docs.espressif.com/projects/esp-idf/en/v6.0/esp32s3/api-reference/system/ota.html)

여기서 안정성은 전원 차단 뒤 유효한 이전 앱으로 부팅하거나, 안전하게 업데이트를 재개하거나, 서명된 파일을 다시 받을 수 있는 상태로 남는다는 뜻이다. 전원이 계속 흔들리는 동안 정상 동작하거나 물리 Flash 고장까지 소프트웨어로 복구한다고 보장하지 않는다. 보장 범위는 §12 시험으로 입증한다.

이번 변경은 회로 생성 입력과 KiCad 산출물에 적용한다. ESP-IDF 프로젝트, STM linker, 실제 부트로더·웹 서버·서명 키 provisioning은 아직 구현하지 않는다. 이 문서의 Flash 표는 후속 구현 계약이며 현재 CSV가 이미 OTA용이라는 뜻이 아니다. 사용자의 단일 MD 요청에 따라 별도 리뷰 MD/인덱스/태스크 파일을 추가하지 않고 본문에 기록한다.

## 2. 기준선과 현재 제약

하드웨어 비교 기준은 `cffa3373e2f9f63291f8e89ebeddaa13dfe0fb70`의 R1이다. [도구 잠금 파일](../../tools/toolchain-versions.json)은 ESP-IDF `v6.0.3` / `76f5dedd9950a3012fee8fb7d5586df21fc67802`, STM32CubeG4 `v1.6.3`를 고정한다. 본문에서는 v6.0 공식 문서와 v6.0.3 Kconfig 원문을 대조했다. 움직이는 `stable/latest` 문서의 새 기능을 현재 SDK에 있다고 가정하지 않는다.

| 항목 | 확인한 현재 상태 | OTA에 미치는 영향 |
|---|---|---|
| Controller | ESP32-S3, Flash16MiB, PSRAM8MiB; Waveshare 보드 | 카메라 미사용 variant의 GPIO41을 복구 버튼에 사용 |
| Communicator | 기존 MINI-1-N4R2에서 **WROOM-1-N16R8**로 변경 | Flash16MiB/Octal PSRAM8MiB; ECC 사용 시 가용7.5MiB |
| STM32 | G474CEU6 Flash512KiB, 기존 linker가 전체 Flash 사용 | 부트로더/슬롯 도입 시 최초 유선 재설치 필요 |
| Bridge | WROOM-1-N8R2, PAIR GPIO4, BOOT GPIO0 | PAIR 재사용으로 추가 회로 없이 복구 진입 가능 |
| 기존 ESP 파티션 | Controller/Comm 모두 단일 `factory`, `otadata` 없음 | 현재 설치본에서 파티션표를 OTA로 바꾸지 않음 |
| 기존 reset | ESP CHIP_PU와 STM PG10-NRST가 `SYS_RESET_N` 공유 | ESP가 STM reset을 당기면 자기 자신도 reset되는 구조를 수정 |
| 전원 | Communicator의 CAN PHY는 차량 전원만 사용 | USB 단독 서비스 시 MCU는 켜지고 CAN PHY는 무전원 |

하드웨어 상세의 기존 reset/허용식은 이번 OTA 변경에 한하여 §6과 생성된 netlist로 대체된다. [R1 회로](../hardware/r1/communicator-circuit.md), [생성 입력](../../tools/hardware/ota_circuits.py), [Controller 헤더](../hardware/controller.md)를 함께 참조한다. 이전 문서의 GPIO43/44 표기보다 R1에서 공식 회로와 대조한 실제 헤더 매핑을 우선하며, 이번 버튼은 UART를 사용하지 않는다.

## 3. 휴대폰 업데이트 절차와 통신 선택

1. 정차 후 장치의 업데이트 화면/서비스 버튼에서 세션을 연다. Communicator는 J31 RUN shunt를 제거한다. 이 커넥터는 케이스 밖 서비스 덮개에서 손으로 조작 가능해야 한다. 전용 앱·드라이버·분해용 도구를 요구하지 않는 기구 설계를 수용 조건으로 둔다.
2. Communicator/Bridge는 C-to-C 5V, 1.5A 이상을 광고하는 충전기/보조배터리를 권고한다. 현재 R1은 A-to-C/default-current 전원만으로 켜지지 않는다. Controller는 Waveshare 전원 입력을 사용한다. 차량 전원과 USB 사이 자동 전환은 도움이 되지만 무중단 보장은 시험 전에는 하지 않는다.
3. 휴대폰을 `CANView-<role>-<short-id>` AP에 연결하고 기기 라벨/화면의 고유 암호를 입력한다. 인터넷 없음 안내에서 연결을 유지한다. 브라우저에서 `http://192.168.4.1`을 연다. QR/captive portal/mDNS는 편의 기능이며 이 고정 주소 경로도 동작해야 한다.
4. 공식 배포처에서 미리 받은 `.cvota` 파일을 선택한다. 파일 선택창, 진행률, 현재/대상 버전, 업데이트 결과만 노출한다. 별도 코드 입력이나 콘솔은 필요 없다.
5. 파일 검증 후 사용자가 설치를 누른다. 화면은 `전송 중 → 검증 중 → 설치 중 → 재시작 중 → 완료/이전 버전 복원`을 구분한다. 전송 100%를 설치 성공으로 표시하지 않는다.
6. 재접속하면 장치에서 읽은 결과를 표시한다. 휴대폰이 잠자기/화면 닫기/통신 끊김 상태여도 이미 검증·준비 완료된 교체는 장치가 수행한다. 전송이 미완성이면 기존 앱을 보존하고 재업로드를 요청한다.
7. Communicator는 완료 후 J31을 다시 연결하고 서비스 모드를 종료한다. 새 handshake·local safety check·새 control lease 없이 CAN TX를 자동 재개하지 않는다. 실패한 업데이트를 같은 부팅에서 무한 자동 재시도하지 않는다.

STA + HTTPS 다운로드는 선택적 후속 기능이다. AP 기본 경로는 인터넷·DNS·RTC·Controller·Bridge에 의존하지 않는다. 휴대폰 hotspot 자체에 연결한 장치에 그 휴대폰이 항상 접속할 수 있다고 가정하지 않는다. BLE는 초기 설정에만 선택적으로 검토하며 Web Bluetooth 지원 차이 때문에 필수 업데이트 경로로 삼지 않는다. ESP-NOW로 펌웨어 전체를 중계하는 기능은 v1 범위에서 제외한다. 업데이트 중에는 해당 장치의 ESP-NOW raw/capture를 중지해 단일 radio의 channel 충돌을 피한다.

Controller OTA는 Communicator에 서비스 상태를 알리고 제어 lease를 반납한다. 상대가 없더라도 Controller 자체 업데이트는 가능하다. 상대 응답이 없으면 마지막 명령을 재전송하지 않으며 Communicator의 자체 lease 만료가 적용된다. Communicator OTA는 J31 하드웨어 차단을 먼저 확인하므로 Controller 응답에 의존하지 않는다. Bridge는 오직 자신의 observer 세션만 종료한다.

## 4. ESP 부팅·Flash 배치

### 4.1 정상 앱과 복구 앱

ESP-IDF `CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE=y`로 시험 부팅 후 명시적으로 확정한다. 앱의 `app_main()` 도달이나 Wi-Fi 송신 callback만으로 확정하지 않는다. 내부 메모리 검사, 설정 snapshot 판독, watchdog, 역할별 최소 장치 검사를 통과한 뒤 `esp_ota_mark_app_valid_cancel_rollback()`을 호출한다. USB 서비스 중 CAN PHY가 꺼져 있거나 상대 Controller가 없는 것은 자체 OTA 실패 사유가 아니다.

복구 앱은 별도 ESP-IDF 프로젝트로 빌드한 `app/test` 이미지다. `CONFIG_BOOTLOADER_APP_TEST=y`, active-low, hold5초를 사용한다. GPIO는 Controller41, Communicator8, Bridge4다. GPIO0은 ROM download strap이므로 이 용도로 쓰지 않는다. 복구 앱은 LCD·SD·PSRAM·정상 앱 NVS schema 없이 AP/서명 검증/업로드/상태 조회/Communicator STM UART 복구를 제공한다. 장치 고유 AP 자격증명은 별도 provisioning 영역의 검증된 두 사본으로 읽고, 모두 손상되면 라벨 secret을 별도 복구 절차로 입력받는 정책을 제조 단계에서 확정해야 한다. 무암호 AP로 자동 전환하지 않는다. [ESP-IDF bootloader](https://docs.espressif.com/projects/esp-idf/en/v6.0/esp32s3/api-guides/bootloader.html)

`test`라는 subtype 자체가 쓰기 보호는 아니다. 공통 flash writer에서 bootloader/partition table/test/provisioning 영역 쓰기를 거부하고, 업데이트 manifest target allowlist에서도 제외한다. ESP Secure Boot는 서명 검증 기능이며 정상 서명 앱의 임의 Flash 쓰기를 막는 메모리 방화벽으로 주장하지 않는다. 최종 앱의 취약점으로 전체 Flash가 손상되는 경우까지 A/B가 보호하지는 않는다.

### 4.2 제안 주소표

주소는 ESP 외부 부팅 Flash의 offset, 길이는 바이트, 구간 끝은 미포함이다. 모든 app은 64KiB 경계, data는 4KiB 경계를 따른다. 선두 256KiB는 공통이며 아래 정본을 토대로 CSV를 생성한다.

| 공통 영역 | 시작 | 길이 | 계약 |
|---|---|---|---|
| bootloader + 예약 | `0x000000` | `0x018000` | Secure Boot 서명 크기까지 확인; 실제 ROM/IDF 크기 상한도 별도 검사 |
| partition table | `0x018000` | `0x001000` | OTA에서 수정 금지 |
| nvs | `0x019000` | `0x006000` | 비권위 캐시만; 자동 전체 erase 금지 |
| otadata | `0x01F000` | `0x002000` | IDF 전용 이중 sector |
| phy_init | `0x021000` | `0x001000` | RF 초기 데이터 |
| nvs_keys | `0x022000` | `0x001000` | 암호화 키 영역 |
| OTA journal A/B | `0x023000` | `0x002000` | 서로 다른4KiB sector |
| config A/B | `0x025000` | `0x008000` | 각각16KiB; version/CRC/commit record |
| provisioning A/B | `0x02D000` | `0x008000` | 각각16KiB; normal OTA 쓰기 금지 |
| 예약 | `0x035000` | `0x00B000` | 앱 시작 `0x040000` |

| 장치 | recovery test | ota_0 | ota_1 | 일반 데이터 |
|---|---|---|---|---|
| Controller16MiB | `0x040000 + 0x100000` | `0x140000 + 0x400000` | `0x540000 + 0x400000` | `0x940000 + 0x6C0000` |
| Communicator16MiB | `0x040000 + 0x200000` | `0x240000 + 0x400000` | `0x640000 + 0x400000` | `0xA40000 + 0x5C0000` |
| Bridge8MiB | `0x040000 + 0x100000` | `0x140000 + 0x280000` | `0x3C0000 + 0x280000` | `0x640000 + 0x1C0000` |

CI 기준은 서명·정렬 padding 포함 이미지 길이 ≤ 슬롯 길이, 그리고 성장 여유128KiB 이상이다. Communicator 정상 앱은3968KiB 이하, 복구 앱은1920KiB 이하를 설계 목표로 둔다. Controller/Bridge 복구 앱은896KiB 이하가 목표다. 실제 완성 이미지 크기는 후속 build gate다. UI/웹 자산은 해당 앱에 포함해 함께 교체하며, 공유 파일시스템을 갱신해야 한다면 versioned immutable object + 이중 manifest를 사용한다. 파일 전체를 제자리 덮어쓰는 업데이트는 금지한다.

사용자가 더 큰 모듈과 N16R8을 지정해 WROOM-1-N16R8을 채택했다. MINI의4MiB 제한안은 역사적 비교안이며 새 회로에 적용하지 않는다. WROOM은18.0×25.5mm 본체와 antenna keepout을 반영해야 하므로 기존70×45mm 보드 목표의 성립을 다시 검토한다. GPIO35/36/37은 R8 Octal PSRAM에 연결되어 외부 사용 금지다. 기존 MINI GPIO33의 mux sense는 WROOM GPIO38/pad31로 옮긴다. PSRAM은80MHz Octal + ECC를 설정하며120MHz 실험 모드를 쓰지 않는다. 공식 R8 온도 범위는 기본 최대65°C, ECC 사용 시 최대85°C 조건이며 실제 보드 열시험을 대체하지 않는다. ECC는8MiB의1/16을 사용한다. [Espressif module 사양](https://documentation.espressif.com/esp32-s3-wroom-1_wroom-1u_datasheet_en.html), [v6.0.3 PSRAM Kconfig](https://github.com/espressif/esp-idf/blob/v6.0.3/components/esp_psram/esp32s3/Kconfig.spiram)

Communicator 일반 데이터 영역 중 `0xA40000 + 0x480000`(4.5MiB)은 한 개의 수신 bundle staging, 나머지 `0xEC0000 + 0x140000`은 비권위 일반 데이터로 배정한다. 최대 정상 ESP 이미지4MiB + STM180KiB + manifest16KiB가 staging에 들어간다. cache는 활성 앱/설정/유일한 정상본과 분리한다. 다운로드 시작 시 이전 staging을 지워도 현재 부팅에는 영향이 없어야 한다. 전체 hash·서명 검증 후에만 PREPARED journal을 commit한다. 재부팅 후 cache를 다시 검증하며 cache 손상 시 기존 호환 앱 조합을 유지하고 재업로드를 받는다.

## 5. STM32 부트로더와 ESP 제어

### 5.1 선택과 메모리

MCUboot의 `swap using offset` 방식으로 primary 고정 실행 주소를 유지한다. primary/secondary에 old/new 이미지를 보존하고 교체 상태를 기록해 재개한다. scratch 방식이나 `BFB2` option-byte 변경으로 bank를 뒤집는 방식은 채택하지 않는다. MCUboot는 이 MCU를 위한 완제품이 아니므로 CMake/bare-metal G474 port의 flash map, signature, fault handling을 구현·검증해야 한다. 검토 시 공식 최신 release는 v2.4.0이며 도입 시 정확한 commit을 lock한다. [MCUboot 설계](https://docs.mcuboot.com/design.html), [port 계약](https://github.com/mcu-tools/mcuboot/blob/main/docs/PORTING.md), [v2.4.0](https://github.com/mcu-tools/mcuboot/releases/tag/v2.4.0)

DBANK=1의 2KiB page를 전제로 한다. production option-byte profile을 최초 유선 provisioning 때 읽어 확인하고 OTA 중 변경하지 않는다. DBANK/WRP/NRST 설정이 다르면 erase 이전에 실패한다. [ST RM0440](https://www2.st.com/resource/en/reference_manual/dm00355726-stm32g4-series-advanced-armbased-32bit-mcus-stmicroelectronics.pdf)

| STM Flash 영역 | 시작 주소 | 길이 | 용도 |
|---|---|---|---|
| bootloader | `0x08000000` | `0x10000` /64KiB | signature 검증, UART 복구, swap; WRP 보호 |
| primary | `0x08010000` | `0x30000` /192KiB | 실행 이미지 + MCUboot header/TLV/trailer |
| secondary | `0x08040000` | `0x30800` /194KiB | offset swap의 추가 page 포함; 새 이미지 시작 `0x08040800` |
| policy/journal A | `0x08070800` | `0x1000` /4KiB | transaction·호환성 기록 |
| policy/journal B | `0x08071800` | `0x1000` /4KiB | 이전 세대 보존 |
| config A | `0x08072800` | `0x4000` /16KiB | 설정 snapshot |
| config B | `0x08076800` | `0x4000` /16KiB | 시험 앱용 snapshot |
| 예약 | `0x0807A800` | `0x5800` /22KiB | 끝 `0x08080000` |

MCUboot header는512B를 기준으로 linker vector 위치 `0x08010200`을 사용한다. `imgtool`과 port의 실제 sector 수·trailer·signature TLV 산식으로 최대 payload를 검증한다. 서명 이미지 전체180KiB 이하를 초기 gate로 두고, bootloader64KiB 이내도 실제 map 파일로 확인한다. 192KiB를 모두 C code에 사용할 수 있다고 계산하지 않는다.

### 5.2 부트·복구 순서

1. ESP는 `ESP_RUN_OK=0`을 유지하고 J31 제거를 확인한다. UART runtime queue와 control lease를 종료한다.
2. `STM_RECOVERY_N`을 LOW로 당기고 `STM_RESET_CMD_N`을 최소10ms LOW → HIGH로 해제한다. PB9 recovery 요청은 부트로더 HELLO를 받을 때까지 유지한다. BOOT0는 LOW다.
3. STM 부트로더는 CAN peripheral·ARM_EDGE를 활성화하지 않는다. UART2를115200 8N1, flow control 없이 시작한다. 검증된 프레임 단위 stop-and-wait로 수신하고, 완전한 flash operation 뒤 ACK한다. 일반 앱의4Mbps/RTS/CTS 상태를 이어받지 않는다.
4. 서명과 board ID를 검증한 manifest를 먼저 받아 secondary만 지운다. chunk 완료·read-back·전체 hash·image signature 검사 후에만 TEST pending 상태를 기록한다.
5. 재부팅 후 부트로더가 swap을 마치고 후보 앱을 실행한다. 단전 시 swap 기록에 따라 계속 진행한다. 후보 앱은 자체 검사와 ESP 관리 프로토콜 호환성 확인 뒤 확정한다. 실패/hang/reset은 revert한다.
6. pending TEST 상태가 없고 유효 primary가 있으면 그것을 부팅한다. 복구 요청 또는 유효 앱 부재이면 서명된 파일을 기다린다. 손상된 journal에서 임의 주소로 jump하거나 bootloader를 자동 erase하지 않는다.

SWD가 STM을 reset해도 ESP reset net은 올라가 있어야 한다. STM 내부 reset 시 PG10-NRST 출력 동작은 option-byte profile로 보장하고 검증한다. IWDG는 후보 앱 hang 복구용으로 사용하며 외부 TPS3431의 CAN latch 차단과 구분한다. TPS3431은 STM을 재부팅시키는 장치가 아니다.

G4 Flash는 doubleword/ECC 제약이 있으므로 기록을8B 정렬하고 같은 doubleword를 부분 재프로그램하지 않는다. 전원 차단 중 ECC 오류를 단순 CRC mismatch로만 처리하지 말고 NMI/Flash 오류를 안전 복구로 연결한다. bank1 쓰기 중 bootloader 실행·ISR fetch stall, 최악 page erase 시간, IWDG window를 port에서 분석한다. 필요한 flash critical code/벡터는 SRAM에 배치하고 `map`으로 검증한다. watchdog을 무기한 끄거나 인터럽트에서 무조건 feed하지 않는다.

### 5.3 ROM BOOT0 경로의 범위

BOOT0는 주 OTA 경로가 아니라 부트로더 최초 설치/물리 서비스용이다. J32 shunt가 없으면 ESP의 BOOT0 요청은 STM에 연결되지 않는다. J31 RUN 연결 상태에서는 추가 AND gate가 BOOT0 요청을 LOW로 강제한다. STM ROM UART를 사용할 때는 AN2606에서 G47/G48·UFQFPN48의 USART2 PA2/PA3 경로와 option-byte boot pattern을 확인하고, AN3155의 autobaud `0x7F`, 8E1, ACK/NACK를 사용한다. custom recovery의8N1/COBS와 섞지 않는다. [AN2606](https://www.st.com/resource/en/application_note/an2606-introduction-to-system-memory-boot-mode-on-stm32-mcus-stmicroelectronics.pdf), [AN3155 Rev21](https://www.st.com/resource/en/application_note/cd00264342-usart-protocol-used-in-the-stm32-bootloader-stmicroelectronics.pdf)

일반 웹/API에는 raw ROM 명령·mass erase·option-byte 수정·RDP 해제를 노출하지 않는다. WRP가 보호하는 부트로더를 ROM으로 다시 쓰려면 보호 해제가 필요하며 데이터 소실 가능성이 있다. 이 작업은 통제된 유선 서비스이고 자동 OTA 복구의 성공 경로로 계산하지 않는다.

## 6. 실제 회로 변경과 핀 계약

회로 정본은 [ota_circuits.py](../../tools/hardware/ota_circuits.py), 연결된 기존 core/power/sensor 생성 입력이다. 추가 부품은 U17 TLV803EA30DPWR, U50 SN74LVC1G07DBVR, U52/U53/U55 SN74LVC1G11DCKR, U54 SN74LVC1G04DBVR, J31/J32 및 pull/decoupling/복구 버튼이다. [TI open-drain buffer](https://www.ti.com/lit/ds/symlink/sn74lvc1g07.pdf)의 DBV pin2=A, pin4=Y(open-drain)를 사용한다. Push-pull 출력으로 NRST를 HIGH 구동하지 않는다.

### 6.1 Reset 분리

U6(ESP)와 U17(STM)은 같은3V3를 각각 감시하고 MR만 `GLOBAL_RESET_N`에 연결한다. 각 reset 출력에는 개별10k pull-up이 있다. 공통 RESET 버튼은 두 MR을 당긴다. U50은 ESP GPIO1의 active-low 요청을 STM reset에 open-drain으로 전달한다. STM의 SWD reset, 내부 reset, U17 출력이 ESP CHIP_PU를 당기는 직접 연결은 제거한다. 모든 reset 관련 test point도 분리한다.

| 기능 | ESP WROOM GPIO / 물리 pad | STM 물리 연결 | reset/default |
|---|---|---|---|
| STM reset 요청 | GPIO1 /39 | U50 → PG10-NRST /7 | 외부10k PU; HIGH=해제 |
| ROM BOOT0 요청 | GPIO2 /38 | U55 →1k →J32 →PB8 /46 | 외부10k PD; J32 open; STM BOOT0도10k PD |
| 정상 동작 허용 | GPIO7 /7 | U53 입력 | 외부10k PD; OTA/boot/recovery에서는0 |
| ESP 복구 버튼 | GPIO8 /12 | 해당 없음 | 외부10k PU; 버튼=LOW |
| STM custom 복구 요청 | GPIO9 /17 | PB9 /47 | 외부10k PU; ESP open-drain LOW=요청 |
| 서비스 인터록 감지 | GPIO48 /25 | J31/U52 | 외부10k PD; J31 없으면0 |
| ESP reset | CHIP_PU /3 | U6 출력 | `ESP_RESET_N` |
| STM reset | 해당 없음 | PG10-NRST /7, SWD J10/10 | `STM_RESET_N` |

기존 UART는 ESP GPIO17TX→STM PA3RX, GPIO18RX←PA2TX, GPIO15RTS→PA0CTS, GPIO16CTS←PA1RTS를 유지한다. 각 physical pad와33Ω 직렬/flow-stop pull은 생성 pinmap에서 대조한다. STM reset 제어를 위해 UART pin을 재배정하지 않는다.

### 6.2 CAN 하드웨어 차단

`SERVICE_RUN`은 J31에 shunt가 있을 때만1이다. J30 `TX_ARM`은 별개로 기존 차량 TX 허용용이다. J31 제거 시 RX mode까지 차단해 모든 PHY가 안전한 기본 상태로 수렴한다. ESP가 GPIO7을 잘못 HIGH로 유지해도 J31 제거는 하드웨어적으로 유효하다.

```text
MCU_HEALTH_N = ESP_RESET_N & STM_RESET_N & SERVICE_RUN
RUN_ALLOWED = MCU_HEALTH_N & ESP_RUN_OK & PHY_RESET_N
RX_ALLOWED = AUTO_GOOD & RUN_ALLOWED & PHY_RESET_N
ARM_HEALTH_N = WD_OK_N & RUN_ALLOWED & AUTO_GOOD
ARM_CLEAR_N = ARM_HEALTH_N & PHY_RESET_N & TX_ARM
ARM_LATCH = DFF(D=1, CLK=STM_ARM_EDGE, asynchronous_clear=ARM_CLEAR_N)
TX_PERMIT = ARM_LATCH & ARM_CLEAR_N & RX_ALLOWED
GPS_PWR_EN = GPS_PWR_REQ & AUTO_GOOD & RUN_ALLOWED
```

U52/U53은 PHY3V3로 구동하며 해당 logic의 Ioff와 기존 FT active-high OE의1k pull-down을 유지한다. PHY3V3가 없을 때의 정적 차단과 전원 하강 중 지연은 구분해 시험한다. J31을 다시 끼우거나 watchdog이 회복되어도 지워진 latch는 새 STM ARM edge 없이는 올라가지 않는다. 서비스 종료 시 stale ARM edge/queue를 폐기한다. 펌웨어 미구현 상태의 GPIO7 기본 LOW는 정상 CAN RX도 차단한다. 이 인터록을 우회하는 임시 pull-up을 추가해서 bring-up하지 않는다.

### 6.3 Controller와 Bridge

Controller 어댑터 J1/13(원보드 J8/13 GPIO41)에10k pull-up과 RECOVERY 버튼, J1/22 RESET에 sink-only HOST_RESET 버튼을 추가한다. 카메라는 분리·비활성화한 variant만 허용한다. GPIO38/39/40 마이크 LVDS 회로와 충돌하지 않는다. 복구 앱이 정상 LCD driver에 의존하지 않도록 IP/암호/버튼 절차를 제품 라벨에 남긴다.

Bridge는 기존 GPIO4 PAIR와 RESET을 재사용하므로 OTA용 추가 IC가 필요 없다. reset 중 PAIR5초는 복구 앱, 정상 실행 중 PAIR3초는 commissioning으로 명확히 구분한다. GPIO0 BOOT는 ROM download다. Bridge의 회로와 권한에 차량 TX 경로를 추가하지 않는다.

## 7. 패키지·인증·보안 계약

패키지는 압축 archive를 해제하는 구조 대신 작은 길이 제한 manifest와 순차 image blob을 갖는 `.cvota` 컨테이너다. 파서가 임의 파일 경로·외부 URL·Flash 절대주소를 받아들이지 않는다. v1은 최대3개 image, manifest16KiB, 개별 blob 길이는 대상 슬롯 이하, HTTP chunk16KiB 이하로 제한한다. Controller/Bridge 단일 image, Communicator는 ESP/STM 최대2개를 선택한다.

| 서명 manifest 필드 | 의미와 검증 |
|---|---|
| `format_version`, `package_id` | version1,128bit ID; 지원하지 않는 버전 거절 |
| `role`, `board_revision`, `layout_id` | 대상 장치·회로·파티션 일치; 사용자 입력으로 override 금지 |
| `release`, `security_epoch`, `key_id` | 역할별 허용 서명키·보안 세대; 임의 최신 문자열 비교 금지 |
| `images[]` | enum target, byte length, SHA-256, firmware version, image signature |
| `compatibility` | ESP/STM/peer의 지원 ABI 범위와 허용 조합 목록 |
| `config_schema` | 읽을 수 있는 schema 범위와 새 snapshot schema |
| `requires` | 최소 bootloader/recovery ABI, hardware capabilities |

정규화된 CBOR manifest의 정확한 byte열을 서명하고, duplicate key·unknown critical field·길이 overflow·중첩 제한 초과를 거절한다. 컨테이너 header의 lengths와 signed lengths를 비교한다. 역할/board/layout/호환성 검증 뒤에만 비활성 슬롯을 지우고, 전체 image 검증 전에는 부팅 표시를 변경하지 않는다. unsigned CRC는 전송 손상 검사일 뿐 인증이 아니다.

ESP는 production Secure Boot V2 + Flash Encryption, STM은 부트로더 내 공개키로 ECDSA-P256/SHA-256 image 검증을 기본으로 한다. manifest 서명은 ECDSA-P256으로 별도 검증한다. STM은 ESP의 검증 결과만 신뢰하지 않고 자기 signed protected TLV의 board/role/ABI와 image hash·서명을 다시 검사한다. Flash 주소는 bootloader의 enum→고정 map으로만 결정한다. private signing key를 장치·웹·Git에 넣지 않는다. dev와 production root는 분리하고 Bridge 서명키로 Communicator 이미지를 허용하지 않는다.

`test` 복구 앱과 ESP 표준 hardware anti-rollback은 함께 사용할 수 없다. 따라서 이번 availability 우선 baseline은 `CONFIG_BOOTLOADER_APP_ANTI_ROLLBACK=n`이며 OTA에서 eFuse security epoch/키 폐기를 변경하지 않는다. 같은 security epoch 내에서 이전 정상 앱 복귀를 허용하고, 일반 웹의 자의적 downgrade는 signed manifest 정책으로 제한한다. 이는 물리 공격까지 막는 단조 hardware anti-rollback 보장이 아니다. 강한 anti-rollback이 요구되면 test 복구 구조·키 관리까지 별도 재설계해야 한다. [v6.0.3 test Kconfig](https://github.com/espressif/esp-idf/blob/v6.0.3/components/bootloader/Kconfig.projbuild), [v6.0.3 rollback Kconfig](https://github.com/espressif/esp-idf/blob/v6.0.3/components/bootloader/Kconfig.app_rollback)

보안 기능의 최초 eFuse/option-byte provisioning은 전원이 안정된 작업대에서만 한다. Secure Boot를 켰다는 이유로 ROM/SWD 복구가 항상 가능하다고 표시하지 않는다. UART secure download/JTAG/RDP/WRP 조합과 서명된 복구 앱까지 시험한 뒤 production으로 잠근다. [Espressif provisioning](https://docs.espressif.com/projects/esp-idf/en/v6.0/esp32s3/security/security-features-enablement-workflows.html)

AP는 장치별 강한 WPA2 암호, 물리적으로 연10분 세션,1개 관리 client를 기본으로 한다. HTTP 로컬 경로는 공용 인터넷에 노출하지 않는다. 업데이트 요청은 로그인 세션·난수 CSRF token·Origin/Host allowlist·Content-Type·request size 검사를 거친다. CORS를 열지 않고 STA에서 수신 업데이트 포트는 기본 닫는다. 상태/log 내 token/키/VIN/좌표/capture를 노출하지 않는다. 타임아웃은 monotonic clock으로 재며 RTC 시간이 틀려도 오프라인 서명 검증이 가능하다. 온라인 HTTPS 인증서 검증을 RTC 문제 때문에 끄지는 않는다.

## 8. 응답 확인·멱등성·전송 중단

브라우저 HTTP200, Wi-Fi 전송 성공, UART ACK, 이미지 검증, 재부팅 성공, 최종 확정은 서로 다른 상태다. 상태 API에는 `device_id`, `boot_id`, `transaction_id`, `package_hash`, `target`, `phase`, `bytes_verified`, `running_version`, `confirmed_version`, `result`, `error_code`를 포함한다. 웹은 서버의 이 값으로 진행률을 복원한다.

| API | 기능 | 완료 응답 조건 |
|---|---|---|
| `GET /api/ota/v1/status` | 역할/설치 상태/직전 결과 | 민감 정보 제외한 현재 장치 상태 |
| `POST /api/ota/v1/transactions` | signed manifest 검증·exclusive session 생성 | target/전원/서비스/공간 검사 후 `READY` |
| `PUT /api/ota/v1/transactions/{id}/images/{target}` | 순서대로 chunk 수신 | offset·chunk hash 확인, Flash write/read-back 완료 |
| `POST .../{id}/prepare` | 전체 이미지/호환성 검증 | 각 대상이 실제로 `PREPARED` |
| `POST .../{id}/activate` | 시험 부팅 승인 | `ACCEPTED`만 반환; 성공 표시는 후속 STATUS |
| `POST .../{id}/cancel` | 준비 전/후 취소 | swap/부팅 전환 시작 뒤에는 `TOO_LATE`, 강제 erase 금지 |

전체 HTTP retry는 같은 idempotency key·transaction·package hash로 수행한다. 같은 요청의 재전송은 저장된 결과를 돌려주며 다른 payload를 같은 ID로 보내면 `CONFLICT`다. 진행 중 다른 업데이트는 `BUSY`다. 인증·CRC·schema·hash 오류는 자동 재시도하지 않는다. Communicator의 HTTP WRITE는 먼저 내부 staging에 기록하고 PREPARE는 bundle 검증 완료를 뜻한다. 이후 각 MCU에 쓰는 PREPARE/시험 부팅 결과는 target별 phase로 구분한다. Controller/Bridge는 직접 비활성 앱 slot에 받는다.

STM custom recovery UART 프레임은 COBS + `0x00` delimiter, little-endian 정수, CRC32C다. header는 `magic:u16, version:u8, type:u8, transaction_id:16B, request_id:u32, offset:u32, payload_len:u16, status:u16`, payload≤512B, CRC4B이며 COBS 포함 frame 상한576B다. magic/version/type을 먼저 검사하고 allocation 없이 정적 버퍼로 처리한다. CRC polynomial/초기값/reflection/xor-out, 모든 enum·golden vector는 구현 때 schema 정본에 고정한다.

명령은 `HELLO, BEGIN, WRITE, STATUS, PREPARE, ACTIVATE_TEST, CANCEL`만 둔다. HELLO에는 MCU identity, board/layout/bootloader ABI, 현재/후보 digest, reset cause, swap state가 있다. ACK는 request/transaction/offset/length/digest를 echo한다. WRITE ACK는 해당 Flash 기록을 read-back한 뒤 보낸다. 유효한 동일 chunk 중복은 재기록하지 않고 ACK한다. 앞선 offset에 다른 데이터, hole, 초과 length는 거절한다. `PREPARED`는 전체 서명 검증·pending intent 영속화 뒤에만 응답한다.

UART response timeout 기본2초, 같은 요청 최대3회 retry, erase/prepare는 `IN_PROGRESS`와 최대30초 deadline을 사용한다. 이 시간은 G4 worst-case flash와 서명 실행 측정 후 줄이거나 늘린다. ESP가 timeout만 보고 이미 실행 중인 erase/swap을 중복 시작하지 않으며 STATUS로 판정한다. ACTIVATE_TEST 뒤 ACK가 유실되면 재부팅 후 boot_id와 실제 image digest를 조회한다. 최종 `CONFIRMED`는 STM 자신이 기록하고 자기 부팅에서 보고해야 한다.

전송 재개는 v1에서 단순하게 제한한다. **같은 부팅·같은 transaction 안에서만 chunk retry를 보장**한다. 전원 차단/ESP·STM reset으로 boot_id가 달라지면 미완료 blob은 처음부터 다시 보낸다. 미완료 secondary/비활성 ESP slot만 지우므로 기존 정상 앱은 유지된다. PREPARED 이후에는 브라우저 연결과 무관하게 local pending state에서 설치/rollback을 이어간다. 앱 교체 중간 복구를 사용자가 다시 전송해야 하는 다운로드 재개와 혼동하지 않는다.

## 9. 상태기계와 두 MCU의 호환성

```text
IDLE -> SERVICE_LOCKED -> RECEIVING -> VERIFYING -> PREPARED
     -> TRIAL_BOOT -> LOCAL_HEALTH_OK -> CONFIRMED -> SERVICE_EXIT
RECEIVING/VERIFYING 중 reset: 기존 앱, 부분 image 폐기 또는 재업로드
TRIAL_BOOT 중 reset/실패: 이전 앱으로 ROLLBACK
swap 중 reset: MCUboot journal에 따라 swap/revert 재개
유효 앱 부재/상태 해석 불가: RECOVERY_WAIT, CAN 계속 차단
```

모든 상태에서 reset 기본값은 GPIO7 LOW이고, Communicator 서비스 동안 J31도 제거된 상태다. 어떤 journal이 손상돼도 service 종료나 CAN 허용을 추정하지 않는다. UI를 사용할 수 있는 Controller는 업데이트 중 주행 데이터가 표시되지 않음을 알리고, 차량 연결이 정상일 때 speed=0이5초 유지되어야 시작한다. CAN이 없는 USB 작업대/복구 경로는 속도 데이터 부재만으로 영구 차단하지 않고 물리 service 진입을 사용한다.

ESP/STM 사이에는 진정한 원자적 동시 commit이 없다. 이를 숨기는 `두 MCU 모두 완료` 플래그 하나를 두지 않는다. 릴리스는 `(ESP_old,STM_old)`, `(ESP_new,STM_old)`, `(ESP_old,STM_new)`, `(ESP_new,STM_new)`와 각각의 recovery ABI를 모두 검사한다. 모든 조합이 관리·복구 통신을 지원해야 한 번의 bundle로 허용한다. 새 기능은 capability가 교집합인 동안만 동작한다.

기본 순서는 전체 bundle 내부 staging 검증 → ESP의 새 호환 앱 설치·자체 확정 → staging 재검증 → STM 설치·시험 부팅·확정이다. 준비가 끝났다면 휴대폰 없이 계속 진행한다. STM 정상본은 MCUboot secondary에 보존한다. 어느 단계에서든 전원이 꺼지면 실행 중 조합을 HELLO에서 재평가한다. 전체 bundle 완료는 두 MCU의 확인된 실제 digest가 manifest와 일치할 때만 기록한다. 이미 확정된 다른 MCU를 임의로 rollback 표시해 원자성을 흉내 내지 않는다. staging이 손상되면 현재 호환 조합을 보존하고 파일 재선택을 요청한다.

네 조합이 호환되지 않는 변경은 먼저 구버전/신버전을 모두 이해하는 중간 호환 릴리스를 설치하는 단계적 migration을 배포한다. 최소 recovery ABI를 넘는 패키지는 설치 전에 거절하고 유선 서비스 필요로 표시한다. Controller/Bridge의 구버전 부재를 이유로 Communicator 자신의 복구를 막지 않는다. 차량 제어 profile/권한은 firmware 호환성과 별도 검증하며 OTA 완료가 VEHICLE_TX 승인으로 이어지지 않는다.

ESP와 STM journal의 각 record에는 generation, phase, target hashes, CRC, 마지막에 기록하는 commit marker가 있다. 반대편 sector의 유효 record를 지우기 전에 새 record write/read-back/commit을 완료한다. 부팅 시 highest valid generation만 채택하고 둘 다 불명확하면 안전 복구를 선택한다. STM MCUboot swap/trailer는 자체 형식을 그대로 사용하며 공통 journal로 대체하지 않는다.

config는 기존 앱이 읽는 snapshot을 보존하고 후보 앱용 snapshot을 별도로 작성한다. 후보 확정 전에 원본 schema를 파괴적으로 migration하지 않는다. rollback 시 이전 앱은 자신의 snapshot을 선택한다. OTA가 NVS 초기화 실패를 만나면 `nvs_flash_erase()`로 pairing/설정을 없애는 예제 코드를 사용하지 않는다. config·profile·firmware의 복구 호환성을 함께 시험한다.

## 10. 외장 SPI NOR 검토 결과

독립 검토자 Heisenberg(`01a073c2-a5ee-7100-8b44-5d2336652ba4`)는 ESP A/B + STM 내부 swap으로 정상 전원 차단 복구가 가능하므로 외장을 필수로 보지 않았다. 원안의 W25Q128JV16MiB 상시 실장은 철회했다. **이번 회로에는 외장 NOR·전용 SPI 배선·footprint를 추가하지 않는다.** 이후 사용자가 N16R8을 지정하여 내부16MiB에 bundle staging까지 확보했다. 외장은 여러 세대 장기 백업 같은 추가 요구가 생길 때 검토한다.

| 질문 | 판정 |
|---|---|
| OTA 도중 단전에서 정상본 보존에 필수인가 | 아니다. ESP 비활성 slot과 STM primary/secondary가 담당 |
| 휴대폰 연결이 끊겨도 설치할 수 있는가 | PREPARED 이후는 가능; 미완료 다운로드는 재업로드 |
| 외장의 추가 가치 | 여러 버전의 signed bundle/STM 정상본을 장기 보관, 재업로드 없는 복구 |
| ESP 내부4MiB 앱 슬롯을 늘리는가 | 아니다. 추가 SPI NOR를 boot-mapped app slot처럼 사용할 수 없음 |
| 추가 위험 | 부품·면적·SPI/전원 검증 부담; 유일한 복구본으로 의존하면 추가 고장점 |
| PSRAM 대체 가능성 | 전원 소실 시 내용이 사라지므로 영구 백업에 사용할 수 없음 |

이 판정은 외장 캐시가 아무 이점도 없다는 뜻이 아니다. unattended fleet 업데이트가 목표가 되면 별도 cache를 선택할 수 있지만 없어도 내부 복구 앱과 재업로드가 동작해야 한다. [ESP SPI Flash API](https://docs.espressif.com/projects/esp-idf/en/v6.0/esp32s3/api-reference/peripherals/spi_flash/index.html)

## 11. 전원 정책과 복구 경계

OTA 시작은 stable power5초, supervisor 정상, 충분한 비활성 저장공간, 서비스 인터록 상태를 검사한다. `USB_SERVICE_SENSE`는 mux source 표시이지 PGOOD가 아니다. 이를 안정된 USB 전압의 증명으로 사용하지 않는다. 실제 rail 측정/qualifier가 없는 조건에서는 전압 수치를 만들어 내지 않고 `전원 연결됨/감시 정상/전압 미측정`을 구분한다.

짧은 순간단전 대응을 위해 기존 mux/decoupling을 유지하지만 큰 supercap으로 업데이트 전체를 끝낼 수 있다고 계산하지 않는다. 수십 초 OTA 유지용 에너지 저장은 부피·돌입전류·수명·buck 안정성 비용이 크다. 이번 방식은 완주에 필요한 hold-up을 전제하지 않고 중단된 작업을 복구한다. 안정된 USB 보조 전원은 권고하지만 복구의 논리적 전제는 아니다.

전압 저하 감지 시 신규 erase/program 요청을 중지하고 진행 중 Flash operation의 제조사 조건을 따른다. 완전히 꺼지면 supervisor/reset/pull/PHY gate가 안전 상태로 수렴한다. 최악 rail collapse 중 잘못된 Flash 쓰기와 CAN dominant pulse는 실물 시험으로 확인한다. 불안정 전원 반복 부팅에서는 최신 update attempt를3회 이상 자동 반복하지 않고 복구 대기로 남긴다. 서비스 종료·pairing 초기화·자동 차량 제어 재개로 탈출하지 않는다.

불변 부트로더/복구 앱이 물리 손상되거나 최초 provisioning이 중단되면 웹 복구가 불가능할 수 있다. ESP ROM/SWD 작업대 복구 가능 여부는 보호 설정에 달렸으며, 복구가 막힌 production 보드는 교체가 필요할 수 있다. 일반 OTA가 이 영역을 쓰지 않는 이유다.

## 12. 구현 모듈과 검증 수용 기준

구현 의존 방향은 웹/ESP-NOW adapter → `ota_manager` 상태기계 → platform flash/crypto/reset/UART이다. `ota_manager`는 ESP-IDF/HAL 레지스터를 직접 참조하지 않는다. STM bootloader와 normal application은 별도 CMake target/링커/서명 artifact다. ISR은 UART 바이트/오류를 bounded queue에 넣고 파싱·검증·Flash 작업을 task/main context로 넘긴다. Flash writer는 target당1개이며 OTA journal 소유권도 그 writer에 둔다.

실행 순서는 다음 gate를 모두 통과해야 한다. 이 목록은 후속 상세 task로 옮길 수 있는 구현 단위이며 아직 수행 완료 목록이 아니다.

| 단계 | 산출물 | 완료 기준 |
|---|---|---|
| OTA-01 | signed container schema/parser/CLI packager | wrong role/layout/signature, integer overflow, malformed CBOR, truncated blob 거절; golden vectors |
| OTA-02 | ESP 파티션·recovery3종 | signed app 크기 gate; PSRAM/SD/LCD/정상 NVS 없이 recovery AP 실행 |
| OTA-03 | G474 MCUboot CMake port | map/WRP/DBANK 검증; header/trailer 경계; interrupted swap/revert 반복 |
| OTA-04 | ESP↔STM boot UART | duplicate/reorder/lost ACK/reset_id, timeout, malformed frame, wrong signed TLV 시험 |
| OTA-05 | 브라우저 UI·세션 | Android Chrome/iOS Safari에서 기본 파일 선택·오프라인AP·재접속·결과 확인 |
| OTA-06 | compatibility·config migration | 네 ESP/STM 조합과 Controller/Bridge old/new, rollback config 보존 |
| OTA-07 | 제조 provisioning | 키/보호 설정/복구 버튼·UART·unplug 절차를 샘플에서 검증; 유선 최초 설치 |
| OTA-08 | HIL 전원·CAN | 아래 고장 주입 및 회로 gate 측정; 차량 연결 전 작업대 완료 |

| 고장 주입 | 기대 결과 |
|---|---|
| 매4KiB ESP erase/write, otadata 각 기록 전후 전원 차단 | 이전 앱 또는 정상 새 앱, invalid image 실행 없음 |
| STM 매2KiB erase,8B write, swap status/confirm 전후 차단 | swap/revert 재개 또는 명시 recovery; bootloader 보존 |
| ESP/STM 한쪽만 reset, 두 쪽 reset, SWD reset | ESP가 STM reset으로 죽지 않음; CAN latch clear |
| 후보 panic/hang/IWDG, UART 무응답/CTS 고정 | 미확정 후보 rollback; old/new digest로 실제 상태 판정 |
| J31 제거/재삽입, GPIO7 HIGH 고착, STM ARM 지속 | J31 제거 시 모든 PHY 차단; 재삽입 시 stale latch 재사용 없음 |
| AUTO5V/PHY3V3/3V3 독립 하강·USB/차량 hot-plug | 역급전/잘못된 PHY enable/dominant pulse 제한을 실제 측정 |
| 휴대폰 잠금/브라우저 종료/AP channel 변경 | 미완료는 재업로드, 준비 완료는 local 설치, 성공 오표시 없음 |
| 잘못된 파일·다운그레이드·다른 보드·서명·빈 용량 | erase 대상 제한, 유효 정상본/설정 보존 |
| 앱 양쪽 invalid·정상 NVS 손상·PSRAM 불량 | 물리 버튼으로 복구 앱, CAN TX0; 숨은 초기화 의존 없음 |
| 잘못된 DBANK/WRP/ROM strap/J32 absent | erase 전에 거절; 무단 보호 해제/ROM 진입 없음 |

단계별 결정적 cut point 전수 시험 + 무작위 시점1000회 이상을 각 hardware variant에서 수행한다. 성공률뿐 아니라 발견된 brick, reset loop, Flash ECC/NMI, CAN dominant 폭, 복구 소요와 로그를 기록한다. 0건 실패도 모든 물리 고장에 대한 수학적 보장으로 표현하지 않는다. host Boolean 검사·ERC 통과로 아날로그/Flash/HIL 시험을 대체하지 않는다.

## 13. 변경 산출물과 실행 검증

회로 열기: [Communicator PDF](../../hardware/communicator/schematic.pdf), [Controller 어댑터 PDF](../../hardware/controller-adapter/schematic.pdf), [Bridge PDF](../../hardware/bridge/schematic.pdf). 부품/핀은 각 보드의 `bom.csv`, `pinmap.csv`, `connectivity.json`, 실제 KiCad `.net`/XML로 교차 검증한다. PCB 배치·배선·제작은 이번 범위가 아니다.

Windows 검증 명령:

```powershell
.\tools\hardware\export-review.ps1
& 'C:\Program Files\KiCad\10.0\bin\python.exe' -m unittest discover -s tools\hardware -v
python tools\validate_document_links.py
git diff --check
```

2026-09-06 실제 실행: KiCad10.0.6 전체 재생성·4보드 ERC 각0개, component/physical pad/BOM/netlist 정합성 PASS, 기존 전원 margin 계산 PASS, hardware 회귀8개 PASS, 문서 링크120개 문서/737개 target 오류0개, `git diff --check` 통과다. Communicator는288개 physical item/936개 named pad, Controller 어댑터는19개/87개다. 128개 Boolean 입력 조합은 정적 service/reset 차단만 검증하며 전압 하강 타이밍을 입증하지 않는다.

`setup-windows.ps1 -VerifyOnly`는 PATH에 `cmake`가 없어 실패했다. 따라서 N16R8 defaults는 공식 SDK Kconfig와 대조했으나 target build·실물 메모리 검사는 미실행이다. firmware OTA/HIL도 미실행이며 이 문서와 회로를 OTA 완제품으로 표시하지 않는다. 기존 MAX20040 U8 footprint의 `PROVISIONAL` 및 전원/HIL gate도 유지한다.

## 14. 독립 적대적 리뷰 기록

원본 요청은 Bridge 없는3장치 독립 OTA, 범용 브라우저 업데이트, ESP가 STM을 관리하는 회로, 전원 차단 우선 설계, 단일 MD와 실제 회로 변경이다. 사용자 추가 요청에 따라 외장 NOR 필요성을 별도 전문 검토한 결과를 §10에 반영했다.

전문 리뷰어 A는 reset/전원/PHY/Flash 고장 복구를, B는 update protocol/서명/버전 조합/저장공간과 사용자 복구 경로를 독립적으로 공격한다. 동일 immutable commit을 object-only로 읽게 하고 원본 결과·severity·수정·재검증을 이 절에 누적한다. 리뷰 결과를 기록하기 전까지 최종 판정은 대기다.
