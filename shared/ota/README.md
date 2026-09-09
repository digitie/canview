# OTA portable parser 구현 중

[T-007](../../docs/tasks/T-007-ota-container.md)의 C99 내부 primitive다. 현재
`src/cbor_head.c`는 CBOR head만 해석하며 완전한 `.cvota` verifier가 아니다.
SDK/HAL/RTOS·heap·전역 가변 상태·ISR·Flash writer 의존성은 없다.

- 입력은 호출 동안만 빌리고 저장하지 않는다. output과 입력은 겹치면 안 된다.
- 최대 9 byte만 읽는다. 모든 argument는 unsigned 64-bit이며 최소 길이만 허용한다.
- truncated prefix는 INCOMPLETE, reserve/indefinite·비최소 길이는 MALFORMED다.
- 허용 major type은 unsigned/bytes/text/array/map이다. 음수/tag/float/simple은 거부한다.
- 본문, UTF-8, map 정렬/중복, depth/schema, role/board/layout, 서명·image는 아직 미검증이다.
- OK는 head 파싱만 성공했다는 뜻이며 erase/write/PREPARED/boot selector 권한이 아니다.

바이트 계약의 근거는 [RFC 8949 §3·§4.2.1](https://datatracker.ietf.org/doc/html/rfc8949#section-4.2.1)이다.
상위 manifest schema와 서명 검증, streaming lifecycle, Python differential·golden,
target 연결·coverage·2인 review는 후속 구현이다. 현재 production app은 이 primitive를
호출하지 않으며 physical/HIL은 NOT_RUN이다. 내부 header는 최종 public API가 아니다.
