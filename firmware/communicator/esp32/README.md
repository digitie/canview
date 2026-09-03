# Communicator ESP32 firmware

`ESP32-S3-MINI-1-N4R2`용 ESP-IDF project가 들어갈 위치다.

- 기준 ESP-IDF: `v5.5.2`
- Flash/PSRAM: 4 MB Quad SPI / 2 MB Quad SPI
- MCU 간 link: UART1 4 Mbps 8-N-1, RTS/CTS
- ESP-NOW 역할: `COMMUNICATOR`
- 상세 pinmap: [`../../../docs/communicator-hardware.md`](../../../docs/communicator-hardware.md)
- UART 계약: [`../../../docs/communicator-uart-protocol.md`](../../../docs/communicator-uart-protocol.md)

`GPIO26`은 N4R2 내부 PSRAM에 연결되므로 외부 GPIO로 쓰지 않는다. `GPIO19/20`은 USB Serial/JTAG를 위해 예약한다.
