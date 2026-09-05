# Communicator ESP32 firmware

`ESP32-S3-MINI-1-N4R2`용 독립 ESP-IDF application bootstrap이다. 현재는 protocol component 경계와 target memory 설정을 검증하는 단계다.

- 기준 ESP-IDF: `v6.0.3` (`esp32s3`)
- Flash/PSRAM: 4 MB Quad SPI / 2 MB Quad SPI
- MCU 간 link: UART1 4 Mbps 8-N-1, RTS/CTS
- ESP-NOW 역할: `COMMUNICATOR`
- 상세 pinmap: [Communicator hardware](../../../docs/hardware/communicator.md)
- UART 계약: [Communicator UART protocol](../../../docs/architecture/protocols/communicator-uart.md)

`GPIO26`은 N4R2 내부 PSRAM에 연결되므로 외부 GPIO로 쓰지 않는다. `GPIO19/20`은 USB Serial/JTAG를 위해 예약한다.

ESP32는 DBC display catalog를 소유하지 않는다. STM32에서 올라온 raw CAN batch와 bus 상태를 ESP-NOW로 전달하는 bridge이며, Controller의 catalog/필터 변경만으로 새로운 CAN 신호를 사용할 수 있어야 한다.

## Windows build

```powershell
. .\tools\environment\setup-windows.ps1
idf.py -C firmware/communicator/esp32 set-target esp32s3
idf.py -C firmware/communicator/esp32 build
idf.py -C firmware/communicator/esp32 size-components
```

`sdkconfig.defaults`는 N4R2의 4 MB Flash와 2 MB Quad PSRAM만 선택하고 NVS encryption key partition을 예약한다. `GPIO26`을 외부 기능에 재사용하지 않으며, `partitions.csv`와 생성된 `sdkconfig`가 서로 다른 module 설정을 갖지 않도록 build log를 확인한다. 현재 bootstrap은 단일 factory image다. OTA A/B는 4 MB image budget과 보안 provisioning을 측정한 뒤 별도 partition 설계로 추가한다.

현재 `app_main()`은 protocol v1.3 통합 전용 대기 상태를 log한다. 현재 저장된 v1.2 incomplete header는 application dependency로 연결하지 않는다. 실제 UART1 DMA/RTS-CTS, ESP-NOW session/QoS, STM32 handoff와 CAN 데이터 전달은 각각의 후속 task에서 구현한다.
