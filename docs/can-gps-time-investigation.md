# CAN 신호의 GPS·시간 정보 조사

## 결론

저장된 `commaai/opendbc` Hyundai DBC를 1차 대상인 2017 Tucson TL 2.0 디젤 4WD BlueLink 후보에 대조한 결과는 다음과 같다.

- 주 후보인 [`hyundai_can.dbc`](../dbc/opendbc/hyundai_can.dbc)에는 위도, 경도, GPS 고도, 지리적 진행방향, 현재 날짜·시각 또는 RTC 값이 없다.
- 따라서 현재 실차 캡처 없이 확정할 수 있는 GPS·시간 CAN 신호는 **0개**다.
- [`hyundai_2015_mcan.dbc`](../dbc/opendbc/hyundai_2015_mcan.dbc)에는 GPS 날짜·시각과 나침반·방위처럼 보이는 이름이 있지만, 구형 Multimedia CAN 비교 파일의 후보일 뿐이며 2017 TL 적용을 보장하지 않는다.
- Controller의 시간은 CAN에서 받는다고 가정하지 않는다. Waveshare 보드의 PCF85063 I²C RTC를 표시·로그·일몰 경고의 기준으로 사용하고, 일출·일몰 값은 GPS 좌표가 검증되기 전까지 설정값 또는 별도 위치 소스의 결과로 취급한다.

분석 입력은 저장소의 [`dbc/README.md`](../dbc/README.md)에 고정한 `commaai/opendbc` snapshot commit `3e92d112129507debe45364891954db70238997a`와 `hyundai_can.dbc`, `hyundai_2015_mcan.dbc`다. 원본 DBC는 수정하지 않았으며 후보 신호의 의미를 실차 검증 전 확정하지 않는다.

## 주 DBC에서 확인한 오인 가능 필드

| 요구 정보 | 조사 결과 | 근거 |
|---|---|---|
| 위도 | 없음 | `LAT_ACCEL`은 위도가 아니라 횡가속도다. [`ESP12`, 0x220](../dbc/opendbc/hyundai_can.dbc#L1228) |
| 경도 | 없음 | `LONG_ACCEL`은 경도가 아니라 종가속도다. [`ESP12`, 0x220](../dbc/opendbc/hyundai_can.dbc#L1228) |
| GPS 고도 | 없음 | `MAF_FAC_ALTI_MMV`는 DBC상 MAF factor이며 고도 단위나 좌표계가 없다. [`EMS12`, 0x329](../dbc/opendbc/hyundai_can.dbc#L1072) |
| GPS heading | 없음 | `YAW_RATE`는 차량 회전각속도이고 조향각은 지리적 heading이 아니다. |
| 현재 날짜·시각 | 없음 | `Vehicle_Stop_Time`은 정차 관련 상태 필드이고 `TRIP_*_Display_*`는 트립 표시 필드다. [`TCU_DCT14`](../dbc/opendbc/hyundai_can.dbc#L825), [`TRIP_A/B`](../dbc/opendbc/hyundai_can.dbc#L1637) |
| 내비 시간·좌표 | 없음 | `Navi_HU`에는 제한속도와 source flag가 있지만 좌표·시각 값은 없다. [`Navi_HU`, 0x544](../dbc/opendbc/hyundai_can.dbc#L1650) |

## 구형 M-CAN 후보

아래 항목은 파일에 정의된 이름과 bit layout을 기록한 것이며, 대상 차량에서 동작한다고 판정한 값이 아니다. 현재 등급은 모두 `C`다.

| 파일상 의미 | Message / ID | signal과 bit | 변환 | 판정 |
|---|---|---|---|---|
| GPS 날짜·시각 후보 | `HU_TMU_E_02` / `0x0FB` | `HU_GPS_Year` `7|8`, Month `15|8`, Day `23|8`, Hour `31|8`, Minute `39|8`, Second `47|8` | 모두 factor `1`, offset `0` | range가 `[0|0]`이고 2017 TL 검증 없음. [`DBC`](../dbc/opendbc/hyundai_2015_mcan.dbc#L55) |
| GPS payload 후보 | `HU_TMU_P_01` / `0x570` | `HU_GPS_Signal` `7|64` | opaque 64-bit | 위도·경도·고도 분할 방식과 단위가 없다. [`DBC`](../dbc/opendbc/hyundai_2015_mcan.dbc#L906) |
| GPS 보조 후보 | `HU_TMU_P_02` / `0x560` | `HU_GPS_Signal2` `7|8`, Signal3 `9|2`, Signal4 `12|3` | Signal2만 `2 Degree` | 좌표 또는 방위인지 알 수 없고 range가 `[0|0]`이다. [`DBC`](../dbc/opendbc/hyundai_2015_mcan.dbc#L986) |
| 내비 compass 후보 | `HU_CLU_PE_12` / `0x1E6` | `Navi_Compass` `45|6` | `7.5 - 7.5°`, 표시 범위 `[0|352.5]` | 내비 표시용 compass 후보이지 GPS course로 확정할 수 없다. [`DBC`](../dbc/opendbc/hyundai_2015_mcan.dbc#L231) |
| 내비 방위 후보 | `HU_CLU_P_00` / `0x506` | `NV_Azimuth` `31|8` | factor/단위 미기재 | 좌표계와 기준방향이 없다. [`DBC`](../dbc/opendbc/hyundai_2015_mcan.dbc#L1424) |
| ETA 시간 후보 | `HU_CLU_P_02` / `0x508` | `NV_TIME_TYPE` `3|4`, `NV_Hour` `15|8`, `NV_Min` `23|8` | factor `1` | 현재 RTC가 아니라 내비 표시·예상시간 후보다. [`DBC`](../dbc/opendbc/hyundai_2015_mcan.dbc#L1398) |

`CLU_ClockInfoReq`와 `CLU_DateInfoReq`는 HU에 정보를 요청하는 필드이지 현재 시간 값이 아니다. [`DBC`](../dbc/opendbc/hyundai_2015_mcan.dbc#L1286)

## 제품 적용 결정

### 시간

PCF85063는 Controller 보드의 `GPIO7=SCL`, `GPIO8=SDA` 공유 I²C에 연결한다. NXP 데이터시트의 7-bit slave address는 `0x51`이며, 실제 Waveshare 보드 리비전은 schematic와 부팅 시 I²C probe로 확인한다. RTC 시간 레지스터는 BCD로 읽고 유효성·oscillator stop 상태를 확인한 뒤 `rtc_quality`를 갱신한다. [NXP PCF85063TP 데이터시트](https://www.nxp.com/docs/en/data-sheet/PCF85063TP.pdf)

UI의 시간 설정은 시·분 선택값만 전송하고 Controller가 현재 날짜를 보존한다. 날짜·시간 전체를 바꾸는 service 명령은 별도의 인증·감사 경로로 둔다. 무선 링크의 monotonic timestamp와 CAN sample ordering은 RTC wall clock과 분리한다.

### 일몰 후 전조등 경고

CAN에 GPS 좌표가 없으므로 다음 중 하나가 있어야 일출·일몰을 자동 계산할 수 있다.

1. 사용자가 Controller 설정에서 일출·일몰 시각을 지정한다.
2. 위치를 제공하는 별도 GPS/휴대폰/서비스가 계산한 일출·일몰 시각을 Controller에 저장한다.
3. 추후 검증된 외부 위치 모듈을 I²C/UART 등 별도 링크로 추가한다.

경고기는 `RTC valid + 일출·일몰 값 valid + 차량 awake + 조명 CAN signal fresh`를 모두 요구한다. 일몰 후 전조등/미등 OFF가 기본 2초 유지될 때만 경고를 켜고, 조명 ON이 1초 유지되면 끈다. 어느 입력이라도 stale이면 경고를 끄며, 낮 시간과 차량 sleep 중에는 경고를 표시하지 않는다.

### 실차 검증 시 승격 조건

구형 후보를 시험할 경우 HU/TMU 장착 여부와 물리 bus를 확인한 뒤 다음을 동시에 기록한다.

- 순정 화면의 날짜·시각·나침반·내비 ETA
- 해당 CAN ID의 주기, DLC, alive/counter/checksum, IGN 상태별 송수신
- 독립 GPS의 좌표·course·UTC와 CAN raw payload
- 차량 시간 변경 및 위치 이동 후 값의 상관관계

반복 캡처와 독립 계측이 맞기 전에는 후보를 기본 UI나 자동 경고 입력으로 승격하지 않는다. 좌표·시간이 확인되지 않은 상태에서 CAN raw를 GPS로 해석하거나, CAN에 없는 좌표를 추정해 표시하지 않는다.
