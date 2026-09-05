# R1 제조사 원문 참조색인

2026-09-05 기준 `pdf/`에 저장된 **PDF 54개, 총 1,709쪽**의 사용처를 연결한다. [manifest](manifest.json)의 저장 문서 54개와 실제 파일을 대조했다. [참조 입력](../../tools/hardware/references.json)은 56개 항목이며, 그중 AN5093와 MAX20040 단독 회로도는 저장되지 않았다. 최신 MAX Rev 16과 land 90-0409의 미확보 상태는 아래에서 별도로 관리한다. 이 색인은 회로·PCB 제작 승인서가 아니다.

## 정본과 읽는 방법

| 찾는 정보 | 정본·해석 |
|---|---|
| 공식 출처·실제 미러·파일 무결성 | [manifest.json](manifest.json)의 같은 `id` 항목: `url`, `download_url`, `file`, `bytes`, `sha256`. `download_url`이 없으면 별도 미러가 기재되지 않은 것이며, 최종 HTTP 리다이렉트까지 기록됐다는 뜻은 아니다. |
| 개정·사용 의도 | 같은 manifest 항목의 `use`와 저장 PDF 본문의 개정 표시. 현재 manifest에는 독립 `revision` 필드가 없고 일부 개정만 `use`에 기록돼 있다. 미기재 개정을 추정하거나 모든 저장본을 최신판으로 간주하지 않는다. 수집 입력의 `use`는 [references.json](../../tools/hardware/references.json)에서 추적한다. |
| 전원·CAN 채택 결정 | [Communicator 회로](../../docs/hardware/r1/communicator-circuit.md), [핀맵·FW 계약](../../docs/hardware/r1/firmware-pinmap.md) |
| 센서·GNSS·원격 수음 결정 | [위치·관성 하드웨어](../../docs/hardware/r1/navigation-hardware.md), [Bridge·Controller·마이크](../../docs/hardware/r1/bridge-controller-microphone.md) |
| 아직 승인하지 않은 조건 | [검증·제작 전 조건](../../docs/hardware/r1/verification.md), [PDF 확보·실패 기록](../../docs/hardware/r1/pdf-acquisition.md) |

아래 파일 링크의 이름은 manifest의 `id`와 동일하다. **모든 쪽수는 표지부터 세는 PDF 1기준**이며 `#page=N`은 첫 참조 쪽이다. 뷰어가 이를 무시하면 쪽 번호를 직접 입력한다. 절·표 이름은 저장본에서 찾을 검색어다. `use`에 적힌 예전 쪽수보다 아래 저장본 기준 위치를 우선한다. 특히 TPS629210 MODE/VSET 표는 11~13쪽, SN74LV1T125 핀 표는 4쪽이다.

MTi 사용자 매뉴얼만은 **PDF 쪽 = 인쇄 쪽 + 3**이다. 예를 들어 인쇄 78쪽 전원 절은 PDF 81쪽이다. 이 보정을 다른 PDF나 별도의 Xsens GNSS 응용 노트에 일괄 적용하지 않는다.

## 1. 차량 입력·보호·AUTO5V — 12개

[Communicator 회로 §2~3](../../docs/hardware/r1/communicator-circuit.md)의 출처다. 제조사 응용회로를 출발점으로 쓰되 TVS 에너지, FET SOA, 보상 루프와 기동 조건은 실제 운용점에서 별도 검증한다.

| 저장 PDF / 전체 쪽수 | 정확한 참조 쪽·절 | 회로 반영 또는 배제 사유 |
|---|---|---|
| [lm7480](pdf/lm7480.pdf#page=3) / 46 | 3쪽 §6 핀, 4~9쪽 §7 정격·전기 특성, 29~32쪽 §10.3 공통 소스 응용, 35쪽 배치, 43~45쪽 DRR 도면·land·paste | LM74800과 공통 소스 FET 2개, OV 차단·VS/CAP 공급에 반영. EP는 FLOAT이며 일반 land 예시를 이유로 GND via를 넣지 않는다. 전류 제한 기능으로 해석하지 않는다. |
| [lm7480-evm](pdf/lm7480-evm.pdf#page=4) / 15 | 4쪽 §3/Fig. 3-1 회로, 9~10쪽 §6.1 배치, 11~12쪽 BOM | 공통 소스 연결·부품 선택 참고. EVM의 CS 15nF는 그대로 채택하지 않고 아래 TI 교정 사례와 데이터시트에 따라 1µF로 반영했다. |
| [lm7480-surge](pdf/lm7480-surge.pdf#page=7) / 13 | 4~6쪽 Design #1~2, 7쪽 Design #3 공통 소스·load dump | 공통 드레인/단일 이상 다이오드와 구분해 공통 소스 구성을 선택. 예제의 200V 표기는 R1의 surge 시험 통과 근거가 아니다. |
| [buk9y12-100e](pdf/buk9y12-100e.pdf#page=2) / 13 | 2쪽 §5 핀, 2~4쪽 한계·SOA/Fig. 3, 5쪽 과도 열 특성, 10쪽 §11 SOT669 외형 | Q1/Q2의 100V LFPAK56, S/G/드레인 베이스 대응에 반영. RDS(on)만으로 load-dump 생존을 승인하지 않는다. |
| [bas21h](pdf/bas21h.pdf#page=2) / 10 | 2쪽 §2 핀, 3~5쪽 정격·특성, 6~7쪽 §9·11 외형·납땜 land | LM74800 CS→VS bootstrap 공급의 200V 다이오드. 극성과 SOD123F를 보존한다. |
| [bzt52h](pdf/bzt52h.pdf#page=2) / 14 | 2쪽 §5 핀, 4~10쪽 §8~10 정격·제너 특성, 11쪽 §11~12 외형·land | VS의 47V 제너 clamp에 반영. 제너 허용차·전류·손실을 포함해 검증하며 nominal 47V만으로 보호 완료를 주장하지 않는다. |
| [littelfuse-451](pdf/littelfuse-451.pdf#page=2) / 4 | 2쪽 품목별 전기 정격·I²t, 3쪽 시간/전류·온도 곡선, 4쪽 치수·권장 패드 | 0451001.MRL 1A와 Nano2 패키지에 반영; 1206 대체 금지. **2023-09-18 구판**, 확인된 공식 2025-12-01 개정과 다르다. 외부 하네스 퓨즈와 협조 시험이 남아 있다. |
| [smbj](pdf/smbj.pdf#page=2) / 6 | 1~3쪽 최대·품목별 전기 정격, 4쪽 펄스/열 곡선, 5쪽 SMB 외형·패드 | SMBJ36CA 입력 TVS. 600W/10·1000µs 정격을 수백 ms load dump 에너지 보장으로 확대하지 않는다. |
| [max20040](pdf/max20040.pdf#page=9) / 20 | 2~6쪽 정격·전기 특성, 3쪽 패키지 표, 9~10쪽 핀, 14~17쪽 FB/보상 설계, 18쪽 배치, 20쪽 개정 이력 | MAX20040B, 5.0875V nominal 분압, 두 LX/BST 루프·Kelvin sense의 근거. **Rev 15, 3/24 구판**이며 Rev 16의 절대최대정격 Note 1·2 변경을 대체하지 못한다. 3쪽의 land ID만으로 footprint 치수 검증을 완료하지 않는다. |
| [max20040-evm](pdf/max20040-evm.pdf#page=3) / 8 | 3쪽 BOM의 D1 BAT54ALT1G, 4쪽 `MAX20040 EV Kit Schematic`, 8쪽 개정 이력 | **Rev 3, 3/24**. 공통 anode D1의 외부 bootstrap 충전 경로를 반영하되 R1은 PMEG4010BEA 2개를 쓴다. 보상 부품은 출발값일 뿐 수정 출력/PCB의 안정성 승인값이 아니다. 단독 schematic PDF와는 다른 문서다. |
| [pmeg4010bea](pdf/pmeg4010bea.pdf#page=1) / 8 | 1쪽 §5 핀, 2~3쪽 §8~10 정격·누설/순방향 특성, 4~5쪽 외형·land | AUTO_VCC4→BST1/BST2 각각 A→K의 40V Schottky. pin1=K/pin2=A, SOD323을 적용. 고온 누설과 충전 peak/역전압은 실측 대상이다. |
| [coilcraft-xal5030](pdf/coilcraft-xal5030.pdf#page=1) / 4 | 1쪽 XAL50xx 규격 표, 2~3쪽 L/전류 곡선, 4쪽 외형·패드/포장 | XAL5030-472MEC 4.7µH 출발값. 포화·RMS 전류와 온도별 유효 L을 확인하며, 작은 외형만으로 인덕터를 교체하지 않는다. |

## 2. 전원 선택·USB-C·레일 판정 — 9개

[Communicator 회로 §3~4](../../docs/hardware/r1/communicator-circuit.md)에 연결된다. 전원 선택 상태와 PGOOD, 하락 검출 지연과 회복 지연을 서로 구분한다.

| 저장 PDF / 전체 쪽수 | 정확한 참조 쪽·절 | 회로 반영 또는 배제 사유 |
|---|---|---|
| [tps629210](pdf/tps629210.pdf#page=11) / 51 | 3쪽 핀, 5~6쪽 전기 특성, 11쪽 §8.3.1 MODE, 12~13쪽 §8.3.3/Table 8-2 VSET, 20~22쪽 L/C 선정, 40쪽 배치 | SYS3V3: 249k VSET, 27.4k MODE, 2.2µH/22µF 출발값. `use`의 15~17쪽을 VSET 표 위치로 그대로 인용하지 않는다. 임의 bulk 추가는 재검증한다. |
| [murata-dfe252012](pdf/murata-dfe252012.pdf#page=1) / 1 | 1쪽 DFE252012P 규격·2.2µH 품목·치수·권장 패드 | 시스템 buck의 2.2µH 후보와 land 참조. 실제 공급 MPN의 전류·높이·유효 L 확인이 필요하다. |
| [tps2116](pdf/tps2116.pdf#page=3) / 25 | 3쪽 핀, 4~6쪽 정격·전기 특성, 12~15쪽 §7.6/§8 우선선택·응용, 17쪽 배치 | AUTO5V 우선/USB 보조 mux와 역류 차단. ST를 PGOOD로 쓰지 않으며 전환 중 droop·유한 역류 응답을 시험한다. |
| [tps2553](pdf/tps2553.pdf#page=13) / 33 | 3쪽 핀, 4~6쪽 정격, 13~14쪽 §8.5.1/Fig. 22 ILIM, 16~21쪽 응용, 30~32쪽 DBV 외형·land·paste | USB 약1A 및 GPS 전류 제한. RILIM 오차/최소·최대를 반영한다. current limit는 과전압 보호가 아니며 무조건 자동 재시도 회로를 복제하지 않는다. |
| [tusb320lai](pdf/tusb320lai.pdf#page=3) / 38 | 3쪽 핀, 10쪽 모드 표, 11쪽 §7.2.2 전류 광고, 14쪽 §7.3.3 dead battery, 26~29쪽 UFP 응용·전원·배치 | PORT=GND, ADDR=NC GPIO sink와 내부 Rd. OUT1 광고에 따라 전원을 허용한다. 외부 Rd 중복·A-to-C/default-current 기동·고전압 PD 협상은 채택하지 않는다. |
| [tps7a20](pdf/tps7a20.pdf#page=3) / 63 | 3~4쪽 패키지별 핀, 5~7쪽 정격, 27~31쪽 §7 커패시터·손실·응용/배치 | PHY/GPS 전원 도메인 및 원격 mic 3.3V/1.8V LDO에 사용. SYS3V3와 PHY3V3를 합치지 않고 dropout·열·역급전을 검증한다. |
| [tlv803e](pdf/tlv803e.pdf#page=4) / 52 | 4~5쪽 DPW 포함 핀, 8~9쪽 임계·reset 시간, 17~19쪽 reset 동작, 20쪽 응용 | SYS/PHY 3.3V reset의 A30 3.08V, release 130~270ms 기준. USB 선연결 뒤 PHY 기동도 초기화하며 MCU 내부 BOR만으로 대체하지 않는다. |
| [tps3700](pdf/tps3700.pdf#page=4) / 27 | 4쪽 핀, 6~7쪽 정확도·타이밍, 13~18쪽 분압·응용 | PROTECTED_VBAT 약8V 판정. 임계·누설·저항 오차를 포함한다. 최종 AUTO5V의 좁은 창은 별도 TPS3890이 맡는다. |
| [tps3890](pdf/tps3890.pdf#page=5) / 27 | 3쪽 핀, 5쪽 §7.5~7.6, 12~16쪽 지연·동작·응용, 24~26쪽 DSE 외형·land·paste | AUTO5V의 1.15V 기준 supervisor와 CT 회복 지연. 하락 지연 max 미명시·작은 collapse 여유를 남은 시험 조건으로 유지한다. |

## 3. CAN 물리계층·독립 TX 차단 — 11개

[Communicator 회로 §5~6](../../docs/hardware/r1/communicator-circuit.md)의 근거다. 수신 허용과 차량 송신 허용은 별개이며, 데이터시트의 기능만으로 차량별 bus 종류나 고장 안전성을 확정하지 않는다.

| 저장 PDF / 전체 쪽수 | 정확한 참조 쪽·절 | 회로 반영 또는 배제 사유 |
|---|---|---|
| [tcan1046av](pdf/tcan1046av.pdf#page=4) / 43 | 4~5쪽 §7 핀, 6~12쪽 정격, 20~25쪽 핀 동작/standby, 26~30쪽 응용·배치 | CAN1/2 PHY. DYY14의 5=GND2, 7=RXD2, 8=STB2, 11=VIO를 반영. reset 중 TXD/STB 외부 pull-up을 유지한다. |
| [tcan1046-evm](pdf/tcan1046-evm.pdf#page=4) / 18 | 4쪽 §1.2.1 회로, 7쪽 §2.1 전원 구성 | bypass와 CAN FD 종단 참고. 차량 중간 노드의 split termination은 DNP이며 EVM 종단/CMC 구성을 전부 복제하지 않는다. |
| [max3055](pdf/max3055.pdf#page=11) / 19 | 1쪽 기본회로, 2~6쪽 정격, 11쪽 핀 설명, 12쪽 INH/기능도, 15~16쪽 모드 표·bus 종단, 17쪽 EMI/반사 | 검증된 저속 FT CAN3에만 사용. BATT/WAKE/INH/ERR과 RTH·RTL 분산 종단을 반영하며 CANH–CANL 120Ω 및 FD 운용은 배제한다. 4.7k는 전체망 100Ω을 혼자 구성한다는 뜻이 아니다. |
| [esd2can24](pdf/esd2can24.pdf#page=3) / 26 | 3쪽 핀, 4~6쪽 정격·용량, 12~13쪽 §7~9 응용·배치 | CAN 포트 ESD 보호와 짧은 return. ESD 정격을 차량 load-dump 전력 흡수 능력으로 해석하지 않는다. |
| [sn74lvc2g125](pdf/sn74lvc2g125.pdf#page=4) / 30 | 4쪽 핀, 5~7쪽 정격·Ioff, 11쪽 기능, 12~14쪽 응용/OE·배치 | FD TX/STB와 GNSS 전원 도메인 격리. 상대 rail과 OE 기본 pull을 함께 설계하며 단순 버퍼 추가만으로 전원 순열 시험을 대신하지 않는다. |
| [sn74lvc2g17](pdf/sn74lvc2g17.pdf#page=3) / 36 | 3쪽 핀, 4~5쪽 정격·입력/Ioff, 8~11쪽 기능·응용 | MAX3055 RXD/ERR 및 도메인 sense의 5V tolerant Schmitt 입력. 무전원 안전 근거 없는 단순 저항 분압안을 배제한다. |
| [sn74lv1t125](pdf/sn74lv1t125.pdf#page=4) / 26 | 4쪽 핀, 5~6쪽 입력 허용 범위·TTL 임계, 12~13쪽 기능·전원 | FT TX/EN 출력을 MAX3055 자체 AUTO5V rail에 맞춘다. 입력 tolerance와 일반적인 출력 Ioff 보장을 혼동하지 않는다. |
| [sn74lvc1g04](pdf/sn74lvc1g04.pdf#page=3) / 47 | 3쪽 핀, 4~6쪽 정격, 10~12쪽 진리표·응용 | TX_PERMIT→active-low OE 및 논리 극성 변환. reset/무전원 외부 pull을 생략하지 않는다. |
| [sn74lvc1g11](pdf/sn74lvc1g11.pdf#page=3) / 37 | 3쪽 핀, 4~5쪽 정격, 9쪽 진리표, 10~12쪽 응용 | rail/reset/물리 ARM/watchdog의 hardware AND에 반영. 소프트웨어 flag 하나로 permit을 우회하지 않는다. |
| [sn74lvc1g74](pdf/sn74lvc1g74.pdf#page=4) / 31 | 4쪽 핀(Q=5, 반전 Q=3), 6~8쪽 동작·타이밍, 10쪽 비동기 clear 진리표 | 고장 시 arm latch clear, 회복 뒤 새로운 STM 상승 edge로만 재무장. watchdog 회복에 따른 자동 TX 복귀를 배제한다. |
| [tps3431](pdf/tps3431.pdf#page=3) / 31 | 3쪽 핀, 5~7쪽 전기·타이밍, 9~13쪽 watchdog/기동·시간 설정, 17~18쪽 가변 시간 응용 | 외부 WDI health 감시와 WDO latch 해제. 120pF 출발값의 오차를 계산하며 무조건 동작하는 timer/DMA 토글을 건강성 증거로 쓰지 않는다. |

## 4. MCU·무선 모듈·커넥터 — 7개

[핀맵·FW 계약](../../docs/hardware/r1/firmware-pinmap.md)에 적용한다. GPIO 번호, chip pad, module pad와 케이블 mating face를 구분한다.

| 저장 PDF / 전체 쪽수 | 정확한 참조 쪽·절 | 회로 반영 또는 배제 사유 |
|---|---|---|
| [stm32g474](pdf/stm32g474.pdf#page=49) / 236 | 49쪽 §4.1 UFQFPN48, 56~72쪽 Table 12 핀, 73~79쪽 Table 13 AF, 82~88쪽 정격/전원, 117~118쪽 HSE, 133쪽 NRST, 203~205쪽 §6.2 외형·권장 footprint | DS12288 Rev 6 저장본. STM32G474CEU6의 3 FDCAN/UART/SWD·전원·EP49=GND에 반영. 다른 package 표나 PG10 GPIO 설정을 복사하지 않는다. AN5093 저장본을 확보했다는 뜻은 아니다. |
| [esp32s3-mini](pdf/esp32s3-mini.pdf#page=10) / 53 | 10~12쪽 §3 핀, 14~16쪽 boot/reset, 27~29쪽 정격/소비전류, 39~41쪽 모듈·주변회로, 43~46쪽 치수·land/배치 | Communicator N4R2의 pad/USB/UART/INS 연결과 RF keepout. PSRAM용 핀을 회수하지 않으며 WROOM pad 번호를 혼용하지 않는다. |
| [esp32s3-wroom](pdf/esp32s3-wroom.pdf#page=10) / 53 | 10~12쪽 §3 핀, 14~16쪽 boot/reset, 27~29쪽 정격/소비전류, 39~41쪽 모듈·주변회로, 43~46쪽 치수·land/배치 | Bridge N8R2, native USB·BOOT/PAIR 최소 보드에 반영. 별도 CAN PHY·TX 권한이나 SD 회로 추가 근거로 쓰지 않는다. |
| [abracon-abm8](pdf/abracon-abm8.pdf#page=1) / 2 | 1쪽 표준 규격·주문 옵션, 2쪽 외형·권장 패드 | 16MHz/CL18pF HSE crystal 출발값. 부하용량·stray·drive limit를 확인하며 STM의 8MHz 예시 값을 그대로 복제하지 않는다. |
| [gct-usb4105](pdf/gct-usb4105.pdf#page=1) / 2 | 1쪽 전기 정격·접점·기계/PCB 도면 | USB-C receptacle의 A/B 접점·shield·패드 대응. 기계적 전류 정격이 CC 광고나 USB 입력 예산을 대신하지 않는다. |
| [jst-gh](pdf/jst-gh.pdf#page=2) / 6 | 1쪽 정격, 2쪽 PCB/조립 도면, 3쪽 housing/header·No.1 표시, 6쪽 품번 체계 | CAN/GNSS/PPS/mic 하네스의 GH 접점·pin1 방향. 기계 탭 MP는 신호 pad가 아니며 PCB top view와 mating face를 뒤집어 확인한다. |
| [jst-xh](pdf/jst-xh.pdf#page=1) / 9 | 1쪽 3A/AWG22 정격, 2쪽 PCB/조립, 3~5쪽 contact/housing/header | 차량 입력 XH2의 극성·crimp·기계 치수. 차량 OEM 커넥터 pinout 근거가 아니며 외부 퓨즈를 생략하지 않는다. |

## 5. GNSS·INS·기압계 — 5개

[위치·관성 하드웨어](../../docs/hardware/r1/navigation-hardware.md)의 선택 근거다. GNSS 모델/firmware와 PPS·프로토콜 호환성은 실물 조합 시험 전까지 후보 상태다.

| 저장 PDF / 전체 쪽수 | 정확한 참조 쪽·절 | 회로 반영 또는 배제 사유 |
|---|---|---|
| [xsens-mti1-current](pdf/xsens-mti1-current.pdf#page=14) / 96 | PDF 14~18쪽(인쇄 11~15) 핀/strap, PDF 19~27쪽(16~24) MTSSP/SPI, PDF 34쪽(31) barometer·36쪽(33) outage, PDF 41~44쪽(38~41) 정격/footprint, PDF 48쪽(45) 세대별 외형, PDF 81~88쪽(78~85) 전원·연결, PDF 93쪽(90) footprint | 2025-04-23 매뉴얼의 MTI-7-5A-T v5. SPI mode3/≤2MHz·filler/guard·open-drain reset, AUX SPI BMP384, VDDA 필터에 반영. GNSS outage 45초 이후 위치/속도 중단을 명시하고 일반 AHRS·무제한 DR로 확대하지 않는다. |
| [xsens-gnss](pdf/xsens-gnss.pdf#page=16) / 31 | 7쪽 §2.3 지원 수신기/인터페이스, 16~17쪽 §4 u-blox 설정, 20~21쪽 §7 NMEA, 22~25쪽 상태·동기·안테나 | MTAN001 Rev A의 GNSS 설정 ownership와 상태 검증 참고. F9P/MTi 실제 버전 조합을 무조건 호환으로 승격하지 않는다. 이 문서는 쪽수 +3 보정 대상이 아니다. |
| [bmp384](pdf/bmp384.pdf#page=45) / 58 | 6~8쪽 §1~2 정격, 41~44쪽 SPI/타이밍, 45~46쪽 §6 핀·SPI 연결, 48쪽 §7.1~7.2 외형·land, 51~53쪽 실장 | MTi AUX SPI 전용 10pad 센서와 100nF bypass. I²C 스캔 대상이 아니며 bottom drawing을 PCB top view와 대조한다. pressure port 코팅/접착제 차폐는 금지한다. |
| [holybro-f9p](pdf/holybro-f9p.pdf#page=2) / 5 | 2쪽 SKU12018 규격, 3쪽 Pin Map, 4~5쪽 케이스·안테나 치수 | H-RTK F9P Helical UART형 출발점. URL 파일명은 V1.0이지만 **저장 본문은 2020-11 V1.1**이다. GH10 데이터와 GH6 PPS를 분리하고 DroneCAN형·다른 SKU와 혼용하지 않는다. |
| [we74279279](pdf/we74279279.pdf#page=1) / 7 | 1쪽 전기 특성·권장 land, 2쪽 임피던스/온도 derating | MTi v5 VDDA 필터의 0402/600Ω@100MHz ferrite 참고. 전원 필터의 DC 저항·전류를 검증하며 CAN CMC로 전용하지 않는다. |

## 6. Controller·RTC·원격 마이크 — 7개

[Bridge·Controller·마이크](../../docs/hardware/r1/bridge-controller-microphone.md)에 적용한다. raw I²S 장거리 배선, 카메라와 GPIO 동시 사용, T5848의 3.3V 직접 구동은 배제한다.

| 저장 PDF / 전체 쪽수 | 정확한 참조 쪽·절 | 회로 반영 또는 배제 사유 |
|---|---|---|
| [waveshare-controller](pdf/waveshare-controller.pdf#page=1) / 1 | 1쪽 전체 schematic 중 J8 확장 헤더·camera/audio GPIO·RTC 전원 회로 | 기존 Controller를 복제하지 않고 최소 mating 어댑터를 만든다. GPIO38/39/40 사용 시 카메라를 비활성화/분리하고 J8 성별·높이·삽입 방향은 실물 확인한다. |
| [pcf85063](pdf/pcf85063.pdf#page=22) / 65 | 5쪽 §7.2 핀, 22쪽 §8.3.1.1 OS flag, 35쪽 §9.5.1~2 주소/읽기·쓰기, 46쪽 응용 | Waveshare가 연결한 PCF85063A Rev 7 원문. 기존 RTC 0x51을 재사용하고 OS flag·날짜·backup 유지/readback을 확인한다. 정확한 보드 suffix나 시간 유효성을 단순 I²C 응답만으로 단정하지 않는다. |
| [t5848](pdf/t5848.pdf#page=7) / 42 | 5쪽 HQ 조건, 7쪽 I²S 전기/타이밍, 8쪽 최대 정격, 10쪽 핀, 30~31쪽 I²S 형식, 35쪽 bypass, 37쪽 land/paste, 39쪽 package | DS-000479 Rev 1.0. 1.8V, pin7=VDD/pin6=SCK, LR=GND와 음향 NPTH를 적용. R1은 32kHz/64fs(2.048MHz); `use`의 48kHz를 현재 설정으로 옮기지 않는다. 16kHz/64fs는 HQ 범위 밖이며 지연·유효 bit를 실측한다. |
| [sn65lvds1](pdf/sn65lvds1.pdf#page=3) / 44 | 3쪽 driver/receiver 핀, 4~8쪽 정격·전기/스위칭, 14~17쪽 기능, 19~27쪽 응용·종단/전송선 | SN65LVDS1/2로 BCLK·WS·DATA 세 차동 pair를 구성하고 receiver 끝에만 100Ω. 수m raw I²S 및 RJ45/PoE 오접속 가능 구성을 배제한다. 3m는 SI 승인 길이가 아닌 목표다. |
| [sn74axc1t45](pdf/sn74axc1t45.pdf#page=3) / 45 | 3쪽 핀, 5~6쪽 정격, 17쪽 §7.3.4 Ioff, 18~20쪽 고정 방향/단방향 응용 | mic의 1.8↔3.3V 고정 방향 변환과 전원 도메인 분리. LVDS 자체가 mic VDD 정격을 해결한다고 가정하지 않는다. |
| [tpd2e2u06](pdf/tpd2e2u06.pdf#page=3) / 23 | 3쪽 핀/정격, 4쪽 용량·전기 특성, 10~12쪽 응용·배치 | USB/GNSS/LVDS 커넥터 근처 ESD 보호. 용량·return 경로를 포함해 신호 품질을 확인한다. |
| [bourns-psmf](pdf/bourns-psmf.pdf#page=1) / 7 | 1쪽 품목 전기 정격, 2쪽 온도 derating, 3쪽 trip 곡선, 4쪽 리플로 조건 | MF-PSMF010X-2 마이크 전원 PTC: 100mA hold/300mA trip 출발점. 정밀 전류 제한기·즉시 차단기로 해석하지 않으며 고온 hold·저항·단락 열을 시험한다. |

## 7. 비교용으로 보존한 미채택 부품 — 3개

저장돼 있다는 이유로 현재 BOM의 실장 부품으로 간주하지 않는다.

| 저장 PDF / 전체 쪽수 | 정확한 참조 쪽·절 | 배제 사유 |
|---|---|---|
| [sn74lvc125a](pdf/sn74lvc125a.pdf#page=3) / 37 | 3쪽 핀, 4~6쪽 정격, 9~12쪽 tri-state/OE 응용 | 과거 4채널 TX gate 비교 근거. 현재 R1은 도메인별 SN74LVC2G125/SN74LV1T125를 사용하므로 이 문서를 현재 buffer pinmap에 적용하지 않는다. |
| [sn74ahct1g125](pdf/sn74ahct1g125.pdf#page=3) / 26 | 3쪽 핀, 4~5쪽 정격·전기 특성, 8~11쪽 기능·응용 | FT TX/EN 비교 후 미채택. 명시되지 않은 Ioff/무전원 보호를 부품 계열명에서 추정하지 않는다. |
| [tps3808](pdf/tps3808.pdf#page=6) / 39 | 6~7쪽 전기 특성, 10~13쪽 임계·히스테리시스·reset 동작, 14~15쪽 응용 | Rev N 비교본. 좁은 FT 5V 판정 창의 최악 정확도·히스테리시스 조건 때문에 미채택; 최종 AUTO5V 판정은 TPS3890을 참조한다. |

## 8. 저장되지 않은 원문과 웹 열람 근거

아래 항목은 **54개 저장 PDF에 포함하지 않는다**. manifest의 `failures`는 AN5093/단독 schematic 두 항목만 담으므로, 그 배열만 보고 최신 개정·land 원문이 모두 확보됐다고 판단하면 안 된다. 이번 색인 작업에서는 새 다운로드를 시도하지 않았다.

| 원문·공식 경로 | 확인 범위와 남은 제한 |
|---|---|
| [MAX20039/MAX20040 Rev 16](https://www.analog.com/media/en/technical-documentation/data-sheets/max20039-max20040.pdf) | 확인된 공식 Rev 16(4/25), 20쪽은 **바이너리 미확보**. 저장 Rev 15와 구분한다. 특히 2~3쪽 절대최대정격/관련 Note 1·2와 20쪽 개정 이력을 다시 확인해야 한다. `max20040-rev16.pdf`는 존재하지 않는다. |
| [ST AN5093](https://www.st.com/resource/en/application_note/an5093-getting-started-with-stm32g4-series--hardware-development-boards-stmicroelectronics.pdf) | 이전 웹 열람에서 Rev 2(2019-10), 35쪽 확인. 전원 6~13쪽, HSE 20쪽, debug·권고·참조회로 25~33쪽이 사용 범위다. **로컬 바이너리 미확보**이므로 이번 Windows PDF 검증 대상이 아니다. |
| [ADI 패키지/land 원문 포털](https://www.analog.com/en/resources/packaging-quality-symbols-footprints/package-index.html) — 21-100068 / 90-0409 | MAX 저장본 3쪽이 outline 21-100068과 land 90-0409를 지정한다. **land 90-0409 원문 미확보**이며 치수 overlay·pad1·EP/paste·via 승인은 남아 있다. 후보 KiCad footprint를 제조사 land 검증 완료로 표시하지 않는다. |
| [MAX20040 단독 schematic](https://www.analog.com/media/en/technical-documentation/eval-board-schematic/max20039_40_evkit_schematic.pdf) | 이전 웹 열람의 1쪽 자료이나 **로컬 바이너리 미확보**. 저장 EVM 4쪽은 bootstrap 근거로 사용할 수 있지만, 이를 추출해 원본 단독 PDF로 대체하지 않는다. |
| [MAX20090/MAX20090B 공식 PDF](https://www.analog.com/media/en/technical-documentation/data-sheets/MAX20090-MAX20090B.pdf#page=29) | 이번 웹 열람에서 Rev 9(3/24), 30쪽 중 **29쪽 Package Information 표** 확인. T2044+4C/21-100172와 T2044Y+4C/21-100068이 land 90-0409를 공유한다. **PDF 바이너리는 미확보**이며 shared-land ID의 교차 근거일 뿐 전체 도면 치수·MAX20040 footprint 승인 또는 MAX20090 회로 채택 근거가 아니다. |

## 9. TI·ST 포럼과 현장 사례

다음은 제조사 포럼의 사용자 측정·지원 답변이다. 데이터시트/정격의 대체 정본이나 CANView 실측 결과가 아니며, 적용 부품·회로·증상 조건을 함께 읽는다. 웹 링크만 보존하며 PDF 확보 목록에 넣지 않는다.

| 근거 링크 | 반영·주의 |
|---|---|
| [TI E2E — LM7480-Q1 DGATE Voltage unstable](https://e2e.ti.com/support/power-management-group/power-management/f/power-management-forum/1575206/lm7480-q1-dgate-voltage-unstable) | TI 답변은 EVM CS 15nF를 1µF로 교정하고 사용자는 A pin 가까이에 적용 후 개선을 보고했다. R1 CS 1µF의 근거다. 글 중 charge-pump 100nF→1µF 시도는 효과가 없었다고 보고됐으므로 CS 교정과 혼동하지 않는다. |
| [ST Community — PG10-NRST를 NRST로 구성하는 방법](https://community.st.com/stm32cubemx-mcus-29/how-we-should-do-to-configure-the-pg10-nrst-pin-as-nrst-we-just-choise-gpio-input-60075) | PG10/NRST 기능이 option byte와 관련됨을 확인하는 사례. R1은 NRST 기능을 보존한다. ST 답변도 RM0440 개정에 따라 절 번호가 바뀌었음을 지적하므로 옛 게시글 절 번호를 현재 RM 절로 복사하지 않는다. |
| [ST Community — STM32G474RE PG10 MCO 사례](https://community.st.com/stm32cubemx-mcus-29/how-to-use-mco-at-pg10-nrest-pin-with-stm32g474re-init-code-from-stm32cubemx-always-works-at-pa8-pin-as-mco-function-61312) | PG10를 GPIO/MCO로 돌리면 외부 reset과 debugger의 hardware-reset 접속을 잃을 수 있다는 사용자 사례다. 해당 전환 코드를 R1에 적용하지 않는다. |

GNSS 하네스의 최신 제품별 접점 설명은 [Holybro 공식 UART F9P pinout](https://docs.holybro.com/gps-and-rtk-system/f9p-h-rtk-series/standard-f9p-uart/pinout)을 함께 참조한다. 기존 회로결정의 GH10-5=SDA/GH6-5=PPS를 확인하는 경로이며, 판매 옵션·구매품 도통 검사를 대체하지 않는다.

## 10. 실제 읽음·검증 범위와 저작권

Windows native KiCad Python 3.11.5에 `.tools/hardware-python`의 PyMuPDF 1.26.4를 로드해 54개 파일의 `%PDF`, 바이트 수, SHA-256을 manifest와 비교했고 모두 일치했다. 1,709쪽을 열어 문자 추출을 수행했다. 색인에는 목차/책갈피, 위 참조 쪽의 절·표제와 선택한 핵심 문구를 확인한 범위를 기록했다. **전체 원문 정독, 모든 표 셀·각주 대조, 모든 그림 육안 검토 또는 land 치수 overlay를 끝냈다는 뜻은 아니다.**

문자 추출이 비어 있는 쪽은 `sn74lvc125a` 29쪽, `sn74lvc1g04` 26쪽, `tlv803e` 42쪽, `tps3431` 27쪽, `tps7a20` 56쪽의 5쪽이다. 파일 손상으로 단정하지 않았고 이 쪽들의 OCR/도면 검증 완료도 주장하지 않는다. 제조사 package 도면·부품 변형·reflow/paste와 전원 정격 최악 조건의 최종 승인은 [제작 전 조건](../../docs/hardware/r1/verification.md)에 남긴다.

공개 다운로드 가능 여부와 재배포 허락은 같은 의미가 아니다. **PDF의 저작권은 각 제조사·원저작권자에게 있으며 CANView 프로젝트 라이선스가 적용되지 않는다.** 원문 고지·상표·개정 표시를 보존하고 각 문서의 이용/재배포 조건을 따른다. 미러 저장본이 공식 서버 파일과 바이트 단위로 같다는 검증은 별도이며, 이 색인의 해시 일치는 저장본과 manifest 사이의 무결성만 뜻한다.
