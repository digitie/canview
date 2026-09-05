# Communicator ESP32 firmware

`ESP32-S3-WROOM-1-N16R8`용 독립 ESP-IDF application bootstrap이다. 현재는 protocol component 경계와 target memory 설정을 검증하는 단계다.

- 기준 ESP-IDF: `v6.0.3` (`esp32s3`)
- Flash/PSRAM:16MiB Quad SPI /8MiB Octal SPI; ECC 사용 시 가용7.5MiB
- MCU 간 link: UART1 4 Mbps 8-N-1, RTS/CTS
- ESP-NOW 역할: `COMMUNICATOR`
- 상세 pinmap: [현재 R1/OTA 핀맵](../../../docs/hardware/r1/firmware-pinmap.md)
- UART 계약: [Communicator UART protocol](../../../docs/architecture/protocols/communicator-uart.md)

`GPIO35/36/37`은 R8 Octal PSRAM에 연결되므로 외부 GPIO로 쓰지 않는다. `GPIO19/20`은 USB Serial/JTAG를 위해 예약한다. mux source sense는 GPIO38로 이동했다.

ESP32는 DBC display catalog를 소유하지 않는다. STM32에서 올라온 raw CAN batch와 bus 상태를 ESP-NOW로 전달하는 bridge이며, Controller의 catalog/필터 변경만으로 새로운 CAN 신호를 사용할 수 있어야 한다.

## Windows build

```powershell
. .\tools\environment\setup-windows.ps1
idf.py -C firmware/communicator/esp32 set-target esp32s3
idf.py -C firmware/communicator/esp32 build
idf.py -C firmware/communicator/esp32 size-components
```

`sdkconfig.defaults`는16MiB Flash와80MHz Octal PSRAM/ECC를 선택한다. 기존 생성 sdkconfig의4MiB/Quad 값은 defaults만 바꿔도 자동 변경되지 않으므로 새로운 SDKCONFIG 경로로 configure하거나 설정을 명시적으로 변경한다. 현재 partitions.csv는 기존 단일 factory 부트스트랩 배치를 유지하며 남은 Flash를 아직 사용하지 않는다. [OTA 설계](../../../docs/architecture/ota.md)의 A/B/recovery 배치는 별도 구현·서명·유선 provisioning 후 적용한다. R8 ECC 온도 조건과 가용 메모리는 실제 boot log와 보드 열시험으로 확인한다.

현재 `app_main()`은 protocol v1.3 통합 전용 대기 상태를 log한다. 현재 저장된 v1.2 incomplete header는 application dependency로 연결하지 않는다. 실제 UART1 DMA/RTS-CTS, ESP-NOW session/QoS, STM32 handoff와 CAN 데이터 전달은 각각의 후속 task에서 구현한다.
