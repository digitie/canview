# OTA portable parser 구현 중

[T-007](../../docs/tasks/T-007-ota-container.md)의 C99 내부 primitive다. 현재
`src/cbor_head.c`와 `src/cbor_document.c`는 head와 문서 구조를 검사하며
완전한 `.cvota` verifier가 아니다.
SDK/HAL/RTOS·heap·전역 가변 상태·ISR·Flash writer 의존성은 없다.

## Head 검사

- 입력은 호출 동안만 빌리고 저장하지 않는다. output과 입력은 겹치면 안 된다.
- 최대 9 byte만 읽는다. 모든 argument는 unsigned 64-bit이며 최소 길이만 허용한다.
- truncated prefix는 INCOMPLETE, reserve/indefinite·비최소 길이는 MALFORMED다.
- 허용 major type은 unsigned/bytes/text/array/map이다. 음수/tag/float/simple은 거부한다.
- head 함수는 본문을 검사하지 않는다. 문서 검사는 아래 별도 함수를 사용한다.
- OK는 head 파싱만 성공했다는 뜻이며 erase/write/PREPARED/boot selector 권한이 아니다.

바이트 계약의 근거는 [RFC 8949 §3·§4.2.1](https://datatracker.ietf.org/doc/html/rfc8949#section-4.2.1)이다.

## 문서 구조 검사

`canview_ota_cbor_validate()`는 root map 한 개를 고정 크기 stack으로 순회한다.
16 KiB, container 깊이8(root=1), key/value/container 합계2048 item을 상한으로
한다. 모든 map의 unsigned integer key는 엄격한 오름차순이고 중복·역순을
거부한다. UTF-8 scalar, payload truncation, trailing byte도 검사한다.
integer key profile은 내부 구현 후보이며 최종 manifest 필드 schema를 확정한
것이 아니다. 서명 대상의 의미·required field·role/board/layout은 검사하지 않는다.

입력은 호출 동안 읽기만 하며 보존하지 않는다. 재귀·heap·callback·공유 가변
상태가 없어서 불변 입력을 사용하는 독립 호출은 thread-safe다. 실행량은 byte와
item 수에 선형이지만 실제 MCU 시간·전체 call-chain stack은 별도 측정해야 한다.
부분 입력은 INCOMPLETE이며 내부 수신 상태를 유지하지 않는다. 상위 streaming
API는 아직 없다. OK는 구조 검사 성공일 뿐 Flash writer를 호출하지 않는다.

`tools/ota/cbor.py`는 별도 Python encoder/decoder이며 동일 제한·반환 분류를
시험한다. host 객체와 출력 buffer를 사용하므로 C의 무할당 구현과 구분한다.
`tests/ota/`의 CTest 세 개는 uint64 경계·전체 첫 byte 공간·UTF-8·중첩·item/byte
상한·잘린 prefix·고정 seed 변이·순환 Python 입력을 검사한다. 재현 명령은
host build 뒤 `ctest --test-dir build/host-debug -R ota-cbor --output-on-failure`다.

상위 manifest schema와 서명 검증, streaming lifecycle, 서명된 golden,
target 연결·coverage·2인 review는 후속 구현이다. 현재 production app은 이 primitive를
호출하지 않으며 physical/HIL은 NOT_RUN이다. 내부 header는 최종 public API가 아니다.

## 서명 prefix 연결 후보

`src/envelope.c`는 작은 header+CBOR+서명 prefix를 검사한다. `tools/ota/envelope.py`
는 같은 prefix 뒤에 image bytes를 순서대로 붙이는 조립 함수다. 압축/파일시스템
경로/임의 Flash 주소는 없다. 아래 byte layout은 T-007 내부 구현 후보이며 전체
manifest schema와 최종 호환성 계약의 review 전에는 배포 포맷으로 사용하지 않는다.

| offset | 크기 | 내용 |
|---|---|---|
| 0 | 8 | ASCII `CVOTA001` |
| 8 | 2 | format version1 |
| 10 | 2 | header 길이24 |
| 12 | 4 | CBOR manifest 길이1..16384 |
| 16 | 2 | manifest signature 길이64 |
| 18 | 2 | header가 주장하는 image 수1..3 |
| 20 | 4 | header가 주장하는 전체 file 길이 |
| 24 | 가변 | 정확한 deterministic CBOR byte열 |
| CBOR 뒤 | 64 | ECDSA-P256/SHA-256 `r[32] || s[32]` |
| 서명 뒤 | 가변 | 순차 image bytes; prefix 함수의 입력에는 넣지 않음 |

header 정수는 little-endian, r/s는 각각 big-endian이다. 전체 file 상한은
3×4MiB+최대 prefix의 format 상한일 뿐 role/slot 허용값이 아니다. 역할별 한두
이미지 제한·서명된 길이/대상과 header 대조·본문 검증은 다음 manifest 단계에
연결해야 한다. 현재 `declared_*` 결과를 신뢰된 설치 정보로 사용하면 안 된다.

검증 callback에는 정확한 CBOR bytes를 한 번 전달한다. 공개키는 신뢰된 caller
context에서 공급하며 파일 속 key를 root로 채택하지 않는다. portable 코드에는
SDK/heap/Flash writer가 없고 provider의 crypto 상태·수명·cleanup은 별도 책임이다.
실패 시 출력은0이며 provider 오류를 전파한다. 서명 OK도 설치 권한이 아니다.

Windows host 교차 시험만 Microsoft CNG의 실제 P256/SHA-256을 사용한다.
`cryptography==48.0.0`으로 메모리에서 임시 개인키를 생성·서명하고 CNG가 검증한다.
개인키는 파일/Git/장치에 저장하지 않는다. 합성 image는 부팅 가능하거나 image
자체 서명이 검증된 firmware가 아니다. board/epoch/호환성·부팅·Flash writer와
영속 golden fixture는 아직 미구현이다. CNG provider는 장치 firmware에 링크하지 않는다.

Windows x64 CPython3.14에서는 먼저 `python -m pip install --only-binary=:all:
--require-hashes -r tools/requirements-ota.lock`을 실행한다. host build 뒤
`ctest --test-dir build/host-debug -R ota-envelope --output-on-failure`로 재현한다.
Linux는 portable 경계 시험만 실행하며 Windows CNG 시험 성공으로 집계하지 않는다.
암호 API 근거는 [Cryptography48 EC](https://cryptography.io/en/48.0.0/hazmat/primitives/asymmetric/ec/),
[CNG verify](https://learn.microsoft.com/en-us/windows/win32/api/bcrypt/nf-bcrypt-bcryptverifysignature),
[CNG 공개키 구조](https://learn.microsoft.com/en-us/windows/win32/api/bcrypt/ns-bcrypt-bcrypt_ecckey_blob)다.
