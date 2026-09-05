# R1 검증 결과와 제작 전 조건

## 검증 계층

| 계층 | 수행 범위 | 이 결과로 알 수 없는 것 |
|---|---|---|
| KiCad 10 native | 네 계층 schematic parse, XML/sexpr export, ERC, PDF | 회로 동작·부품 정격·납땜/PCB |
| export validator | schematic 입력↔KiCad netlist, 전 부품 named-pad set, NC, BOM, 회귀 pin oracle | 제조사 도면과 모든 land 치수의 실측 일치 |
| 수치 검사 | DC 분압 허용치, supervisor window, watchdog 상한 | 전원 loop·capacitor bias·surge SOA·thermal |
| protocol host | 고정 wire 길이, signed range, golden, malformed, version/capability, snapshot digest | 실제 firmware session/RTS/RF/end-to-end |

최신 실제 결과는 [validation.json](../../../hardware/validation.json)과 [margin-check.json](../../../hardware/margin-check.json)이다. native KiCad version10.0.6을 사용했다. WSL의 `rg`/다운로드/patch는 보조이며 생성·ERC·host test는 Windows executable로 실행했다. 물리 STM/ESP·실물 보드·차량·오실로스코프는 이 작업 환경에 없으므로 target/HIL을 실행한 것처럼 기록하지 않는다.

2026-09-05 수정본 실제 실행: 네 보드 ERC0/waiver0, Communicator254·Bridge43·adapter16·mic30개, **총343개 BOM item/1,235 named pad**의 정합성 PASS. BOM 수에는24개 PCB test pad와 DNP가 포함되고 virtual PWR_FLAG는 제외된다. 센서 host 시험16개와 hardware net/정상상태 Boolean 회귀시험5개가 통과했다. 회로도는 기능별 sheet와 전역 net label로 연결하며 국부 wire와 실제 netlist가 일치한다.

최초2인 리뷰의 P1 세 건을 수정했다: Q1/Q2를 DC VGS±20V BUK7Y12로 교체, FT U28/U29를 active-high OE+GND pull-down으로 변경, USB CC detector 전원을 mux 앞3.3V로 분리. [test_safety_contracts.py](../../../tools/hardware/test_safety_contracts.py)는 부품·pin/net·pull과32가지 Boolean 조합을 검사한다. **아날로그 과도응답/HIL simulation이 아니며**, 수정 immutable commit의 독립 재검토와 실물 gate는 별도다.

vendor PDF는56개/1,745쪽/95,230,736byte의 `%PDF`·SHA-256·바이트 수·전체 페이지 parse 검사 오류0이었다. AN5093와 MAX 단독 schematic은 다운로드 timeout으로 미확보이며 원문을 재작성해 대신하지 않았다. 저장 구판과 제조사 land 원본 미확보도 아래 gate에 남긴다.

export script에서 native child 종료 전에 다음 명령이 실행되어 이전 회로와 새 BOM이 섞이는 현상을 재현했다. `Start-Process -Wait`로 바꾸고 전체 재생성 후 독립 pin/net validator로 검사한다. 명령 exit0 하나만으로 갱신 성공을 판정하지 않는다.

후속 감사에서 Windows CRLF가 Git LF로 바뀌며 최초 candidate의 `.net`/`connectivity.json` 해시8개가 달라지는 문제를 재현했다. 생성 text는 hash 계산 전에 LF로 정규화하고 `.gitattributes`로 checkout도 LF로 유지한다. 제조사 PDF와 binary는 변경하지 않는다. `validate_exports.py --git-revision <commit>`은 worktree 대신 Git blob의 실제 bytes와 저장 hash를 비교하는 read-only 검사다.

## 아직 닫히지 않은 제작 gate

1. **MAX20040 land90-0409** 원본의 전체 치수 overlay. 제조사 MAX20090 package 표에서 T2044+4C/21-100172와 T2044Y+4C/21-100068이 같은 land90-0409를 공유함을 확인했다. 저장한 KiCad TQFN20/EP2.9×2.9는21-100172 기반 **후보 footprint**이며, BOM note에 PROVISIONAL을 남겼다. 같은 land ID가 직접 치수 검증 완료를 뜻하지 않는다. pad1 방향·EP/paste·thermal via를 승인하기 전 PCB 제작 금지다.
2. reference 최신 개정과 미확보 PDF 원본. [확보 기록](pdf-acquisition.md)과 [manifest](../../../hardware/references/manifest.json)의 failures를 따른다. 구판 PDF가 있다고 최신판 변경을 무시하지 않는다.
3. Waveshare header mating 높이/간섭, GPS harness contact 방향, 실제 부품 MPN/리플로 공정. R/C의 GEN-*를 공급사 part로 확정하고 정격을 낮추지 않는다.
4. PCB placement/routing/DRC/방열/노이즈와 전원 compensation. bare schematic으로 board outer size나 thermal 결과를 확정하지 않는다.
5. MAX3055 VCC5V의 positive ripple/overshoot와 빠른 하강 검출. 정적 계산의 여유가 좁으므로 실제 최악값이 벗어나면 regulator/supervisor/hold-up를 재설계한다.
6. reverse/load-dump/FET SOA/TVS/fuse 협조, USB 전환·unpowered injection, CAN bus fault, CAN3 실제 물리계층, mic 케이블 SI, INS 동작/재획득.

이를 단순 “남은 문서 작성”으로 취급하지 않는다. [T-100](../../tasks/T-100-communicator-schematic.md), [T-101](../../tasks/T-101-hardware-bringup.md), [T-100b](../../tasks/T-100b-navigation-audio-bringup.md)에 설계 확정/실측 gate로 남긴다. 전체 G1 또는 차량 TX 승인은 열리지 않는다.

## 전력 예산과 고장 시 부하 제거

아래는 설계 할당치이며 **측정한 소모전류가 아니다**. 각 chip의 최대치, 실제 RF duty, CAN dominant 부하, ambient temperature를 반영해 업데이트한다.

| 부하 | 초기 할당 | 수용 기준 |
|---|---:|---|
| SYS3V3 ESP RF burst+STM+MTi/BMP+logic | 지속≤650mA,짧은 peak≤800mA | TPS6292101A에서 droop/reset 없음, junction margin 확인 |
| PHY3V3+AUTO5V CAN PHY | AUTO5V 환산≤180mA 예상 | 두 HS+FT 실제 bus 조건과 fault 최대치 재계산 |
| 외부 F9P | 약250mA nominal | GPS limiter61.9k의 약378~480mA 범위보다 startup peak 작은지 실측 |
| USB-only | SYS5V 입력≤약0.7A 목표 | CC≥1.5A,USB limiter 약1A로 boot/inrush 통과 |
| remote mic | cable≤60mA 목표 | PTC 고온 hold 전류보다 여유,최대R·케이블drop 후LDO dropout 확인 |

650mA3.3V를5.0875V/효율90%로 공급하면 약469mA이며, CAN180mA+GPS250mA와 합쳐 약899mA다. MAX20040B1.2A nominal 대비 약25%의 계산 여유다. 이 효율과 부하는 가정이다. GPS short의 limiter480mA와 RF peak800mA, CAN worst case가 겹치면1.2A를 넘을 수 있다. AUTO_GOOD 하락 시 hardware가 GPS와 PHY를 먼저 끄고 latch를 지우도록 했으며, 이를 “정상 동작 유지”로 포장하지 않는다. GPS 재기동은 firmware 최대3회/10분, core 안정 후 enable로 제한한다. fault 제거가 MCU reset보다 충분히 빠른지 시험한다.

cold crank에서 buck-boost output1.2A가 유지된다고 가정하지 않는다. Vbat가8V 판정 아래로 내려가면 CAN/GPS를 정지하고 MCU가 유지되는지/깨끗하게 reset되는지를 본다. 이 장비는 engine ECU가 아니므로 출력 연속성보다 bus를 놓는 상태가 우선이다.

## 보호 계산 절차

정적 분압은 `Vtrip=Vref×(1+Rtop/Rbottom)±Ileak×Rtop`을 R/Vref/온도 허용치 양끝에서 계산한다. AUTO5V FB1µA는 datasheet의 FB=VCC 조건을 보수적 allowance로 사용한 것이며1.25V에서 보증된 한계라고 주장하지 않는다. [check_margins.py](../../../tools/hardware/check_margins.py)가 가정을 출력한다.

surge 시험 전 계산서에는 최소 다음을 채운다.

- pulse 파형/폭/반복/발생원저항과 harness 인덕턴스. 실제 적용 규격·차량 clamp가 없는 상태에서 TVS rating 한 줄로 승인 금지.
- TVS `I(t)=(Vsource(t)-Vclamp(I,T))/Rsource`, `E=∫Vclamp×I dt`, transient thermal impedance와 반복 derating.
- Q1/Q2의 VDS/ID 시간곡선, 온도별 DC/pulse SOA·Zth·VGS clamp, LM turnoff delay 동안의 에너지. RDS(on)만 비교하면 안 됨.
- F1/외부 fuse I²t와 선재 온도·차량 battery short current, TVS short 고장 시 차단과 퓨즈 접점 안전.
- converter bootstrap의 VCC–BST diode reverse/forward pulse와 hot leakage. EVM30V BAT54A의 여유를 늘리려고 R1은40V PMEG4010BEA 두 개를 사용하며 actual BST/LX overshoot와 gate bias를 확인한다.

## PCB/net 제약

4층 출발: top component+critical loop, 인접 연속GND plane, power/signal, bottom 보조signal. RF와 전력부의 return을 끊는 임의 split ground는 하지 않는다.

- 입력 connector–fuse–TVS loop는 짧고 넓게, clamp GND return이 MCU reset/GNSS ground를 가로지르지 않게 배치한다.
- LM CS loop, MAX20040 두LX–L–bootstrap loop, TPS629210 input/SW/output loop를 제조사 EVM에 맞춰 좁힌다. COMP/FB/HSE/MTi는 switch node와 분리한다.
- power conductor 폭/via 수는 목표 전류와 copper stackup으로 계산한다. 임의 최소폭0.15mm를 모든 power net에 적용하지 않는다.
- UART4Mbps TX/RTS series33Ω는 송신원 바로 옆. RX/CTS는 Schmitt 임계와 ringing을 확인한다. 서로 다른 driver를 같은 net에 묶지 않는다.
- USB2 D±는 board stackup의90Ω differential, LVDS cable100Ω, CAN pair는connector–TVS–PHY 간 stub를 최소화한다. 종단은각variant에만실장한다.
- antenna keepout은 Espressif module drawing대로 모든 관련 copper/metal에서 확보한다. MTi magnetometer는 FET/inductor/스피커/강철fastener와 거리를 둔다.
- STM EP49=GND, MTi 중앙 금지 영역, LM EP13=FLOAT, mic acoustic NPTH를 혼동하지 않는다. via-in-pad/paste window는 조립공정 승인 후 넣는다.

## HIL 수용 표

| 시험 | 관찰 | 통과 기준 |
|---|---|---|
| 전원 순열 | USB/자동차/SWD 모든 투입·제거,slow ramp/fast drop | USB-only PHY/GPS OFF,상대 전원 backfeed/injection 정격 초과 없음 |
| USB CC 전원 | VBUS4.75/5.0/5.25/5.5V,CC default/1.5A/3A,detach | U1 VDD2.7~5.0V 안,default-current limiter OFF,pre-attach/suspend 전류 적합 |
| FET gate 정격 | Q1/Q2 VGS 정상 및 ON/OFF/reverse transient | DC±20V 이내·pulse/SOA 여유,gate ringing 포함 |
| brownout |5V·3V3·PHY3V3 독립 fault,scope TXD/EN/STB/RESET | 전원 미유효 때 dominant 없음,회복 후 자동re-arm 없음 |
| watchdog | task정지,WDIHIGH/LOW고착,부팅중timeout,ARMCLK고착 | 마지막 유효edge 이후 end-to-end100ms이내TX차단,새edge전복귀없음 |
| CAN fault | CANH/L short/open/교차배선,단일선FT동작 | PHY정격범위시험,장비가정상차량bus를지속점유하지않음 |
| GPS fault |UARTstuck,잘못된baud,길이폭주,PPS손실,power short | 센서만invalid,CANcapture영향차단,DR45초상한 |
| 원격 mic |0.5/1/3m,온도,SD클리핑,탈착/단락 | ESPsetup/skew여유,FFT잡음/오토볼륨발산없음 |
| RTC |전원차단,OSflag,잘못된날짜,timejump | invalid시일몰경고오판안함,TTL/lease영향없음 |

현 단계에서 위 HIL 항목은 모두 미실행이다. 시험은 먼저 절연된 bench와 dummy load/CAN analyzer에서 하고, 차량 연결은 기존 gate 순서를 따른다.
