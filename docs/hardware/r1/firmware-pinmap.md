# R1 핀맵과 firmware 계약

숫자는 **GPIO 번호와 패키지 pad 번호를 구분**한다. 모든 R/C·커넥터·NC까지 포함한 [Communicator pinmap.csv](../../../hardware/communicator/pinmap.csv)가 전개 정본이다. 아래 표는 BSP 작성용 요약이며 차량 하네스의 신호 위치를 확정하지 않는다.

## STM32G474CEU6 / UFQFPN48

| 기능 | 포트 / 물리 pad | AF·방향·초기값 |
|---|---|---|
| FDCAN1 RX/TX | PA11/33, PA12/34 | AF9, RX 입력/TX recessive. gate 뒤 TCAN1 |
| FDCAN2 RX/TX | PB12/25, PB13/26 | AF9, RX 입력/TX recessive. gate 뒤 TCAN2 |
| FDCAN3 RX/TX | PA8/30, PA15/38 | AF11, CAN3는 Classic125kbps. PA15는 reset debug PU |
| TCAN1/2 STB 요청 | PA4/12, PA5/13 | GPIO output HIGH=standby로 시작 |
| MAX3055 EN 요청 | PA6/14 | GPIO output LOW=standby로 시작 |
| UART CTS | PA0/8 | USART2 AF7, ESP RTS 입력, HIGH=flow stop |
| UART RTS | PA1/9 | USART2 AF7, ESP CTS로 출력 |
| UART TX/RX | PA2/10, PA3/11 | USART2 AF7, 4,000,000bit/s 8N1 |
| ARM edge | PA7/15 | reset external PD, 초기 LOW, health 승인 뒤 rising pulse |
| 외부 watchdog WDI | PB0/17 | task health pulse, falling 간격≤30ms |
| gate 실제 sense | PB1/18 | GATE_SENSE 입력, 요청 상태와 혼동 금지 |
| 차량 rail sense | PB2/19 | AUTO_GOOD_MCU 입력 |
| FT ERR | PB14/27 | FT_ERR_N 입력, CAN3 실장 시 mandatory |
| SWDIO/SWCLK | PA13/36, PA14/37 | SWD만 사용. JTAG/PB4를 CAN TX로 쓰지 않음 |
| HSE | PF0/5, PF1/6 | 16MHz crystal, bypass 아님 |
| BOOT0 | PB8/46 | external10k GND, option byte boot 설정 검증 |
| reset | PG10-NRST/7 | STM_RESET_N; ESP와 분리, NRST 기능 보존 |
| custom bootloader 요청 | PB9/47 | STM_RECOVERY_N, ESP GPIO9 open-drain, external10k PU |
| 전원 | VBAT1, VDD23/35/48 |3V3; VBAT도 배터리 아닌 MCU rail |
| 아날로그 | VREF+20, VDDA21 | VDDA decoupling, GND=EP49 |

R1 clock 설정 출발값은 HSE16MHz→PLLM4/N80/VCO320MHz→PLLR2/SYSCLK160MHz, AHB160MHz, APB1/2 각80MHz, PLLQ4/FDCAN80MHz다. USART2 kernel=PCLK1 80MHz, oversampling16/BRR20으로4,000,000bit/s를 정확히 나눈다. CPU170MHz를 유지한 채 존재하지 않는 독립80MHz UART source를 가정하지 않는다. voltage scale/flash wait-state/PLL lock/실제 주파수는 T-102 target에서 검증한다.

FDCAN80MHz의 bench 출발값은500kbps: prescaler10,seg1=13,seg2=2,SJW2(16TQ,87.5%);125kbps: prescaler40,동일TQ다. FD8Mbps는 data prescaler1,seg1=7,seg2=2,SJW2(10TQ,80%)부터 transmitter delay compensation과 oscillator·harness 허용치를 검산한다. 레지스터의 N-1 encoding을 HAL 인자와 혼동하지 않는다. 실제 차량 bitrate/sample point는 capture로 정하며8Mbps는 PHY 상한이지 세 채널 동시 수집·무선 전달 보장이 아니다.

## ESP32-S3-WROOM-1-N16R8

| 목적 | GPIO / module pad | 상대·주의 |
|---|---|---|
| STM UART RTS | 15/8 | STM PA0 CTS |
| STM UART CTS | 16/9 | STM PA1 RTS |
| STM UART TX | 17/10 | STM PA3 RX |
| STM UART RX | 18/11 | STM PA2 TX |
| native USB D−/D+ | 19/13,20/14 | ESD/22Ω 뒤 connector |
| GPS direct TX 선택 | 4/4 | DNP 링크. 기본 MTi TX와 동시 실장 금지 |
| GPS RX tap | 5/5 | GNSS TX buffered input, 기본 read-only |
| PPS | 6/6 | GNSS PPS buffered input, 측정 epoch 동기 |
| MTi SPI SCLK/MOSI/MISO/CS | 10/18,11/19,12/20,13/21 | mode3,≤2MHz, single owner |
| MTi DRDY | 14/22 | IRQ는 timestamp+task notification만 |
| MTi reset | 21/23 | **open-drain**, HIGH를 강제로 출력하지 않음 |
| GPS power 요청 | 47/24 | external100k PD, AUTO_GOOD/RUN_ALLOWED과 hardware AND |
| mux source 상태 | 38/31 | USB_SERVICE_SENSE; 기존 MINI GPIO33에서 이동, PGOOD 아님 |
| BOOT / EN | 0/27, CHIP_PU/3 | BOOT release·ESP_RESET_N |
| STM reset 요청 | 1/39 | LOW → U50 open-drain → STM_RESET_N |
| STM ROM BOOT0 요청 | 2/38 | J31 제거 + J32 service shunt 때만 전달 |
| 정상 동작 허용 | 7/7 | ESP_RUN_OK external10k PD, OTA에서는 LOW |
| ESP 복구 버튼 | 8/12 | RECOVERY_BUTTON_N, external10k PU |
| STM custom 복구 요청 | 9/17 | STM PB9/47에 open-drain LOW |
| 서비스 인터록 | 48/25 | J31 없으면 LOW, 전체 PHY 차단 |

module pad2=3V3, pad1/40/41=GND다. GPIO35/36/37(pad28/29/30)은 R8 Octal PSRAM용이며 NC로 남긴다. Flash16MiB/PSRAM8MiB, ECC 사용 시 가용7.5MiB다. 상세 reset·Flash·OTA 계약은 [OTA 설계](../../architecture/ota.md)를 따른다.

## 커넥터

| 보드 / ref | 핀 순서 | 제한 |
|---|---|---|
| Communicator J2 / XH2 | 1차량+ /2GND | 외부 fused IGN/ACC, 무극성 아님 |
| Communicator J20 / GH8 | 1CAN1H,2CAN1L,3CAN2H,4CAN2L,5CAN3H,6CAN3L,7/8GND | CAN3 실제 FT bus 판정 전 연결 금지 |
| Communicator J40 / GH10 |1GPS5V OUT,2GPS RX,3GPS TX,10GND;4~9 NC | GNSS 기준 이름. TX/RX 방향 반대로 읽지 않음 |
| Communicator J41 / GH6 |5PPS IN,6GND;1~4 NC | UART2 데이터/전원 연결 안 함 |
| J10 SWD /2×5 1.27mm |1VTref,2SWDIO,3GND,4SWCLK,5GND,6/7/8NC,9GND,10NRST | 현재 footprint는 unshrouded header; keyed cable/adapter 또는 silkscreen과 pin1 오삽입 방지 필요 |
| J30 ARM /1×2 1.27mm |1PHY3V3,2TX_ARM | shunt 기본 미장착 |
| J31 RUN /1×2 1.27mm |1SYS3V3,2SERVICE_RUN | OTA 때 제거; 부재 시 RX/TX 모두 차단 |
| J32 ROM /1×2 1.27mm |1ROM_BOOT_LINK,2BOOT0 | shunt 기본 부재; 통제된 물리 서비스 전용 |
| 마이크 GH8 / 양쪽 |1MIC5V,2GND,3BCLK+,4BCLK−,5WS+,6WS−,7DATA+,8DATA− | 네 twisted pair,1:1, Ethernet/PoE 아님 |

Connector PCB top view와 케이블 mating face는 좌우가 반대다. 주문할 때 제조사 contact1 표시와 crimp 삽입 방향을 도통 검사한다. JST `MP` 두 고정 탭은 NC이며 신호용9번 접점이 아니다.

## 부팅·복구 구현 순서

1. BSP GPIO 초기값 PA4/5 HIGH,PA6/7 LOW를 먼저 latch에 쓰고 output mode를 설정한다. ESP GPIO7은 LOW로 시작해 J31·정상 앱 상태를 검사한 뒤에만 HIGH로 한다. UART CTS/RTS는 외부 PU 덕분에 두 MCU 준비 전 flow stop이다.
2. STM HSE/clock, FDCAN message RAM/filter/bitrate, interrupt와 DMA/ring buffer를 설정한다. 오류 시 TX arm하지 않고 fault를 기록한다.
3. GPIO request와 관계없이 actual rail/gate sense를 읽는다. `CAPTURE_ONLY`에서 TX_ARM shunt가 꽂혀 있어도 ARM edge를 내지 않는다.
4. scheduler health가 CAN worker/UART parser/safety executor 진척을 모두 확인한 경우에만 WDI falling pulse를 발생시킨다. 외부 WDT가 먼저 timeout해도 재무장 이전이면 안전하게 기다린다.
5. RX_ALLOWED가 안정되고 filter 구성이 끝나면 STB 요청 LOW,FT EN 요청 HIGH로 수신한다. TX buffers는 계속 닫힌다.
6. 별도 vehicle/bench 권한·lease·local snapshot·build mode·physical arm을 검사한 뒤 단 한 번 ARM edge를 낸다. gate sense가 기대값과 다르면 바로 disarm한다.
7. watchdog/rail/lease/session fault 뒤에는 이전 승인 상태를 복구하지 않는다. request queue를 비우고 새 session/arm 검사를 수행한다.

센서 ISR에서 MTSSP/UBX parsing, flash 쓰기, CAN 전송을 하지 않는다. ESP sensor task가 bounded packet parser와 최신 snapshot만 소유한다. STM은 GNSS/IMU의 값을 차량 safety 입력 대신 쓰지 않는다. BSP/CMake/ESP-IDF에 적용하는 코드는 [T-100b](../../tasks/T-100b-navigation-audio-bringup.md)에서 이 pinmap revision과 함께 검증한다.
