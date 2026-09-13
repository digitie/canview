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

2026-09-13 현재 body/호환성 모형1462건과 Windows 실제 P256+SHA-2561468건이
통과했다. 모형1462건·typed1422건은 ASan/UBSan도 통과했다. 합산 coverage는
body.c100%, manifest.c 함수100%·행97.72%·분기93.70%다.
앞선 typed 교차1422/1425건과 CBOR11989건·서명 prefix230건도 유지한다.
STM native 검사도 추가했다. 공식 MCUboot imgtool v2.4.0 생성물의 전체 hash·P256
서명과 protected metadata/manifest를 대조한다. 실제 CNG2287건, 비암호 모형
ASan/UBSan2284건과 native_stm.c coverage 함수100%·행98.14%·분기94.74%다.
byte 변이/절단 건수는 ECDSA DER 길이에 따라 달라진다. 현재 source의 전체 Windows
Host Debug/Release는 각각130/130 통과다.

version floor, ESP native image signature/protected
metadata, prefix 부분 수신 조립, 정식 schema·CLI·signed golden과 실제 target
provider/통합·최종 독립 2인 리뷰는 남아 있다. Arm object compile을 최종
ELF/MAP/BIN gate로 대체하지 않는다. physical/HIL은 NOT_RUN, 차량 TX는 NO-GO다.
[CBOR checkpoint 리뷰](../reviews/adversarial/2026-09-13-T-007-cbor.md)의 A/B static
PASS는 이후 구현이나 전체 task의 최종 검토 결과가 아니다. 수용 기준은 미완료다.

## 구현 접근

[공통 단순화 원칙](../../AGENTS.md#2-작업-원칙)을 적용한다. 작은 서명 manifest와
순차 image만 사용하고, 압축·임의 경로·플러그인·범용 패키지 기능은 추가하지 않는다.
Controller/Bridge는 한 image, Communicator는 ESP/STM 최대 두 image로 구현한다.
이미지 서명·부팅·Flash 처리는 기존 SDK/부트로더 기능을 먼저 재사용한다.
다음은 ESP native image 검증과 version floor, 정식 schema/CLI 및 target 연결이다.
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

아래는 이 task가 생성·확정할 미래 산출물이다. 경로가 아직 없다는 사실을 검증 통과로 해석하지 않는다.

```text
schema/cvota-v1.schema.json
protocol/schema/ota-container-v1.yaml
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
