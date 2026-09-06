# Bridge·Controller와 원격 I²S 수음

## Bridge

[Bridge KiCad PDF](../../../hardware/bridge/schematic.pdf)는 ESP32-S3-WROOM-1-N8R2와 USB-C/3.3V 전원·reset·BOOT/PAIR·LED만 둔다. N8R2=flash8MB/PSRAM2MB이며 Communicator는 OTA 변경으로 N16R8이다. 두 보드는 GPIO 할당과 메모리 설정이 다르다. USB 전원 정책은 Communicator와 같은 TUSB320LAI+TPS2553이며 **C-to-C 5V1.5A 이상 광고가 필요**하다. 5V PD 이상의 전압은 요청하지 않는다.

U16 TPS7A2033은 USB_VBUS에서 직접 감지부용 USB_CC3V3를 만든다. TUSB320LAI VDD와 OUT1 pull-up/U2 inverter는 이 rail에만 연결하며 VBUS에 직접 연결하지 않는다. 전력 limiter 앞에서 기동하므로 SYS3V3에 의존하는 순환 부팅 조건이 없다. 정격·시험 조건은 [Communicator 전원 설계](communicator-circuit.md#4-usb-c와-시스템-전원)를 공유한다.

ESP GPIO19/20 native USB, GPIO4 PAIR 버튼(active-low), GPIO5 LED다. GPIO0은 BOOT 버튼이다. ESP-NOW/Wi-Fi는 같은2.4GHz radio/channel을 공유한다. channel policy와 웹 quota는 기존 Bridge architecture를 따르며 센서 추가가 CAN capture/control airtime을 잠식하지 않는다. Bridge는 센서를 관찰할 수 있지만 Controller control lease나 차량 TX gate를 가질 수 없다. 이번 최소 보드에는 SD/eMMC 회로를 추가하지 않았고 기존 선택적 저장 설계와 구분한다.

## Controller 어댑터와 RTC

Waveshare 공식 schematic의 J8은 **2×16 2.54mm SMT 암소켓**이며 공식 STEP의 부품명도 이를 확인한다. 어댑터 J1은 mating male header다. 기계적 결합 높이·삽입 길이·장착 방향은 구매 보드와 확인해야 하며 단순 pin 수만 맞춰 조립하지 않는다. GPIO38/39/40은 camera와 공유되므로 camera driver를 비활성화하고 camera를 분리한 수음 variant만 사용한다.

| host J8 접점 | 연결 | 역할 |
|---|---|---|
|2|HOST5V|마이크 cable power source; host 공급 방식에 따른 실제5V 확인|
|3/4/29/30|GND|공통 return|
|7 / GPIO38|MIC_BCLK|ESP I²S1 master BCLK|
|9 / GPIO39|MIC_WS|ESP I²S1 WS|
|11 / GPIO40|MIC_SD|ESP I²S1 data input|
|13 / GPIO41|RECOVERY_BUTTON_N|OTA 복구 버튼, camera 미연결 variant|
|22 / RESET|HOST_RESET_N|복구 진입용 sink-only reset 버튼|
|31/32|3V3|host LVDS endpoint 전원|

기존 Waveshare **PCF85063 계열 I²C RTC0x51**를 재사용한다. 추가 RTC는 실장하지 않는다. 공식 자료는 PCF85063A 원문을 연결하므로 구매 board marking/schematic suffix를 함께 확인한다. 기존 GPIO7/8 I²C 소유 task에서 RTC register를 읽고 oscillator-stop flag/날짜 범위를 검증한다. RTC 읽기 성공만으로 시간이 정확하다고 판정하지 않는다. backup battery 유무·전압·전원 차단 후 유지 시간을 시험하고 RTC에 충전 전압을 임의 주입하지 않는다.

수동 날짜·시간/시간대, 유효 GNSS UTC anchor, 차량 검증 CAN time은 기존 source 우선순위 정책으로 처리한다. RTC write 후 readback이 완료되어야 성공을 표시하며 GNSS가 연결되었다고 사용자의 시간대를 덮어쓰지 않는다. 일몰 경고는 valid 위치/시간과 검증된 전조등 상태가 있을 때만 계산한다. CAN 기반 자동 밝기 요구는 유지하며 마이크·IMU를 밝기 센서 대용으로 쓰지 않는다.

## 긴 케이블의 I²S

raw single-ended I²S를 수m 배선하지 않는다. 다음 세 신호를 각각LVDS twisted pair로 보낸다.

```text
ESP BCLK → SN65LVDS1 → BCLK± → SN65LVDS2 → AXC1T45 → T5848 SCK
ESP WS   → SN65LVDS1 → WS±   → SN65LVDS2 → AXC1T45 → T5848 WS
ESP DIN  ← SN65LVDS2 ← DATA± ← SN65LVDS1 ← AXC1T45 ← T5848 SD
```

네 번째 pair는5V/GND다. GH8 양쪽을1:1로 연결하고100Ω termination은 **각 receiver 끝에만** 둔다. RJ45는 Ethernet/PoE 오접속 위험 때문에 사용하지 않는다. 커넥터 가까이에 TPD2E2U06 ESD, host5V에 MF-PSMF010X-2 0805 PTC(15V,hold100mA@23°C,trip300mA)를 둔다. PTC는 정밀 전류 제한기나 즉시 차단기가 아니며 최대 저항/고온 derating/단락 온도를 측정한다.

T5848 `MMICT5848-00-012`는 **1.8V 전용**이다. remote TPS7A2033→TPS7A2018과 고정 방향 SN74AXC1T45를 사용한다. LR=GND(왼쪽 slot), WAKE/THSEL=NC, SD100k pull-down이다. pin7=VDD, pin6=SCK다. package 본문 한 곳의 VDD pin 번호 오기를 pin configuration/land drawing과 대조했다. footprint의 ground3는 음향 구멍 주변 annulus이며 중앙0.8mm NPTH에 copper/paste가 덮이지 않게 한다.

### 샘플링·지연

- 32kHz sampling, stereo64fs, BCLK2.048MHz, slot32bit×2, I²S24bit word의 유효20bit를 사용한다. HQ BCLK2.0~3.7MHz 범위다. 16kHz/64fs의1.024MHz는 HQ 범위를 벗어나므로 그대로 사용하지 않는다.
- I²S의 WS 변경 뒤1bit 지연을 적용하고 unsigned PCM/좌우 slot 오해를 host fixture로 검증한다. ESP driver가 DMA에 정렬하는24bit 위치를 확인한다.
- 목표 cable≤3m는 **설계 목표**다. 반주기244ns에서 outbound/inbound cable delay(설계 가정 약5ns/m씩),LVDS 두 방향,AXC 두 방향,T5848 SD valid최대75ns,ESP setup과 jitter를 모두 빼야 한다. 길이만으로 승인하지 않는다.
- 검증 실패 시 먼저 BCLK/WS skew·termination·GND return을 수정한다. 안전하지 않은 상태에서 samplingrate를 무조건 낮추면 mic HQ 범위를 위반할 수 있다. 필요하면 remote MCU가 local I²S를 취득해 packet 전송하는 **다음 revision**으로 전환한다.

### FFT·기존 음량 자동화 입력

DMA buffer는 고정 할당하고 audio task가 DC 제거/Hann window/FFT를 수행한다. 예를 들어1024sample=32ms, bin31.25Hz,50% overlap이면16ms hop이다. FFT task와 UI update는 분리하고 UI는10~20Hz 최신 요약만 사용한다. raw FFT를 ESP-NOW로 계속 송신하지 않는다. dBFS와 보정된 dB SPL을 구분하며 SPL calibration 전 화면에 물리 소음 dB로 확정하지 않는다.

road-noise trigger는 기존 주파수 band/level hysteresis/지속시간/볼륨 ramp 설계를 유지한다. 사람 목소리/음악 자체가 마이크에 들어오는 자기증폭 feedback을 막도록 출력 음량 변화 직후 holdoff·상승 상한·baseline 추적을 검증한다. mic disconnect·clipping·clock loss면 자동 증가를 멈추고 manual/current volume 상태를 보존한다. [마이크 PDF](../../../hardware/microphone/schematic.pdf)와 [T-100b](../../tasks/T-100b-navigation-audio-bringup.md)에서 SI/음향 시험을 추적한다.
