# Communicator 전원·CAN 상세 회로

정본 연결은 [KiCad PDF](../../../hardware/communicator/schematic.pdf), [전 패드 pinmap](../../../hardware/communicator/pinmap.csv)이다. 부품 reference는 재생성 시 R/C 추가로 이동할 수 있으므로 아래 net 이름과 IC reference를 함께 검색한다. 원문 페이지는 [근거 색인](../../../hardware/references/README.md)에 모았다.

## 1. 전원 분리

```text
차량 12V → 외부 harness fuse → J2 → F1 → TVS → LM74800 + FET×2
                                                ↓ PROTECTED_VBAT
                                           MAX20040B
                                                ↓ AUTO5V (nominal5.0875V)
                   ┌────────────────────────────┼─────────────────┐
             CAN PHY5V / PHY3V3             GNSS 전류제한         TPS2116 VIN1
                                                                  ↓
USB-C → CC1/CC2 전류 광고 확인 → 약1A 제한 → TPS2116 VIN2 → SYS5V →3V3
                                                                  STM/ESP/INS
```

USB-only에서도 MCU, INS와 firmware 진단은 가능하지만 CAN PHY와 외부 GPS 전원은 OFF다. USB는 단순 data-only가 아니며, 종전 `PGOOD → 3V3 EN` 직결안은 이 전원 mux 구조로 대체한다. SWD VTref는 출력 전압 참조일 뿐 외부 전원 입력이 아니다. 모든 GND는 공통인 **비절연** 보드이므로 USB 접지와 차량 접지 간 전위차 안전을 별도로 확인한다.

## 2. 차량 입력·surge

| 부품/net | 연결과 설계 근거 |
|---|---|
| J2 | JST XH2, 1=VBAT, 2=GND. 검증된 IGN/ACC의 외부 1A 퓨즈 가지를 기본으로 한다. OBD/차량 하네스 핀 번호가 아니다. |
| F1 | Littelfuse 0451001.MRL, 1A/125V, 실제 Nano2 6.1×2.69mm. 단락 시 교체형이며 1206으로 대체하지 않는다. 차량 tap 바로 옆 외부 퓨즈와 협조 확인. |
| D100 | SMBJ36CA, 양방향 36V TVS, 커넥터/퓨즈 다음. 600W는 10/1000µs 정격이지 350ms load-dump 에너지 보장이 아니다. |
| Q1/Q2 | BUK7Y12-100E, 100V·**DC VGS ±20V** LFPAK56 N-FET common-source. Q1 D=입력, Q2 D=출력, 두 S=CS. LM74800 최대 HGATE14.5V/DGATE13V에 대해 DC 여유5.5/7V. 이전 BUK9Y12-100E의 ±10V DC 정격은 부족하므로 사용 금지. 같은 footprint라도 무검증 대체 금지. |
| U7 LM74800 | HGATE→입력 FET, DGATE→출력 FET. A/OUT=CS, C=PROTECTED_VBAT. EP13은 FLOAT이며 GND via를 넣지 않는다. |
| VS/CAP | raw fused→10k/0.25W/150V→VS, CS→BAS21H→VS, VS 47V zener/100nF100V, CAP–VS 100nF25V. bootstrap supply를 PCB GND로 잘못 귀환하지 않는다. |
| CS | 1µF100V. TI 포럼의 EVM DGATE 발진 교정값을 반영했으며 오래된 15nF 예제를 그대로 복제하지 않는다. |
| OV | PROTECTED_VBAT→100k→OV→4.75k→GND. 두 저항0.1%/10ppm, 약27.15V nominal. VOVR1.195~1.267V, 온도 오차와200nA 입력 누설을 포함한 정적 범위26.23~28.07V. MAX20040 입력 상한36V보다 여유를 둔다. |

LM74800은 이상 다이오드/OV disconnect 제어기이지 전류 제한기가 아니다. TVS clamp·FET VDS·선로 인덕턴스·OV 반응 지연을 함께 보아야 한다. 음의 입력과 input/output 커패시터의 잔류 전압을 포함한 fault를 시험한다. 비동작 시 차량 대기전류 1mA 달성을 주장하지 않는다. **이번 R1은 외부에서 차단되는 IGN/ACC 전원용**이며 상시 BAT+에 무조건 연결하지 않는다. 상시 전원/CAN wake variant는 별도 회로와 task가 필요하다.

## 3. AUTO5V와 저전압 판정

U8 `MAX20040BATPA/VY+`의 2.2MHz buck-boost를 사용한다. FB를 VCC에 연결하는 fixed5V 대신 **30.7k/10k,0.1%,10ppm/K** 분압으로 5.0875V nominal을 만든다. 좁은 MAX3055 4.75~5.25V 범위와 독립 supervisor의 정확도·히스테리시스를 같이 검산하기 위해서다. 핀4 `AUTO_VCC4` net은 이름과 달리 내부 약4.6V bias 전원이며 외부 부하 전원으로 쓰지 않는다.

- L=4.7µH XAL5030-472MEC, input 4.7µF50V×2, output 22µF25V×2. 제조사 effective-C/peak current를 실제 operating point로 확인한다.
- BST1–LX1, BST2–LX2 각각 100nF50V. FSW=12k. COMP=22k+680pF 직렬,22pF 병렬은 EVM 출발값이며 수정 출력·PCB의 loop stability 승인값이 아니다.
- AUTO_VCC4에서 BST1/BST2로 각각 PMEG4010BEA Schottky를 둔다(A=bias,K=BST). EVM의 공통 anode BAT54A bootstrap 구성을40V 부품2개로 옮겼다. 이 외부 충전 경로를 생략하지 않는다. 고온 누설·bootstrap 전압·충전 peak는 검증 대상이다.
- OUT_S는 출력 capacitor 양극을 Kelvin sense하고 FB/COMP/AGND는 LX 인덕터 hot-loop와 분리한다.
- 최초 기동은 전기적 최대 startup threshold를 고려해 4.5V 이상에서 시험한다. “기동 후2V 유지”와 “2V에서 새 부팅”을 구분한다.
- U13 TPS3700은 PROTECTED_VBAT 약8V를 검사한다. U14 TPS389001은 실제 AUTO5V를 32k/10k0.05%,10ppm/K로 검사한다. `AUTO_GOOD = BATT_GOOD & AUTO5_GOOD & AUTO_PGOOD`다.
- U14 CT10nF는 약10.7ms nominal의 **회복 지연**이다. 전압 하락 감지는 별도 propagation delay이며 10ms를 기다린다는 뜻이 아니다.
- [수치 검산](../../../hardware/margin-check.json)의 DC engineering bound는 약4.976~5.199V, 판정 fall 약4.768~4.893V, rise 약4.783~4.933V다. 남는 positive ripple/overshoot 예산은 약51mV뿐이다. 공급 collapse 최소 전압 여유 약18mV와 TPS3890 하락 지연의 max 미명시 때문에 **빠른 collapse 시험은 필수 미완료 gate**다. 전 범위 보장을 얻었다고 표시하지 않는다.

PHY3V3는 AUTO5V의 TPS7A2033에서만 생성한다. SYS3V3와 PHY3V3에 각각 TLV803EA30DPWR를 두어 USB 선연결 후 차량 전원 도착 때에도 PHY latch가 초기화된다. supervisor는 3.08V nominal, release130~270ms다. ESP CHIP_PU와 STM PG10-NRST는 SYS_RESET_N을 공유한다. STM 내부 BOR/IWDG는 이 외부 보호를 대체하지 않는다.

## 4. USB-C와 시스템 전원

TUSB320LAI는 PORT=GND(UFP), ADDR=NC(GPIO), EN_N=GND이며 내부 Rd를 사용한다. CC에 외부 5.1k를 중복 추가하지 않는다. `OUT1`이 1.5A/3A 광고에서만 LOW가 되는 표를 이용해 inverter→TPS2553 EN을 구동한다. 26.7k ILIM으로 약1A 제한한다. **C-to-C 5V1.5A 이상 source가 필요하며 default-current/A-to-C 포트로는 보드가 켜지지 않는다.** USB PD 고전압을 협상하지 않는다. 이미 차량 전원으로 켜졌다면 USB data는 사용할 수 있다.

TPS2116은 VIN1=AUTO5V, VIN2=USB_LIMITED이며 source 역류를 차단한다. PR1 분압100k/30.1k, MODE=AUTO5V로 차량 우선 운용한다. ST는 source 선택 상태이지 PGOOD가 아니다. 전환 중 finite reverse response와 SYS5V droop를 실제 케이블·부하에서 확인한다. SYS3V3 TPS629210은 249k VSET=3.3V,27.4k MODE,2.2µH/22µF 출발값이다. downstream bulk를 임의로100µF 더하지 않는다.

**USB CC 감지 전원은 mux·limiter 앞의 별도 U16 TPS7A2033**이다. USB_VBUS→U16→USB_CC3V3로 TUSB320LAI VDD, OUT1 pull-up과 U2 inverter를 공급한다. TUSB320LAI 권장 VDD 상한5.0V 때문에 VBUS 직결을 금지한다. U16 입력6V 허용,3k 최소부하를 포함한 output ±1.5% 범위3.2505~3.3495V이며 SYS3V3 없이 먼저 켜진다. U3는 VBUS를 전력 입력으로 유지하고 EN만3.3V 논리로 받는다. USB pre-attach/suspend 전류·VBUS5.25/5.5V·detach 재기동을 검증한다.

## 5. CAN PHY와 TX gate

TCAN1046AV DYY14의 **pin5=GND2,7=RXD2,8=STB2,11=VIO**가 맞다. 이전 pinmap의 5/7/8/11 배치는 사용 금지다. CAN1/2 TXD와 STB에 각각 PHY3V3 외부10k pull-up을 둔다. CAN FD 라인의 TVS는 ESD2CAN24-Q1, 종단60.4Ω×2+4.7nF는 차량 중간 노드에서 모두 DNP다. CMC는 회로에 필수 부품으로 추가하지 않았으며 실측 EMC 문제가 있을 때 PCB 개정으로 검토한다.

MAX3055는 **검증된 ISO11898-3 저속 FT bus에만** 연결한다. CAN3의 FDCAN controller는 Classic CAN/125kbps로 운용하며 FD frame을 쓰지 않는다. STB=AUTO5V, EN 외부10k pull-down으로 reset에서 Power-On Standby다. BATT=PROTECTED_VBAT, WAKE pull-up, INH은 별도 관찰/방전 경로, ERR은 STM 필수 입력이다. RTH→CANH/RTL→CANL에 각각4.7k가 기본 FIT이며 기존 차량 네트워크의 등가 종단·노드 수를 측정해 검증한다. CANH–CANL 120Ω은 절대 사용하지 않는다. 4.7k는 전체망100Ω을 단독으로 만든다는 뜻이 아니다.

MAX3055 RXD/ERR은 SN74LVC2G17의 5.5V tolerant/Ioff 입력을 거친다. 과거 저항 분압안으로 MCU 무전원 역급전 안전을 증명하지 않는다. FT TX/EN은 AUTO5V의 **SN74AHCT1G126-Q1**(주문 MPN `CAHCT1G126DBVRQ1`)을 써서 output이 MAX 자체 rail을 따른다. U28 OE=TX_PERMIT, U29 OE=RX_ALLOWED로 **HIGH일 때만** 구동하며 pin1마다1k GND pull-down을 둔다. PHY3V3가 사라지면 Ioff 상태의 허용 논리 출력과 무관하게 OE가 LOW로 수렴하고 TXD=HIGH/EN=LOW가 된다. 기존 `/OE`를 PHY3V3에 pull-up한 FT 연결은 전원 소실 시 반대로 enable되므로 폐기했다. FD TX/STB는 PHY3V3의 SN74LVC2G125를 유지한다. AHCT 출력의 일반적 Ioff를 주장하지 않으며 MAX와 같은 AUTO5V에 묶어 별도3.3V reservoir의 역급전 경로를 줄인다.

```text
RX_ALLOWED = AUTO_GOOD & SYS_RESET_N & PHY_RESET_N
ARM_HEALTH_N = WD_OK_N & SYS_RESET_N & AUTO_GOOD
ARM_CLEAR_N = ARM_HEALTH_N & PHY_RESET_N & physical_TX_ARM
ARM_LATCH = DFF(D=1, CLK=STM_ARM_EDGE, asynchronous_clear=ARM_CLEAR_N)
TX_PERMIT = ARM_LATCH & ARM_CLEAR_N & RX_ALLOWED
TX_OE_N = !TX_PERMIT
FT_TX_OE = TX_PERMIT  (pin1 external1k pulldown)
FT_EN_OE = RX_ALLOWED (pin1 external1k pulldown)
```

CAN capture용 PHY normal과 차량 TX permit은 별개다. ARM shunt가 없어도 정상 전원에서 수신할 수 있고, 세 TX buffer는 off/recessive다. physical arm 해제·전원·reset·watchdog 실패가 latch를 지운다. WDO가 회복되거나 전압이 돌아와도 **새로운 STM ARM 상승 edge 없이는 송신이 복귀하지 않는다**. CLK가 HIGH 고착되어도 재무장 edge가 생기지 않는다.

TPS3431 CWD=120pF C0G5%, nominal64.288ms, capacitor tolerance/10pF stray/IC9.5%를 포함한 계산 상한 약71.8ms다. WDI falling edge는 health task가30ms 이내 주기로 발생시킨다. timer PWM/DMA/ISR에서 무조건 토글하면 main task가 죽어도 watchdog을 속이므로 금지한다. SN74LVC1G74의 Q는 **pin5**, pin3은 반전 Q다.

## 6. 고장과 잔여 한계

| 상황 | 의도한 회로 결과 | 별도 확인 |
|---|---|---|
| USB-only | MCU/INS ON, PHY/GPS OFF, TX deny | 전 pin 역급전·USB source 전환 |
| MCU reset/HSE 실패 | SYS_RESET 또는 외부 pulls로 standby, latch clear | startup glitch·PG10 설정·clock fault |
| ESP hang | STM 안전 정책/lease 만료로 TX stop | STM이 UART 장애를 health에 포함하는 FW |
| STM hang | 외부 watchdog 후 latch clear, GPIO pull로 recessive | <100ms end-to-end 실제 측정 |
| 자동차5V 저하 | AUTO_GOOD off, RX/TX 차단, GPS 전원 off | 빠른 dV/dt, supervisor 지연, MAX logic abs max |
| PHY3V3만 소실, AUTO5V/MCU 정상 | FT active-high OE pull-down→TXD recessive/EN disabled | LDO short, 양 MCU 요청 HIGH/펄스 고착, 하강 지연·glitch 실측 |
| 정상 형식의 잘못된 반복 송신 | watchdog만으로 차단 못 할 수 있음 | STM executor rate limit·local precondition |
| gate IC 출력 short/PHY 내부 고장 | 단일 hard gate로 모든 단일고장 보장 불가 | fault injection, dominant timer, CAN bus 격리 필요성 재평가 |

이 설계는 fault-tolerant 방향의 다층 차단이지 기능안전 인증 또는 “어떤 고장에서도 차량 bus에 영향 없음” 보장이 아니다. 안전 관련 진술은 bench evidence와 함께 승격한다.
