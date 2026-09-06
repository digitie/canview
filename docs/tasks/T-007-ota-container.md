# T-007 OTA-01 서명 컨테이너와 packager

- 상태: `BLOCKED`
- 우선순위: `P0`
- Gate: `G0 / OTA-01`
- 선행: `T-001`

## 목표

OTA §7의 `.cvota`를 모든 역할이 같은 byte 계약으로 검증하게 한다. 현재 parser·packager가 없으므로 이 task는 설계 기록의 구현 전환이며 배포 승인이 아니다.

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
