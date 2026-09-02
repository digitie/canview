# Hyundai DBC 저장소

## 1. 저장 목적

이 디렉터리는 `commaai/opendbc`에서 가져온 Hyundai·Kia·Genesis 계열 CAN 정의를 `canview`의 재현 가능한 입력으로 보관한다. DBC 파일은 차량 CAN raw frame을 사람이 읽을 수 있는 message/signal 값으로 decode하기 위한 정의이며, 파일에 정의된다고 해서 모든 연식·트림·시장 사양에서 동일하게 동작한다는 뜻은 아니다.

원본 DBC는 프로젝트에서 임의로 수정하지 않았다. 차량별 보정이나 캡처 결과는 원본 파일을 덮어쓰지 않고 별도 파일과 검증 기록으로 추가한다.

## 2. upstream 고정 정보

| 항목 | 값 |
|---|---|
| 저장소 | [`commaai/opendbc`](https://github.com/commaai/opendbc) |
| snapshot commit | [`3e92d112129507debe45364891954db70238997a`](https://github.com/commaai/opendbc/tree/3e92d112129507debe45364891954db70238997a) |
| snapshot 날짜 | 2026-09-03 기준 checkout |
| 라이선스 | MIT. 원문은 [`dbc/opendbc/LICENSE`](opendbc/LICENSE) |
| 1차 대상 | 2017 Hyundai Tucson TL, 2.0 디젤, 4WD, BlueLink |
| 1차 주 후보 | `hyundai_can.dbc` |

### 저장 파일과 역할

| 로컬 파일 | upstream 원본 경로 | 역할 |
|---|---|---|
| [`hyundai_can.dbc`](opendbc/hyundai_can.dbc) | `opendbc/dbc/generator/hyundai/hyundai_can.dbc` | 현재 Hyundai 공통 CAN generator source. 1차 대상의 주 후보 |
| [`hyundai_2015_ccan.dbc`](opendbc/hyundai_2015_ccan.dbc) | `opendbc/dbc/hyundai_2015_ccan.dbc` | 구형 Classic CAN 비교용 |
| [`hyundai_2015_mcan.dbc`](opendbc/hyundai_2015_mcan.dbc) | `opendbc/dbc/hyundai_2015_mcan.dbc` | 구형 Multimedia CAN 비교용 |
| [`LICENSE`](opendbc/LICENSE) | `LICENSE` | upstream MIT License 원문 |

현재 1차 대상은 Classic CAN 후보이므로 CAN-FD generator와 include 파일은 이 snapshot에 복사하지 않았다. CAN-FD 차량을 추가할 때는 `hyundai_canfd.dbc`와 상대 include인 `_hyundai_canfd_common.dbc`를 같은 upstream commit에서 함께 고정한다.

## 3. 무결성 확인

현재 저장해야 할 SHA-256은 다음과 같다. upstream을 갱신할 때는 commit과 해시를 함께 바꾼다.

| 파일 | SHA-256 |
|---|---|
| `hyundai_can.dbc` | `a3fb6e98bfdc0041914643f1422b1bb9290462981c40249a05cd7773e71a8bc3` |
| `hyundai_2015_ccan.dbc` | `d7f822e3a8d220cfde83aec2ae4ed28ddbc44a38ff29bea2e12e3e321d73a607` |
| `hyundai_2015_mcan.dbc` | `b18e69bdd30e53fe689b23fbd59779095249a1bb5b2391435e814a490fa924e4` |
| `LICENSE` | `6543c4a66b46affddae1b9ae1e1992687f0b7191e74ba3a7235388b9f42a6d13` |

확인 명령:

```bash
sha256sum dbc/opendbc/*.dbc dbc/opendbc/LICENSE
```

## 4. 1차 차량 적용 방법

upstream의 일반 `HYUNDAI_TUCSON` 플랫폼은 `hyundai_can_generated`를 사용한다. 따라서 generator source인 `hyundai_can.dbc`를 1차 decode 후보로 선택했다. 하지만 upstream에는 2017 Tucson TL의 엔진·구동·BlueLink 조합을 별도 fingerprint로 확정한 항목이 없으므로, 이 선택은 “후보”이며 실차 검증 결과가 아니다.

실차 적용 순서는 다음과 같다.

1. 차량의 실제 CAN connector, bus, bitrate를 listen-only로 확인한다.
2. ignition off/on, idle, 주행, 변속, 제동, 4WD 상태를 별도 캡처한다.
3. `hyundai_can.dbc`로 ID와 signal을 decode한다.
4. 계기판·실측값과 비교해 신호를 A/B/C/X 등급으로 기록한다.
5. 검증된 신호만 기본 화면에 표시하고, 미확인 값은 `candidate`/`raw`로 표시한다.

차량별 결과는 [`docs/target-vehicle-2017-tucson.md`](../docs/target-vehicle-2017-tucson.md)에 기록한다.

## 5. 우선 확인할 신호

다음은 `hyundai_can.dbc`를 읽어 정리한 1차 후보 목록이다. ID는 표준 11-bit decimal/hex 표기이며, 실차 bus 위치와 존재 여부는 아직 확정하지 않았다.

| Message (ID) | 주요 signal | DBC 변환 |
|---|---|---|
| `CLU11` (`0x4F1`) | `CF_Clu_Vanz` | factor `0.5`, `km/h or MPH` |
| `CLU12` (`0x5B0`) | `CF_Clu_Odometer` | factor `0.1`, `km` |
| `WHL_SPD11` (`0x386`) | `WHL_SPD_FL/FR/RL/RR` | factor `0.03125`, `km/h` |
| `EMS11` (`0x316`) | `N`, `TQI`, `VS` | `N`: `0.25 rpm`; `VS`: `1 km/h` |
| `EMS12` (`0x329`) | `TEMP_ENG`, `TPS`, `ENG_VOL` | 온도 `0.75/-48`; TPS `0.4694836/-15.0234742`; 용적 `0.1` |
| `EMS14` (`0x545`) | `VB`, `EMS_VS` | `VB`: `0.1015625 V`; `EMS_VS`: `0.0625 km/h` |
| `EMS16` (`0x260`) | `ENG_STAT`, `GLOW_STAT` | 상태 code/flag |
| `EMS19` (`0x492`) | `CR_Ems_EngOilTemp`, `DPF_LAMP_STAT` | 온도 `0.75/-40` |
| `TCU11` (`0x111`) | `G_SEL_DISP`, `GEAR_TYPE`, `N_TC` | `N_TC`: `0.25 rpm` |
| `TCU12` (`0x112`) | `CUR_GR`, `VS_TCU`, `N_TC_RAW` | 현재 기어/속도/RPM 후보 |
| `_4WD11` (`0x428`) | `_4WD_TYPE`, `_2H_ACT`, `_4H_ACT`, `LOW_ACT`, `AUTO_ACT`, `LOCK_ACT` | bit flag/상태 code |
| `_4WD12` (`0x429`) | `FRSS`, `FLSS`, `RRSS`, `RLSS`, `CLU_PRES` | wheel speed byte, pressure factor `0.0625` |
| `_4WD13` (`0x42A`) | `_4WD_CURRENT`, `_4WD_POSITION`, `_4WD_STATUS` | current/position/status 후보 |
| `ESP12` (`0x220`) | `LAT_ACCEL`, `LONG_ACCEL`, `YAW_RATE` | accel factor `0.01`; yaw factor `0.01` |
| `SAS11` (`0x2B0`) | `SAS_Angle` | signed factor `0.1 deg` |
| `MDPS11`/`MDPS12` (`0x381`/`0x251`) | steering angle/torque | 표시 전용 후보 |
| `TPMS11` (`0x593`) | tire status/pressure bytes | 단위·위치 별도 검증 |
| `SCC11` (`0x420`) | `ObjValid`, `ACC_ObjDist`, `ACC_ObjRelSpd` | read-only 참고 |
| `LKAS11` (`0x340`) | warning/request fields | 송신 금지 |

DBC의 sender/receiver 표기와 실제 차량 bus의 위치는 서로 다른 정보다. `sender`가 `EMS`라고 되어 있어도 차량에서 어느 물리 CAN bus에 보이는지는 캡처로 정한다.

## 6. Python decode 예

검증용으로만 사용한다. 차량에 연결하거나 frame을 송신하는 코드가 아니다.

```bash
python -m pip install cantools
```

```python
import cantools

db = cantools.database.load_file("dbc/opendbc/hyundai_can.dbc")
message = db.get_message_by_frame_id(0x4F1)
decoded = message.decode(bytes.fromhex("00 00 00 00"), decode_choices=False)
print(message.name, decoded)
```

실제 payload의 byte order, counter, checksum, multiplex 조건을 캡처와 함께 확인한다. 임의 payload를 차량 bus로 전송하지 않는다.

## 7. 갱신 절차

upstream DBC를 갱신할 때는 다음을 모두 한 변경으로 남긴다.

1. upstream commit을 선택하고 URL을 기록한다.
2. 원본 상대 경로를 보존해 필요한 include/source를 복사한다.
3. 파일을 수정하지 않고 SHA-256을 계산한다.
4. 이 문서의 commit, 날짜, 해시, 파일 역할을 갱신한다.
5. 1차 대상 차량에서 재캡처하거나 기존 캡처를 다시 decode한다.
6. 변경 전후의 signal 목록과 해석 차이를 기록한다.

## 8. 라이선스

저장된 DBC와 `LICENSE`는 `commaai/opendbc` snapshot에서 가져왔다. 배포·변경·고지 시 [`dbc/opendbc/LICENSE`](opendbc/LICENSE)의 MIT 조건을 따른다. 프로젝트 문서의 차량별 검증 결과와 새 보정 DBC는 원본 upstream 파일과 별도로 관리한다.
