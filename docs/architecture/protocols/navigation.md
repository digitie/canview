# 위치·관성·기압 센서 확장 계약

## 상태와 적용 경계

이 문서는 새 Communicator 센서 회로에 대응하는 **추가 설계 계약**이다. 기존 ESP-NOW `1.3`/UART `1.0` 구현 준비 상태를 동작하는 firmware로 바꾸었다는 뜻이 아니다. 적용 대상은 ESP-NOW `1.4`와 UART `1.1`이며 기존 message ID·CAN record·수신 필터 의미를 변경하지 않는다. [센서 payload schema](../../../protocol/schema/navigation-v1.json)와 host 검증기는 이 확장의 크기·단위·범위 정본이다. transport의 인증·CRC·세션·재전송은 기존 [ESP-NOW](esp-now.md), [UART](communicator-uart.md) 계약을 그대로 따른다.

장치 내부 책임은 다음과 같다.

- Communicator ESP32: 외부 GNSS UART의 read-only tap, GNSS/INS 모듈 host SPI, 센서 건강 상태와 시간 anchor의 소유자. BMP384는 MTi AUX SPI에 연결되며 ESP가 같은 bus를 직접 구동하지 않는다.
- GNSS/INS 모듈: 자체 관성 융합·데드레코닝. 자세만 계산하는 IMU를 위치 DR로 표시하지 않는다.
- STM32: CAN 수집 clock·최종 차량 송신 안전 판정. GNSS/INS 데이터는 CAN safety 신호의 대체물이 아니다.
- Controller: 표시·시간 원천 선택·일몰 계산·단위 변환. 위치 원본의 영구 기록은 기본 꺼짐.
- Bridge: 별도 허가된 센서 stream을 read-only 관찰. 위치 저장·내보내기는 사용자가 선택하며 차량 제어 권한은 생기지 않는다.

센서 장애는 CAN capture를 재부팅시키지 않는다. 센서 driver timeout·CRC 오류·잘못된 제조사 packet 길이는 해당 센서만 `INVALID`로 전이시킨다. 임의 CAN ID를 추가할 때 Communicator firmware를 수정하지 않는 기존 generic CAN 경로는 그대로 유지한다.

## 메시지와 협상

| ESP-NOW ID | 메시지 | 방향 | 계약 |
|---|---|---|---|
| `0x70` | `SENSOR_CAPABILITIES` | ESP→Controller/Bridge | 센서 종류, 제조사/firmware 식별, 지원 rate, PPS 유무, DR 가능 여부 |
| `0x71` | `NAV_STATE` | ESP→허용된 peer | 위치·고도·속도·오차·fix/DR 품질, QoS0 |
| `0x72` | `IMU_STATE` | ESP→허용된 peer | 자세·가속도·각속도·자기계 보정 품질, QoS0 |
| `0x73` | `BARO_STATE` | ESP→허용된 peer | 압력·온도·기압고도·기준압 revision, QoS0 |
| `0x74` | `SENSOR_SUBSCRIBE` | Controller/Bridge→ESP | peer별 추가/변경/삭제, 원자적 적용, QoS1 |
| `0x75` | `SENSOR_RESULT` | ESP→요청 peer | 최종 적용 여부·실제 rate·revision·거부 원인 |
| `0x76` | `UTC_ANCHOR` | ESP→허용된 peer | boot monotonic↔UTC 대응, 오차 범위·원천 |
| `0x77` | `SENSOR_HEALTH` | ESP→허용된 peer | sensor reset count, timeout/CRC/drop, calibration 상태 |

capability 이름은 `sensor.discovery.v1`, `sensor.nav.v1`, `sensor.imu.v1`, `sensor.baro.v1`, `sensor.utc.v1`이다. 이름을 기존 미동결 bitset의 임의 비트로 해석하지 않는다. T-002의 전체 schema 생성 시 번호를 배정한다. 상대가 `1.4`와 해당 capability를 모두 협상하지 못하면 그 메시지는 보내지 않는다. 구버전 peer에게 NAV를 decoded CAN signal이나 unknown raw ID로 위장하지 않는다.

`SENSOR_CAPABILITIES`의 feature_mask는 bit0=NAV,1=IMU,2=BARO,3=UTC,4=GNSS/INS DR이며 나머지는 거부한다. hardware_profile은0=없음,1=MTi7+호환 GNSS/PPS+BMP384,2=ESP-direct raw GNSS다. profile2는 DR bit를 올리지 않는다. 제조사 firmware version은 `(major<<24)|(minor<<16)|patch`이며 patch는16bit다. rate mask의 bit 순서는 schema `rates_hz` 배열 순서다. 지원 가능한 rate만 광고하며 default rate와 혼동하지 않는다. `max_dr_age_ms`는0=DR미지원, MTi7은45000 이하이고 peer_budget_bps는 추가 센서 encoded application bytes/s다.

배열 길이 밖 rate bit와 대응 feature가 없는 rate mask는 거부한다. DR bit는 profile1·NAV bit·1~45000ms age limit을 모두 요구하며, DR 미지원이면 age limit은0이어야 한다. profile0은 feature를 광고하지 않는다. host codec의 version 인자는 정확히 `[major, minor]` 두 u8 정수이며 float·bool·추가 원소를 허용하지 않는다. 이 검사는 인증·협상 완료를 대신하지 않는다.

UART `1.1`에는 `0x60 CLOCK_ANCHOR_QUERY`, `0x61 CLOCK_ANCHOR_REPLY`만 추가한다. ESP sensor telemetry 전체를 STM32로 왕복 복사하지 않는다. query payload는 `esp_boot_id:u64`, `query_id:u32`, `esp_t1_us:u64`이고 reply는 이 세 값과 `stm_boot_id:u64`, `stm_t2_us:u64`, `stm_t3_us:u64`를 반송한다. ESP는 수신 순간 `t4`를 기록한다. 기존 correlation/session 검증에 더해 boot ID와 query ID가 일치해야 한다. clock offset은 왕복 지연의 절반을 오차에 포함해 계산하고 음수 왕복 지연·boot 변경·2초 이상 지연 응답은 폐기한다.

## 좌표·시간·품질

- 좌표는 WGS84 `latitude_e7`, `longitude_e7`; API 배열로 내보낼 때만 `(longitude, latitude)` 순서다. 바이트 순서는 모두 little-endian이다.
- 속도는 mm/s, 고도는 mm, 압력은 Pa, 온도는 centi°C, 가속도는 mm/s², 각속도는 mdeg/s다. wire에 IEEE float·NaN·Infinity를 넣지 않는다.
- GNSS 타원체고도와 평균해수면고도, 기압고도를 분리한다. datum을 모르는 NMEA 고도를 MSL로 승격하지 않는다.
- `source_boot_id`와 `sample_time_us`를 함께 써야 한다. 재부팅 전후 monotonic time을 하나의 시간축으로 잇지 않는다.
- `sample_time_us`는 측정 epoch에 대응하는 ESP monotonic 시간이다. epoch 대응을 못 했으면 수신 시각을 쓰되 `time_uncertainty_us`에 serial 지연·batching을 포함하고 PPS 품질을 주장하지 않는다.
- `validity` 비트가 없는 field는 0으로 정규화한다. 위치 0,0을 valid fix로 대신 쓰지 않는다. 마지막 정상 값을 보존할 수는 있지만 stale 숫자를 현재값으로 전송하지 않는다.
- `fix_type`: 0 INVALID, 1 GNSS_2D, 2 GNSS_3D, 3 GNSS_DR_FUSED, 4 DR_ONLY. DR_ONLY에는 마지막 GNSS 이후 경과시간과 수평·수직 오차를 반드시 포함한다. 제조사 INS status가 DR 유효를 보장하지 않으면 UNKNOWN/INVALID다.
- yaw는 차량 정면 기준 ENU의 방위각이 아니다. `yaw_mdeg`는 차량 전방이 북쪽일 때 0, 동쪽일 때 90000인 heading으로 변환한다. 장착 quaternion·축 변환 revision과 자기 교란 상태를 건강 메시지에 둔다.
- 센서가 지구 중력을 포함하는 acceleration인지 제거한 linear acceleration인지 capability에 고정한다. 초기 표시는 후자만 사용하고 중력 제거 실패 시 validity를 내린다.
- GNSS-derived UTC는 `UTC_VALID`/leap-second 상태를 검증한다. RTC를 지속적으로 점프시키지 않는다. 큰 시간차는 정차 상태 확인·사용자 설정 정책을 따른다. TTL·lease·watchdog은 언제나 monotonic이다.
- PPS가 없는 cased GPS를 연결했다고 PPS 정확도의 시간 동기화나 GNSS/INS 호환성을 주장하지 않는다. 부품·배선별 capability를 다르게 낸다.

좌표계·품질 enum은 아래처럼 고정한다. MTi native ENU 값을 그대로 vehicle 축이라고 내보내지 않는다.

| 필드 | 값과 의미 |
|---|---|
| profile1 IMU vector | 차량 body frame X=전방,Y=좌측,Z=상방, 중력 제거 가속도. `mount_revision`은 장착 변환 설정 revision. 원천 보정/중력 제거 불가 시 가속도 validity=0 |
| calibration_status |0 UNKNOWN,1 CALIBRATING,2 VALID,3 INVALID. 제조사 quality와 설정 revision이 일치할 때만2 |
| source_quality |0 INVALID,1 GNSS_SERIAL,2 GNSS_PPS,3 HOLDOVER. PPS만 들어온다고 UTC 유효가 되지 않음 |
| leap_status |0 UNKNOWN,1 VALID_NO_PENDING,2 INSERT_PENDING,3 DELETE_PENDING. 0이면 UTC source_quality=0 |
| SENSOR_HEALTH.state |0 ABSENT,1 INITIALIZING,2 READY,3 DEGRADED,4 REACQUIRING,5 FAULT |

UTC INVALID의 UTC/uncertainty 값은0이며 사용할 수 없다는 의미다. HOLDOVER는 마지막 유효 UTC 이후 오차를 계속 증가시키고3초 초과 시 INVALID로 전환한다. leap pending은 UTC epoch가 이미 GNSS에서 변환된 값이며 수신자가 임의로1초를 추가하지 않는다. `last_good_age_ms=0xffffffff`는 정상 sample 이력이 없음을 뜻한다.

`SENSOR_CAPABILITIES`는 협상/명시 조회에만 응답하고 HEALTH는 허가된 peer에 최대1회/초, 변화가 없으면5초 간격으로 보낸다. 이 관리 응답도 sensor/전체 radio budget에 포함하며 rate-limit 초과 시 최신 상태로 병합한다. 무제한 discovery/event flood 경로를 만들지 않는다.

DR은 무제한 위치 유지 기능이 아니다. MTi7의 GNSS outage **45초 이후 위치·속도 출력 중단**을 반영해 `DR_ONLY`는 last_fix_age≤45000ms일 때만 허용한다. 제조사 quality가 먼저 invalid가 되면 즉시 내려야 한다. GNSS 복귀 후 innovation이 제조사 기준을 벗어나면 `REACQUIRING` 동안 위치 점프를 정상 추적으로 표시하지 않는다. 차속·가속 기반 SPORT 판단은 검증된 CAN 신호만 사용하고 GNSS/INS는 관찰 보조다.

## 구독과 대역폭

초기 연결의 구독 목록은 비어 있다. CAN filter는 센서 구독을 암묵적으로 허용하지 않는다. 각 peer에 NAV/IMU/BARO/UTC의 네 구독 slot만 허용하고, 인증된 role·위치정보 접근 허용·capability의 교집합을 검사한다.

| stream | 기본 Hz | 선택 가능 Hz | stale 판정 |
|---|---:|---|---|
| NAV | 10 | 1, 2, 5, 10, 20, 25 | `max(3×period, 500ms)` |
| IMU | 20 | 1, 5, 10, 20, 50, 100 | `max(3×period, 250ms)` |
| BARO | 2 | 1, 2, 5, 10 | 3초 |
| UTC | 1 | 1 | 3초 후 source quality 하락 |

센서 자체 지원 rate보다 큰 값은 거부한다. NAV/IMU/BARO 각각 제조사 측정 rate와 송신 rate를 분리하며 새 sample 없이 과거 값을 반복해 rate를 맞추지 않는다.

초기 구현의 추가 센서 예산은 **전체 peer 합산 encoded ESP-NOW application bytes 8,192 B/s**, peer별 4,096 B/s다. 기존 전체 radio airtime·CAN/control 예산이 더 작으면 작은 쪽을 적용한다. payload만 계산하지 않고 transport header, auth tag, sensor body 길이를 합산한다. 재전송 가능한 설정·상태 응답도 같은 총 radio budget에 포함한다. driver/MAC header·충돌·암호화 overhead는 T-201의 실제 airtime 측정으로 별도 검증한다.

`rate × encoded_frame_size` 합계가 예산을 넘으면 `BUDGET_EXCEEDED`로 **전체 변경을 거부**한다. 조용히 rate를 낮추지 않는다. 한 peer의 높은 rate 때문에 다른 peer의 기존 구독을 축소하지 않는다. 기본값의 실제 body 크기는 schema 검사 결과로 계산한다. 100Hz IMU와 25Hz NAV 동시 요청이 언제나 가능하다는 뜻은 아니다.

일반 stream은 latest-only slot당 1개, 재전송 없음, drop 누적 계수 32-bit saturating이다. queue가 막히면 가장 오래된 sensor telemetry부터 폐기하고 control/heartbeat queue로 밀어 넣지 않는다. 최대 구독 수·peer 수·queue bytes는 부팅 때 고정 할당한다.

`SENSOR_SUBSCRIBE`에는 다음을 넣는다: `operation`(0=GET,1=UPSERT,2=DELETE,3=CLEAR), `sensor_kind`(1=NAV,2=IMU,3=BARO,4=UTC), `rate_hz`, `count_limit`(0=무제한, 그 외 1..65535), `expected_revision:u32`, `request_id:u64`, `target_boot_id:u64`. 알 수 없는 field/flag, 삭제 시 0이 아닌 rate/count, revision mismatch는 거부한다. GET/CLEAR는 kind/rate/count=0이어야 한다. GET의 expected_revision은 무시하며 현재 상태를 반환한다. CLEAR는 **요청 peer의 센서 구독만** 비운다. 차량 제어 권한·다른 peer·CAN filter에는 영향을 주지 않는다. count는 MAC 큐에 성공적으로 제출한 새 sample 수다. 무선 전달·UI 표시 횟수로 해석하지 않으며 제출 실패는 count를 소모하지 않는다.

revision은 peer session 내0부터 시작하며 적용 성공마다1 증가한다. count 만료로 slot을 자동 제거할 때도 증가한다. `0xffffffff` 이후에는 wrap하지 않고 BUSY로 거부하여 session 재협상을 요구한다. rate/count 값이 같은 UPSERT도 새로운 count 실행 요청이므로 revision이 증가하며, 같은 request 재전송만 실행을 반복하지 않는다. 이미 없는 slot의 DELETE와 빈 CLEAR는 성공 no-op이며 revision은 유지한다.

## 응답 확인과 재부팅

transport ACK는 수신 확인, `SENSOR_RESULT(APPLIED)`는 실제 원자 적용 완료다. 이 둘을 혼동하지 않는다. 결과에는 request ID, target boot ID, 이전/새 revision, 실제 구독 목록 digest, 적용 monotonic time, rate/count, 오류 enum을 포함한다.

동일 authenticated peer+session+request ID+boot ID의 재시도는 canonical request bytes digest까지 같을 때만 캐시된 결과를 반환한다. 다른 payload면 `IDEMPOTENCY_CONFLICT`; 연결 종료 후 새 세션에서 과거 request를 실행하지 않는다. RAM result cache는 peer당16개,30초이며 동시 pending 변경은 peer당1개다. request_id는 session 내 단조 증가하고 수신 측은 high-water mark를 유지한다. 캐시에 없고 high-water 이하인 ID는 재실행하지 않고 RESULT_EXPIRED로 응답한다. 결과가 유실되면 동일 요청 재전송, 캐시 만료 후에는 새 ID의 GET snapshot으로 revision/digest를 대조한다. 불명 상태에서 새로운 반대 명령을 자동 실행하지 않는다.

`SENSOR_RESULT`는 항상 NAV/IMU/BARO/UTC 순서의 네 rate/count slot을 포함한다. 없는 slot은 rate=count=0이다. count_limit은 등록 당시 한도이지 감소 중인 잔여 count가 아니다. 만료되면 slot을 지우고 revision을 갱신한다. APPLIED 응답의 snapshot은 그 적용 시점이며 뒤늦게 받은 응답을 현재 snapshot으로 덮어쓰지 않는다. SHA-256-128은 `source_boot_id:u64 LE || current_revision:u32 LE || 각 slot(rate:u16 LE,count:u16 LE)` 총28byte의 SHA-256 **첫16byte**다. 인증 tag가 아니며 transport 인증을 대체하지 않는다.

| status | 의미 | 적용/확인 |
|---|---|---|
| 0 APPLIED | 원자적 변경 또는 성공 no-op | 해당 request가 적용되었음 |
| 1 SNAPSHOT | GET 결과 | 변경하지 않음, applied_time_us=0 |
| 2 MALFORMED | 길이/범위/reserved 오류 | 변경 없음 |
| 3 UNAUTHORIZED | role·위치 관찰 허가 없음 | payload snapshot은 노출하지 않음 |
| 4 UNSUPPORTED | capability/센서/rate 미지원 | 변경 없음 |
| 5 REVISION_MISMATCH | 기대 revision 불일치 | 최신 snapshot 확인 후 새 요청 |
| 6 BUDGET_EXCEEDED | peer/전체 budget 초과 | 전체 변경 거부 |
| 7 IDEMPOTENCY_CONFLICT | 같은 ID에 다른 bytes | session 오류 기록, 재실행 안 함 |
| 8 WRONG_BOOT | 이전 부팅 대상 | 재협상 |
| 9 BUSY | pending 변경·revision 소진 | 완료/재협상 후 새 요청 |
| 10 RESULT_EXPIRED | 과거 ID 결과 소실 | 재실행 금지, 새 GET |

인증 전 요청에는 이 메시지 자체를 응답하지 않는다. 인증은 되었지만 센서 관찰 권한이 없는 peer의 status3은 ID/boot/status 이외 값과 digest를0으로 정규화한다. 그 외 오류는 현재 허가된 snapshot, applied_time_us=0을 반환한다. APPLIED와 SNAPSHOT의 digest는 수신 측에서 재계산한다. 아래 host codec은 wire/범위 검사이며 실제 session cache, role 검사와 atomic allocator는 firmware 후속 작업이다.

구독은 RAM 상태다. ESP 재부팅 후 비어 있는 상태로 시작하고 boot ID가 달라진 결과는 이전 요청의 성공으로 처리하지 않는다. Controller가 capabilities와 snapshot을 다시 확인한 뒤 재등록한다. 영구 센서 calibration·기준압·장착 방향 변경은 기존 owner-targeted config transaction으로 분리하며 정차/service 상태에서만 가능하다. flash 쓰기 후 readback/digest 검증 전에는 APPLIED를 보내지 않는다.

## 오류 처리·시험

- UART GNSS: 길이 상한 1,024B, UBX checksum 또는 NMEA checksum, version/정밀도 검증. 500ms 중간 packet timeout, 3회 연속 실패 시 parser resync; GNSS 전원 재시작은 최대 3회/10분.
- GNSS/INS: vendor packet 최대 길이를 schema로 제한하고 3회 timeout이면 해당 stream invalid. calibration 상태를 정상으로 추정하지 않는다.
- 기압계: MTi AUX SPI 소유이므로 ESP에서 I²C recovery 또는 BMP register 직접 쓰기를 하지 않는다. MTi health/timeout과 pressure range로 invalid를 전파한다. 기준압 범위80,000..110,000Pa, 현 위치에 보정되지 않은 고도는 relative 표시.
- golden vector: byte order, signed extrema, GNSS absent/DR_ONLY, boot 변경, pressure 범위, unknown validity bit, 과대 payload, truncated payload, rate 예산 초과, 중복 요청 및 충돌, 구버전 capability 미지원.
- 실제 RF/STM UART end-to-end 통합, RTC write/readback, INS GNSS outage 재획득, moving vehicle 시험은 [T-100b](../../tasks/T-100b-navigation-audio-bringup.md)에서 수행한다. host codec만으로 완료 처리하지 않는다.
