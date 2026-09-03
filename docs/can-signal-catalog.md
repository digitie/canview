# 2017 Tucson TL CAN 신호 후보 카탈로그

## 1. 판정 범위

대상은 2017 Hyundai Tucson TL 2.0 디젤 4WD BlueLink다. 이 조합의 공개 fingerprint는 [commaai/opendbc](https://github.com/commaai/opendbc)에 별도로 고정되어 있지 않다. 이 문서의 모든 ID와 signal은 저장한 upstream [`3e92d112` snapshot](https://github.com/commaai/opendbc/tree/3e92d112129507debe45364891954db70238997a)의 **실차 검증 전 후보**다.

- `C`: DBC 이름과 bit 정의만 있는 후보. 대상 차량 적용 미확인
- `B`: 대상 차량에서 ID·방향·일부 값이 맞지만 enum·scale·주기 중 하나가 미확정
- `A`: 반복 캡처, 독립 계측 또는 순정 표시와 일치하고 stale 기준까지 확정
- `X`: 대상 차량에 없거나 다른 bus/format으로 확인

현재 저장소 기준 확정 등급은 모두 `C`다. 실차 프로파일에서만 `A/B/X`로 승격한다. DBC 세 파일은 message 주기를 정의하지 않으므로 표의 주기는 `실측 필요`로 둔다. C-CAN/M-CAN/FT-CAN 명칭과 실제 Communicator 물리 채널 1/2/3의 대응도 먼저 listen-only로 찾아야 한다.

## 2. 우선 구현 신호

### 2.1 주행·클러스터·내비게이션

| 용도 | Message / ID / DLC | 후보 signal | DBC 변환 | 예상 bus | 등급·정책 |
|---|---|---|---|---|---|
| 지속 속도 | `CLU11` `0x4F1`, 4 | `CF_Clu_Vanz`, `CF_Clu_VanzDecimal` | 0.5 km/h + 0.125 보조 | C-CAN | C, 표시 |
| 독립 속도 | `WHL_SPD11` `0x386`, 8 | `WHL_SPD_FL/FR/RL/RR` | 0.03125 km/h | C-CAN | C, 표시·interlock |
| 엔진 RPM | `EMS11` `0x316`, 8 | `N` | DBC scale 기준 | C-CAN | C, 표시 |
| 종·횡가속 | `ESP12` `0x220`, 8 | `LONG_ACCEL`, `LAT_ACCEL`, status/diag, alive/checksum | 0.01 m/s², offset -10.23 | C-CAN | C, 표시·SPORT 입력 |
| 주행 mode switch | `CLU13` `0x50C`, 8 | `CF_Clu_DrivingModeSwi`, `CF_Clu_ActiveEcoSW` | raw enum 미확정 | C-CAN | C, 수신만 |
| 주행 mode feedback | `HU_CLU_PE_11` `0x1E5`, 8 | `C_DrivingModeState`, `C_DrivingModeOn`, `C_Clu_ActiveEcoSW` | raw enum 미확정 | M-CAN | C, 수신만 |
| 카메라 제한속도 | `Sign_Detection` `0x4EC`, 8 | `SpeedLim_Nav_Cam`, `SpeedLim_Nav_Cam2` | 1 km/h 또는 mph, unit 확인 | 후보 C-CAN | C, 표시 |
| 내비 제한속도 | `Navi_HU` `0x544`, 8 | `SpeedLim_Nav_Clu`, `SpeedLim_Nav_General/Cam` | 1/count, valid flag 조합 | 후보 C-CAN | C, 표시 |
| 상세 제한속도 | `HU_CLU_PE_11` `0x1E5`, 8 | `Navi_SpdLimit`, type/unit, fixed/mobile/red-light camera flags | 1/count | M-CAN | C, 표시 |
| 지도 제한속도 | `HU_Navi_E_00` `0x03E`, 8 | `Navi_SLIF_SpdLimit`, valid/map 정보 | 1/count | M-CAN | C, 표시 |

속도 제한 overlay는 숫자 하나만 수신했다고 켜지 않는다. 값 범위, unit, 일반/카메라 source flag, timestamp가 모두 유효해야 한다. `0`, `255`와 지도 미수록 상태의 의미는 실제 5W 화면과 동시에 캡처해 codebook으로 만든다.

### 2.2 4WD·TPMS

| 용도 | Message / ID / DLC | 후보 signal | DBC 변환 | 등급·정책 |
|---|---|---|---|---|
| 4WD mode·결합 | `_4WD11` `0x428`, 8 | `_4WD_TYPE`, `_2H_ACT`, `_4H_ACT`, `AUTO_ACT`, `LOCK_ACT`, `CLU_DUTY` | duty 1%/bit, 선언 범위 0–64% | C, 표시 |
| torque 후보 | `_4WD11` `0x428`, 8 | `_4WD_TQC_CUR`, `_4WD_SUPPORT`, `_4WD_ERR` | 1 Nm/bit 선언이나 16-bit 전 범위는 비현실적 | C, raw 검증 |
| wheel·clutch | `_4WD12` `0x429`, 8 | `FRSS/FLSS/RRSS/RLSS`, `CLU_PRES` | wheel km/h, pressure 0.0625 bar - 50 | C, 표시 후보 |
| actuator·thermal | `_4WD13` `0x42A`, 6 | `_4WD_CURRENT`, `_4WD_POSITION`, `_4WD_CLU_THERM_STR`, `_4WD_STATUS` | 0.390625 A, 0.015625°, 1% | C, 진단 |
| 공기압 | `TPMS11` `0x593`, 6 | `PRESSURE_FL/FR/RL/RR`, `STATUS_TPMS`, `TPMS_W_LAMP` | pressure 단위 미기재 | C, 표시 |

순정 AWD 화면처럼 네 바퀴 gauge를 그리되 현재 DBC만으로 실제 바퀴별 torque percentage를 만들지 않는다. `CLU_DUTY`나 command torque를 임의로 4개 바퀴에 나누는 계산은 금지한다. 초기 프로파일은 `rear coupling index`로 검증하고, UI의 네 wheel 값은 검증된 모델이 생길 때만 채운다. TPMS 위치는 한 바퀴씩 압력을 바꿔 확인한다.

### 2.3 연비·엔진·DPF

| 용도 | Message / ID / DLC | 후보 signal | DBC 변환 | 등급·정책 |
|---|---|---|---|---|
| 평균 연비 | `CLU13` `0x50C`, 8 | `CF_Clu_AvgFCI`, `CF_Clu_AvgFCU` | 0.1/count, unit enum 0–3 | C, 표시 |
| 평균·순간 연비 | `CLU_HU_P_05` `0x5D7`, 8 | `Clu_AFC`, `Clu_IFC` | 각 0.1/count, 0–99.9 | C, M-CAN 표시 후보 |
| 평균 연비 보조 | `CLU_HU_PE_02` `0x1DA`, 8 | average fuel 관련 값 | DBC range 불완전 | C, 교차검증 |
| 엔진 기본 | `EMS11` `0x316`, 8 | `N`, `TQI`, `VS`, ignition 상태 | torque 계열 0.390625% | C, 표시·SPORT 입력 |
| accelerator·engine temp | `EMS12` `0x329`, 8 | `TPS`, `ACC_ACT`, `TEMP_ENG`, `ENG_VOL`, `BRAKE_ACT` | message multiplex 여부 확인 | C, 표시·interlock |
| 전압·연료온도 | `EMS14` `0x545`, 8 | `VB`, `EMS_VS`, `TEMP_FUEL`, `L_MIL` | `VB` 0.1015625 V | C, 진단 |
| engine state | `EMS16` `0x260`, 8 | `ENG_STAT`, `GLOW_STAT` | enum 미확정 | C, 표시 |
| DPF 경고·오일온도 | `EMS19` `0x492`, 8 | `DPF_LAMP_STAT`, `CR_Ems_EngOilTemp` | lamp raw 0–3, oil 0.75 - 40 °C | C, 표시 |

중요한 한계는 `EMS19`에 DPF soot/load, 재생 진행률, EGT와 차압이 없다는 점이다. 따라서 DPF 화면의 실제 부하율·재생 단계는 UDS/OBD 진단 PID 또는 별도 실차 신호가 검증되기 전까지 데모 데이터로만 둔다. `DPF_LAMP_STAT`도 raw `0–3`의 off/on/blink/fault 의미를 계기판과 대조하기 전에는 정상/경고 문구로 단정하지 않는다.

순간 연비는 M-CAN `Clu_IFC`가 수신되는 경우 이를 우선한다. 수신되지 않으면 injector quantity를 추정식으로 변환해 꾸미지 않고 `—`로 둔다.

## 3. 인포테인먼트·오디오 후보

| 용도 | Message / ID / DLC | 후보 signal | 방향 후보 | 정책 |
|---|---|---|---|---|
| 현재 HU 음량 | `HU_CLU_PE_05` `0x197`, 8 | `HU_VolumeStatus`, `HU_MuteStatus` | HU→CLU/HUD | 수신 표시 |
| AMP 현재 음량·balance | `AMP_HU_PE_03` `0x183`, 8 | `AMP_MainVolumeSetting`, `AMP_BalanceSetting` | AMP→HU | 수신·snapshot 후보 |
| AMP mute·rear mute | `AMP_HU_PE_02` `0x181`, 8 | `AMP_MuteState`, `AMP_RearSpMuteState` | AMP→HU | 수신 feedback 후보 |
| auto volume 상태 | `AMP_HU_PE_01` `0x180`, 8 | `AMP_AutoVolumeState` | AMP→HU | OEM SDVC 충돌 감지 |
| AMP capability | `AMP_HU_P_01` `0x580`, 8 | `AMP_SupportRearSpMute`, `AMP_SupportAutoVolume`, 기타 support bit | AMP→HU | 기능 노출 gate |
| volume·balance 설정 | `HU_AMP_E_08` `0x00F`, 8 | `AMP_MainVolumeSet`, `AMP_BalanceSet` | HU→AMP | 검증 전 송신 금지 |
| mute·rear mute 설정 | `HU_AMP_E_02` `0x009`, 8 | `AMP_Mute`, `AMP_RearSpMute` | HU→AMP | 검증 전 송신 금지 |
| auto volume 설정 | `HU_AMP_E_01` `0x008`, 8 | `AMP_AutoVolume` | HU→AMP | OEM SDVC와 동시 사용 금지 |

현재 volume만 UI에 표시하고 ± 버튼은 제공하지 않는다. 취침/뒷좌석+는 단일 raw 신호가 아니라 적용 전 AMP snapshot과 feedback을 가진 profile transaction으로 구현한다. fader/balance의 byte order·중립점·step 방향, rear mute capability, HU command counter/주기와 차량 AMP 탑재 여부가 모두 확인되기 전에는 송신하지 않는다.

## 4. BCM·IPS·도어·전원 후보

### 4.1 수신 표시

| 용도 | Message / ID / DLC | 후보 signal | 정책 |
|---|---|---|---|
| 앞문·트렁크 | `CGW1` `0x541`, 8 | `CF_Gway_DrvDrSw`, `CF_Gway_AstDrSw`, `CF_Gway_TrunkTgSw` | 수신 표시 |
| 뒷문 | `CGW2` `0x553`, 8 | `CF_Gway_RLDrSw`, `CF_Gway_RRDrSw` | 수신 표시 |
| 통합 도어 | `GW_DDM_PE` `0x521`, 8 | `C_DRVDoorStatus`, `C_ASTDoorStatus`, `C_RLDoorStatus`, `C_RRDoorStatus`, `C_TrunkStatus` | 2-bit codebook 후 표시 |
| M-CAN 도어 | `GW_DDM_PE` `0x1EB`, 8 | 같은 계열 2-bit status | C-CAN과 gateway 상관 분석 |
| 도어별 잠금 | `GW_IPM_PE_2` `0x16B`, 8 | `C_DRVUnlockState`, `C_ASTUnlockState`, `C_RLUnlockstate`, `C_RRUnlockState`, `C_TMULockFeedBack` | 상태 표시 후보 |
| 미등·전조등 | `CGW1` `0x541`, `CGW2` `0x553`, `GW_IPM_PE_1` `0x16A/0x522` | headlamp, light switch, `C_TailLampActivity` | 자동 밝기·상태 입력 |
| 밝기 의도 | `CLU11` `0x4F1` | `CF_Clu_RheostatLevel` 0–31 | 방향 검증 후 자동 밝기 |
| IGN·starter | `CGW1` `0x541` | `CF_Gway_IGNSw`, `Ign1`, `Ign2`, `StarterRlyState` | 상태·interlock |
| 배터리 sensor | `BAT11` `0x549`, 8 | current, SOC, voltage, temperature, SOH, invalid/error | 외부 계측 대조 후 표시 |
| IPS/SJB 진단 | `CGW2` `0x553` | `CF_Gway_IPMDiagState`, `CF_Gway_SJBDiagState` | 진단 flag만 표시 |

도어 열림, 실제 잠금 상태, 잠금 요청 feedback은 서로 다른 의미다. `locked = !door_open`이나 `command accepted = all doors locked`로 계산하지 않는다. `0x16B`가 없거나 codebook이 불완전하면 잠금 상태는 미지원으로 둔다. 공개 DBC에는 개별 Intelligent Power Switch 출력 명령이 없으므로 `IPM/SJB diag` bit를 장치 제어로 오해하지 않는다.

### 4.2 잠금 제어 검토

| 후보 | signal | 결론 |
|---|---|---|
| M-CAN `TMU_GW_E_01` `0x043`, 8 | `C_ReqDrLock`, `C_ReqDrUnlock`, hazard/horn/engine operate | 잠금은 연구 보류, 잠금 해제·시동은 금지 |
| C-CAN `TMU_GW_E_01` `0x53A`, 8 | `CF_Gway_TeleReqDrLock`, `CF_Gway_TeleReqDrUnlock` 등 | gateway 변환 후보, 검증 전 송신 금지 |
| `CGW1/2` key·RKE·passive access bit | 사용자 입력·상태 event 후보 | 원본 BCM ID 충돌 위험, 송신 금지 |
| `NM_TMU`, `NM_IPM`, `NM_CGW` | network management | wake/sleep 영향, 절대 송신 금지 |

BlueLink 잠금/해제는 단순 2-bit frame보다 사용자 인증, TMU session, gateway source bus, network wake-up과 feedback 정책을 포함할 수 있다. DBC에서 이름이 발견됐다는 이유로 OEM 인증 경로를 우회하거나 과거 frame을 replay해서는 안 된다. 특히 잠금 해제는 CANView에 동등한 사용자 인증·암호학적 세션·감사 기능이 마련되기 전까지 구현하지 않는다.

## 5. 제어 등급

| 등급 | 허용 범위 | 현재 기능 |
|---|---|---|
| P0 수동 표시 | listen-only 캡처와 freshness 검증 후 표시 | 속도, RPM, 4WD 후보, TPMS, 연비, DPF lamp, 문·조명·전압 |
| P1 monitor-only | 명령 조건을 평가하지만 차량에 송신하지 않음 | SPORT 자동, 오디오 profile, 소음 보정 |
| P2 bench command | 분리된 ECU/bench에서 allow-list command와 feedback 검증 | AMP volume/balance/rear mute, drive-mode button event |
| P3 제한 실차 | 정차 또는 명시한 운행 조건, lease, timeout, rollback, 물리 TX 차단 검증 후 opt-in | 향후 SPORT·오디오 profile |
| HOLD | 인증·wake·payload 의미가 불명확하거나 안전/보안 영향이 큼 | door unlock, engine operate, NM, IGN, IPS 출력, SCC/LKAS/MDPS/TCU/4WD 명령 |

Controller는 raw CAN ID/payload를 Communicator로 지시할 수 없다. 명령은 STM32 vehicle profile의 고정 builder와 allow-list만 사용하며 link loss, stale input, unexpected feedback, bus error, watchdog reset에서 PHY가 hardware-default standby로 돌아가야 한다.

## 6. 실차 검증 절차

1. 차량 옵션, 시장, HU·AMP·TMU·BCM/IBU·4WD ECU 부품번호를 기록하고 VIN은 마스킹한다.
2. 세 채널을 물리 TX disable과 FDCAN bus-monitoring mode로 두고 bitrate·idle voltage·connector pin을 찾는다.
3. C-CAN/M-CAN 후보를 동시 캡처해 `0x043→0x53A`, `0x1EB→0x521`, `0x16A→0x522` 같은 gateway 변환 가설을 timestamp로 확인한다.
4. 각 ID의 최소/중앙/p95/최대 frame 간격, IGN OFF 마지막 frame, event burst를 측정해 freshness를 정한다.
5. 속도·RPM·압력·전압은 순정 화면과 독립 계측값에 대조한다.
6. 4WD는 AUTO/LOCK, 저속 회전, 완만한 가감속을 구분해 clutch duty·pressure·wheel speed를 함께 기록한다.
7. 연비는 trip reset과 정속/감속/공회전을 구분하고 단위 enum을 확인한다.
8. DPF는 순정 경고, 정비 진단기, 냉간/열간 상태를 대조한다. 경고등만으로 soot load를 역산하지 않는다.
9. 문은 각 래치, 수동 노브, 실내 switch, key fob, passive handle, BlueLink를 한 번에 하나씩 조작해 open/lock/request/feedback을 분리한다.
10. 송신 연구는 bench에서 시작하고 control lease 만료, ACK timeout, 반복 명령 제한, OEM 수동 조작 우선과 snapshot 복원을 검증한다.

원본 파일과 hash는 [`dbc/README.md`](../dbc/README.md), 기능별 계산과 release gate는 [기능 설계](feature-design.md), Controller–Communicator 명령 수명주기는 [ESP-NOW 프로토콜](esp-now-protocol.md)에 있다.
