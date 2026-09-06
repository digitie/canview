# 펌웨어 진입점

현재 코드는 기능 구현 전의 안전 부팅/공용 codec 기반이다. 후속 agent는 [기반 구조와 인수인계](../docs/architecture/firmware-foundation.md)를 먼저 읽고 자신의 상세 task만 연다.

| 대상 | 프로젝트 |
|---|---|
| Controller | [controller](controller/README.md) |
| Communicator ESP32 | [communicator/esp32](communicator/esp32/README.md) |
| Communicator STM32 | [communicator/stm32](communicator/stm32/README.md) |
| Diagnostic Bridge | [diagnostic-bridge](diagnostic-bridge/README.md) |

공용 C99 코드는 ../shared, 장치 조합은 app/startup.c와 각 bsp, SDK 호출은 platform에 둔다.
BSP는 firmware/boards/boards.json과 원본 pinmap에서 생성한 board_pins.h를 사용한다.
기존 Controller components와 STM32 auto_sport는 legacy host 회귀에만 포함하고 새 target image에서는 제외한다.
