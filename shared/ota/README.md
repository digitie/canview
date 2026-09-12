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
