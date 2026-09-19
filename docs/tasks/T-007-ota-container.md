# T-007 OTA-01 서명 컨테이너와 packager

- 상태: `IN_PROGRESS`
- 우선순위: `P0`
- Gate: `G0 / OTA-01`
- 선행: `T-001`

2026-09-09 T-104 PR #33 merge `d229772`/origin/main 확인 뒤 시작했다.
T-001은 완료됐으며 상세 과거 구현·검증 이력은 [journal](../journal.md)에 보존한다.

## 현재 구현과 검증

[OTA 내부 모듈](../../shared/ota/README.md)의 C99 CBOR 구조·서명 prefix·typed
manifest와 순차 본문 길이/SHA-256 검사를 연결했다. identity/target/slot 상한,
unsigned header 대조, uint64 sequence 보존, ABI/config 내부 정합성을 검사한다.
본문 시작 전 로컬 snapshot과 구·신 ABI 네 조합·MCU별 boot/recovery·hardware
capability·config 읽기 범위도 대조한다. 미확인 로컬 정보는 거부한다.

body는 chunk를 보존하지 않고 SDK provider를 사용한다. offset 중복/누락·partial
input/reset·provider/cleanup 실패와 재진입을 검사한다. 성공 상태는
`HASHES_MATCHED`이며 native image 검증이나 erase/PREPARED 승인이 아니다.

2026-09-15 현재 body/호환성/floor 모형1702건과 Windows 실제 P256+SHA-2561708건이
통과했다. body 모형은 ASan/UBSan도 통과하고 함수·행·분기100%다.
이번 manifest.c 모형 coverage는 함수100%·행93.78%·분기90.00%다.
typed 교차1437/1440건과 CBOR11989건·서명 prefix232건, floor C3847건도 통과했다.
STM native 검사도 추가했다. 공식 MCUboot imgtool v2.4.0 생성물의 전체 hash·P256
서명과 protected metadata/manifest를 대조한다. 실제 CNG 교차 시험을 유지하며 앞선 비암호 모형
ASan/UBSan2284건과 native_stm.c coverage는 함수100%·행98.14%·분기94.74%다.
byte 변이/절단 건수는 ECDSA DER 길이에 따라 달라진다. 이전 PR35 최종 source
`12100ac`의 Windows Host Debug/Release는 각각140/140 통과다. 공통 metadata 모형 ASan/UBSan과
함수6/6·행60/60·분기76/76, 기존 공식 imgtool/CNG STM 회귀도 재검증했다.

ESP의 [read-only SDK adapter](../../tests/fixtures/idf-ota-image/README.md)를 추가했다.
실제 ESP-IDF6.0.3의 전체 hash/native RSA verifier를 compile/link하고 ELF/MAP/BIN을
생성했다. 수정 후 SDK 경고0, signed 모형 ASan/UBSan과 함수·행·분기100%다.
서명 활성/비활성·0값·다른 scheme·FPGA host6변형도 검사한다. 반환 metadata는
기존 STM 검사와 공통화한 portable C99 함수로 role/board/layout/epoch/ABI/u64 sequence와
ESP version을 대조한다. Communicator BSP에서 generated board/staging 계약을 확인하고
SDK→metadata를 연결했다. 정상 firmware/body와 단일 Flash owner 연결은 남아 있으며
실제 RSA 실행은 NOT_RUN이다. template staging의 encrypted flag와 복구 app label도
공식 SDK partition 도구로 대조했다. factory-only 설정/실제 파티션은 변경하지 않았다.
BSP 연결 모형 ASan/UBSan과 함수1/1·행29/29·분기32/32를 통과했고 실제 SDK
fixture의 BSP→SDK/metadata 함수 compile/link와 ELF/MAP/BIN 경고0도 확인했다.

version floor 비교는 body 시작 전에 연결했다. 낮은 sequence/CONFLICT를 거절하며
실제 정상 앱 증거 없이 ALREADY_INSTALLED/REPAIR_REQUIRED를 추정하지 않는다.
영속 policy/설치 상태 provider 연결, ESP native 검사의 정상 OTA owner/body 연결,
native signing CLI·signed golden과 실제 target
provider/통합·최종 독립 2인 리뷰는 남아 있다. Arm object compile을 최종
ELF/MAP/BIN gate로 대체하지 않는다. physical/HIL은 NOT_RUN, 차량 TX는 NO-GO다.
[CBOR checkpoint 리뷰](../reviews/adversarial/2026-09-13-T-007-cbor.md)의 A/B static
PASS는 이후 구현이나 전체 task의 최종 검토 결과가 아니다. 수용 기준은 미완료다.

[현재 구현분 리뷰](../reviews/adversarial/2026-09-15-T-007-current.md)는 최초 A/B CONDITIONAL이다.
반복 cleanup의 최초 오류 보존 P2와 staging64KiB 생성기 검사 P2, 이전 README 문구 P3를
수정했고 [원 reviewer 재확인](../reviews/adversarial/2026-09-15-T-007-current-post.md)에서
네 finding 모두 FIXED·새 finding 없음, A 정적 PASS·B CONDITIONAL이다.
PR #35는 최종 CI34909236819 6/6·target21개 hash/bytes·source7개·target logs26 경고0
확인 뒤 `6cf1b8e`로 merge했다. 당시 일시중지했으며2026-09-19 사용자의 명시적 요청으로
남은 구현을 재개한다. 전체 task의 미완료 수용 기준은 유지한다.

## 구현 접근

기존 C/Python 계약을 `schema/cvota-v2.schema.json`과
`protocol/schema/ota-container-v2.yaml`에 기록했다. JSON→CBOR 작성 도구는 기존
typed 검사기를 재사용한다. 2026-09-19 detached P256 서명과 image를 조립·검사하는
host CLI 및 C 고정 buffer prefix 부분 수신을 추가했다. 실제 P256/SHA-256 CNG 경로와
90개 교차 사례가 일치했으며 정상 native 검증/설치 권한으로 표시하지 않는다. native signing
도구와 연결하는 packager·signed golden이 다음 작업이다. owner/app 확장 전에 이 필수
산출물을 완성하며 정상 target·영속 policy·review gate를 생략하지 않는다.
JSON 변환은 역할3종/u64 경계에서 기존 C typed parser와 대조했고 malformed 입력과
CLI 출력 보존을 시험했다. JSON Schema metaschema는 로컬 jsonschema4.26.0으로 확인했다.
CI의 새 CTest는 추가 dependency 없이 schema mapping·기존 validator·C 대조를 검사한다.

이번 checkpoint `63c8727`의 Windows Debug/Release는 각각142/142, 별도 Linux Git
checkout의 ASan/UBSan은137/137 통과다. 새 prefix_feed의 행93.33%·분기96%,
init/finish의 행·분기100%는 collector 전용 실행 결과다. 전체 envelope.c·target/HIL
coverage로 확대하지 않는다. 독립 A/B 정적 리뷰에서 P0/P1/P2는 없으며 B의 P3 두 건
(이전 시험 귀속·host RAM 설명)은 ec44647의 [원 reviewer 재확인](../reviews/adversarial/2026-09-19-T-007-prefix-packager-post.md)에서
모두 FIXED·A/B 정적 PASS다. 이후 metadata 추가분과 전체 task의 완료 승인은 아니다.

SDK fixture에 합성 native metadata168B와 `UINT64_MAX`를 실제 custom descriptor로
넣었다. ESP-IDF6.0.3 ELF/MAP/BIN 생성·경고0, BIN offset288의 값·168개 변이·4개
절단 거절을 로컬에서 확인했다. 자동 서명·Flash·production identity는 사용하지 않았다.
다음 native signing/golden의 입력이며 정상 제품 firmware 연결 완료가 아니다.

[공통 단순화 원칙](../../AGENTS.md#2-작업-원칙)을 적용한다. 작은 서명 manifest와
순차 image만 사용하고, 압축·임의 경로·플러그인·범용 패키지 기능은 추가하지 않는다.
Controller/Bridge는 한 image, Communicator는 ESP/STM 최대 두 image로 구현한다.
이미지 서명·부팅·Flash 처리는 기존 SDK/부트로더 기능을 먼저 재사용한다.
[ADR-009](../adr/009-ota-native-image-alignment.md)의 SDK 재사용 정렬을 revision2로 구현했다.
다음은 native image signing과 결합한 packager, 이어 BSP 검사의 OTA owner/body와 실제 policy/target 연결이다.
별도 범용 기능을 추가하지 않는다. 이 순서는
아래 수용 기준이나 OTA 정본의 호환성·복구·서명 검사를 줄이는 예외가 아니다.

## 목표

OTA §7의 `.cvota`를 모든 역할이 같은 byte 계약으로 검증하게 한다. 현재 부분 검사기를 정식 schema·packager와 실제 target에 연결하는 task이며 배포 승인이 아니다.

## 고정 결정

[OTA 정본 §7–8](../architecture/ota.md)을 따른다. manifest ECDSA-P256, 정규 CBOR, role/board/layout/epoch와 signed release_sequence를 검사한다. 경로·외부 URL·임의 Flash 주소를 입력으로 받지 않는다. dev key와 production key를 분리하고 private key는 저장소·장치에 넣지 않는다.

## 구현 범위

- 컨테이너 schema와 C/Python bounded parser, enum→target map, CLI packager·검사기
- manifest 16 KiB, image 수/길이·총길이 overflow, role별 image 허용 조합 검증
- ESP/STM image signing 도구 연결점과 동일 manifest byte열 재현; 서명된 합성 golden fixture
- streaming parser의 입력 buffer 소유권·수명·부분 입력/reset API, allocation/CPU budget

## 범위 밖

파티션 최초 설치, 웹/API, 실제 eFuse/option-byte 설정, 제품 서명키 생성/배포.

## 예상 변경 파일

아래 schema 파일은 생성했지만 최종 리뷰 전 구현 후보다. 파일 존재만으로 수용 기준
또는 실제 target 연결이 완료됐다고 해석하지 않는다.

```text
schema/cvota-v2.schema.json
protocol/schema/ota-container-v2.yaml
shared/ota/
tools/ota/
tests/ota/
```

## 수용 기준

- [ ] wrong role/board/layout/key/epoch/서명·unknown critical field·duplicate CBOR key·과도한 중첩·truncated blob을 거절한다.
- [ ] 정수 경계·겹친 blob·중복 target·서명 lengths와 header 불일치·zero/초과 길이를 C/Python에서 동일하게 거절한다.
- [ ] manifest 서명·role/board/layout·signed length/호환성 검증 전 erase를 금지한다. 검증 후 enum map의 비활성 slot/staging에만 수신용 erase/write를 허용한다. 전체 image 검증 전 PREPARED/boot selector 변경은 금지하며 parser 결과와 writer 권한을 분리한다.
- [ ] signed release_sequence:u64를 문자열/JavaScript 부동소수로 비교하지 않고 image와 manifest 불일치를 거절한다.
- [ ] 정확한 공식 signing 도구 version/commit과 golden digest를 고정하고 secret 없는 clean host 환경에서 음성 fixture가 아닌 합성 binary fixture만으로 검사한다.

## 검증 계획

이 task에서 `tests/ota/test_container.py`와 C parser CTest target을 만든 뒤 host sanitizer·C/Python golden differential test를 실행한다. 생성물 drift와 서명 실패 fixture는 CI를 실패시켜야 한다. 아직 없는 명령을 통과로 집계하지 않는다.

## evidence와 rollback

schema/도구 digest, fixture별 기대 reject 이유와 실제 결과를 남긴다. 포맷 변경은 새 version으로 하고 기존 verifier를 조용히 완화하지 않는다.
