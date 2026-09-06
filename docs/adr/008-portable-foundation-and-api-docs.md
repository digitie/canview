# ADR-008: 공용 C99 기반과 생성 API 문서

- 상태: accepted
- 날짜: 2026-09-06
- 근거: 사용자 기반 코드·C99·warning0·광범위 시험 요청, Doxygen 사용 허용 확인
- ADR-005의 SDK 버전·Windows 정책은 유지하고 새 공용 component의 책임만 구체화한다.

## 결정

1. 이번 기반의 공용 protocol/app/BSP 자체 코드는 strict ISO C99로 유지한다. C99 typedef 기반 compile-time assertion과 명시적인 byte codec을 사용한다.
2. vendor SDK 전체를 C99로 내리지 않는다. IDF platform adapter는 SDK GNU 모드를 유지하며 자체 코드의 경고는 오류로 처리한다. host legacy 자동화 C11 prototype은 별도 target으로만 보존한다.
3. 새 canview_foundation IDF component가 공용 source를 노출한다. 전체 ABI 동결 전 v1.2 canview_protocol prototype과 새 v1.3 envelope를 같은 image에 연결하지 않는다.
4. Sphinx9.1.0 + Breathe4.36.0 + Doxygen1.18.0 + Furo2025.12.19로 API 사이트를 생성한다. Python dependency는 hash lock, native 도구는 URL/SHA256을 고정한다.
5. public API의 brief·모든 parameter·return은 실제 XML을 검사한다. warning을 숨겨 문서 gate를 통과시키지 않는다. Markdown 설계 정본과 생성 API는 역할이 다르므로 같은 설계를 복제하지 않는다.
6. 기반 부팅은 안전 idle만 제공한다. bench factory partition과 OTA 설계 template을 구분하며 제품 recovery/서명/rollback 구현을 가장하지 않는다.

## C 표준 자문 결과

사용자가 요청한 서브에이전트 Boole의 독립 읽기 전용 자문은 C17을 일반 권고했다.
현재 도구체인에 C99 전용 제약은 없으며 _Static_assert를 표준으로 사용할 수 있다는 이유다.
현재 codec의 caller 소유·단일 worker 계약에는 C99도 충분하므로 사용자 지시를 우선해 이번에는 전환하지 않는다.
C17 전환 자체는 atomic/heap/RTOS를 추가하지 않으며 binary 비용은 별도 비교가 필요하다.
volatile은 동기화 수단이 아니고 atomic 사용도 ISR lock-free/DMA/cache 안전성을 자동 보장하지 않는다.

- [고정 IDF v6.0.3 C 언어 설정](https://github.com/espressif/esp-idf/blob/76f5dedd9950a3012fee8fb7d5586df21fc67802/docs/en/api-guides/c.rst): 기본 gnu23 및 component별 override.
- [GCC 표준](https://gcc.gnu.org/onlinedocs/gcc/Standards.html), [MSVC 언어 모드](https://learn.microsoft.com/en-us/cpp/build/reference/std-specify-language-standard-version): MSVC에는 strict C99 모드가 없으므로 Windows clang -std=c99로 검사한다.
- [GCC volatile](https://gcc.gnu.org/onlinedocs/gcc/Volatiles.html), [IDF atomic 구현](https://github.com/espressif/esp-idf/blob/76f5dedd9950a3012fee8fb7d5586df21fc67802/components/esp_libc/src/stdatomic.c): 동시성은 adapter별 별도 설계·검증 대상이다.

## 문서 도구 대안

Hawkmoth도 검토했으나 기존 C @param/@return 계약과 연결되는 Doxygen/Breathe를 선택했다.
Doxygen을 낡았다는 이유로 제외하지 않는다. [Zephyr의 문서 생성](https://docs.zephyrproject.org/latest/contribute/documentation/generation.html)도
Doxygen XML과 Sphinx/Breathe를 결합한다. Furo는 검색·모바일 폭·light/dark 표현을 담당하며 C 추출기를 대체하지 않는다.
[Doxygen 공식 배포](https://www.doxygen.nl/download.html)에서 1.18.0 native archive digest를 확인했다.

## 영향과 남은 검증

공용 source의 MCU 독립성·warning0·unit test·문서 gate는 각각 독립적으로 실행한다.
host 성공은 target/HIL/차량 승인에 대체되지 않는다. C17 전환이나 atomic 도입은 추후 명시적인 결정과 테스트를 거친다.
범위·API 수명·후속 agent 작업은 [기반 구조](../architecture/firmware-foundation.md)가 정본이다.
