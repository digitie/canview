# 1차 대상 차량: 2017 Tucson TL

이 문서는 [문서 지도](../README.md)에 속한 1차 차량 식별과 evidence 승격 기준이다. signal별 후보와 등급은 [signal catalog](signal-catalog.md)를 따른다.

## 대상 식별

| 항목 | 값 |
|---|---|
| 제조사/차종 | Hyundai Tucson TL |
| 연식 | 2017 |
| 엔진 | 2.0 디젤 |
| 구동 | 4WD |
| 연결 사양 | BlueLink |
| CAN 세대 가정 | Classic CAN 후보. 실차 캡처로 확인 필요 |
| DBC 주 후보 | `dbc/opendbc/hyundai_can.dbc` |

이 문서는 차량을 정확히 식별하기 위한 기준과 검증 계획이다. `commaai/opendbc`의 현재 차량 목록에는 `HYUNDAI_TUCSON` 플랫폼과 2019 디젤·2021 Tucson 관련 문서는 있으나, 2017년식 Tucson TL의 연식·엔진·구동·BlueLink 조합을 별도 fingerprint로 고정한 항목은 확인되지 않았다. 따라서 아래 메시지·신호는 “실차 검증 전 후보”다.

## upstream 매핑의 의미

upstream Hyundai 차량 정의는 일반 Tucson 플랫폼에 `hyundai_can_generated`를 사용한다. 저장소의 `hyundai_can.dbc`는 이 generated 정의를 만들기 위한 현재 generator source다. 이 파일이 2017 TL의 모든 ECU와 모든 옵션을 그대로 보장한다는 뜻은 아니다.

다음 차이는 반드시 보존한다.

- `hyundai_can.dbc`: 현재 공통 Hyundai CAN generator source, 1차 decode 후보
- `hyundai_2015_ccan.dbc`: 구형 Classic CAN 참고 파일
- `hyundai_2015_mcan.dbc`: 구형 Multimedia CAN 참고 파일
- exact 2017 TL profile: 아직 실차 캡처에서 만들어야 하는 프로젝트 검증 결과

## 1차 후보 메시지

아래 ID와 신호명은 `hyundai_can.dbc`에 정의된 값이다. 표시 구현에서는 DBC decode 성공 여부와 freshness를 함께 보존한다.

| 기능 | Message | CAN ID | 주요 신호 | 비고 |
|---|---|---:|---|---|
| 계기판 속도/크루즈 | `CLU11` | `0x4F1` (1265) | `CF_Clu_Vanz`, cruise 상태 | 속도 단위 신호 확인 |
| 내비 제한속도 | `Sign_Detection`/`Navi_HU` | `0x4EC`/`0x544` | `SpeedLim_Nav_*`, source flag | 5W 표시와 unit·valid 대조 |
| 주행거리 | `CLU12` | `0x5B0` (1456) | `CF_Clu_Odometer` | 단위 `km` |
| 네 바퀴 속도 | `WHL_SPD11` | `0x386` (902) | `WHL_SPD_FL/FR/RL/RR` | 각 14 bit, factor 0.03125 km/h |
| 엔진 기본 | `EMS11` | `0x316` (790) | `N`, `TQI`, `VS` | RPM·토크 요구율·속도 후보 |
| 엔진 온도/스로틀 | `EMS12` | `0x329` (809) | `TEMP_ENG`, `TPS`, `ENG_VOL`, `BRAKE_ACT`, `ACC_ACT` | multiplex 상태 확인 |
| 엔진/차량 전압 | `EMS14` | `0x545` (1349) | `VB`, `EMS_VS`, `TEMP_FUEL`, `L_MIL` | DBC 단위 확인 |
| 엔진 상태/예열 | `EMS16` | `0x260` (608) | `ENG_STAT`, `GLOW_STAT`, cruise lamp | 상태 코드 해석 필요 |
| 오일/DPF | `EMS19` | `0x492` (1170) | `CR_Ems_EngOilTemp`, `DPF_LAMP_STAT` | 표시용 후보 |
| 변속기 | `TCU11` | `0x111` (273) | `G_SEL_DISP`, `GEAR_TYPE`, `N_TC`, `TEMP_AT` | 기어 codebook 캡처 필요 |
| 변속기 보조 | `TCU12` | `0x112` (274) | `CUR_GR`, `VS_TCU`, `N_TC_RAW` | 현재 기어 후보 |
| 4WD 상태 | `_4WD11` | `0x428` (1064) | `_4WD_TYPE`, `_2H_ACT`, `_4H_ACT`, `LOW_ACT`, `AUTO_ACT`, `LOCK_ACT` | TL 옵션 확인 필요 |
| 4WD 속도/압력 | `_4WD12` | `0x429` (1065) | `FRSS`, `FLSS`, `RRSS`, `RLSS`, `CLU_PRES` | 후보, 실차 검증 필요 |
| 4WD actuator | `_4WD13` | `0x42A` (1066) | `_4WD_CURRENT`, `_4WD_POSITION`, `_4WD_STATUS` | read-only 진단 후보 |
| 차체 동역학 | `ESP12` | `0x220` (544) | `LAT_ACCEL`, `LONG_ACCEL`, `CYL_PRES`, `YAW_RATE` | status/diagnostic 함께 확인 |
| 조향각 | `SAS11` | `0x2B0` (688) | `SAS_Angle` | signed 0.1 deg 후보 |
| 조향 토크/각 | `MDPS11`/`MDPS12` | `0x381`/`0x251` | `CR_Mdps_StrAng`, `CR_Mdps_StrTq` | 송신 금지, 표시만 |
| TPMS | `TPMS11` | `0x593` (1427) | tire status/pressure bytes | 단위·타이어 위치 검증 필요 |
| 평균 연비 | `CLU13` | `0x50C` (1292) | `CF_Clu_AvgFCI`, `CF_Clu_AvgFCU` | unit enum 확인 |
| 평균·순간 연비 | `CLU_HU_P_05` | `0x5D7` (1495) | `Clu_AFC`, `Clu_IFC` | M-CAN 탑재·수신 확인 |
| 조명·밝기 | `CGW1`/`CLU11`/M-CAN 후보 | profile 확인 | `CF_Gway_HeadLampLow`, `CF_Gway_LightSwState`, `CF_Clu_RheostatLevel`, `C_TailLampActivity` | 자동 밝기 입력, actual lamp 우선 |
| 도어 열림 | `CGW1`/`CGW2` | `0x541`/`0x553` | 앞·뒤·trunk switch | 수신 표시 후보 |
| 도어 잠금 상태 | `GW_IPM_PE_2` | `0x16B` (363) | 도어별 `UnlockState`, TMU feedback | 열림 상태와 별도 검증 |
| SCC | `SCC11`/`SCC12` | `0x420`/`0x421` | ACC/object/status | read-only만 |
| LKAS | `LKAS11` | `0x340` (832) | warning/request 상태 | torque/request 신호 송신 금지 |

## 실차 캡처 계획

차량 배선에 연결하기 전에 CAN gateway와 화면의 전원·보호·listen-only 동작을 벤치에서 검증한다. 그 다음 다음 상태를 각각 별도 파일로 캡처한다.

1. 차량 잠금·시동 OFF 상태
2. ACC/IGN ON, 엔진 미시동
3. 공회전
4. 정차 상태의 변속기 P/R/N/D 전환
5. 저속 직진 및 좌·우 조향
6. 제동·가속 변화
7. 2WD/4WD 관련 switch 또는 주행 모드 변화(차량 기능을 방해하지 않는 범위)
8. BlueLink/인포테인먼트 상태 변화
9. 냉간·열간 운전에서 엔진·DPF·전압 신호 변화
10. 미등·전조등·AUTO 조명과 cluster rheostat 각 단계
11. 냉간 공기압 실측값과 TPMS 표시, 한 바퀴씩 소폭 압력 변화

각 캡처의 메타데이터에 다음을 기록한다.

- 차량 VIN 원문은 저장하지 않거나 마스킹한 식별자
- 캡처 일시와 차량 상태
- 실제 연결 커넥터/핀
- bus 이름과 bitrate
- standard/extended frame 비율
- Communicator firmware와 DBC commit
- 배터리 전압 및 종료 방법

## 판정 기준

| 등급 | 조건 |
|---|---|
| A | 해당 ID가 예상 주기로 나타나고, 신호 변화가 계기판/실측값과 일치 |
| B | ID와 일부 신호는 일치하나 단위·codebook 또는 옵션이 미확인 |
| C | upstream 정의와 이름만 일치하고 실차 의미 검증이 안 됨 |
| X | 해당 차량 캡처에서 보이지 않거나 다른 bus/bitrate로 확인됨 |

이 A/B/C/X는 과거 조사표의 지역적 등급이며 다른 문서의 문자 등급과 자동 변환하지 않는다. runtime은 [구현 준비 §9.2](../architecture/implementation-readiness.md#92-evidence와-품질)의 evidence 등급과 독립 freshness를 사용하며 [T-005](../tasks/T-005-canonical-model.md)가 이를 구현한다. `UNKNOWN`(근거 없음)과 별도 candidate 심사 결과 `REJECTED`(제외/반대 근거)를 같은 등급으로 합치지 않는다. 기본 운전자 화면은 별도 ignition cycle 반복·독립 기준·unit/range/stale·반례와 승인 profile gate를 통과한 VERIFIED 값만 표시한다. B/C 후보는 Signal Lab에서만 읽고 일반 UI에는 `—`다. X의 미지원/반대 evidence와 오래된 sample(STALE)은 서로 다른 상태이며 한 번의 미수신만으로 차량 미탑재를 확정하지 않는다.

기능별 후보, 인포테인먼트·BCM·IPS·도어 잠금의 위험도와 실차 검증 순서는 [CAN 신호 후보 카탈로그](signal-catalog.md)에 정리한다.

## 제어 기능 경계

1차 차량 검증은 수신 전용이다. `SCC`, `LKAS`, `MDPS`, `TCU`, `4WD`, `NM`, `IGN`, 도어 잠금 해제 메시지의 송신은 구현·문서 예제·테스트에서 기본적으로 금지한다. 제어가 필요해지면 차량별 명령의 목적, 주기, alive counter/checksum, 진단 영향, wake-up, 인증, rollback, 물리적 비상 해제부터 별도 검토한다.
