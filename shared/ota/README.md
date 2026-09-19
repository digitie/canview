# OTA portable parser 구현 중

[T-007](../../docs/tasks/T-007-ota-container.md)의 C99 내부 primitive다. 현재
`src/cbor_head.c`와 `src/cbor_document.c`는 head와 문서 구조를 검사하며
완전한 `.cvota` verifier가 아니다.
SDK/HAL/RTOS·heap·전역 가변 상태·ISR·Flash writer 의존성은 없다.

## Schema와 서명 전 JSON 입력

[wire 계약](../../protocol/schema/ota-container-v2.yaml)은 기존 C/Python의 CVOTA002
header·정렬·enum·native metadata 배정을 기록한다. [JSON schema](../../schema/cvota-v2.schema.json)는
packager 작성 입력이며 wire JSON이 아니다. `x-cbor-key` 순서로 integer-key map을 만들고
package_id/sha256의 소문자 hex를 CBOR byte string으로 변환한다. JSON schema는 기본
구조·범위를 기술하며 ABI 교차 제약·중복 target·총길이는 기존 typed 검사기가 맡는다.
JSON 숫자는 정수 literal만 허용한다. 소수/지수 표현과 NaN/Infinity를 거부하며
u64를 문자열이나 JavaScript Number로 변환하지 않는다.

서명 전 CBOR는 다음 명령으로 생성한다. Python 표준 JSON parser와 기존 typed
검사를 재사용하며 새 JSON Schema 엔진이나 암호 구현을 추가하지 않았다.

```powershell
python -B tools/ota/manifest_json.py input.json output.cbor
```

입력은 최대32KiB UTF-8이며 duplicate/unknown key, 경로/URL/Flash 주소 필드를 허용하지
않는다. 출력은 최대16KiB CBOR이고 기존 파일을 덮어쓰지 않는다. 성공 메시지의
`UNSIGNED_MANIFEST`는 인증·native image/hash 검증이나 완전한 `.cvota` 생성이 아니다.
다음 packager 단계에서 실제 native image 검증·서명과 결합해야 한다. 현재 schema도
최종 독립 리뷰 전 구현 후보이며 배포 승인을 뜻하지 않는다.

## 바깥 컨테이너 조립·검사

`tools/ota/container.py`는 위 CBOR의 외부 detached raw P256 서명과 image들을 조립하고,
신뢰된 공개키/identity로 manifest·전체 길이·zero padding·image SHA-256을 검사한다.
기존 `cryptography`와 typed 검사기를 재사용하며 개인키를 생성·읽기·저장하지 않는다.
새 파일만 만들고 기존 출력은 덮어쓰지 않는다. 입력 검증 실패 시 출력 파일을 만들지
않으며 OS 쓰기 실패의 부분 파일도 성공으로 표시하지 않는다.

```powershell
python -B tools/ota/container.py --public-key trusted.pem --role 1 --board BOARD --layout LAYOUT --epoch 7 --key-id 11 assemble input.json signature.bin output.cvota esp.bin stm.bin
python -B tools/ota/container.py --public-key trusted.pem --role 1 --board BOARD --layout LAYOUT --epoch 7 --key-id 11 check output.cvota
```

서명 입력은 정확한 CBOR이고 raw 서명은 big-endian `r[32] || s[32]`다. image 순서는
manifest와 같아야 한다. 성공은 `MANIFEST_AND_HASHES_MATCHED`일 뿐 native image 서명,
로컬 compatibility/floor, Flash 쓰기·설치 승인이 아니다. 정상 native packager/golden과
target owner 연결은 아직 남아 있다. host 입력은 format 상한으로 제한하며 MCU에는
전체 package를 메모리에 올리지 않는다.

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
부분 입력은 INCOMPLETE이며 CBOR 검사기 자체는 내부 수신 상태를 유지하지 않는다.
부분 수신은 아래 prefix 조립기가 담당한다. OK는 구조 검사 성공일 뿐 Flash writer를 호출하지 않는다.

`tools/ota/cbor.py`는 별도 Python encoder/decoder이며 동일 제한·반환 분류를
시험한다. host 객체와 출력 buffer를 사용하므로 C의 무할당 구현과 구분한다.
`tests/ota/`의 CTest 세 개는 uint64 경계·전체 첫 byte 공간·UTF-8·중첩·item/byte
상한·잘린 prefix·고정 seed 변이·순환 Python 입력을 검사한다. 재현 명령은
host build 뒤 `ctest --test-dir build/host-debug -R ota-cbor --output-on-failure`다.

상위 manifest schema·서명 검사·body streaming은 아래 절의 구현을 사용한다.
완전한 native signed packager/golden·정상 target 연결과 최종 검증은
남아 있다. 현재 production app은 이 primitive를
호출하지 않으며 physical/HIL은 NOT_RUN이다. 내부 header는 최종 public API가 아니다.

## 서명 prefix 연결 후보

`canview_ota_prefix_init/feed/finish()`는 caller의 고정16KiB급 buffer에서 prefix만
조립한다. 단일 owner가 task stack 밖에 보관하며 입력 chunk는 호출 중만 빌린다.
호출당 최대16KiB 복사·전체 최대16472B이고 heap/암호 callback은 없다. body가 같은
chunk에 있으면 `consumed` 뒤를 보존해 다음 단계에 전달한다. NULL/겹침/미초기화
인자는 context를 변경하지 않으며 길이·중복·누락 오류는 init 전까지 보존한다.
timeout/연결 종료 시 owner가 init으로 부분 입력을 폐기해야 한다. 자동 재개나 시간
판정은 하지 않는다. 완료 OK는 미인증 byte 조립일 뿐이며 기존 body open의 서명·정책
검사를 통과해야 한다. CTest `ota-prefix-stream`, `ota-container`가 수신 경계와
실제 P256/SHA-256 CNG body 연결·borrowed buffer 수명을 검사한다.

`src/envelope.c`는 작은 header+CBOR+서명 prefix를 검사한다. `tools/ota/envelope.py`
는 같은 prefix 뒤에 image bytes를 순서대로 붙이는 조립 함수다. 압축/파일시스템
경로/임의 Flash 주소는 없다. 아래 byte layout은 T-007 내부 구현 후보이며 전체
manifest schema와 최종 호환성 계약의 review 전에는 배포 포맷으로 사용하지 않는다.

| offset | 크기 | 내용 |
|---|---|---|
| 0 | 8 | ASCII `CVOTA002` |
| 8 | 2 | format version2 |
| 10 | 2 | header 길이24 |
| 12 | 4 | CBOR manifest 길이1..16384 |
| 16 | 2 | manifest signature 길이64 |
| 18 | 2 | header가 주장하는 image 수1..3 |
| 20 | 4 | header가 주장하는 전체 file 길이 |
| 24 | 가변 | 정확한 deterministic CBOR byte열 |
| CBOR 뒤 | 64 | ECDSA-P256/SHA-256 `r[32] || s[32]` |
| 서명 뒤 | 가변 | 64KiB 정렬의0 padding과 순차 image bytes; prefix 입력에는 넣지 않음 |

header 정수는 little-endian, r/s는 각각 big-endian이다. 전체 file 상한은
3×(4MiB+64KiB)+최대 prefix의 보수적 format 상한일 뿐 role/slot 허용값이 아니다. 역할별 한두
이미지 제한·서명된 길이/대상과 header 대조·본문 검증은 다음 manifest 단계에
연결해야 한다. 현재 `declared_*` 결과를 신뢰된 설치 정보로 사용하면 안 된다.

검증 callback에는 정확한 CBOR bytes를 한 번 전달한다. 공개키는 신뢰된 caller
context에서 공급하며 파일 속 key를 root로 채택하지 않는다. portable 코드에는
SDK/heap/Flash writer가 없고 provider의 crypto 상태·수명·cleanup은 별도 책임이다.
실패 시 출력은0이며 provider 오류를 전파한다. 서명 OK도 설치 권한이 아니다.

Windows host 교차 시험만 Microsoft CNG의 실제 P256/SHA-256을 사용한다.
`cryptography==48.0.0`으로 메모리에서 임시 개인키를 생성·서명하고 CNG가 검증한다.
개인키는 파일/Git/장치에 저장하지 않는다. 합성 image는 부팅 가능하거나 image
자체 서명이 검증된 firmware가 아니다. board/epoch/호환성은 아래 typed manifest와
preflight가 검사한다. 부팅·Flash writer 연결과 영속 signed golden fixture는 남아 있다.
CNG provider는 장치 firmware에 링크하지 않는다.

Windows x64 CPython3.14에서는 먼저 `python -m pip install --only-binary=:all:
--require-hashes -r tools/requirements-ota.lock`을 실행한다. host build 뒤
`ctest --test-dir build/host-debug -R ota-envelope --output-on-failure`로 재현한다.
Linux는 portable 경계 시험만 실행하며 Windows CNG 시험 성공으로 집계하지 않는다.
암호 API 근거는 [Cryptography48 EC](https://cryptography.io/en/48.0.0/hazmat/primitives/asymmetric/ec/),
[CNG verify](https://learn.microsoft.com/en-us/windows/win32/api/bcrypt/nf-bcrypt-bcryptverifysignature),
[CNG 공개키 구조](https://learn.microsoft.com/en-us/windows/win32/api/bcrypt/ns-bcrypt-bcrypt_ecckey_blob)다.

## Typed manifest 검사 후보

ESP native 검증의 SDK adapter와 실제 compile/link fixture는
[SDK fixture 계약](../../tests/fixtures/idf-ota-image/README.md)에 있다. SDK 타입은
platform/BSP 경계에만 두며 이 portable 모듈에 포함하지 않는다. 반환한 signed
metadata 대조는 아래 공통 검사기로 수행하며 body/설치 provider 연결은 아직 남아 있다.

`canview_ota_manifest_check()`는 기존 prefix/서명 검사를 호출한 뒤 고정 구조체를
채운다. 범용 객체 tree나 heap 없이 필드를 순서대로 읽고 입력 pointer를 보존하지
않는다. 실패 시 결과 전체를0으로 만든다. verify callback의 독립 context/out
재진입은 가능하고, 동일 출력이나 입력을 callback에서 변경하는 것은 금지한다.
실행은 최대16KiB 구조 검사와 서명1회, image≤3·ABI 조합≤16의 고정 반복으로
제한된다. MCU의 실제 timing/전체 call-chain stack은 아직 측정하지 않았다.

아래 integer key는 **미배포 내부 후보**다. 정식 schema/packager/native image
연결과 최종 리뷰 전 wire 계약을 확정하거나 외부에 배포하지 않는다.

| root key | 필드 | 형식 |
|---|---|---|
| 0 | format_version | uint32, 값2 |
| 1 | package_id | bytes16 |
| 2 | role | Communicator1, Controller2, Bridge3; peer 권한 enum과 별개 |
| 3, 4, 5 | board_revision, layout_id, release | 1..63 printable ASCII; 표시 버전은 비교하지 않음 |
| 6, 7 | security_epoch, key_id | uint32 |
| 8 | images | descriptor map 배열 |
| 9 | compatibility | key0 ESP 범위,1 STM 범위,2 peer 범위,3 허용 ESP/STM ABI 쌍 배열 |
| 10 | config_schema | `[read_min:uint32, read_max:uint32, snapshot:uint32]` |
| 11 | requires | `[minimum_bootloader:uint32, minimum_recovery:uint32, capabilities:uint64]` |

image map key0..6은 차례로 target enum, length:uint32, SHA-256:bytes32,
version:text, release_sequence:uint64, native signature 형식, ABI:uint32다.
signature 형식1은 ESP Secure Boot V2,2는 MCUboot P256이며 실제 서명은 native
image 안에 있다. manifest의 이 숫자만으로 이미지 서명이 검증됐다고 판단하지 않는다.
모든 map은 정확한 필수 key 집합만 허용하고 누락/추가/중복/역순을 거부한다.

target1은 Communicator ESP(4MiB),2는 Communicator STM(180KiB),3은 Controller
(4MiB),4는 Bridge(2.5MiB)다. Controller/Bridge는 해당 image1개, Communicator는
중복 없는 ESP/STM1~2개만 허용한다. zero/초과 길이와 잘못된 native signature
형식을 거부하고 offset은 prefix 끝/직전 image 끝을64KiB 경계로 올려 만든다. 외부 offset,
경로, 주소, recovery/bootloader target은 없으므로 겹친 blob을 지정할 수 없다.
계산한 image 수와 전체 길이는 header의 주장과 정확히 대조한다.

정렬 이유와 version1 거절은 [ADR-009](../../docs/adr/009-ota-native-image-alignment.md),
byte 계약은 [OTA §7](../../docs/architecture/ota.md#7-패키지인증보안-계약)을 따른다.
prefix 검사는 padding을 읽거나 검증한 것으로 간주하지 않는다.

identity는 신뢰된 BSP/provisioning caller가 공급한다. 파일/HTTP에서 기대 role,
board/layout, epoch, key_id를 가져오면 안 된다. key_id는 verify context에서 실제
선택한 역할별 root와 같아야 한다. 현재 합성 host fixture의 board/layout 문자열은
실제 보드나 승인된 Flash layout ID가 아니다.

ABI 범위의 min≤max, image ABI 포함, 조합 범위/중복·Communicator 비어 있지 않은
조합 목록, config snapshot의 범위 포함을 검사한다. **실제 실행중/새 image와 네
old/new 조합의 호환성 대조, requires 충족, version floor와 본문 검사는 아래 별도
단계에서 수행한다.** STM native 검사와 ESP SDK adapter·BSP 연결은 구현했다.
정상 OTA owner/장치 실행·target 통합은 아직 남아 있다.
OK는 erase/write/PREPARED/boot selector 권한이 아니다. 정식 CLI·golden/target 연결도 남았다.

`ctest --test-dir build/host-debug -R ota-manifest --output-on-failure`로 C/Python
typed 교차 시험을 실행한다. portable probe는 서명 mock을 사용하며 Windows CNG
probe를 사용하는 별도 시험만 실제 서명 검증이다. 양쪽은 잘못된 role/board/layout/
epoch/key, 필드 누락/추가, 중복 target/key, 길이 상한, uint64 sequence 보존,
truncation, ABI/config 경계를 검사한다. 실제 firmware 설치·물리/HIL은 NOT_RUN이다.

## 순차 image 본문 검사

`body_open()`은 완전한 prefix의 manifest·로컬 호환성·version floor를 검증한 뒤 첫 SHA-256
operation을 시작한다. `body_feed()`는 절대 file offset과0..16KiB chunk를 받아
prefix 실제 크기부터 padding·이미지 경계를 순서대로 처리한다. padding은0인지
검사만 하며 hash에서 제외한다.64KiB buffer는 없다. image bytes는 복사/보존하지 않고 SDK provider의
update에 전달한다. 끝에서 signed descriptor의 SHA-256과 대조한 뒤 operation을
정리한다. 모든 image가 맞아야 `HASHES_MATCHED`가 된다. `body_finish()`는 EOF를
확인하며 아직 부족하면 `INCOMPLETE/FAILED`로 종료한다. 이름은 모두
`canview_ota_` prefix를 가진 내부 C API다.

- caller는 body 객체를 처음에 `{0}`으로 초기화하고 한 task에서 직렬 호출한다.
  prefix/identity/runtime/floor/chunk는 호출 중만 빌리며 descriptor와 hash 함수표는 복사한다.
  provider context는 reset 성공까지 유효해야 한다. 활성 body 복사/memset은 금지한다.
- offset 중복·누락, 길이 초과, hash/provider 오류는 FAILED이며 manifest 결과를
  지우고 floor 판정도 무효화한다. 실패한 stream에 재전송해 이어 쓰지 않는다.
  reset 후 prefix부터 다시 검증한다.
- `body_reset()`은 부분 start/수신/finish/실패를 정리한다. cleanup 실패 시 context와
  자원 소유 상태를 보존해 reset을 재시도할 수 있다. 원 오류와 cleanup 오류는 별도다.
- busy flag는 동일 객체 callback 재진입만 막는다. thread lock이 아니며 ISR에서
  호출하지 않는다. 호출당 hash에 전달하는 데이터는 최대16KiB, image 경계 반복≤3이다.
  빈 chunk는 offset을 진행시키지 않는다. transport timeout은 caller가 reset으로 종료한다.

암호 구현은 provider가 소유한다. Windows host는 기존 CNG P256 verifier를
`tests/ota/cng_provider.c`로 추출해 재사용하고 CNG SHA-256 operation을 연결했다.
Windows SDK가 hash object를 할당/해제하므로 이 **host provider의 할당**을 portable
core의 무힙 특성과 혼동하지 않는다. 실제 장치용 SDK provider는 아직 연결하지 않았다.
[CNG 생성](https://learn.microsoft.com/en-us/windows/win32/api/bcrypt/nf-bcrypt-bcryptcreatehash),
[완료](https://learn.microsoft.com/en-us/windows/win32/api/bcrypt/nf-bcrypt-bcryptfinishhash),
[해제](https://learn.microsoft.com/en-us/windows/win32/api/bcrypt/nf-bcrypt-bcryptdestroyhash)의
공식 계약을 따른다.

`ctest --test-dir build/host-debug -R ota-body --output-on-failure`로 실행한다.
portable probe의 sum/length 모형은 실패·수명 시험일 뿐 SHA-256 검증이 아니다.
Windows crypto probe는 역할별 임시 공개키, 실제 P256 서명과 SHA-256, `abc` known
answer, 본문 변이·최대 image·잘린 입력·provider/cleanup 실패를 시험한다. 시험은
native firmware가 아닌 합성 bytes만 사용하며 개인키를 저장하지 않는다.

`HASHES_MATCHED`는 **native image signature/protected metadata·최신 version floor 재검증,
설치 직전 로컬 상태 재확인, Flash read-back, 설치 또는 PREPARED 승인과 별개**다.
이 모듈에는 writer·boot selector callback 자체가 없다. prefix의 부분 수신 조립,
전체 signed packager/검사 CLI/golden, 실제 ESP/STM provider와 정상 target 통합은 남아 있다.
schema와 서명 전 JSON 작성 도구는 위 절의 구현을 사용한다.

## 로컬 호환성 사전 검사

별도 framework 대신 기존 manifest 검사 뒤에 `canview_ota_manifest_preflight()`를
연결했다. body open도 이 함수를 호출하므로 호환성이 맞기 전 hash operation을
시작하지 않는다. 검사 실패 시 해석 결과를 모두 지운다.

- 로컬 snapshot은 BSP와 신뢰된 boot/정상 앱 metadata에서 caller가 읽는다.
  파일/HTTP 입력을 복사하지 않는다. `available=false`면 INCOMPLETE이며0으로 추정하지
  않는다. recovery 실행 중에도 esp/stm_abi는 recovery 자신의 값이 아닌 정상 앱 또는
  검증된 보존 정상본의 값이다. 이를 확인하지 못하면 사전 검사를 통과할 수 없다.
- Communicator의 `(old,old)`, `(new,old)`, `(old,new)`, `(new,new)`가 모두 signed
  조합 목록에 있어야 한다. 한 image만 포함하면 나머지 MCU는 기존 ABI를 유지한다.
  같은 ABI의 중복 조합은 목록에 한 번만 있으면 된다. 범위와 조합 수는 기존 parser가 제한한다.
- ESP/STM 각각의 bootloader/recovery ABI가 manifest 최소 요구 이상인지 확인한다.
  Controller/Bridge에서는 사용하지 않는 stm_*를 검사하지 않는다. 외부 peer의 연결이나
  ABI는 이 설치 사전 조건에 넣지 않는다. peer 기능 협상과 차량 권한은 별도다.
- 필요한 hardware capability는 로컬 bit 집합의 부분집합이어야 한다. 보존 중인 config
  schema는 후보의 읽기 범위 안이어야 한다. snapshot을 수정하거나 migration하지 않는다.

이 함수는 서명된 후보의 주장과 로컬 snapshot을 비교할 뿐이다. native image의
서명/보호 metadata 대조는 별도 단계다. STM 검사와 ESP SDK adapter·공통 metadata
검사는 아래 구현을 사용하며 정상 owner/target 연결은 미완료다.
snapshot을 보존하지 않으므로 설치 owner는 transaction/상태 변경 뒤 다시
검증해야 한다. floor 비교는 아래 검사, 영속 갱신과 write/activation 권한은 별도 owner다.

## 버전 하한 사전 검사

`canview_ota_floor_check()`는 preflight 뒤, 첫 본문 hash operation 전에 호출한다.
기존 [OTA §7.1](../../docs/architecture/ota.md#71-영속-버전-하한과-복원-예외)의 정책을
순수 C99 비교로 구현하며 영속 journal·범용 policy engine을 추가하지 않는다.

| 후보와 로컬 record 비교 | 반환/판정 |
|---|---|
| sequence < minimum_sequence | `STALE`, 수신 후보 거절 |
| 같은 sequence, 다른 confirmed_digest | `AUTH_FAILED`, `CONFLICT` |
| 같은 sequence/hash, 실제 정상 설치 검증 | `ALREADY_INSTALLED` 사전 판정 |
| 같은 sequence/hash, 정상 앱 손상/부팅 선택 불가 확인 | `REPAIR_REQUIRED` 후보 |
| sequence > minimum_sequence | `UPGRADE` 후보 |
| policy 미확인 또는 동일 이미지의 실제 설치 상태 미확인 | `INCOMPLETE` |

sequence는 uint64 직접 비교한다. `floor_result.images[]`는 manifest image 순서이며
record 순서는 독립이다. 누락/중복/잘못된 역할·target·enum·board/layout/epoch를
검사하고, 어느 target이든 실패하면 판정 전체를0으로 지운다. 성공도 floor를 변경하지 않는다.

`floor.ready`는 신뢰된 journal owner가 valid copy 선택과 CONFIRM_INTENT/실제 boot
상태 조정을 완료했다는 뜻이다. `{0}`이나 두 copy 손상을 floor0으로 간주하지 않는다.
이 경우 owner는 RECOVERY_LOCKED와 쓰기 금지를 유지한다. 현재 비교기는 copy를 읽거나
CRC/commit을 검증하지 않는다. `floor`는 영속 wire가 아닌 호출 중만 빌리는 snapshot이다.
네트워크 입력으로 구성하지 않으며 task owner가 Flash/정책 변경과 직렬화해야 한다.

`BOOTABLE`은 실제 정상 앱의 전체 hash/native 서명/board/layout/epoch/부팅 선택
metadata 검증을 뜻한다. 그 앱의 observed sequence/digest도 floor와 대조한다.
recovery 실행이나 과거 transaction 성공은 설치 증거가 아니다. 읽기 실패는 UNKNOWN,
손상/선택 불가를 확인한 경우만 DAMAGED다. 같은 정식 이미지의 복구에도 전체 native
검증·새 activation commit·trial이 필요하다. 자동 rollback/확정/floor 갱신은 하지 않는다.

`ctest --test-dir build/host-debug -R 'ota-version-floor|ota-body' --output-on-failure`
로 순수 비교와 body 연결을 검사한다. 영속 A/B·실제 앱 검사 provider·전원 차단 검증은
아직 남아 있으며 host snapshot 모형을 물리/HIL 결과로 표시하지 않는다.

## ESP-IDF 암호 provider

[PSA adapter](../../firmware/platform/esp32s3/ota_crypto.h)는 기존 manifest callback과
body SHA256 함수표에 ESP-IDF6.0.3의 PSA Crypto를 연결한다. BSP가 공급하는 역할별
신뢰 P256 공개키를 volatile/VERIFY_MESSAGE 전용으로 import한다. 입력 package의
공개키나 개인키·영속 key 생성은 처리하지 않는다. context마다 key1개/hash operation1개를
소유하며 크기는 manifest/chunk16KiB, raw signature64B, digest32B로 제한한다.

한 task가 직렬 호출하며 ISR/동시 호출은 금지한다. 입력은 호출 중만 빌리고, context와
겹치는 입력/출력과 SDK 재진입은 거절한다. setup/update/finish 실패 후에는 reset을
완료해야 재사용할 수 있다. body_reset 성공 뒤 crypto_close로 key를 해제한다.
abort/destroy 실패 시 handle을 지우지 않고 close 재시도 동안 새 검증/hash를 차단한다.
SDK 내부 자원 부족은 실패로 전파하며 portable core에 SDK header를 넣지 않는다.

`ota-psa-provider` CTest는 실제 adapter의 실패·수명·재진입 **모형**이다. 실제 SDK
fixture에서도 compile/link하지만 모형 PASS를 실제 PSA 암호 실행으로 표시하지 않는다.
정상 OTA owner/BSP root provider 연결과 MCU 실행은 아직 남아 있다. 설치/Flash 권한은 없다.

## STM native image 검사

`canview_ota_stm_image_check()`는 MCUboot v2.4.0의 비압축·비암호화 P256 image를
검사한다. 공식 imgtool은 `tools/toolchain-versions.json`의 commit
`6d3b3d2c38ab20c242e5b9abb04d050086383eb2`로 고정한다. header512B, 일반 앱 image
최대180KiB, load address/flags0, 별도 slot padding/trailer가 없는 profile이다.
protected TLV 한 개 뒤 SHA256·KEYHASH·P256 DER TLV만 허용한다.

검사기는 전체 image SHA256을 manifest와 비교하고, header+code+protected TLV의
SHA256과 native signature도 독립 검사한다. DER은 정규 positive integer 두 개를
raw r/s로 옮길 뿐 암호 연산은 SDK가 소유한다. root는 입력 이미지가 아니라 신뢰된
STM 전용 공개키/context에서 공급하며 SPKI DER key hash도 대조한다. 함수는 불변
전체 image를 호출 중만 빌린다. memory-mapped staging도 가능하지만 실제 Flash
reader·SDK provider·T-107 bootloader 연결은 아직 없다. vector/부팅 가능성 검사는
이 image-format 시험의 성공만으로 주장하지 않는다.

보드·역할·sequence는 native 표준 header만으로 표현할 수 없으므로 MCUboot의 기존
vendor protected TLV(tag `0x00A0`)를 사용한다. 내부168B metadata 후보는 다음과 같다.
ESP에는 SDK의 `.rodata_custom_desc`에서 같은 정보를 읽고 공통 검사기로 대조한다.
실제 이미지에 descriptor를 생성·삽입하는 packager/target 연결은 아직 없다.
아래 배정은 wire schema에 기록했으며 최종 독립 리뷰 전 구현 후보다.

| offset | 내용 |
|---|---|
| 0 | `CVIMG001`8byte |
| 8,10 | version1:u16, size168:u16 |
| 12,16,20,24,28 | role, target, security_epoch, ABI, reserved0:u32 |
| 32 | release_sequence:u64 |
| 40,104 | board_revision, layout_id: 각각64byte |

정수는 little-endian, 문자열은 printable ASCII1..63byte/NUL/나머지0이다. 이 값들과
manifest/local identity가 다르면 거부한다. image version도 native header와 정규
`major.minor.revision+build` 문자열을 대조한다. 서명된 metadata의 잘못된 역할이나
sequence는 서명이 유효해도 거부한다. 성공은 영속 floor/REPAIR/activation 승인이 아니다.

`native_metadata.c`는 기존 STM 대조를 공통화한 무상태 C99 코드다. STM과 ESP가
별도 parser를 유지하지 않는다. `canview_ota_native_metadata_check()`는 정확한168B와
유효한 role/target/signature 조합을 요구한다. `canview_ota_esp_metadata_check()`는
여기에 SDK `app.version[32]`의1..31byte printable ASCII/NUL/zero padding 대조만 더한다.
ESP SDK native 서명·전체 hash 성공 뒤, 같은 불변 이미지의 반환 metadata를 전달해야 한다.
이 함수에 웹 입력 metadata만 주어 얻은 OK는 서명 증거가 아니다. `secure_version`을
u64 release_sequence나 제조 epoch로 변환하지 않으며 epoch는 위 custom 필드에서 대조한다.
Communicator의 [BSP 연결](../../firmware/communicator/esp32/bsp/ota.h)은 generated board ID와
고정 `bundle_stage`의 주소/크기를 검사한 뒤 SDK→metadata를 호출한다. 입력은 인증된
COMM_ESP descriptor와 신뢰된 provisioning identity이며 외부 partition pointer/주소를
받지 않는다. template staging의 `encrypted` flag와 BSP 위치/크기도 generator가 소유한다.
실제 OTA task의 단일 owner/Flash 불변 보장·body orchestration과 다른 장치 연결은 남아 있다.
`ctest --test-dir build/host-debug -R ota-native --output-on-failure`로 공통 경계와
기존 공식 imgtool/CNG STM 회귀시험을 실행한다.

Windows에서 `MCUBOOT_ROOT`를 위 commit의 clean checkout으로 지정하고 기존 OTA
wheel lock을 설치한 뒤 `ctest --test-dir build/host-debug -R ota-native-stm
--output-on-failure`를 실행한다. 기본 경로는 `C:/cv/mcuboot-2.4.0`이다. CI도 같은
commit을 clone/확인한다. 시험은 imgtool의 실제 생성/검증과 CNG 검증을 사용하며
개인키는 메모리에만 둔다. 합성 code bytes는 부팅 가능한 firmware가 아니다.
`--model`은 Python 계산 결과를 돌려주는 비암호 callback 모형으로 별도 집계한다.

공식 근거:
[MCUboot imgtool](https://github.com/mcu-tools/mcuboot/blob/6d3b3d2c38ab20c242e5b9abb04d050086383eb2/docs/imgtool.md),
[image 형식](https://github.com/mcu-tools/mcuboot/blob/6d3b3d2c38ab20c242e5b9abb04d050086383eb2/docs/design.md),
[ESP-IDF custom descriptor](https://github.com/espressif/esp-idf/blob/v6.0.3/docs/en/api-reference/system/app_image_format.rst).
