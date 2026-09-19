# CANView 작업 일지

## 2026-09-19 (codex, identity overlap 사전 거절 oracle)

[82c5193 재검토](reviews/adversarial/2026-09-19-T-007-stage-post.md)에서 원 A는 PASS,
원 B는 identity 사례의 B-STAGE-01 잔존 P2로 CONDITIONAL을 반환했다. 양쪽 원문을
보존하고 B의 OPEN을 유지했다. 해당 조건만 삭제한 C mutant가 기존 시험에서 exit0인
것을 실제 재현했다. reset 전에 body EMPTY를 검사해 하위 parser가 입력을 지운 후
같은 오류를 반환하는 경로와 사전 거절을 구별한다. Production C는 변경하지 않았다.

다섯 실제 C mutant와 baseline, model/CNG stage27그룹씩 통과했다. 전체 Debug150/150
27.52초·Release150/15023.42초, 수정 모형 ASan/UBSan27그룹도 통과했다.
로그: build/t007-stage-post2-{debug,release}-test.log,
before-fix 재현은 build/t007-stage-identity-before-fix.log다. source/합성 digest를 먼저
고정하고 전체 시험을 실행했다. 원 A/B의 재확인과 새 candidate CI는 아직 남아 있다.

569cc83 CI35424935438은6/6, target21개 bytes/hash·source7개·target logs28개
warning/error0을 대조했다. Manifest SHA256:
`707b25dc3d9234cea85e581e4418e91e591309950d628db257d6718cfdadf30b`.
82c5193 독립 Linux clone ASan/UBSan139/13919.30초와 구분한다. 정적 예산 기록
부족은 원 A/B FIXED지만 physical/HIL·총 자원/시간은 NOT_RUN, 차량 TX NO-GO다.

## 2026-09-19 (codex, stage 인자 방어 oracle 수정)

569cc83의 [독립 리뷰](reviews/adversarial/2026-09-19-T-007-stage.md)는 A 정적 PASS,
B CONDITIONAL이다. B-STAGE-01 P2는 여러 잘못된 인자가 겹쳐 의도한 방어 삭제를
검출하지 못하는 시험 결함이다. hash context overlap 조건만 삭제한 실제 C mutant가
기존 시험에서 exit0인 것을 재현했다. production stage.c는 변경하지 않았다.

유효한 입력의 begin/close 양성 대조 뒤 인자 하나만 변경하도록 시험을 고쳤다.
중첩 구조는 stage의 정렬된 충분한 공간에 유효 값을 복사하고 제어 필드는 보존한다.
Hash start 모형은 불법 context를 역참조하지 않는다. 같은 방어 삭제 mutant는 이제
CHECK exit1로 실패하며 compile 오류/crash/timeout을 성공으로 세지 않는다.
기존 call-order3개와 함께4개 mutant, model/CNG27그룹씩을 확인했다.

최종 Debug150/15016.24초·Release150/15014.11초, build warning/error0이다.
로그는 build/t007-stage-post-debug-final-test.log 및
build/t007-stage-post-release-test.log다. 첫 Debug 전체 실행은 문서의 합성 digest를
갱신하던 중 시작해 capture identity 시험1개가 실패했다. source/fixture를 고정한
뒤 위 전체 회귀를 다시 실행했다. 실패 로그도 t007-stage-post-debug-test.log에 남긴다.
수정된 stage 모형 ASan/UBSan27그룹·함수6/6·행91/91·분기90/92를 확인했다.
기존569cc83의 독립 Linux 전체139/139와 구분한다. 원 A/B post-fix 확인·새 CI는 남았다.
실제 Flash/HIL·장치 자원/시간은 NOT_RUN, 전체 T-007 완료는 아직 선언하지 않는다.

## 2026-09-19 (codex, stage 정적 자원 근거와 Linux 전체 회귀)

stage candidate569cc83의 ESP fixture DWARF에서 prefix16488B/body856B/stage896B/
PSA108B를 재확인했다. `.su`의 자체 frame과 상한 연산량을 OTA README에 모았다.
stage는 body를 포함하며 SDK 내부 heap·호출 chain·시간을 무사용/통과로 추정하지 않는다.
실측은 해당 target owner와 T-508 gate에 남긴다. 문서 변경에 따른 합성 digest만
갱신했고 Windows Python/link 회귀2/2·strict docs71 API가 통과했다.

Windows worktree를 /mnt/f에서 직접 사용한 보조 Linux 전체 실행은138/139였다.
python-unit에서 Git commit 식별자가 없어 HIL evidence validation이 실패했다.
WSL git이 .git의 F:/dev/canview/... 포인터를 Linux 상대 경로로 해석하는 것을
직접 재현했다. source 결함으로 우회하거나 evidence validator를 완화하지 않았다.
새 독립 clone /tmp/canview-stage-repo-Lk67Ca에서 동일569cc83을 detached checkout해
Clang ASan/UBSan·leak 검사139/13918.29초가 통과했다. configure/build 경고0,
로그는 clone의 configure.log/build.log/test.log다. 기본 checkout은 변경하지 않았다.
이 성공은 후속 문서 source나 실제 장치 Flash/HIL·시간·heap 측정 성공이 아니다.

## 2026-09-19 (codex, C 수신 저장 순서 연결)

기존 parser/body를 재사용한 작은 C99 stage open/feed/finish/reset을 추가했다.
서명·identity·길이·호환성·floor 실패 시 begin0회, chunk 거절 뒤 write0회,
전체 body/hash 성공 뒤 native callback1회와 실패 후 진행 차단을 검사한다.
정상 begin/write의 양성 기준을 먼저 실행하므로 writer 부재를0회 PASS로 세지 않는다.
heap·RTOS·설치 상태기계·영속 journal·boot selector는 추가하지 않았다.
SDK와 Flash map의 실제 enforcement는 후속 BSP owner 책임으로 유지한다.

- Windows model/CNG stage 입력27그룹씩: 각 정상 입력에서 failure/reentry/cleanup
  시나리오와 chunk1/31/최대 길이를 실행했다. native/storage는 모형이다.
- 실제 C mutant3종(조기 begin, 거절 후 write, native 오류 무시)은 CHECK exit1로
  검출했다. baseline exit0을 선행하며 compiler 오류/timeout/crash는 성공이 아니다.
- Debug150/15052.33초·Release150/15044.42초, configure/build warning/error0.
  로그: build/t007-stage-full-{debug,release}-{build,test}.log.
- WSL Ubuntu26.04 Clang ASan/UBSan·leak 검사 통과. build:
  /tmp/canview-stage-check-VLrbYx. stage.c 함수6/6·행91/91·분기90/92.
- 실제 ESP-IDF6.0.3 fixture ELF/MAP/BIN·metadata172개 음성 시험 통과,
  build/t007-stage-sdk.log warning/error0. BIN SHA256:
  `d6119ceb51fb4cef0f2e141aefe7bbffd6659da961eb5e61b1b49c12287d06f2`.
  API4개 target link, DWARF stage896B, 자체 frame open48B·나머지32B.
  실제 저장 provider나 정상 owner 연결·device 실행 근거는 아니다.
- strict Doxygen/Sphinx 통과: build/t007-stage-docs.log, public API71개.
  합성 HIL source digest만 갱신했으며 실제 차량 evidence는 수정하지 않았다.

최초 host fixture는 stage를 context 첫 멤버로 둬 overlap 검사에서 거절됐다.
CHECK abort의 Windows CRT 대기로 Python30초 timeout이 났고 실행 중 재링크도
permission denied였다. 원 process 종료와 process inventory를 확인한 뒤 fixture를
별도 stage pointer로 고치고 CHECK를 exit1로 바꿨다. 이후 위 전체 회귀가 통과했다.
첫 WSL 변수 전달은 quoting 때문에 /configure.log 권한 오류였다. 독립 mktemp와
확정 절대 경로로 다시 실행했으며 실패한 최초 명령은 PASS로 집계하지 않는다.

native CLI6cf1106은 [원 A/B 재확인](reviews/adversarial/2026-09-19-T-007-native-post.md)에서
정적 PASS·관련 finding FIXED다. 같은 source CI35423635752는6/6 성공이며 내려받은
target21개 bytes/hash·source7개·target logs28개 warning/error0을 직접 대조했다.
manifest SHA256: `efdfff60aa2cd3db8215bb267e1dab8b2044d3fa5f7b27ea0230f85f2b7bce9e`.
이 CI/리뷰를 새 stage source에 재사용하지 않는다. stage 독립 리뷰/최종 CI와
T-007 AC3·최종 수용 감사는 OPEN이다. 실제 Flash/HIL·device crypto·장치
timing/heap/stack 실측은 NOT_RUN, 차량 TX NO-GO, PR36 Draft를 유지한다.

## 2026-09-19 (codex, native ESP 서명 블록 결합 회귀)

7cb07be의 독립 A 리뷰가 A-NCLI-01 P2를 발견했다. espsecure의 외부 공개키 서명
검증만으로는 block의 scheme·내장 key와 결합되지 않았다. scheme와 RSA n/e/rr/m
필드를 각각 변경하고 block CRC·outer hash·outer 서명을 갱신한5건이 기존 코드에서
거절되지 않는 것을 실행해 확인했다. 장치 검증 우회나 Flash 실행 증거는 아니다.

새 암호 구현 대신 고정 공식 pre-calculated-signature helper로 실제 RSA 서명을
검증하고 동일 block을 재구성해 byte 단위로 대조한다. 올바른 scheme/key/digest와
서명이 서로 다른 block에 있어도 통과하지 않는다. CRC를 갱신한 서명 변이까지
6개 필드 거절·정상3개 슬롯·교차 block 거절을 추가했다. 도구 버전·dirty checkout·
timeout/실행 실패와 CLI 출력 미생성 시험도 보강했다. 총8개 unittest method 통과,
로그는 build/t007-native-block-binding-test.log다. 원 리뷰어 재확인은 아직 남아 있다.

T-007에는 상위 OTA 정본과 후속 T-204/T-107/T-205의 책임을 대응했다. 실제 writer
부재를 금지 쓰기0회 성공으로 보지 않으며 AC3 연결 시험은 여전히 OPEN이다.
문서 링크 검사에서 잘못 적은 T-508 파일명을 실제 경로로 수정했다.

앞선 c7f5780의 CI35422223529는6/6 성공이다. target artifact21개 bytes/hash와
source7개를 Windows CRLF로 재구성해 대조했고 target log28개 warning/error0이었다.
manifest SHA256: `32c53499dca5b0916a5d8d916034cf38b79bbf3a975259af8c64ca230cbf7985`.
실제 ESP host RSA10건도 통과했다. 이후 native source의 CI 결과로 재사용하지 않는다.
Physical/HIL·device crypto·Flash·장치 timing/heap은 NOT_RUN, 차량 TX는 NO-GO다.

## 2026-09-19 (codex, 일반 native-aware CLI 연결)

기존 container.py에 `--native`를 연결했다. Outer 검증 뒤 공식 espsecure5.4.0의
RSA3072·esptool ESP32-S3 image parser와 고정 clean MCUboot v2.4.0 imgtool verifier를
사용한다. CANView metadata168B·version·정확한 u64를 비교하며 host 설치 권한을
발급하지 않는다. 조립도 native 검사 뒤에만 새 출력을 생성한다.

6개 unittest가 보존 golden, 다른 board/layout의 ESP3역할, outer가 유효한 native
변조와 sequence/version/ABI 오류, key/SDK 오류, 기존 출력 보존을 검사한다.
ESP 역할별 fixture는 기존 SDK image를 공식 parser로 읽고 checksum/digest를 갱신해
공식 서명 도구에 넘겼다. 시험 개인키는 메모리에만 만들며 파일/Git에 저장하지 않는다.
초기 시험에서 공식 signing API가 BytesIO.name을 요구해 실패했고 합성 입력 이름을
제공하도록 수정했다. 실패를 성공으로 바꾸거나 원 golden을 변경하지 않았다.

esptool에는 Windows wheel이 없어 --only-binary 다운로드가 실패했다. 공식 PyPI
sdist SHA256과 dependency wheel, setuptools82.0.1 backend를 별도 lock에 고정했다.
SDK를 수정하지 않고 build/ota-native-clean venv에 설치했다. 캐시 없이 sdist를 다시
빌드한 설치와 병렬로 실행됐던 host 결과는 최종 gate로 사용하지 않고, 설치 process의
정상 종료 뒤 Debug/Release를 다시 실행해 각각147/147 통과했다.
pip check·generator·문서/task 검사도 통과했고 host compiler/linker/CMake 경고0이다.
로그는 build/t007-native-*.log, 최종 시험은 t007-native-final-{debug,release}-test.log다.
모듈 README 변경으로 합성 T103 digest를
`de0355a890823f2bf64879c6655fdfcaadefaf130ab4ea95aec51991387c19e5`로 갱신했다.
실제 physical evidence나 private 자료는 변경하지 않았다.

receiver P2는 [원 A/B 재검토](reviews/adversarial/2026-09-19-T-007-receiver-post.md)에서
FIXED·정적 PASS다. c7f5780 CI35422223529의 Windows/Linux 등5개 job이 통과했고
target job은 진행 중이다. 이후 native CLI 구현의 검증·리뷰는 별도다.
T-007 전체 acceptance는 아직 OPEN/BLOCK이며 scope 충돌·writer/정책/target 예산
gate를 생략하지 않는다. Physical/HIL/Flash/device PSA NOT_RUN, 차량 TX NO-GO다.

## 2026-09-19 (codex, receiver oracle 조기 인증 오류 회귀 수정)

0acdc45의 [A/B 독립 원문](reviews/adversarial/2026-09-19-T-007-receiver.md)을 보존했다.
A PASS/B CONDITIONAL이며 B-RX-01 P2를 실제 CNG C mutant로 재현했다. 본문 마지막
byte를 변조하기 전에 AUTH_FAILED가 나도 이전 status-only oracle은 성공했다.
변이 feed 자체의 오류와 끝 offset을 함께 요구하도록 고쳤다. 새 회귀 CTest는
조기 open/feed 오류에서 exit1, 이전 약한 oracle 대조에서 exit0을 확인한다.
컴파일 실패를 기대 실패로 인정하지 않으며 제품 source는 변경하지 않는다.

Windows Debug/Release146/146, SDK 재빌드·metadata172개 음성 사례·경고0을 확인했다.
로그는 `build/t007-receiver-oracle-{test,sdk}.log`와
`build/t007-receiver-post-{debug,release}-test.log`다. 원 reviewer 재확인과 새 CI는 남아 있다.
0acdc45 CI35421702811의 Windows job 실패는 Doxygen 공식 ZIP 다운로드 실패다.
진행 중인 target job은 유지했으며 개별 rerun은 GitHub가 거절했다. 이전 d87516a
CI35420771316은6/6과 target21개/source7개/log28개 경고0을 감사했고 이후 source에
재사용하지 않는다. 로컬 strict Doxygen/Sphinx·API71개도 통과했다.
실제 device PSA/Flash/HIL·총 stack/heap/timing은 NOT_RUN, 차량 TX는 NO-GO다.

## 2026-09-19 (codex, 실제 SDK C parser/body 수신 연결·전체 수용 감사)

기존8개 portable C 파일을 IDF component로 등록하고 read-only signed golden을
fixture BIN에 넣었다. manifest P256/SHA256은 실제 SDK PSA를 사용하며 native
image 검증/Flash/설치 승인 경로는 아니다. 같은 receiver.c를 실제 Windows CNG로
실행해 정상·서명/본문 변조·identity 불일치4건을 검사했다. Debug/Release145/145다.
Windows host driver의 fopen deprecation 실패는 fopen_s로 수정했으며 경고를 억제하지 않았다.

SDK build는 `build/t007-idf-receiver-sdk-build.log`, host는
`build/t007-receiver-{debug,release}-{configure,build,test}.log`에 남겼다.
SDK ELF/MAP/BIN과 parser/body/PSA symbol을 확인했다. BIN SHA256은
`17bc7bf232acc156c94690a3f331fdb955a21129afc9d383da78cec241c046bc`이며 unsigned fixture다.
metadata168개 변이+4개 절단을 거절했다. 기존 보존 golden은 과거 provenance 입력을
계속 사용하며 새 fixture로 재서명하지 않았다.

DWARF sizeof prefix/body/PSA=16488/856/108B이며 함수 static이다. 자체 `.su`에서
app_main480B·receiver624B·manifest_check880B를 확인했다. SDK 전체 stack은 아니다.
fixture sdkconfig.defaults의 main stack을16384B로 예약하고 기존 ignored sdkconfig의
같은 항목도 갱신해 재빌드했다. watchdog은 변경하지 않았다. MCU timing/high-water/
heap/실제 PSA/Flash/HIL은 NOT_RUN이다. 새 firmware component/모듈 README 때문에
합성 T103 source identity를 `8ac453bbece8b1f635ed989bb57b6807f8087d382be5318a43ada718e55e63f6`로
갱신했으며 실제 physical evidence를 변경하지 않았다.

d87516a의 [독립 전체 수용 감사](reviews/adversarial/2026-09-19-T-007-acceptance.md)는
A/B BLOCK이다. 원문2개와 공통 request를 보존했다. Native 일반 검사·task 책임 충돌·
target/예산은 OPEN, README의 구현된 prefix/golden 상태 문구는 수정 후보다.
다음은 기존 공식 도구를 이용한 일반 native-aware CLI 연결이며 새 framework는 없다.
기본 checkout의 사용자 dirt는 그대로 보존했다. PR36 Draft·차량 TX NO-GO를 유지한다.

## 2026-09-19 (codex, PSA ready 중첩 시험·golden artifact 감사)

6f078ac의 원 A/B [재검토 원문](reviews/adversarial/2026-09-19-T-007-psa-post.md)을
보존했다. B-PSA-01 P3는 FIXED지만 동일한 신규 P2(A-PSA-POST-01/B-PSA-02)를
받았다. union context가 unready라 signature 중첩 조건을 없애도 같은 오류로
통과할 수 있었다. 별도 context를 정상 init한 뒤 시험하고 SDK 호출 수 불변,
후속 정상 verify와 close, reentry 대상 교체를 확인하도록 고쳤다.

Signature 중첩 조건만 제거한 실제 C mutant는 GCC 빌드 뒤 새 CHECK에서 실패했다.
정상 source는 PASS였다. `build/t007-psa-overlap-mutant.log`가 근거다.
이전 권한 상수를0x1000으로 되돌린 별도 header mutant도 독립 typedef 검사에서
컴파일 실패했다. `build/t007-psa-usage-mutant.log` SHA256은
`548c812f245b4e0f0f71bb343ba53aa04217f564e090387a43d778e1c9c6f65f`다.
제품 provider를 수정하거나 경고를 억제하지 않았다. 새 ASan/UBSan 모형도 통과했고
adapter 행149/149·분기114/118은 유지했다.
Windows Debug/Release144/144와 build 경고0을 재확인했다.
로그는 `build/t007-psa-overlap-{debug,release}-{build,test}.log`다.

262bf09의 CI35418535641 target artifacts를 실제 내려받아21개 ELF/MAP/BIN
bytes/SHA256과 source7개 Git object의 Windows CRLF byte열을 대조했다.
28개 target log에서 실제 warning/error 진단0, 공식 ESP RSA host10건도 확인했다.
Manifest SHA256은 `ebbccfbab14726e514d99c87610f49c37e434c985adb5f1c621a29ff8af5eec3`다.
이 감사는 golden 수정본262bf09만의 증거이며 이후PSA source의 최종 gate가 아니다.
PSA6f078ac CI35420077728의 GCC·Clang·sanitizer는 성공했고 Windows/target은 확인 당시
진행 중이었다. 이번 P2 수정본의 reviewer/CI는 별도 확인해야 한다.

정상 OTA owner·root·영속 policy 연결 미완료, physical/HIL NOT_RUN·차량 TX NO-GO를 유지한다.

## 2026-09-19 (codex, PSA 리뷰·GCC 시험 수정)

70c7a38의 [독립 원문과 disposition](reviews/adversarial/2026-09-19-T-007-psa.md)을
보존했다. A는 정적 PASS, B는 모형 VERIFY_MESSAGE=0x1000 오류 B-PSA-01 P3를
보고했다. 공식 값0x0800으로 고치고 host·SDK fixture 양쪽에서 독립 기대값을
compile-time 검사한다. 실제 제품 adapter는 처음부터 공식 macro를 사용했다.

CI35419613785의 GCC job105834675834가 시험의 작은 context→큰 배열 인자를
`-Werror=stringop-overread`로 거절했다.65B union backing으로 바꿔 합법적인 크기의
중첩 입력을 시험한다. 경고나 음성 사례를 삭제하지 않았다. 완료 job 로그는
`gh run view --log`가 전체 run 진행 중이라 거절해 jobs API의 logs endpoint로 받았다.
원 로그는 `build/t007-psa-ci-gcc.log`다. 수정 후 GCC O3 strict와 Clang ASan/UBSan
모형은 통과했고 행149/149·분기114/118도 유지했다.

별도 실제 PSA Windows host 실행을 시도했으나 공식 TF-PSA-Crypto의 standalone
CMake는 clang에 MSVC 옵션(/W3,/utf-8,/WX)을 넘겨 실패했다. 같은 고정 Clang의
clang-cl로 바꾼 뒤에는 Espressif port의 `mbedtls/bignum.h`가 필요해 빌드 실패했다.
SDK를 수정하거나 가짜 header/암호를 넣지 않았으며 이 추가 host 실행은 NOT_RUN이다.
실패 로그는 `build/t007-psa-host-sdk{,-cl}-{configure,build}.log`에 보존했다.

실제 ESP-IDF fixture 재빌드는 경고0으로 통과했다.262144B BIN SHA256은
`fadfa94c32694baf02caa5d0e268665b796691aa4e72431e7395fc8768a5f18d`이고
metadata168변이·4절단 시험도 통과했다. `build/t007-psa-post-sdk-build.log`가 근거다.
최종 수정 source의 Windows Debug/Release144/144와 build 경고0도 재확인했다.
로그는 `build/t007-psa-final-{debug,release}-{build,test}.log`다.
이전 golden은 바꾸지 않았다. 원 reviewer 재검토·수정본 CI는 아직 남아 있다.
physical/HIL NOT_RUN·vehicle TX NO-GO·정상 OTA owner 미연결을 유지한다.

## 2026-09-19 (codex, ESP-IDF PSA C 암호 provider)

`firmware/platform/esp32s3/ota_crypto.c`에 공식 PSA API 어댑터를 추가했다.
자체 암호 구현이나 범용 framework 없이 volatile P256 공개키 한 개와 SHA256 operation
한 개를 단일 owner가 관리한다. 실패한 hash setup/update/finish 뒤 reset, abort/destroy 실패
뒤 handle 보존과 재시도를 시험했다. 입력 상한·중첩·NULL과 SDK 호출8단계×오류5종,
각 호출 중 재진입도 검사했다. 모형 암호 결과를 실제 암호 검증으로 표시하지 않는다.

Windows Debug/Release는 각각144/144 통과했다. 로그는
`build/t007-psa-{debug,release}-{build,test}.log`다. WSL Clang ASan/UBSan 모형 시험도
통과했고 `build/t007-psa-sanitize-coverage.log`의 adapter coverage는 함수11/11,
행149/149, 분기114/118(96.61%)이다. 전체 OTA나 target coverage로 확대하지 않는다.

ESP-IDF6.0.3 `idf.py -C tests/fixtures/idf-ota-image -B build/idf-ota-image build`로
실제 ELF/MAP/BIN을 생성했다. `build/t007-psa-sdk-build.log`의 실제 진단은 경고/오류0,
nm으로 adapter와 psa_import_key/verify_message/hash/abort/destroy symbol을 확인했다.
262144B BIN SHA256은 `37c90183bac8c3f7d8a90c1862d3e5ed47c123aa35dfd34e1d7ca606718449b9`다.
합성 descriptor offset288/168개 변이/4개 절단 검사는 통과했고, 이전 signed golden은
그 입력 source에 고정된 artifact이므로 새 unsigned BIN으로 덮어쓰지 않았다.
synthetic HIL fixture의 source digest만 현재 source로 갱신했다. physical evidence가 아니다.

앞선 signed golden 원 A/B 재검토 raw와 통합 report는6209eac에 보존했다.
262bf09의 CI35418535641은6/6 성공했으나 그 artifact 감사는 아직 하지 않았다.
PSA 추가분은 독립 리뷰 전이다. 정상 OTA owner·trusted root·영속 policy 통합은 남아 있고
장치 암호 실행·Flash·physical/HIL은 NOT_RUN, 차량 TX는 NO-GO다.

## 2026-09-19 (codex, signed golden STM 서명 거절 경로 보강)

`bd9a1a6`의 새 golden CTest를 포함해 로컬 Debug/Release는 각각143/143 통과했다.
빌드 로그의 compiler/linker/CMake 경고는0이다. 별도 clean detached checkout에서
public-only golden 검증도 통과했다. 그 시험은 기존 동일 C 소스의 CNG 실행파일을
재사용했으며 새 checkout의 전체 재빌드라고 주장하지 않는다. 시험 뒤 clean 상태를
확인해 임시 worktree만 제거했다. 추적 파일은 commit에서 재생성할 수 있다.

Reviewer A 원문 A-SG-01 P2를 수용했다. 최초 STM 음성 두 건은 내부 SHA256 또는
root hash에서 거절돼 P256 verify 실패를 직접 입증하지 못했다. payload와 내부 SHA256
TLV·whole hash를 갱신하되 원본 서명을 유지한 네 번째 사례를 추가했다. native probe
호출 횟수도 정상3·내부 hash 실패2·root 실패0·P256 실패3으로 대조한다.
실제 C 소스의 verify 반환을 무시해 OK로 바꾼 임시 변이 실행파일을 경고0으로 빌드했고
시험이 `[0,12,12,0] != [0,12,12,12]`로 실패했다. 실제 repository verifier는 바꾸지 않았다.
변이 로그 `build/t007-golden-mutant-test.log` SHA256은
`71351ea9c7637624b0cd917ecf94a9fea602cf28d2afd52cc5f3ae4667a5c4b8`다.

Host 초기 환경 로드는 현재 worktree의 도구 cache를 중복 추출하기 시작해 해당 두
실행만 중단했다. 검증된 `t104-stm32-uart-control`의 고정 도구 환경을 재사용했다.
기본 checkout의 사용자 파일은 변경하지 않았다. physical/HIL NOT_RUN·차량 TX NO-GO를 유지한다.

## 2026-09-19 (codex, 공식 native 서명 합성 golden)

공식 espsecure5.4.0과 MCUboot imgtool v2.4.0을 재사용했다. 실제 ESP-IDF 합성
BIN에 RSA3072/PSS 서명을 추가하고 별도의 STM 합성 payload/P256·outer manifest/P256를
하나의394310B 컨테이너로 조립했다. SHA256은
`68eb18e10bf35d351c1604500bf85f6e95aa41c6b49477ffbbdacd9477902655`다.
세 시험 개인키는 메모리 전용이며 공개키·컨테이너·provenance만 보존했다.
무작위 서명 재생성이 아니라 보존된 서명/image로 조립한 byte 일치를 재현 gate로 사용한다.

첫 생성 시 ESP-IDF Python에 cbor2가 없어 실패했다. 제품 SDK 환경을 바꾸지 않고 기존
OTA 시험 venv의 lock된 의존성에 esptool5.4.0을 추가해 생성했다. pip show의 CP949
출력 오류는 도구 정보 출력 문제이며 이후 Python은 UTF-8로 실행했다.
첫 CNG STM golden 시험은 시험 wire의 signed_size/hash/signature를0으로 전달해
INVALID_ARGUMENT를 반환했다. 기존 probe의 입력 계약대로 실제 값을 전달한 뒤
CNG 수신12건·native STM3건과 공식 imgtool 검증, ESP host RSA10건이 통과했다.
firmware 검사를 완화하거나 실패를 성공으로 계산하지 않았다.

정상 OTA owner·영속 policy·제품 signing CLI/target 통합은 남아 있다.
장치 native 실행·Flash·physical/HIL은 NOT_RUN, 차량 TX는 NO-GO다.

## 2026-09-19 (codex, SDK metadata와 prefix checkpoint artifact 감사)

`624848a`에서 C const custom descriptor를 실제 ESP-IDF6.0.3 시험 image에 넣었다.
자동 서명/키/provisioning/Flash 동작 없이 ELF/MAP/BIN 경고0을 확인했고
`tests/ota/check_sdk_metadata.py`로 offset288의168B·u64최대값과168개 byte 변이·4개
절단 거절을 확인했다. BIN SHA256은 `e9dc9177c696a13bdba0631c1da2b3ec125b688862ab2c04921c05c0675d3735`,
로그는 로컬 `build/t007-sdk-metadata-build.log`다. 이는 합성 descriptor 위치/값
증거이며 native 서명·정상 device firmware·boot·HIL 성공이 아니다.

앞선63c8727의 CI35416056122는6/6 성공했다. 내려받은 target21개 ELF/MAP/BIN의
bytes/SHA256을 manifest와 대조했고 source7개도 해당 Git object의 Windows CRLF
byte열과 일치했다. Manifest SHA256은
`5a72e85918d474e679b1eb5fff255fef4a24bdd6aa225658a0cdacaa6fdb21d4`다.
26개 target log에 실제 warning/error 진단은0개다. 최초 넓은 `error` 검색은 SDK의
`error.c.obj` 컴파일 행5개를 검출했다. 내용을 읽어 source filename임을 확인했고
warning 전체·error 진단·CMake/fatal/FAILED 패턴으로 재검사했다. 진단을 억제하지 않았다.
Windows CI Debug/Release LastTest.log도 각각142개 성공·실패0·CNG90교차를 확인했다.
이전63c8727 검증을 이후metadata/새HEAD 검증으로 확대하지 않는다. 이후21cb047의
CI35416950979는 진행 중이다. 전체 T-007·native signing/golden·정상 owner 연결은 남아 있다.

## 2026-09-19 (codex, T-007 prefix·packager checkpoint)

`63c87272fe9f2b00b76893055bfcc8a9ca71cf26`을 Draft PR36에 push했다. C99 고정 buffer
prefix 조립과 host detached P256 서명 조립/검사만 추가했으며 native 설치 승인은 아니다.
기존 SDK/cryptography와 parser를 재사용한다. 실제 CNG 교차90개(collector 경로45개),
Windows Debug/Release142/142 및 Linux 독립 checkout ASan/UBSan137/137이 통과했다.
최초 Debug141/142는 합성 T103 fixture digest mismatch였으며 실제 계산한 source digest
`8e128a30cfe2b153bcd0e1f17267078fe22dc12c2c4cbe374c85018bab325fe1`로 fixture5행과
helper 기대값만 갱신했다. 실제 capture evidence는 변경하지 않았다.
Release 최초 재실행은 실행 중 HEAD 변경에 따른 identity mismatch, WSL 최초 실행은
Windows worktree의 `.git` 경로 해석 실패로 완료 gate를 통과하지 못했다. 고정 commit과
Linux native Git checkout으로 재실행해 위 결과를 확인했다. 검사 기준은 완화하지 않았다.

로그는 로컬 `build/t007-packager-debug.log`, `build/t007-packager-release-rerun.log`,
`build/t007-packager-sanitize-nativegit.log`에 보존했다. 새 collector coverage의
prefix_feed 행93.33%·분기96%, init/finish100%를 확인했다. 최초 llvm-cov 호출은
PowerShell 인자 분리로 실패했고 `-instr-profile=...` 전체를 인용한 재실행이 성공했다.
전체 envelope.c coverage나 MCU timing 성공으로 집계하지 않는다.
CI35416056122의 host5개 job은 통과했고 target job은 진행 중이다.
독립 A 정적 PASS/B CONDITIONAL의 원문을 보존했다. B의 P3 두 표현을 수정했으며
원 reviewer 재확인 전이다. 전체 T-007 IN_PROGRESS·physical/HIL NOT_RUN·차량 TX NO-GO다.

## 2026-09-19 (codex, 명시적 재개와 T-007 후속 구현)

사용자가 "이어서 완주까지 진행"을 요청했다. PR35 MERGED, 열린 PR 없음과
origin/main `6cf1b8e57840a27b83c407d1325a92f869cf2f5d`를 Git/GitHub로 확인했다.
기본 checkout은 기존 branch/사용자 파일을 보존했다. clean T-007 worktree에서
`codex/t007-ota-packager`를 origin/main 기준으로 만들고 로컬 SDK/evidence를 재사용한다.
이전 source 재구현 없이 signed package 작성·검사와 C 수신 연결부터 진행한다.
물리/HIL NOT_RUN, 차량 CAN TX NO-GO, 전체 T-007 IN_PROGRESS를 유지한다.

## 2026-09-15 (codex, 현재 PR merge 후 일시중지 범위 확정)

사용자의 "지금 작업까지만 머지하고 작업 일시중지", "완료시키고 머지 후 일시중지"를
현재 PR #35 구현분의 검증·merge 요청으로 적용한다. 앞선 범위 질문을 이유로 계속
대기하지 않는다. 전체 T-007 수용 기준을 축소하거나 완료로 바꾸지 않으며, 남은
packager/정상 OTA 통합과 다음 task는 재개 요청 전 시작하지 않는다.
review 원본은 그대로 보존하고 현재 상태/후속 작업 안내만 갱신했다.
현재 target CI 및 review 기록 반영 뒤 CI와 artifact 감사를 마친 다음 merge한다.
physical/HIL NOT_RUN, 차량 CAN TX NO-GO는 유지한다.

## 2026-09-15 (codex, 현재 구현분 post-fix 독립 리뷰 closure)

`63197e3`를 같은 reviewer2명에게 object-only로 재검토시켰다. 요청·submission ID·시각과
두 원본을 [post-fix 기록](reviews/adversarial/2026-09-15-T-007-current-post.md)에 연결했다.
A는 정적 PASS, B는 CI/artifact 조건부 CONDITIONAL이며 네 finding은 모두 FIXED,
새 P0/P1/P2/P3는 없다. B의 CONDITIONAL을 PASS로 변경하지 않았다. 양 원본 보존 후
결과를 종합하고 두 reviewer 실행을 종료했다. 이번 변경은 review/상태 기록뿐이다.

CI34908276012는23:25 UTC 확인 시5/6 성공이고 target은 실행 중이었다. Windows artifact를
내려받아 Debug/Release140 Passed/0 Failed, body mock1702/CNG1708, host-sim12 PASS와
commit/source identity를 확인했다. 실제 MCUboot CNG 시험은 이번 실행2283건이며 DER 길이에
따라 건수가 달라진다. 정확한 log digest와 scope는 post-fix 기록에 있다. Node/Git 경고와
예상 argparse 음성시험 출력을 포함한 전체 CI를 warning0으로 표시하지 않는다.

현재 source 결함 리뷰는 닫혔지만 target artifact/source/hash 감사와 기록 반영 후 CI,
사용자의 완료·merge 범위 답변은 남아 있다. T-007 전체 acceptance를 완료로 바꾸지 않았고
추가 기능/다음 task는 시작하지 않는다. physical/HIL NOT_RUN, 차량 CAN TX NO-GO다.

## 2026-09-15 (codex, 현재 구현분 reviewer finding 수정)

`21909e5` 기준 A/B 원본을 모두 보존했다. 각각 CONDITIONAL이며 P0/P1 없음,
A-01/B-01 P2와 A-02/B-02 공통 문서 P3가 있었다. 상세 원문과 disposition은
[통합 리뷰](reviews/adversarial/2026-09-15-T-007-current.md)에 있다. 추가 기능은 구현하지 않았다.

- A-01: 이미 FAILED인 body의 reset 재시도는 최초 error를 유지하고 cleanup 오류만 별도로
  전달한다. 새 scenario9는 최초 AUTH_FAILED/INCOMPLETE 뒤 cleanup이 추가로2번 실패한 뒤
  성공하는 경로를 역할3종에서 검사한다. 수정 전 모형 시험의 exit1을 실제 확인했다.
- B-01: staging data_offset64KiB 정렬을 generator에서 검사한다. Flash 끝은 보존하면서
  offset을4KiB씩15가지 이동한 회귀가 수정 전 모두 실패했고 수정 후 통과했다.
  현재 board 설정과 generated13개 출력은 변경하지 않았다.
- A-02/B-02: 기존 검사기/adapter 구현과 미완료 owner·signed packager·target 통합을
  README에서 구분했다. 새 계층/SDK/의존성 없이 기존 함수와 검사 조건만 수정했다.
- `python -B tests/ota/test_body.py build/host-debug/canview-ota-body-probe.exe` 모형1702건,
  crypto probe `--crypto` 실제 P256/SHA-2561708건. generator11개 시험 통과.
- WSL Clang ASan/UBSan 모형1702건 통과. 새 build/ota-review-fix-body.profraw/.profdata로
  body.c 함수10/10·행220/220·분기94/94 확인. core와 body_probe를 현재 source에서 직접
  `-fsanitize=address,undefined -fprofile-instr-generate -fcoverage-mapping`으로 빌드했다.
- Windows strict 전체 Debug140/140(30.81s), Release140/140(24.99s). build compiler/linker/CMake
  warning/error0. 로그는 build/ota-review-fix-{debug,release}-{build,test}.log다.
- 문서331개/링크1363개·plan49 오류0, board generation check 통과.
  firmware source SHA256 `639bfe1c01453de483e2dd3dbc9b6540c9064e3433526a053a8360f2d2cec1e3`.
  T103 합성 fixture identity만 동기화했으며 physical evidence는 바꾸지 않았다.

직전 candidate의 CI34906746695는6/6 성공했다. 이후 수정 source의 CI/target artifact
감사는 별도이며 원 reviewer post-fix 재확인도 남아 있다. 현재 구현분 merge와 T-007 전체
완료를 구분한다. 범위 질문은 답변 대기, physical/HIL NOT_RUN, 차량 TX NO-GO를 유지한다.

## 2026-09-15 (codex, 현재 구현분 closure 준비·사용자 일시중지 요청)

사용자는 현재 작업 완료·merge 뒤 일시중지를 요청했다. T-007 전체 완성인지 현재
구현분 merge인지 질문했으며 아직 답변을 받지 않았다. 새 packager 구현은 시작하지
않았다. 두 경우 공통인 독립 review와 현재 CI 확인만 진행하며 다음 task는 시작하지 않는다.

- source candidate `21909e586d8fc9e7ebf8d0a4aacd7037e1d7f962`, base `d229772de77a48ae197e2ff1b4e55b6cef9a88ed`.
- 2026-09-14 23:03:32 UTC 새 A/B 요청. execution ID와 공통 manifest는
  [A 실행 기록](reviews/adversarial/evidence/2026-09-15-T-007-current-reviewer-a.md),
  [B 실행 기록](reviews/adversarial/evidence/2026-09-15-T-007-current-reviewer-b.md)에 보존했다.
  object-only 독립 검토 중이며 원본 verdict는 미수신이다. 60초 관찰 timeout 뒤에도 재시작하지 않았다.
- CI `34906746695`: Windows host/GCC/Clang/sanitizer/browser 성공, target 실행 중.
  `gh run download 34906746695 --name windows-foundation-evidence --dir build/ci-34906746695-windows`로
  받은 LastTest.log에서 Debug/Release 각각140 Passed/0 Failed, coverage 실행9 Passed/0 Failed 확인.
  Debug log SHA256 `2d82973bb2fb2e98ec469c189a573733c3d444997b38f49803043e16935d8d9d`,
  Release log SHA256 `0fd049f44aacb6a115a631d4a5bd8ac589ceac70ccde0a2f58d25b0a45c37767`.
- 내려받은 host-sim report는 candidate/source hash와 일치하며
  `python -B tests/hil/validate_evidence.py build/ci-34906746695-windows/hil-host --expect-status PASS`
  12 scenario PASS. physical/HIL은 report 자체도 NOT_RUN이다.
- job `104185088872`의 실제 로그에서 ASan/UBSan136/136과 OTA 모형 실행을 확인했다.
  Windows CNG 4개 시험을 Linux에서 실행한 것으로 집계하지 않는다.
- Windows job `104185089154`의 로그 진단 검색6건은 Node deprecation4건, annotated Git tag
  경고1건, argparse 필수 인자 누락을 의도한 음성 시험 error1건이다. 별도로 runner의
  Node20→24 경고 annotation도 있다. Git은 바로 다음 줄에서 고정 MCUboot commit으로
  checkout했고 이후 실제 imgtool/CNG 시험이 통과했다. compiler/linker/CMake 경고와
  구분하며 전체 CI warning0을 주장하지 않는다. 로그는 build/ci-34906746695-windows-job.log와
  build/ci-34906746695-sanitizer-job.log에 있다.

PR35는 Draft이며 merge하지 않았다. target artifact/hash 대조와 원본 review/disposition은
남아 있다. 차량 TX NO-GO, physical/HIL NOT_RUN과 미완료 T-007 수용 기준을 유지한다.

## 2026-09-15 (codex, OTA schema와 서명 전 manifest 작성 도구)

직전 턴은 BSP 구현·push progress다. `7b36478` clean에서 시작했고 CI `34905471810`은
이번 재확인까지 in_progress였다. 동일 실행을 재시작하지 않았다. T-007은 IN_PROGRESS다.

T-007 수용 범위와 기존 C/Python을 다시 대조했다. 정상 OTA app/owner 확대 전에 누락된
schema와 packager 입력을 완성하는 순서로 조정했다. 기존 byte 계약을 바꾸지 않고
schema/cvota-v2.schema.json과 protocol/schema/ota-container-v2.yaml에 기록했다.
JSON 이름→CBOR integer key는 schema의 x-cbor-key를 재사용한다. JSON 표준 parser와
기존 typed validator를 쓰며 범용 JSON Schema 엔진/새 암호 구현을 만들지 않았다.

tools/ota/manifest_json.py는 최대32KiB UTF-8 JSON을 검증하고 서명 전 CBOR만 생성한다.
duplicate/unknown field, float/NaN, 잘린 입력, 잘못된 hex/type/range/ABI/target 조합을
거부한다. u64를 Python 정수로 보존하며 출력 파일은 exclusive create로 기존 파일을
덮어쓰지 않는다. 실제 signing/native image 검증과 전체 cvota packager는 아직 남아 있다.
UNSIGNED_MANIFEST 출력에 header·dummy signature·image bytes를 넣지 않는다.

- 최초 직접 시험에서 테스트의 read_text가 Windows cp949를 사용해 한글 schema 읽기에
  실패했다. UTF-8을 명시하고 같은 직접 명령과 CTest를 재실행해 통과했다.
- `python -B tests/ota/test_manifest_json.py build/host-debug/canview-ota-manifest-probe.exe -v`
  4개 unittest 통과. 각 JSON prefix 절단, role3종×sequence6종의 C 대조, schema
  매핑/enum/header/native layout 상수와 CLI 기존 출력 보존·실패 입력 비노출을 확인했다.
- 로컬 기존 jsonschema4.26.0의 Draft202012Validator.check_schema, 역할3종 positive,
  newline/role/signature negative 통과. runtime/CI dependency를 추가하지 않았다.
  새 CTest는 mapping drift와 기존 Python/C validator 대조이며 metaschema 엔진 실행은 아니다.
- Windows strict build warning/error0, Debug140/140(30.64s), Release140/140(24.27s).
  build/ota-schema-debug.log, build/ota-schema-release.log와 각 -build.log에 기록했다.
- 문서328개/링크1353개·plan49개 오류0, generated board check 통과. firmware source SHA256
  `33e89e01de8b93e58ec5aac7580fa50ab97c9e2743a5611e88296bf283608ae6`.
  protocol schema/README 추가를 반영해 T103 합성 fixture만 동기화했다.

문서화 스킬에 따라 schema/도구 책임과 검증 한계를 기존 README/OTA 정본에 연결했다.
이번 변경에 firmware C/SDK 경로 수정은 없으며 target/sanitizer를 새로 실행한 것으로
집계하지 않는다. 이후 전체 target CI·native signing/golden·prefix 수신/owner/영속 policy와
독립2인 리뷰가 남았다. physical/HIL은 NOT_RUN, 차량 TX는 NO-GO다. 사용자 파일은 보존했다.

## 2026-09-15 (codex, Communicator OTA BSP 읽기 검증 연결)

직전 턴은 구현·push progress다. `ba10e46` clean에서 시작했다. CI `34904547650`은
재확인 시 in_progress이며 그 부모 `69508bf`의 `34903724621` 성공을 확인했다.
관찰 대기만으로 CI를 재시작하지 않았다. T-007/PR35는 IN_PROGRESS/Draft다.

Communicator BSP `ota.c`는 외부 partition pointer/주소를 받지 않고 고정 bundle_stage를
찾는다. generated board ID/위치/크기, COMM_ESP target과 서명 scheme, prefix 뒤 정렬된
offset을 검사한 다음 기존 SDK 전체 hash/native 서명과 공통 metadata 대조를 호출한다.
정상 OTA task가 호출을 직렬화하고 Flash 불변을 보장해야 하며 현재 이 owner 연결은 없다.
새 mutex/상태기계/writer/부팅 선택은 만들지 않았다. 구조/C 스타일 스킬은 SDK 경계에,
문서화 스킬은 기존 SDK README의 신뢰 입력·수명·미완료 범위 갱신에 적용했다.

- generator에 staging 위치/크기의 BSP macro와 encrypted flag를 연결했다. 최초 공식
  SDK partition parser 실행에서 기존 recovery label이 SDK subtype 이름과 겹치는
  WARNING을 발견했다. 세 template의 label만 recovery_app으로 바꾼 뒤 다시 실행해
  경고0을 확인했다. 주소/크기/app-test subtype과 factory-only sdkconfig는 불변이다.
- `python C:/cv/esp-idf-6.0.3/components/partition_table/gen_esp32part.py --offset 0x18000
  --flash-size 16MB firmware/communicator/esp32/partitions.ota-template.csv build/ota-communicator-esp32-partitions.bin`
  및 Controller16MB/Bridge8MB도 성공. CI의 같은 SDK parser/warning scan에 세 검사를 추가했다.
  partition BIN은 생성만 했으며 flash/migration하지 않았다.
- Windows Debug139/139(34.93s), Release139/139(28.78s). 로그는
  build/ota-comm-bsp-debug.log와 build/ota-comm-bsp-release.log다. generator10개 시험도 통과했다.
- BSP/SDK/metadata/STM 관련9개 CTest 통과. BSP 모형은 실제 metadata 코드를 실행하고
  SDK만 대체한다. NULL·wrong board/role/target/scheme·offset·partition 위치/크기/type·
  SDK 오류·전체168B 변조·version 불일치를 시험했다. 모형은 실제 RSA 검증이 아니다.
- WSL Clang21 ASan/UBSan 통과, BSP 함수1/1·행29/29·분기32/32·region66/66.
  build/ota-comm-bsp.profdata와 build/ota-comm-bsp-asan에 해당한다.
- 실제 SDK fixture에서 BSP/SDK/metadata 심볼을 nm으로 확인했다. build/ota-comm-bsp-idf.log
  compiler/linker/CMake warning/error0. unsigned fixture BIN262144B SHA256
  `df141c306db9b3b5d60b0f73ad4d6d3fe8ec5af3cfff724475b5889640b934a5`,
  ELF4155012B `63e4d15b3cb5f2f6743a66b03daf484f04b110c24d9f263c3e7da7003357fcf4`,
  MAP3025942B `7ea16451eb3db1a0233b41cdda25398bc84af8c75a340de1ee807dae3f5a6ec4`.
- generated board drift 없음, 문서328개/링크1351개·plan49개 오류0. firmware source SHA256
  `421f40a151ec0de52f6d9ad928deac78a4f9c8b9dd148632c58a983a23c71fd1`.
  T103 합성 fixture만 동기화했다. 생성기로 생긴 내용 동일 line-ending 변경은 Git 정규화한다.

다음은 body 완료→BSP 검사를 단일 OTA owner에 연결하고 실제 signed descriptor 생성과
schema/CLI/golden을 완성하는 작업이다. 정상 target 통합·영속 policy·독립2인 리뷰는 남아
있다. physical/HIL·장치 RSA 실행은 NOT_RUN, 차량 TX는 NO-GO다. 사용자 파일을 보존했다.

## 2026-09-15 (codex, STM/ESP native metadata 공통 대조)

직전 턴은 구현·push progress였다. `69508bf` clean에서 시작했으며 부모 CI
`34903724621`은 이번 검증 중 재확인까지 in_progress다. 실행 중인 CI를 재시작하지 않았다.

기존 STM `CVIMG001`168B 대조를 `shared/ota/src/native_metadata.c`로 옮겼다.
ESP에도 같은 함수를 사용하고 SDK `app.version[32]`의 정확한 문자열 대조만 더했다.
role/target/signature 조합을 명시적으로 거부하며 epoch와 u64 sequence를 서로 다른
필드로 유지한다. SDK `secure_version`을 release_sequence로 변환하지 않는다.
새 ESP parser/암호/범용 abstraction을 추가하지 않았다. C 스타일·구조 스킬은
무상태/무힙과 SDK 경계를, 문서화 스킬은 기존 README의 입력 수명·검증 책임 갱신에 적용했다.

- Windows Debug138/138(29.38s), Release138/138(24.61s). 기존 공식 MCUboot imgtool/CNG
  STM 교차 시험을 포함한다. `build/ota-metadata-debug.log`, `build/ota-metadata-release.log`.
- 새 CTest는 NULL·길이0..169/SIZE_MAX·전체168byte 각8bit 변이·role/target/signature
  조합·u64 경계·문자열 최대/미종단/비ASCII/zero padding 불일치를 검사했다.
- WSL Clang21 strict C99 ASan/UBSan 통과. `native_metadata.c` 함수6/6·행60/60·
  분기76/76·region108/108. `build/ota-metadata.profdata`, `build/ota-metadata-asan`.
- 실제 ESP-IDF6.0.3 fixture에서 SDK와 공통 검사기를 compile/link했다. SDK version32B와
  custom168B의 compile-time drift assertion을 추가했다. `nm`에 공통 두 함수와
  adapter/SDK verifier가 존재한다. fixture의 NULL negative 외 실제 호출은 미실행이다.
- `build/ota-metadata-idf.log`의 compiler/linker/CMake warning/error0. unsigned fixture
  BIN262144B SHA256 `83b8dbc6d34f346fb8f4c7a9abc5a9fedb03314e1e47f152641f55efb316e2ee`,
  ELF4150236B `22f12bf0c3f628800a0e1ad0fbf97ba0a9362a274c1ba260500d6dae95b52c17`,
  MAP3022892B `95c84d2f6d48e0bcee87523e5acfc1155fc975c2545e682ad6b50e3191cf4c22`.
- generated board check 통과, 문서328개/링크1351개·plan49개 오류0.
- firmware source SHA256 `5cb77580ff911e2bedf542d2f94e734365b5a481e6aec6916a797504db22afaf`.
  T103 합성 fixture만 동기화했다. 사용자 파일·physical evidence는 변경하지 않았다.

다음은 SDK→metadata의 BSP/body provider 연결과 owner/Flash 불변 보장이다.
실제 descriptor 생성·signed golden·schema/CLI, 영속 policy/정상 target 통합과 최종
독립2인 리뷰는 남아 있다. Task/PR은 IN_PROGRESS/Draft다. physical/HIL·장치 RSA
실행은 NOT_RUN, 차량 TX는 NO-GO이며 writer/boot selector/provisioning은 실행하지 않았다.

## 2026-09-15 (codex, ESP native SDK read-only adapter 실제 빌드)

`8749529` clean에서 시작했다. 앞선 정렬 구현 턴은 progress이며 동일 CI
`34901848402`는 작업 시작 시5개 job 성공/target-firmware-windows 실행 중이었고,
커밋 직전 재확인에서6개 job 모두 성공했다. 관찰 timeout으로 재시작하지 않았다.
이 결과는 `8749529`에만 해당한다. T-007/PR #35는 계속 IN_PROGRESS/Draft다.

`firmware/platform/esp32s3/ota_image.c`는 BSP가 준 실제 partition의 내부 Flash·범위·
정렬·signature sector 길이를 검사한다. SDK의 DATA hash 모드로 signature sector까지
전체 file hash를 대조하고 `esp_image_verify()` 뒤 app/custom descriptor를 읽는다.
SDK를 재사용하므로 자체 SHA/RSA/ESP image parser와 Flash writer를 추가하지 않았다.
signature 비활성·0값·다른 scheme·FPGA 및 bootloader alias는 fail-closed다.
실제 Flash 암호화가 켜졌는데 staging이 암호화되지 않았으면 거절한다.

문서화 스킬의 경계·소유권 계약을 [SDK fixture README](../tests/fixtures/idf-ota-image/README.md)에
기록했다. SDK 타입은 platform/BSP에만 있다. 단일 task/Flash 불변은 caller 계약이며
아직 정상 app orchestration에 연결되지 않았다. raw signed metadata와 CANView 정책
대조·body provider·generated staging 암호화 flag 확인은 다음 작업이다.

- SDK compile/link fixture의 실제 ELF/MAP/BIN 생성. BIN262144B,
  ELF4141676B, MAP3019229B. `nm`으로 adapter, 전체 file hash, image verifier와
  `esp_secure_boot_verify_sbv2_signature_block` 심볼을 확인했다.
- `CONFIG_SECURE_SIGNED_ON_UPDATE=y`/RSA scheme의 실제 sdkconfig를 확인했다.
  자동 signing은 꺼져 있으므로 이 BIN은 서명된 CANView 배포 이미지가 아니다.
- 최초 SDK 빌드에서 deprecated `esp_flash_encryption_enabled()` 경고를 발견했다.
  `esp_efuse_is_flash_encryption_enabled()`로 교체하고 재빌드 log의
  compiler/linker/CMake warning/error0을 확인했다. 경고를 억제하지 않았다.
- 최초 `-B ../../../build/idf-ota-image`는 기대와 달리 `F:/build/idf-ota-image`에
  생성됐다. 이후 현재 checkout의 절대 `build/idf-ota-image` 경로로 다시 빌드했다.
  앞선 디렉터리는 삭제/재사용하지 않았으며 아래 증거는 checkout 안의 후속 빌드다.
- Host unsigned 시험의 사용하지 않는 `external_chip` 변수로 strict compile이
  실패했다. 해당 변수의 선언을 실제 사용하는 signed 시험 scope로 옮긴 뒤 재검증했다.
- Windows Debug137/137(28.42s), Release137/137(23.91s). 로그는
  `build/ota-esp-sdk-debug.log`, `build/ota-esp-sdk-release.log`다. 새6개 SDK 설정
  모형은 실제 adapter의 bounds·hash/SDK/read 실패·출력0·호출 순서를 검사한다.
- WSL Clang21 signed SDK 모형 ASan/UBSan 통과. adapter 함수1/1·행54/54·분기70/70.
  `build/ota-esp-sdk.profdata`는 해당 모형 실행이며 실제 RSA 실행 증거가 아니다.
- SDK 로그 `build/ota-esp-idf-build-fixed.log`, BIN SHA-256
  `ac04f8f05241f0d7caf1aaa968a7517ce3007929abacdf027fdf89a2dea7b6df`,
  ELF `b639a8736dd1f67ca8a8b5ba8c24602a7dde776d9c173f73964bbf3c259635da`,
  MAP `7acdd68d9f7f41864bcc685ce98e3bbcfb8ffafb5a8589bd9c702b4874af1e14`.
  local dirty source의 compile fixture이며 CI/production artifact로 재사용하지 않는다.
- firmware source SHA-256은
  `f6652e2099743ab2431633ceb308bbd12605015d175b4d25f55dde6f34e2d468`.
  T103 합성 fixture 식별자만 동기화했다. physical evidence를 수정하지 않았다.
- CI에 실제 SDK fixture build·서명 설정 확인·ELF/MAP/BIN manifest/hash·로그 warning
  검사를 연결했다. 기존 target18개에 probe3개를 더 보존한다. 이 새 CI는 아직 미실행이다.

SDK API는 실제 빌드했으나 board에서 signature를 실행하지 않았다. physical/HIL,
Flash/boot selector/provisioning은 NOT_RUN, 차량 TX는 NO-GO다. 최종 독립2인 review,
정식 schema/CLI/golden, metadata 정책·정상 target integration은 미완료로 남긴다.

## 2026-09-15 (codex, OTA revision2 정렬과 순차 padding 검사)

`4d99222` clean worktree에서 시작했다. PR #35는 OPEN/Draft이며 그 기준선의
CI `34731902196` SUCCESS를 확인했다. 이전 턴은 실제 SDK/BIN 정렬 제약을
확인한 progress였고, 이번에는 C/Python 구현과 회귀시험으로 반영했다.

[ADR-009](adr/009-ota-native-image-alignment.md)에 따라 `CVOTA002`/header2/signed
manifest2로 구분한다. 작은 prefix 이후 각 native image를64KiB에 정렬하며 빈 공간은
0만 허용한다. C body는 prefix 실제 길이부터 chunk를 읽어 padding을 검사하고,
image hash에서는 제외한다.64KiB RAM buffer·별도 ESP parser·Flash writer는 없다.
native metadata/Flash layout version은 바꾸지 않았다. AGENTS의 단순화 원칙을 유지한다.

기존 payload 모든 byte 변이/절단, 단일·역순·최대 image, provider/cleanup/reentry
시험을 유지했다. padding 양 끝과4KiB 경계±1,64KiB 길이±1, 구버전/미래 버전,
완료 뒤 추가 byte를 보강했다. Python assembler 결과를 독립 padding fixture와
대조하고 같은 byte열을 C stream에 넣었다. padding 전체에 대한 전수 변이로
표시하지 않는다. 마지막 추가 시험은 probe의 scenario 상한이7인 탓에 중단됐고
Debug129/131과 ASan probe exit1을 확인했다. 새 scenario8을 명시적으로 허용한 뒤
아래 전체 검증을 재실행했다. 해당 실패를 firmware 결함이나 PASS로 기록하지 않는다.

- Windows strict build 뒤 Host Debug131/131(37.14s), Release131/131(24.64s).
  로그: `build/ota-alignment-debug.log`, `build/ota-alignment-release.log`.
- body 모형1696건, Windows 실제 P256+SHA-2561702건. typed manifest1437/1440건,
  실제 P256 prefix232건, CBOR11989건, floor3847건과 STM native CNG2285건도 통과.
- WSL clang21 ASan/UBSan으로 현재 body1696건, typed manifest1437건과 envelope
  경계 C 시험 통과. `build/ota-alignment.profdata`는 이 세 실행의 profile만 합쳤다.
  body.c: 함수10/10·행219/219·분기92/92. envelope.c: 함수3/3·행80/89·분기32/38.
  manifest.c: 함수16/16·행377/402·분기216/240. 이전 profile 수치를 합산하지 않았다.
- Cortex-M4 GCC15.3 freestanding/strict object compile 통과. body의 보고된 frame은
  open40B/feed56B, helper8~16B이며 전체 call-chain/SDK stack 측정은 아니다.
- `python -B tools/generate_boards.py --check`, document links328문서/1347target,
  plan49task 검증 통과. 전체 host에는 SDK config negative와 generated drift도 포함된다.
- Doxygen1.18/Sphinx9.1 strict build 통과. 추출71개는 기존 public API 범위이며
  private OTA header까지 자동 문서화했다고 주장하지 않는다.
- firmware source SHA-256은
  `18263a9cc9035e7b5601cd3532df3e934c75e97b7172a88e3bbc00a9ae42f5bb`다.
  T103의 합성 JSONL5행과 합성 helper 기대 hash만 동기화했다. 물리 evidence가 아니다.

새 source의 CI와 최종 target 통합·artifact/hash/warning 감사, T-007 최종 독립2인
리뷰는 남아 있다. 다음은 정렬된 staging에서 SDK native verifier·signed metadata를
연결하는 작업이다. 실제 eFuse/Flash/board/HIL은 NOT_RUN, 차량 CAN TX는 NO-GO다.
T-007/PR을 완료·ready·merge하지 않았으며 다른 worktree의 사용자 변경을 보존했다.

## 2026-09-13 (codex, ESP native SDK 재사용의 정렬 제약 확인)

`5b3a711` clean 기준에서 ESP-IDF6.0.3 source와 실제 CI image를 조사했다.
독립 read-only 조사 agent `01a09874-f824-77e2-8ea6-866a6f4cf962`(Pasteur)도
같은 결론을 반환했다. 이는 설계 조사이며 hostile review/PASS가 아니다.

확인한 제약:

- `components/bootloader_support/src/esp_image_format.c:1179` 부근에서 서명 끝을
  **절대 Flash 주소**의4KiB로 올림한다. 임의 offset에 둔 compact image를 그대로
  `esp_image_verify()`에 넘기면 image-relative 서명 배치와 다를 수 있다.
- 같은 파일916행 부근은 mapped segment의 Flash/load 주소를 MMU page로 대조한다.
  `components/soc/Kconfig:25`와 현재8/16MiB board 기준은64KiB다. 따라서4KiB
  정렬만으로는 부족하며, SDK 전체 verifier 재사용에는 image 시작64KiB 정렬이 필요하다.
- `bootloader_flash/src/bootloader_flash.c:53`의 앱용 mmap handle은 전역 하나다.
  OTA/boot 검증 관련 SDK 호출을 한 owner가 직렬화하고 중첩 호출하지 않아야 한다.
- `esp_image_format.c:43`의 `CONFIG_SECURE_SIGNED_ON_UPDATE` 조건을 확인했다.
  서명 검증이 꺼진 bench 빌드를 native signature 성공으로 처리하면 안 된다.

CI34730820401(c0de352)는6/6 성공을 확인했다. `gh run download 34730820401
-n target-firmware-images -D build/ci-34730820401-images`로 실제 산출물을 받았다.
IDF export 후 esptool5.4.0의 `LoadFirmwareImage('esp32s3', path)`로 Communicator,
Controller, Bridge BIN을 각각 읽었다. mapped segment 두 개씩에서 SDK의
`(base + segment.file_offs + 8) % 65536 == segment.addr % 65536` 조건을 재현했다.
세 이미지 모두 base0xA40000/0xA50000은 일치하고0xA41000은 불일치했다.
이는 실제 BIN으로 주소 조건을 재현한 것이며 장치의 verifier/서명 실행 시험은 아니다.

조사에 사용한 BIN SHA256:

- Communicator: `909350e56679dcce4d33cd3285e89f3e007d18a814536b9e499c669fb24b889a`
- Controller: `588705269905bb3ca564eac4f6fb7ee68d9160a97125bcbae5c3b22ffa26667a`
- Bridge: `7cfb7bfbb47d4b23b2cc9b5c8a7bae5549a4130fbb917c2e6b0975ce1fcaa6fd`

단순한 대안은 prefix 뒤/각 image 앞에 canonical zero padding을 두고64KiB 정렬된
bundle을 staging에 그대로 저장하는 방식이다. 임의 주소 필드나 별도 ESP parser,
논리/물리 offset 변환 계층이 필요 없다. 마지막 image 뒤 padding은 불필요하다.
`boards.json`의 staging base0xA40000은 이미64KiB 정렬이다. 최대4MiB ESP와180KiB
STM을 양쪽 순서로 계산하면 bundle은4444160/4456448B이며 staging4718592B에 들어간다.
다만 padding은16KiB 이하 chunk로 검사해야 하며64KiB prefix buffer를 만들면 안 된다.

이 정렬은 현재 compact 후보의 wire 해석을 바꾼다. 이번 조사에서는 parser/서명 byte
계약을 조용히 바꾸지 않았다. 다음 구현에서 포맷 revision/정식 schema와 정렬 규칙을
명시하고 C/Python·padding 변이·streaming·SDK 통합을 함께 검증해야 한다.
기존 v1 체크포인트 결과를 새 배치의 검증으로 재사용하지 않는다.
T-007/최종 2인 리뷰는 미완료, physical/HIL NOT_RUN, 차량 CAN TX NO-GO다.

## 2026-09-13 (codex, OTA version floor와 동일 이미지 복구 사전 판정)

`floor.c/h`에 OTA §7.1의 순수 C99 비교를 구현하고 body open에서 호환성 검사 뒤,
첫 hash 시작 전에 필수 호출한다. uint64 하한 미만은 STALE, 같은 sequence/다른
confirmed digest는 AUTH_FAILED(CONFLICT)다. 실제 정상 앱 검증 snapshot이 일치해야
ALREADY_INSTALLED, 손상/선택 불가가 확인돼야 REPAIR_REQUIRED 후보가 된다.
미확인 policy/설치 상태는 INCOMPLETE이며 영속 floor0으로 초기화하지 않는다.

불필요한 저장 계층이나 범용 policy engine은 추가하지 않았다. 스킬의 무힙·소유권·
문서화 규칙을 적용해 입력은 호출 중 불변으로 빌리고 판정만 복사한다. 실패 시
두 target의 판정을 모두 지운다. 영속 journal A/B 선택·CONFIRM_INTENT·실제 설치본
검사·floor 갱신·automatic rollback은 기존 설계의 owner가 구현할 별도 범위다.

검증과 실패 수정:

- body probe 호출부 변경 중 무관한 manifest_preflight NULL 시험 두 곳에 인자를
  하나 더 넣어 strict compile이 실패했다. 해당 두 호출만 복원한 뒤 재빌드/회귀했다.
- `ctest --test-dir build/host-debug -R 'ota-version-floor|ota-body' -V`: 순수 C3847건,
  본문 모형1480건, 실제 CNG P256+SHA2561486건 통과. 미확인/누락/잘못된 policy,
  u64 하한/충돌/NULL은 첫 hash operation 전에 거절됨을 probe에서 확인했다.
- `tests/ota/test_floor.c`: 네 target, u32/u53/u63/u64 경계 교차, 모든 digest bit,
  잘못된 count/role/target/enum/text, 부분 성공 폐기와 입력 불변성을 검사했다.
- WSL Clang21 ASan/UBSan: floor3847건과 body 모형1480건 통과.
  `build/ota-floor.profdata`의 floor.c 함수5/5·행105/105·분기94/94;
  `build/ota-floor-body.profdata`의 body.c 함수10/10·행203/203·분기84/84다.
  floor는 암호 연산을 하지 않으며 body 모형은 실제 암호 검증으로 집계하지 않는다.
- 고정 Windows Clang23/CMake strict build 뒤 전체 Debug131/131(32.54초),
  Release131/131(26.47초). `ctest --test-dir build/host-debug --output-on-failure
  --output-log build/ota-floor-debug.log` 및 host-release/ota-floor-release.log로 재현한다.
- Arm GNU15.3.rel1 `-mcpu=cortex-m4 -mthumb -ffreestanding -Os -fstack-usage` strict
  object compile 통과. floor_check 단일 frame40B다. target ELF/MAP/BIN 증거는 아니다.
- source digest `76c48306a09135c595f5dc80aa4e3a7e383edf80348b8699d3b221f685a0d5f8`로
  T103 합성 fixture만 갱신했다. 실제 물리 evidence는 변경하지 않았다.
- 직전 c0de352 CI34730820401의 host/portability/sanitizer/browser job 성공,
  target job 실행 중을 확인했다. 이후 floor 변경의 CI 또는 target 통합 증거로 쓰지 않는다.

T-007 IN_PROGRESS/PR #35 Draft다. ESP native 검증·prefix 부분 조립·정식 schema/CLI/
signed golden·실제 provider/target 통합과 최종 독립 2인 리뷰는 남아 있다.
physical/HIL NOT_RUN, 차량 CAN TX NO-GO와 writer/activation 권한 분리를 유지한다.

## 2026-09-13 (codex, MCUboot native P256 image 검사)

공식 MCUboot v2.4.0을 `C:/cv/mcuboot-2.4.0`에 clone했다. 실제 HEAD는
`6d3b3d2c38ab20c242e5b9abb04d050086383eb2`, clean이며 imgtool CLI는2.4.0을 출력했다.
commit은 toolchain manifest, Windows 의존성은 기존 OTA wheel lock에 고정했다.
CI host job도 같은 source를 clone/검증한 뒤 시험한다. upstream 원본은 수정하지 않았다.

`native_stm.c`는 header512/일반 image180KiB 상한, flags/load address0, protected
vendor TLV와 SHA256/KEYHASH/P256 DER의 고정 profile을 검사한다. 전체 image digest와
manifest, protected board/role/layout/epoch/ABI/sequence와 local identity/descriptor,
native version과 정규 version 문자열을 대조한다. native signed-region hash와 P256
signature도 SDK로 독립 검사한다. Windows의 기존 CNG P256 검증을 digest helper로
추출해 재사용했다. DER 정수 변환 외에 암호 알고리즘을 새로 구현하지 않았다.

검증:

- `python -B tests/ota/test_native_stm.py build/host-debug/canview-ota-native-stm-probe.exe`:
  실제 imgtool 생성/검증·CNG SHA256/P2562283건. wrong root/whole hash, 모든 byte 변이/
  절단, 유효하게 재서명한 잘못된 metadata, u64 경계, DER 음수/길이/중복 TLV,
  provider 실패와 최대 image를 검사했다. 임시 image 파일만 생성하며 개인키는 메모리 전용이다.
- WSL Clang21 ASan/UBSan C probe와 같은 script의 `--model --wsl`:2284건.
  Python이 계산한 hash/서명 결과를 반환하는 모형이며 실제 암호 통과로 집계하지 않는다.
  무작위 ECDSA DER 길이에 따라 byte별 시험 건수가 달라진다.
- `build/ota-native-stm.profdata`: native_stm.c 함수9/9·행158/161(98.14%)·
  분기180/190(94.74%). 최초 default.profraw는 생성 위치를 확인한 뒤 ignored
  build/ota-native-stm-initial.profraw로 이동했고 후속 profile은 build에 직접 출력했다.
- Windows Clang23 strict C99/CMake build, CLI dependency 수정 후 최종
  Debug130/130(29.95초), Release130/130(25.61초). 로그는 ignored
  build/ota-native-stm-debug-final.log와 build/ota-native-stm-release-final.log다.
  앞선 envelope/body/manifest 암호 회귀도 포함한다.
- Arm GNU15.3.rel1 Cortex-M4 freestanding object compile. native check 단일 frame160B,
  helper16~56B다. 실제 target ELF/MAP/BIN/call-chain budget 완료는 아니다.
- Doxygen1.18/Sphinx9.1 strict build 통과, 기존 공개 API71개 계약 PASS다.
  이 XML 입력 목록에는 새 내부 OTA header가 없으므로 그 API의 추출 증거로 확대하지 않는다.
- 이전0e62ec6 CI34729444268 success를 확인했다. 이후 source에 재사용하지 않는다.
  합성 source digest는 `8bddaf30c8f30e20d0ebec4c5faea3207cd4b5669327e05af1ad7f47a1fccb0a`다.

wheel 설치 시 user Python의 click/cffi가 lock 버전으로 갱신됐다. pip의 user Scripts
PATH 안내 경고는 target compiler warning과 구분하며 해당 CLI를 설치 검증으로 사용하지
않았다. 명령은 Python module/API로 실행했다. 개발환경 문서에는 공용 Python 보존이
필요한 경우 venv와 CMake Python 경로를 함께 지정하도록 기록했다.
분리된 기존 OTA venv에서 재확인하니 imgtool API 시험은 통과했지만 CLI는 PyYAML
누락으로 실패했다. CLI import에 필요한 PyYAML wheel/hash를 lock에 추가하고 시험에도
실제 CLI version 호출을 넣었다. 환경의 우연한 전역 패키지로 숨기지 않는다.
수정 뒤 같은 분리 venv의 CLI+native CNG2287건과 위 최종 전체 회귀가 통과했다.

스킬의 인터페이스/소유권 규칙에 따라 기존 OTA README에168B metadata 후보와
공식 extension 근거를 기록했다. 정식 schema/ADR 동결·ESP native 검사·packager/golden·
version floor·target SDK/Flash/bootloader 통합과 최종 2인 리뷰는 남아 있다.
합성 bytes의 format/crypto 성공은 부팅 가능성·실물 복구 검증이 아니다.
T-007 IN_PROGRESS, PR #35 Draft, physical/HIL NOT_RUN, 차량 CAN TX NO-GO를 유지한다.

## 2026-09-13 (codex, OTA 로컬 호환성 사전 검사)

`manifest_preflight()`를 기존 C 검사기에 추가하고 body open에서 필수 호출한다.
서명/identity 검사 뒤 신뢰된 로컬 snapshot으로 ESP/STM 구·신 네 조합을 signed
allowlist와 비교한다. 한 image만 업데이트하면 다른 MCU의 ABI는 그대로 유지한다.
MCU별 bootloader/recovery 최소 ABI, hardware capability 부분집합과 보존 config의
읽기 범위를 검사한다. 미확인 snapshot은 INCOMPLETE이며 provider 시작 전 거부한다.
외부 peer의 존재/ABI를 Communicator 자체 복구의 설치 조건으로 추가하지 않는다.

기존 파일/API를 확장했으며 새 framework·wire 필드·writer는 만들지 않았다.
embedded-cstyle/architecture/documentation 스킬의 고정 메모리·소유권 원칙에 따라
snapshot의 출처/수명/실패 동작을 header와 기존 OTA README에 기록했다.

검증 결과:

- 고정 Windows Clang23 strict C99 build, 전체 Host Debug129/129(37.22초),
  Release129/129(28.44초). `ctest --test-dir build/host-debug --output-on-failure`
  및 host-release. ignored 로그: `build/ota-preflight-debug.log`,
  `build/ota-preflight-release.log`.
- `python -B tests/ota/test_body.py build/host-debug/canview-ota-body-probe.exe` 모형1462건,
  crypto-probe.exe와 `--crypto` 실제 P256+SHA-2561468건. 네 조합 각각 누락,
  단일 target·순서 반전·16개 조합·0/u32 최대 ABI, 모든64bit capability 누락/허용,
  boot/recovery/config 경계와 재진입/cleanup 회귀를 포함한다.
- WSL Clang21 ASan/UBSan으로 body1462건과 기존 manifest1422건을 각각 재빌드/실행했다.
  새 임시 profile 디렉터리를 합친 `build/ota-preflight-combined.profdata`에서
  manifest.c 함수16/16·행386/395(97.72%)·분기223/238(93.70%),
  body.c 함수10/10·행198/198·분기82/82(100%)다. provider/장치 전체 coverage가 아니다.
- Arm GNU15.3.rel1 Cortex-M4 strict freestanding object compile 통과.
  `build/ota-preflight-arm.su`의 preflight 단일 frame40B, runtime48B,
  기존 manifest_check864B다. call-chain stack/target ELF/MAP/BIN 증거는 아니다.
- 문서327개/target1340개·task49개 오류0. 합성 source digest는
  `fae13dde1175135377bf9abc93b2b470e0b4934b7c2d6f8b696b0fd0e8d93cf5`이며
  T103 합성 fixture 식별자만 갱신했다. 과거 physical evidence는 그대로다.

snapshot은 저장하지 않으므로 설치 owner가 transaction/상태 변경 뒤 다시 검사해야 한다.
native image 서명/보호 metadata와 manifest 대조, version floor/CONFLICT/REPAIR,
schema/CLI/golden·prefix 조립·실제 target 통합·최종 독립 2인 리뷰는 미완료다.
T-007 IN_PROGRESS, PR #35 Draft, physical/HIL NOT_RUN, 차량 CAN TX NO-GO다.

## 2026-09-13 (codex, OTA 순차 본문 hash와 수신 lifecycle)

기존 manifest 검사 뒤에 body open/feed/finish/reset을 연결했다. 완전한 prefix를
검증하고0..16KiB chunk의 절대 offset·image 길이·SHA-256을 순서대로 대조한다.
chunk/prefix/identity pointer를 저장하지 않고 descriptor와 hash 함수표만 복사한다.
hash context는 NULL을 거부하고 reset 성공까지 빌린다. 중복/누락 offset, 크기
초과, hash/provider 오류는 FAILED이며 manifest를 지운다. cleanup 실패는 자원을
잃지 않고 원 오류와 별도로 기록해 reset을 재시도한다. 동일 객체 callback 재진입은
busy로 거부하며 thread 동기화는 caller의 단일 owner 계약이다.

암호를 새로 구현하지 않았다. 기존 Windows P256 verifier를 host 전용
`tests/ota/cng_provider.c`로 추출해 기존 envelope 시험과 함께 사용하고, 같은 SDK의
SHA-256 operation을 연결했다. Windows provider의 SDK hash allocation은 portable
core의 무힙 특성과 구분한다. provider는 장치 binary에 아직 링크하지 않는다.

최종 검증:

- Windows Clang23/CMake4.4.3/Ninja1.13.2 strict C99 build, Debug129/129(25.43초),
  Release129/129(20.90초). 로그는 ignored `build/ota-body-debug-final.log`,
  `build/ota-body-release-final.log`다.
- `python -B tests/ota/test_body.py build/host-debug/canview-ota-body-probe.exe`:
  sum/length 모형의 수명/실패900건. 모형을 SHA-256 성공으로 집계하지 않는다.
- 같은 script와 `build/host-debug/canview-ota-body-crypto-probe.exe --crypto`:
  실제 Cryptography48 P256 manifest + Windows CNG SHA-256906건. 역할별 임시
  key와 잘못된 root, 잘못된 서명, `abc` known answer·모든 본문 byte 변이,
  최대 slot 길이·image 경계 chunk·EOF·중복/누락·reset·provider/cleanup 실패를 검사했다.
  개인키는 메모리에서만 생성하며 합성 bytes는 부팅 가능한 firmware가 아니다.
- WSL Clang21 ASan/UBSan과 위 모형900건 통과. 매번 새 profile 디렉터리를 사용했고
  `build/ota-body.profdata`의 body.c 함수10/10·행198/198·분기82/82(모두100%)다.
  전체 OTA/native provider/target coverage gate 완료로 확대하지 않는다.
- Arm GNU15.3.rel1 Cortex-M4 freestanding object compile 통과. 단일 frame은
  open48B/feed32B/finish24B/reset864B, helper16~56B다. reset의 aggregate 초기화
  temporary를 포함한 값이며 SDK 포함 call-chain stack·MCU timing·ELF/MAP/BIN
  검증을 대신하지 않는다.
- 직전 b25b69a의 CI34727655450는 success다. 이후 본문 source의 CI/artifact
  검증으로 재사용하지 않는다. 새 staged diff와 secret/VIN은 push 전에 별도 확인한다.

EOF+cleanup 실패 시험의 초기 입력은 Communicator 첫 image를 끝내 reset 실패가
먼저 발생했다. 기대했던 EOF 경로가 아니므로 첫 image가 끝나기 전에 끊도록 시험
입력을 수정했다. 이후 두 모형/실제 암호 시험과 전체 회귀를 재실행했다. 구현의
거절 조건을 완화하지 않았다. 합성 source digest는
`94c2f6fdffc8e820205326b8866bd6cea32960b8edb89fd48771444fa2b78684`다.

스킬의 소유권·정리·상태 계약을 OTA README에 기록했고, 상세 task의 중복 진행
이력은 현재 구현/검증으로 줄였다. 기존 journal/review 원본은 변경하지 않았다.
HASHES_MATCHED는 native image signature/protected metadata, 현재/후보 ABI·
requires/version floor, Flash read-back·PREPARED/boot selector 권한이 아니다.
prefix 부분 수신 조립·정식 schema/CLI/golden·실제 ESP/STM provider와 target 통합,
최종 독립 2인 리뷰는 남아 있다. PR #35 Draft, T-007 IN_PROGRESS,
physical/HIL NOT_RUN, 차량 CAN TX NO-GO를 유지한다.

## 2026-09-13 (codex, OTA typed manifest와 대상·길이 대조)

기존 prefix/실제 서명 경로 뒤에 고정 구조체 decoder를 연결했다. 임의 주소나
경로를 허용하지 않고 역할별 일반 앱 enum·길이 상한으로 제한한다. 신뢰된 로컬
role/board/layout/epoch/key_id를 대조하고, image offset을 순차 합으로 계산해
header image 수·총길이에 대조한다. uint64 release_sequence를 그대로 보존한다.
required field/unknown field/중복 target/key, 잘못된 native signature 형식,
ABI 범위와 조합·config snapshot의 내부 정합성도 거부 경로로 시험했다.
새 범용 tree/framework/암호 구현 없이 기존 bounded parser와 provider를 재사용했다.

검증 결과:

- Windows 고정 Clang23/CMake4.4.3/Ninja1.13.2에서 strict C99 build 통과.
- `ctest --preset host-debug --output-on-failure`, `host-release`: 각각127/127.
- `python -B tests/ota/test_manifest.py build/host-debug/canview-ota-manifest-probe.exe`:
  서명 mock을 사용한 구조/출력 교차1422건. NULL/크기/identity/실패 출력0과 callback
  별도 out 재진입·provider 실패도 C probe에서 실행한다.
- 같은 script에 `build/host-debug/canview-ota-envelope-probe.exe --crypto`:
  Cryptography48→Windows CNG 실제 P256 서명 교차1425건. 개인키는 메모리에서만 생성했다.
- WSL Clang21의 현재 C source에 ASan/UBSan·coverage instrumentation을 함께 적용하고
  같은 portable 교차1422건 통과. manifest.c 함수12/12, 행304/313(97.12%),
  분기169/184(91.85%). profile은 새 `mktemp -d`에서 실행별로 생성·merge해 이전
  run을 섞지 않았다. ignored 결과는 `build/ota-manifest-sanitize`,
  `build/ota-manifest.profdata`다. 전체 OTA coverage gate 완료는 아니다.
- Arm GNU15.3.rel1 `-mcpu=cortex-m4 -mthumb -ffreestanding -fstack-usage` object
  compile 통과. `build/ota-manifest-arm.su`의 public 함수 frame864B/helper16~64B는
  crypto 포함 전체 call-chain stack 또는 target ELF/MAP/BIN 완료의 증거가 아니다.
- 직전 commit da63535의 CI34726660432는6/6 success. 이후 source와 별개이며 해당
  artifact/hash를 이번 세션에서 재감사했다고 주장하지 않는다.

ABI 회귀 추가 직후 재빌드 전 executable로 실행한 두 교차 시험은 old code가 ABI
범위 밖 값을 수락해 실패했다. 현재 source를 재빌드한 뒤 두 교차 시험과 전체
Debug/Release를 다시 실행해 통과했다. 테스트 기대값이나 validator를 완화하지 않았다.
합성 T103 fixture의 source digest만
`1217e49b4b47a40731e8ba7fc82072bc72e0bf6a89ec9f8a2f244a26279b35da`로 갱신했다.

문서 스킬의 책임/수명 설명을 OTA README에 반영했다. resume는 오래된 설치·WSL
차단 설명과 시간순 상세 이력을 제거하고 현재 상태/다음 작업/debt/안전 경계로
줄였다. 과거 journal·review·ADR 원본은 수정/삭제하지 않았다.

현재 manifest는 미배포 내부 후보이며 정식 schema/CLI, 실제 실행중/후보 ABI 조합·
requires/version floor, image 본문 hash/native 서명과 protected metadata,
streaming·target 연결·최종 독립 2인 리뷰는 남아 있다. CBOR checkpoint A/B를
새 코드의 리뷰로 대체하지 않는다. PR #35는 Draft이며 T-007은 IN_PROGRESS,
physical/HIL은 NOT_RUN, 차량 CAN TX는 NO-GO다.

## 2026-09-13 (codex, 실제 서명 prefix 교차 시험)

작은 header+CBOR+64B 서명 prefix와 순차 image 조립을 연결했다. 자체 암호
알고리즘을 만들지 않고 Cryptography48.0.0 및 Windows CNG P256/SHA-256을
사용한다. key는 시험 실행 메모리에서만 생성하고 출력/파일/장치/Git에 쓰지 않았다.
실제 signature/prefix230건에는 wrong key/invalid point, signature 각 byte 변이,
manifest 변이, 유효 서명이 붙은 잘못된 CBOR, 전체 prefix 절단·최대 길이가 있다.
조립기의 잘못된 이미지/서명 길이와 signer가 입력 list를 바꾸는 경우도 검사했다.

`tools/requirements-ota.lock`은 Windows x64/CPython3.14용 실제 PyPI wheel
세 개의 SHA-256을 고정한다. 새 ignored venv에 `--no-index --find-links
build/ota-wheels --only-binary=:all: --require-hashes`로 설치하고230건을 재실행했다.
전체 Debug125/125(26.17초), Release125/125(21.63초), WSL Clang21의 portable
envelope 경계 ASan/UBSan, Cortex-M4 freestanding compile이 통과했다. Arm
envelope 단일 frame64B/helper16B는 전체 crypto call-chain/target evidence가 아니다.
Windows CNG dll 관찰 버전은10.0.26100.8875다. Windows CI에도 같은 lock을 설치한다.

합성 source digest는 `1d1b8d52deff4ed54d4784bce898f5dfd7f1304bb93ca74d5d56f8cbe20e2635`다.
앞선 checkpoint6d83962의 독립 A/B 정적 리뷰는 finding0/PASS로 원문 보존했다.
A 원문의 절대 checkout link 때문에 최초 문서 검사1건이 실패했다. 원문을 바꾸거나
validator를 완화하지 않고 보고서 전체를4-backtick 원문 인용으로 감쌌고
327문서/1348target 오류0으로 재검증했다. 이후 추가된 envelope 코드에는 앞선
리뷰 verdict를 적용하지 않는다. 전체 task의 최종 독립 리뷰는 별도 필요하다.

staged `git diff --check`는 B 원문의 Markdown hard-break 공백4행을 보고했다.
반환 원문과 보존 본문을 직접 대조해 동일함을 확인했고 원문 공백은 유지했다.
해당 원본 파일만 제외한 staged diff 검사는 오류0이다. 코드 검사나 전역 Git
설정은 완화하지 않았으며 이4건을 전체 diff 오류0으로 보고하지 않는다.

현재 검사 성공은 prefix의 구조와 manifest 서명만 뜻한다. unsigned header의
declared image/total은 서명된 manifest 필드와 아직 대조하지 않으므로 설치 정보가
아니다. role/board/layout/epoch/key_id/호환성·native image 자체 검증·streaming과
정식 packager CLI/golden/target 연결이 남아 있다. Flash erase/PREPARED 권한,
physical/HIL NOT_RUN과 차량 TX NO-GO는 변하지 않았다.

## 2026-09-13 (codex, 단순화 원칙과 T-007 CBOR 문서 검사)

사용자가 OTA container의 필요성을 먼저 설명하고 전체적으로 간단한 방법을
사용하라고 요청했다. AGENTS에 가장 단순한 구현·기존 SDK 재사용·실제 사용처
없는 범용화 금지·복잡성 도입 전 대안 설명을 기록했다. 안전/검증 gate는 유지한다.
T-007은 작은 manifest+순차 image와 합성 파일 생성→C 검증 연결을 다음 작업으로
두며, 새로운 범용 package framework나 자체 암호 알고리즘을 만들지 않는다.

중단 전 CBOR 문서 검사 변경을 보존하고 정수 key 정렬·UTF-8·depth8·item2048·
16KiB 한도, 모든 첫 byte·prefix·고정 seed 변이·Python 순환 입력 회귀를 보강했다.
`ctest --preset host-debug --output-on-failure`는123/123(26.03초), Release는
123/123(21.67초)이다. `. F:/dev/canview-wt/t104-stm32-uart-control/tools/environment/foundation-windows.ps1`
로 설치된 pinned 도구만 재사용하고 현재 source를 빌드했다. focused Python/C
11,989건과 WSL Clang ASan/UBSan probe·C test가 통과했다. Arm GCC의 Cortex-M4
freestanding object compile에서 document validator frame216B, UTF-8 helper40B,
key helper16B를 관찰했다. 이는 전체 target binary/실측 call-chain evidence가 아니다.

합성 T103 fixture source digest는 `a3a72738db64375a554557cbc819fbd61cdf4725b97ca6ad22497c8bac7b6810`
이다. `validate_document_links.py`(324문서/1343target 오류0), `validate_plan.py`
(49task 오류0), `git diff --check`를 실행했다. Git LF→CRLF 안내는 target compiler
warning과 구분한다. 기존 head490d2f8 CI34338457956은6job success로 확인했으나
이번 변경의 CI/target/review evidence를 대신하지 않는다. physical/HIL NOT_RUN,
vehicle TX NO-GO와 issue34 OPEN을 유지한다.

## 2026-09-09 (codex, T-007 CBOR head 첫 C99 구현)

Draft PR #35에서 heap·SDK·writer 권한 없는 최대 9-byte CBOR head decoder와
unsigned 64-bit/잘린 prefix/비최소 encoding/major type/reserve 경계 회귀를 추가했다.
새 worktree의 중복 toolchain 다운로드는 중단하고 이미 설치·검증한 동일 버전의
Windows 도구로 소스를 실제 빌드했다. Host Debug 121/121(16.88초), Release
121/121(17.38초), WSL Clang 21.1.8 ASan/UBSan focused 시험이 통과했다. Arm GNU
15.3.rel1 Cortex-M4 freestanding C99 object(1352 B)와 단일 stack frame 72 B를
확인했다. 이는 전체 target binary·OTA 설치 또는 physical 증거가 아니다.

Focused llvm-cov는 PowerShell의 unquoted profile 인자로 첫 report 조회가 실패했다.
같은 생성 profile을 정확히 quote해 조회한 결과 함수1/1·행62/62·분기26/26 모두
100%였다. 최초 명령 실패를 coverage 실패/성공으로 혼동하지 않으며 전체 container
coverage나 CI gate가 완성됐다고 주장하지 않는다.

새 shared source를 포함하는 합성 T103 fixture digest는
`7f8620834513cb2e1e9ac7ea849758a99f7361ca2a70c24dccc6652f57ed1867`로 갱신했다.
과거 capture/서명·review 원문을 바꾸지 않았다. 전체 container/서명·schema·Python
differential·target 연결·2인 적대적 리뷰는 남아 있다. B-12/B-13 clean review
worktree 두 개는 process 부재·main ancestry 확인 후 git worktree remove로 정리했다.
원문/로그는 별도 보존했고 source는 Git에서 복구 가능하다. A debt 및 기존 build
worktree·dirty 기본 checkout은 보존했다.

## 2026-09-09 (codex, PR #33 merge와 T-007 시작)

최종 evidence-only `3ff04b7` CI 34335812873의 6개 job이 모두 성공했다. 다운로드한
manifest의 head/base/run identity, 이미지18/18 bytes·SHA-256, source provenance6/6,
target log21 compiler/linker/CMake warning/error0을 확인했다. Manifest SHA-256은
`1dd10972ccc9ec565f726ac21a58862ea10db704a576b67f9188cf1a421a0514`다. Node20→24
runner 안내는 별도 후속 항목으로 PR에 공개했다. A 면제는 사용자 PR #33 한정이며
원문·P1 확인 debt·P2 defer를 닫지 않았다. 18:55:58 KST `gh pr merge --merge
--match-head-commit`으로 merge했고 `d229772de77a48ae197e2ff1b4e55b6cef9a88ed`가
fetch한 origin/main과 일치하며 candidate가 ancestor임을 확인했다. 이슈 #34 OPEN.

dirty 기본 checkout을 보존하고 `codex/t007-ota-container` 별도 worktree를 만들었다.
선행 T-001 완료와 STM32 boot/core 순서를 근거로 T-007을 선택했다. CBOR primitive
구현부터 시작하며 정식 container/서명/target/review/CI gate는 아직 남아 있다.
physical/HIL·provisioning NOT_RUN, 차량 CAN TX NO-GO를 계속 유지한다.

## 2026-09-09 (codex, T-104 완료 응답 복구와 B closure)

앱 조회에서 비었던 A-10/B-11의 기존 final 응답/작성 원문을 로컬 기록에서 복구했다.
실제 서비스 차단 A-08과 조회 누락을 구분하고 원문을 변경 없이 보존했다. B-11의
과거 타 review 발췌 노출도 숨기지 않았다. B-12는 SDK/mock·문서 P3 두 건 PASS,
B-13은 원 B P1/P2 여섯 건 PASS를 반환했다. A-10 P2 회귀 공백은 owner digitie,
이슈 #34/T-104, 다음 admission 변경·software qualification 전 gate로 defer했다.
PR #33 한정 A 면제만 적용하며 최신 CI/artifact와 merge 확인은 아직 남아 있다.
다음 공용 core 선행 T-007은 merge 확인 뒤 시작하고 물리·차량 gate는 열지 않는다.

## 2026-09-09 (codex, T-104 A 중단 이슈와 사용자 한정 면제)

사용자가 A 중단의 상세 이슈화·skip·merge·다음 단계 진행을 명시했다. GitHub
이슈 #34를 만들고 A-08의 service JSON·22640 ms 실패·원 P1/P2 재확인 미완료,
수정 evidence와 후속 owner/gate를 기록했다. A의 verdict나 원 finding 심각도는
변경하지 않았고 PR #33 한정 예외와 일반 runbook의 충돌을 명시했다. 코드/SDK/
하드웨어 오류나 사용량 초과라고 추측하지 않았으며 서비스 차단을 우회하지 않았다.

B-09 원문을 보존하고 P3 DMAMUX fake 7/8→26/27, vendor LL 독립 C99 assertion,
model 추출 malformed/missing/duplicate unit, 두 worker의 watchdog 문서를 수정했다.
실제 Arm compiler의 RX=7/TX=8 negative probe는 각각 exit 1로 거부됐고 정상은
exit 0이다. host fixture digest는 firmware 문서 변경까지 포함하여 갱신했다.
초기 host 119/120은 raw report의 detached 경로 link 3건 때문이었으며 원문을
fence로 보존해 navigation에서 분리했다. validator 오류 0을 확인했다.

STM32 Debug/Release clean target binary와 54+2 SDK/model 상수, warning/error 0을
재확인했다. 이전 CI 34329023006도 6/6 success, manifest/artifact 18/18·source6/6,
target log21개 diagnostic0을 독립 대조했다. 새 candidate CI와 B 후속 확인은
merge 전에 별도로 완료한다. 물리/HIL은 NOT_RUN, 차량 CAN TX는 NO-GO다.

## 2026-09-09 (codex, T-104 UART review 재수정)

수정 commit `14ea3c9`를 push하고 A-08/B-09 및 최초 P1 reviewer A-10/B-11에
독립 재검토를 요청했다. A-08은 service 보안 제한으로 final report 전에 중단됐다.
service 원문을 보존하고 `INCOMPLETE/BLOCK`으로 기록했으며 제한 우회나 merge를
수행하지 않았다. 다른 실행의 결과를 원 A의 P1 closure로 대체하지 않는다.

`bdc6798`의 A-05/B-07 report를 원문 보존하고 C runtime reset에서 TX DMA를
PRIMASK 안에서 정지한 뒤 이전 completion/error latch를 폐기했다. RX 오류는
유지하며, unread byte와 producer 불명 손실을 분리해 기록한다. time-sync
COMMIT에는 tick 호출 순서와 무관한 1초 경계를 적용했다. HELLO build ID는
GNU linker SHA-1 앞 16 byte를 BSP에서 복사하고 ELF note/symbol/BIN 대조와
잘못된 note·symbol·BIN negative 시험으로 검증한다. SHA-1 ID는 인증이 아니다.

Windows Debug/Release 120/120, STM32 Debug/Release clean target 및 warning/error
scan 0, STM32 coverage, SDK 13/13, Doxygen/Sphinx strict와 TSan pool 1/1을
확인했다. WSL 보충 GCC 실행은 Windows worktree의 `.git` 경로를 해석하지 못해
Python provenance 시험이 실패했다. 확인된 Git directory를 WSL의 `GIT_DIR`와
`GIT_WORK_TREE`로 명시해 같은 HEAD를 검증한 뒤 전체 GCC/sanitizer 각각 120/120을 확인했다.
이 환경 실패를 C firmware 실패나 PASS로 바꿔 기록하지 않는다. main checkout의
사용자 변경과 기존 build directory는 보존했다. reviewer·CI가 닫히기 전 merge하지
않으며 physical/HIL·board flash·차량 TX는 계속 `NOT_RUN`/`NO-GO`다.

## 2026-09-09 (codex, T-103 hostile finding fix와 adapter coverage)

초기 immutable T-103 candidate에 대한 독립 Reviewer A/B raw report는 각각
`CV-HOSTILE-20260909-T103-A-01`과 `CV-HOSTILE-20260909-T103-B-01`이며 둘 다
`BLOCK`이다. 원문은 `docs/reviews/adversarial/evidence/2026-09-09-T-103-reviewer-a.md`
와 `...-reviewer-b.md`에 그대로 보존했다. A는 batch callback reentry, cross-channel
timestamp wrap, PSR/ECR timestamp 오염, IRQ acknowledge, session/owner와 FIFO loss를
지적했고, B는 no-TX evidence parser, timestamp wrap, filter budget, adapter coverage와
bounded parser를 지적했다. 이 report를 PASS로 낮추지 않고 post-fix 재검토를 요구한다.

그 finding을 반영해 module batch를 callback 이후 transactional commit으로 바꾸고,
channel별 timestamp epoch와 status/frame timestamp를 분리했다. IRQ는 RX flag를
FIFO drain 전에 acknowledge하고 FIFO/raw-ring loss를 latch하며, singleton owner와
stop/start session reset을 명시했다. no-TX analyzer는 bounded JSONL/schema/duplicate/
sequence/complete/TX-gate 검증으로 fail-closed하게 고쳤다. module과 CMSIS adapter의
strict C99 fake-register 시험 및 mutation 경계를 추가했다.

source fix candidate `3e13b2ca6e72a3aec5a32a6357285c614bc191f9`에서 T103 focused
CTest 2/2, 전체 Windows CTest 118/118, no-TX helper 6/6, WSL 일반 clone의
ASan/UBSan 전체 CTest 118/118과 독립 coverage function 100%/line≥95%/branch≥90%를
통과했다. FDCAN module coverage는 function 100%/line 99.1%/branch 94.1%, CMSIS
adapter fake-register coverage는 function 100%/line 98.6%/branch 93.2%다.
STM32 Debug/Release target clean build와 warning/error scan 0건도 갱신했다. post-fix
reviewer/CI는 아직 남아 있으며, 실제 board flash, FDCAN electrical/bitrate/IRQ
latency, reset/brownout, analyzer TX-zero와 차량 capture는 장비가 없어 `NOT_RUN`,
차량 CAN TX는 `NO-GO`다.


## 2026-09-08 (codex, T-500 merge closure)

T-500 최종 candidate `ff3121ce04328ff61a73f13492f8be9927f0dc98`의 독립
Reviewer A `CV-HOSTILE-20260908-T500-A-10`과 Reviewer B
`CV-HOSTILE-20260908-T500-B-10` raw report를 보존하고, 두 reviewer의
`PASS` 및 unresolved P0/P1/P2/P3 0건을 통합 report에 기록했다. 초기·중간
BLOCK, invalid candidate와 incomplete service 실행은 PASS로 산정하지 않았다.

PR [#30](https://github.com/digitie/canview/pull/30)을 ready로 전환한 뒤
GitHub Actions `34235313714`의 windows-c99, target-firmware-windows,
Linux GCC/Clang, sanitizer, browser contract 6개 job success를 확인하고
merge했다. merge commit `8f5d97ff924fe7fdb757a3a86a30cde5a80c2a09`와
`origin/main`이 일치하며 ancestor 검증도 통과했다. T-500 focused unit 46/46,
전체 Python 97/97, host 12/12, selected scenario/evidence validator,
compileall, document link와 plan 검사를 다시 확인했다.

g2 read-only rig는 physical backend가 없어 `SKIPPED`이고, 실제 board
flash/HIL·power/reset/brownout·CAN analyzer·차량 bus·provisioning과 차량
CAN TX는 각각 `NOT_RUN`/`NO-GO`로 유지한다. 다음 구현은 T-103 STM32
3채널 FDCAN capture-only C firmware다.

## 2026-09-08 (codex, T-102 merge와 T-500 host harness 시작)

PR #29의 최종 문서 closure head `10716b12a19b7244982d6e1572f1fad81057d348`에서 GitHub Actions `34218499019`의 6개 job 전체 성공을 다시 확인하고, PR #29를 merge commit `50410ba23fcecfa1f28cea837d04a061c201d648`로 `origin/main`에 통합했다. T-102 candidate `1894117`은 source/review/CI closure를 통과했지만 board flash, G1/G2 physical/HIL, UART/FDCAN 계측, Flash root, 차량 CAN과 provisioning은 `NOT_RUN`이며 CAN TX는 `NO-GO`다.

T-103이 요구하는 공용 선행으로 T-500을 `codex/t500-hil-harness`에서 시작했다. `tests/hil/`에 JSON-compatible YAML 1.2 scenario 12개, deterministic host adapter, seed·firmware/harness source digest·scenario digest report, monotonic JSONL offset, capture-only TX zero/allow-list/budget/first-violation analyzer와 fail-closed lab rig 계약을 추가했다. malformed input·중복 key·tampered offset도 fail-closed로 거부한다. host subset은 실제 HIL이 아니며, 연결되지 않은 rig는 `SKIPPED/BLOCKED`로만 출력한다.

## 2026-09-08 (codex, T-102 review closure 준비)

T-102 최종 candidate는 18941170ef475777c62db2f1471b74f937c807ea이다. 346257b에서 발견된 C preprocessing phase-order P2를 line splice 후 comment removal 순서로 수정하고, 직접·alias FDCAN member와 mode directive의 LF/CRLF split-comment 조합 및 reversed-order mutation 회귀시험을 추가했다. 최종 immutable reviewer A CV-HOSTILE-20260908-T102-POSTFIX-A-005와 B CV-HOSTILE-20260908-T102-POSTFIX-B-005는 각각 PASS, unresolved P0/P1/P2/P3 0건을 반환했다. 원문은 [통합 report](reviews/adversarial/2026-09-08-T-102.md)와 [A raw](reviews/adversarial/evidence/2026-09-08-T-102-postfix-reviewer-a-r4.md), [B raw](reviews/adversarial/evidence/2026-09-08-T-102-postfix-reviewer-b-r4.md)에 보존했다.

최종 소스 수정 뒤 Host Debug/Release 116/116, Clang ASan/UBSan 116/116, TSan pool 1/1, coverage 116/116 및 9/9 subset, STM32 core gate 6/6, generator/sdkconfig/plan/link, Doxygen/Sphinx strict, STM32 Debug/Release clean-first ELF/MAP/BIN/HEX와 warning/error 0을 다시 확인했다. GitHub Actions `34216963785`의 Linux sanitizer/portability 2종, Windows C99, browser contract, target firmware 총 6개 job과 target artifact/warning gate가 PASS했다. PR #29 merge closure가 남아 있으며, 실제 board flash/HIL·ST-LINK/serial·clock/reset/rail/brownout·UART/FDCAN 계측·Flash root 배치·차량 CAN·provisioning은 NOT_RUN, 차량 CAN TX는 NO-GO다.

## 2026-09-08 (codex, T-102 hostile review finding fix)

T-102의 immutable candidate를 독립 검토한 A(`CV-HOSTILE-20260908-T102-A-002`)와 B(`CV-HOSTILE-20260908-T102-B-002`)의 원문을 evidence에 보존했다. 두 report의 reset reason 직렬화 P1, 초기화 전 HardFault 무한 대기 P1, health fault latch·module/BSP 경계·CAPTURE_ONLY target-wide 강제·negative test·CubeG4 provenance·fixed-width 정수 finding을 반영했다. diagnostic record는 version 2와 offset 5 reset-reason byte를 사용하고, HardFault는 IWDG 준비 여부와 무관하게 system reset을 먼저 요청한다. build metadata는 BSP provider target으로 분리했으며 target forced include, link anchor, FDCAN TX source/symbol gate와 실 compiler override fixture를 추가했다.

수정 후 pinned Windows Host Debug/Release CTest는 각각 116/116, STM32 function100%/line≥95%/branch≥90% coverage, STM32 Debug/Release target ELF/MAP/BIN/HEX와 post-build gate는 warning/error 0으로 통과했다. post-fix candidate의 fresh reviewer 재검토와 CI는 아직 남아 있고 physical board/HIL·clock/reset/rail/brownout·UART/FDCAN·Flash root·차량 CAN evidence는 `NOT_RUN`, 차량 CAN TX는 `NO-GO`다.

## 2026-09-08 (codex, T-102 STM32 C source와 target 검증)

`codex/t102-stm32-platform`에서 사용자의 G1 이전 firmware 구현 허용과 C 작성 지시에 따라 T-102 source를 진행했다. STM32 `CAPTURE_ONLY` build contract, generated hardware digest와 build metadata, RCC reset reason, static stack watermark, protected service policy skeleton, little-endian diagnostic record를 추가하고 기존 safe GPIO·HSE/PLL·TIM2/SysTick·IWDG·cooperative scheduler와 연결했다. stack watermark의 host register-model 경계 및 scan timeout 시험을 보강했으며, target linker는 실제 reserved stack window를 `__stack_limit`으로 export한다. FDCAN/UART 송수신과 차량 CAN TX는 열지 않았다.

검증은 pinned Windows Clang 23.1.0/CMake 4.4.3/Ninja 1.13.2와 Arm GNU 15.3.Rel1/STM32CubeG4 1.6.3에서 수행했다. Host Debug/Release 전체 CTest는 각각 115/115, 공용·ESP32·STM32 coverage, board generator/config/plan/link 검사는 PASS였다. Doxygen 1.18.0 API 32개와 Sphinx strict도 PASS였고, STM32 Debug/Release ELF/MAP/BIN/HEX 및 post-build memory·stack·symbol gate가 warning/error 0으로 통과했다. 실제 board flash/HIL, clock/reset/rail/brownout 계측, UART/FDCAN, Flash root 배치와 차량 evidence는 장비·선행 조건이 없어 `NOT_RUN`이다. 다음은 immutable candidate commit, 독립 reviewer A/B, Draft PR/CI closure다.

## 2026-09-08 (codex, T-400 reviewer service 미완료 기록)

PR #28 source candidate `19a42339b2b37981a0bdfe976e3e073809825027`와 base `9fe46c753be151e6aa23f0fdc95fd527f86cf82d`를 고정해 Reviewer A/B를 독립 실행했다. broad scope, bounded scope, 최소 object-only scope의 세 번 시도 모두 raw report를 반환하지 않고 `running` 상태가 지속되어 coordinator가 shutdown했다. 최신 문서 candidate `3eb3647776215e472467bedbcdda9858fdbeb52f`에 대해서도 A `01a07e53-9c6b-7a32-abbd-d1c4807c5805`와 B `01a07e53-9d79-7211-983c-20b400406661`을 마지막 bounded 재시도했지만 같은 상태로 종료됐다. 실제 reviewer가 읽은 파일·실행 명령·finding은 반환되지 않았으므로 `NOT_REPORTED`로 남겼고, [통합 실행 기록](reviews/adversarial/2026-09-08-T-400.md)과 [A](reviews/adversarial/evidence/2026-09-08-T-400-reviewer-a.md)/[B](reviews/adversarial/evidence/2026-09-08-T-400-reviewer-b.md) evidence에 `INCOMPLETE/BLOCK`을 보존했다. 이전 source run은 5개 job 성공이었지만 최신 문서 candidate의 PR run `34171708930`은 기록 시 Windows C99와 target job이 `in_progress`였으며, required review raw report가 없어 ready/merge하지 않는다.

물리 board flash/HIL, ST-LINK/serial, AP association, phone browser, rail/reset/brownout, PSRAM/clock/watchdog soak, ESP-NOW observer/capture, production provisioning 및 차량 CAN evidence는 계속 `NOT_RUN`이다. Diagnostic Bridge의 CAN TX/raw replay/control lease는 `NO-GO`다.

## 2026-09-08 (codex, T-400 C web bootstrap hardening과 target 재검증)

T-400 source-only 예외 범위에서 `canview_bridge_web`의 REST 상태 mutex와 WebSocket I/O mutex를 분리하고, handshake event 전용 buffer와 bounded receive를 연결했다. start 실패 시 HTTP server·mutex·credential state를 정리하고, AP 설정 stack과 session body/response/subprotocol token을 처리 후 zeroize하도록 보강했다. WebSocket subprotocol의 comma/공백·중복·빈 항목·잘못된 token을 fail-closed로 검사하고, fixed JSON arena 사용량 overflow와 malformed body 길이도 방어한다. Diagnostic Bridge의 read-only capability(`control_scope=0`, `vehicle_tx=false`)와 raw replay/control lease 부재는 유지했다.

검증 결과는 다음과 같다.

- `python -B tests/test_bridge_web_assets.py -v`: 3/3 PASS, `python -B -m unittest discover -s tests -p 'test_*.py'`: 47/47 PASS.
- Windows host Debug/Release `bridge-auth`: 각각 1/1 PASS, 전체 CTest는 각각 111/111 PASS(`uart-fault-stream` 제외).
- WSL Clang ASan/UBSan 전체 CTest 111/111 PASS(`uart-fault-stream` 제외), coverage·ESP core coverage gate PASS.
- `python -B tools/generate_boards.py --check`, sdkconfig negative/positive 12/12, 문서 link 검사 238 documents·1180 local targets 오류 0.
- ESP-IDF `6.0.3` 실제 Diagnostic Bridge target build에서 BIN/ELF/MAP를 재생성했다. app binary `0xcf870` bytes, app partition 여유 68%, 최신 성공 log의 warning/error scan 0건이며 `idf.py size-components`도 exit code 0이다. CI/독립 review는 다음 closure에서 다시 연결한다.

이번 단계에서도 board flash/HIL, ST-LINK/serial, AP association, phone browser, rail/reset/brownout, PSRAM/clock/watchdog soak, ESP-NOW observer/capture, production provisioning과 차량 CAN evidence는 실행하지 않아 `NOT_RUN`이다. CAN TX는 계속 `NO-GO`이며, immutable commit·fresh 독립 hostile reviewer 2명·Draft PR CI는 아직 남아 있다.

## 2026-09-08 (codex, T-400 source-only web bootstrap 구현)

사용자가 `G1 이전 fw 구현 허용`과 `C로 작성`을 명시했으므로, `codex/t400-bridge-web-bootstrap`에서 T-400의 firmware source 구현을 시작했다. `canview_bridge_auth`에는 SDK-independent C99 challenge·one-time PIN·memory token·expiry·5회 lockout 상태기계를 추가했고, `canview_bridge_web`에는 ESP-IDF `esp_http_server`, `cJSON`, WebSocket, fixed-buffer DNS와 gzip 내장 shell을 추가했다. `app_main.c`는 board preflight→safe GPIO→fixed pool/runtime/core→read-only NVS credential→web start 순서의 단일 owner lifecycle을 사용한다. 모든 web 응답의 `control_scope=0`, `vehicle_tx=false`를 유지하고 raw CAN/replay/control lease 경로는 만들지 않았다.

source 상태에서 다음을 확인했다.

- Windows pinned Clang23.1.0/CMake4.4.3/Ninja1.13.2 환경의 `build-bridge-auth` CTest `bridge-auth`: 1/1 PASS.
- `python -B tests/test_document_links.py -v`: 5/5 PASS. managed component/build generated README를 문서 navigation에서 제외하는 회귀를 추가했고 `tools/validate_document_links.py`: 238 documents, 1180 targets, errors=0.
- `python -B tools/generate_boards.py --check`와 `tools/check_sdkconfig.py firmware/diagnostic-bridge/sdkconfig --board bridge-r1-n8r2`: PASS.
- ESP-IDF `6.0.3`에서 `firmware/diagnostic-bridge`로 이동한 뒤 `idf.py build`를 실행해 compiler/linker 단계와 app image 생성을 확인했다. `build/canview_diagnostic_bridge.bin`, `.elf`, `.map`가 생성됐고 app partition 여유는 68%였다. `idf.py size-components`도 종료 code 0이다.

G1 board flash, ST-LINK/serial, AP association, Android/iOS browser, rail/reset/brownout, PSRAM/clock/watchdog 장시간 시험, ESP-NOW observer/capture, encrypted production provisioning과 차량 CAN evidence는 실행하지 않아 `NOT_RUN`이다. source implementation은 사용자의 예외로 진행했지만 물리 gate는 닫지 않았고 CAN TX는 계속 `NO-GO`다. immutable commit, 새 독립 hostile reviewer A/B raw report, CI와 PR closure는 아직 남아 있다.

## 2026-09-08 (codex, T-400 G1 physical 장비 가용성 확인)

Windows의 `Win32_SerialPort`와 present PnP 장치를 `ST-LINK`, `STM32`, `ESP32`, `CP210`, `CH340`, `FTDI`, `USB Serial` 및 관련 vendor ID로 읽기 전용 조회했으나 대상 COM port·debug probe·USB-UART·MCU 장비는 하나도 감지되지 않았다. 따라서 T-400의 board flash/HIL, ST-LINK/serial console, rail/reset/brownout, PSRAM/clock/watchdog soak은 실행하지 않았고 모두 `NOT_RUN`이다. 이 결과는 host/CI target build를 physical evidence로 승격하지 않으며 G1이 닫히기 전 SoftAP·HTTP·인증 또는 차량 CAN/TX 범위에 진입하지 않는다. 차량 CAN evidence, provisioning, vehicle TX release도 계속 `NOT_RUN`이고 CAN TX는 `NO-GO`다.

## 2026-09-07 (codex, T-400 P2 source/CI/PR closure)

PR #25의 final candidate `5b6a2994d676784ab02bdbda54f1e50a9454b45c`에서 CI `34126431204`의 Windows C99, target-firmware-windows, Linux GCC/Clang portability, Linux ASan/UBSan 다섯 job이 모두 success로 종료했다. 업로드 artifact를 별도 디렉터리에 내려받아 `target-artifacts.json`의 18개 path·byte·SHA-256을 재계산해 모두 일치시켰고, target build log의 compiler/linker/CMake warning scan은 0건이었다.

GitHub GraphQL quota가 일시 소진되어 Draft 해제는 reset 뒤에만 재시도했다. REST로 head·mergeability를 다시 대조한 뒤 ready-for-review mutation, check 5/5 success와 `CLEAN`을 확인하고 remote branch 삭제 없이 merge했다. PR #25의 merge commit `d8d80578050a9d368f6b4f81fae8301d27084fc2`는 `origin/main`과 일치하며 ancestor 검증을 통과했다. T400-P2-04를 포함한 source P0/P1은 남지 않았지만, board flash/HIL, ST-LINK/serial, rail/reset/brownout, PSRAM/clock/watchdog 장시간, vehicle CAN/evidence, provisioning, vehicle TX release는 계속 `NOT_RUN`이고 CAN TX는 NO-GO다. 따라서 T-400은 G1 physical evidence 전까지 `IN_PROGRESS`이며 SoftAP·HTTP·인증·차량 CAN/TX 범위에 진입하지 않는다.

## 2026-09-07 (codex, T-400 post-fix coverage evidence P2)

`8f32c07810d76cd3e652ccd08b801400aac5a770`의 첫 post-fix A/B 실행은 whole diff가 상대 raw evidence를 포함해 독립성이 무효가 되었고, 두 원문을 `BLOCK`으로 그대로 보존했다. 이를 PASS나 source finding으로 바꾸지 않았다. 이후 raw evidence를 읽지 않은 fresh source-only A는 P0–P3 없음·physical/HIL 조건부, B는 P0/P1 없음과 coverage evidence P2를 반환했다. B의 P2는 app coverage runner가 `preflight` scenario 및 실제 두 wrong-BSP cross-link executable을 profile/export 대상으로 포함하지 않은 점이다.

이를 defer하지 않고 `tools/check_esp32_core_coverage.py`가 app `preflight`를 수집하고 두 wrong-BSP executable의 독립 `.profraw`, `esp_core.c` export와 app function coverage가 없으면 실패하도록 수정했다. early fail-closed composition report는 full app threshold 분모와 분리한다. pinned LLVM 23.1.0 coverage gate는 portable·adapter·app 기준을 통과하고 두 새 profile/export도 확인했다. 이 coverage-evidence delta의 immutable commit, fresh A/B re-review, CI target artifact/warning scan이 남아 있으며 physical/HIL, flash, rail/reset/brownout, 장시간 watchdog/PSRAM, vehicle CAN/evidence, provisioning, vehicle TX release는 계속 `NOT_RUN`이고 CAN TX는 NO-GO다.

immutable `813d19cfaf0893e67c9591750f73363e9b67aaa1`의 delta를 raw evidence와 다른 reviewer output을 제외한 allowlist에서 A/B가 다시 읽었다. A는 P0–P3 없음·실행/HIL `NOT_RUN`의 `CONDITIONAL`, B는 T400-P2-04 `CLOSED`·P0–P3 없음의 `PASS`를 반환했다. P0/P1은 남지 않았지만 CI target artifact/warning scan과 G1 physical evidence 전에는 PR merge·SoftAP 진입 또는 차량 safety release를 승인하지 않는다.

## 2026-09-07 (codex, T-400 P2 initial review와 post-fix 검증)

PR #25 source candidate `f35779a78603dd8241ead9a8baa7307d629bf33a`를 독립 object-only reviewer A/B가 실제 읽었다. 두 reviewer는 runtime profile 비교가 `app_main()`의 `enter_safe_state()` 뒤에 있어 wrong BSP GPIO가 먼저 실행되는 P1을 확인했다. A는 runtime open과 pool init의 ISR state mutation P2를, B는 board ID만 hash한 profile이 stale pin contract를 구분하지 못하는 P2를 추가로 확인했다. 원문·첫 object 부재 incomplete attempt와 disposition은 [T-400 review](reviews/adversarial/2026-09-07-T-400.md)에 보존한다. incomplete report를 PASS로 바꾸지 않았고 PR은 Draft를 유지한다.

post-fix source는 app preflight를 GPIO·idle·SDK open보다 앞에 두고, profile 입력을 canonical board manifest+pin source로 확장했다. runtime open과 pool init은 ISR validator를 상태 변경 전에 호출한다. 실제 app+BSP wrong-link 두 방향은 GPIO와 runtime open 0회로 terminal idle만 호출하는 host negative test를 추가했다. pinned Windows host Debug/Release는 `uart-fault-stream` 86,400초 장시간 시험 제외 각각 110/110, ESP coverage gate는 PASS다. ESP-IDF 6.0.3과 Arm GNU 15.3에서 STM32 Debug/Release 및 Communicator·Bridge·Controller·fixture의 BIN/ELF/MAP 18개를 다시 생성했다. 이 작업 트리 target output은 post-fix commit 전 `f35779a-dirty` metadata였으므로 final immutable commit의 target artifact와 CI warning scan은 다음 단계에서 재확인한다. physical/HIL, flash, rail/reset/brownout, 장시간 watchdog/PSRAM, vehicle CAN/evidence, provisioning, vehicle TX release는 `NOT_RUN`이며 CAN TX는 NO-GO다.

## 2026-09-07 (codex, T-400 P2 runtime/BSP source 검증)

T-400a handoff의 compile-time board profile을 generator와 모든 BSP port에 추가하고, Communicator ESP/Bridge runtime이 링크된 board profile을 platform open 전에 대조하게 했다. 실제 교차 object link 두 방향은 runtime open을 호출하지 않은 채 거부한다. ESP runtime core callback은 open owner task·non-ISR·non-reentrant latch를 요구하고, fixed pool은 lock 전 context validator로 ISR/무효 context를 거부한다. Communicator ESP와 Bridge safe GPIO는 partial failure에도 모든 지정 safe pin을 시도하고 최초 오류를 보존한 뒤 lifecycle을 중단한다. Bridge의 read-only 경계와 CAN TX NO-GO는 변경하지 않았고 SoftAP·HTTP·인증은 시작하지 않았다.

Windows pinned Clang23.1.0/CMake4.4.3/Ninja1.13.2에서 host Debug/Release는 각각 108/108을 통과했다. 별도 86,400초 `uart-fault-stream`은 이번 실행에서 제외되어 `NOT_RUN`이다. ESP core coverage는 portable/pool 및 SDK fixture 각각 function/line100%, branch98.04% 이상과 adapter branch92.13% 이상을 통과했고, generator·actual sdkconfig·plan/link·Python43/43·Sphinx/Doxygen strict(공개 API29개)도 통과했다.

검증된 ESP-IDF6.0.3/Arm GNU15.3.Rel1/STM32CubeG4 1.6.3에서 STM32 Debug/Release, Communicator ESP32, Diagnostic Bridge, Controller 및 public-component fixture의 BIN/ELF/MAP 18개를 생성했다. 경고/error 패턴 scan은 0건이었다. 첫 Bridge `fullclean`은 partial non-CMake build directory를 안전하게 거부했고, 삭제 없이 남아 있던 `sdkconfig`의 esp32s3 target으로 새 configure·`idf.py build`를 실행해 artifact를 생성했다. 실제 flash/HIL, rail·reset/brownout, 장시간 PSRAM/clock/watchdog, 차량 CAN/capture, provisioning, vehicle TX release와 local WSL sanitizer는 `NOT_RUN`이며 CAN TX는 계속 NO-GO다. immutable commit, fresh 2인 adversarial review와 CI 전에는 이 handoff나 T-400을 완료로 표시하지 않는다.

## 2026-09-07 (codex, T-400a merge와 T-400 handoff)

PR #23은 final branch CI `34114919104`의 Windows C99, target-firmware-windows, Linux GCC/Clang portability, Linux ASan/UBSan 다섯 job success 뒤 merge commit `25eba080907257c6d90abaeec6d578d9dff6585a`로 `origin/main`에 통합됐다. candidate `0e1edb6`가 `origin/main`의 조상임을 확인했다. A-05/B-07의 `CONDITIONAL` raw review는 unresolved P0/P1 없음으로 닫았고, P2 세 건은 owner=T-400, G1 gate, 목표=2026-09-14로 유지했다.

T-400은 board identity/profile binding, callback stage/ISR contract, partial safe GPIO failure policy를 SoftAP·HTTP·인증보다 먼저 구현한다. actual board flash/HIL, rail·reset/brownout, 장시간 PSRAM/clock/watchdog, 차량 CAN/capture, provisioning, vehicle TX release는 장비 부재로 `NOT_RUN`이며 CAN TX는 계속 NO-GO다. 사용자 소유 `docs/session-continuation-prompt.md`는 untracked로 보존하고 stage하지 않았다.

## 2026-09-07 (codex, T-400a clean CI target artifact audit)

CI run `34112160182`의 target-firmware-windows는 PR merge ref `2e49280`에서 success로 종료했다. 업로드한 target image와 log artifact를 직접 내려받고 `target-artifacts.json`의 18개 path/byte/SHA-256을 재계산한 결과 모두 일치했다. STM32 Debug/Release, Communicator ESP32, Diagnostic Bridge, Controller 및 public-component fixture의 BIN/ELF/MAP를 포함하며 target command의 compiler/linker/CMake warning scan도 success condition으로 통과했다. PR merge version/source-path가 달라 local ELF/MAP/ESP app hash와 byte-identical하다고 주장하지 않는다.

같은 run의 전체 failure는 `windows-c99`에서 Doxygen 1.18.0 official archive download가 retry 후 빈 응답으로 끝난 것이 원인이다. 현재 URL의 HTTP 200 헤더는 재확인했지만, 일시 접근 회복을 CI 전체 PASS로 대체하지 않는다. B-06은 이전 evidence-only candidate에 clean target audit이 없다는 P1을 정확히 유지했다. 새 CI run에서 Windows C99를 다시 성공시키고, clean target audit을 포함한 evidence candidate의 같은 reviewer 재검토 전까지 P1은 unresolved다.

후속 run `34113636144`의 Windows C99 job은 success로 종료했다. B-07은 clean merge ref target job, artifact ID/digest, 18/18 manifest/hash audit, warning scan과 `5d6fac4` source 연결을 object-only로 재검토해 B-05/B-06 provenance P1을 `FIXED`로 확인했다. B-07은 physical/HIL `NOT_RUN`과 final branch CI를 분리한 `CONDITIONAL` verdict이며, raw report를 보존한다.

## 2026-09-07 (codex, T-400a immutable evidence와 독립 재검토)

post-fix source candidate `5d6fac4`를 Git object-only 기준선으로 새 execution ID A-05/B-05에 독립 전달했다. A-05는 지정 embedded runtime 범위에서 P0–P3 없음 `CONDITIONAL`을 반환했다. B-05는 source/config 정적 대조에서 새 P0/P2/P3는 없었으나, candidate 안의 target evidence가 temporary working-tree만 가리키는 provenance P1로 `BLOCK`을 반환했다. 두 raw report는 evidence 디렉터리에 원문으로 보존한다. B-05 시작 시각은 reviewer service가 `NOT_RECORDED`로 반환해 coordinator가 추정값으로 채우지 않았다.

`5d6fac4` source에서 새 STM32 Debug/Release 및 Bridge·Communicator·Controller ESP-IDF target build의 명령·hash·warning scan을 target evidence에 분리 기록했다. public-component fixture도 추가 build/warning scan을 했지만, evidence Markdown을 편집한 dirty worktree에서 시작돼 `5d6fac4-dirty` version이므로 clean candidate artifact로 승격하지 않았다. clean CI target artifact와 B-05 원 reviewer 재검토가 이를 닫아야 한다.

CI run `34112160182`는 Linux GCC/Clang portability와 Linux ASan/UBSan이 성공했고 Windows C99 job은 실패했다. target-firmware job의 완료 로그와 Windows failure log를 확인하기 전까지 CI PASS, PR ready/merge, 다음 T-400 시작을 주장하지 않는다. flash/HIL, 전원/reset/brownout, 장시간 PSRAM/clock/watchdog, 차량 CAN/evidence, provisioning, TX release는 계속 `NOT_RUN`이다.

## 2026-09-07 (codex, T-400a ESP target 복구와 commit 준비)

Windows native ESP-IDF 6.0.3를 직접 초기화해 Communicator ESP32, Diagnostic Bridge, Controller의 새 build directory에서 실제 target build를 다시 실행했다. 세 대상 모두 bootloader/app BIN, ELF, MAP를 생성했고 compiler·linker·CMake `warning:`은 0건이었다. Bridge/Communicator의 생성 sdkconfig도 각 보드 계약으로 재검증했다. 부트로더 configure 출력의 `CONFIG_ESP_INT_WDT_TIMEOUT_MS=800`은 기본값 300과의 명시적 설정 차이를 알리는 Kconfig notification이며 compiler warning으로 분류하지 않았다.

현재 commit 이전 working tree에서 생성한 artifact hash와 명령은 [target post-fix evidence](reviews/adversarial/evidence/2026-09-07-T-400a-target-postfix.md)에 기록했다. commit 후 immutable hash 기준으로 target artifact를 재생성하고, 독립 post-fix reviewer 2명의 raw report와 CI가 닫히기 전에는 PR #23을 ready/merge하지 않는다. WSL sanitizer, Windows bootstrap verifier, flash/HIL/전원/차량/보안 provisioning은 여전히 `NOT_RUN` 또는 별도 실패 원인을 유지한다.

## 2026-09-07 (codex, T-400a post-fix review closure 시도)

T-400a 최초 독립 리뷰의 P1 (safe callback preflight, stale watchdog feed time, sdkconfig allowlist)을 수정하고, P2 (USB bridge generator contract와 app composition fixture)는 후속 T-400 owner와 gate를 지정해 defer했다. host Debug/Release 106/106, strict documentation, generator·sdkconfig negative case와 STM32 Debug/Release target을 재실행했다.

새 reviewer execution A-03/B-03과 제한 범위 재시도 A-04/B-04는 source line-level raw report를 반환하지 못해 모두 `incomplete BLOCK`으로 보존했다. 이를 PASS로 변환하지 않았으며, T400a 통합 review, PR merge와 Task 완료는 두 독립 post-fix raw report가 실제 읽은 파일·명령·finding·verdict를 남길 때까지 차단한다. 이 시점의 ESP target build와 GitHub CLI 접근은 별도 recovery가 필요했다.

## 2026-09-07 (codex, T-200a merge와 T-400a 시작)

PR #22의 최종 `84080fe`에서 clean STM32 Debug/Release·ESP4종 binary warning0, local26개/원격18개 artifact 크기·SHA-256, CI5개 SUCCESS를 확인했다. 두 reviewer가 최초 P1 2건/P2 2건 모두 FIXED와 최종 문서 delta PASS를 독립 확인했다. `--match-head-commit` merge 결과는 `2222290`이며 [최종 evidence](reviews/adversarial/evidence/2026-09-07-T-200a-merge.md)에 원문·검증·NOT_RUN을 연결했다.

사용자 MCU/core 순서에 따라 새 `codex/t400a-bridge-core-bench`에서 T-400a를 분리했다. Bridge N8R2는 현재 실제 SDKCONFIG에서도 Quad/80MHz였으며 기존 GPIO4/5와 R14 외부 pull-up mapping을 유지한다. 공용 core와 보드별 메모리·GPIO 계약을 분리하고 Communicator 회귀를 보존한다. SoftAP/HTTP/무선/OTA·실물 acceptance는 T-400에 남겼고 단순 task 분리를 완료로 표시하지 않는다. embedded 구조/C/RTOS/driver/ISR/문서 지침을 다시 대조했으며 새 ISR·통신 task는 이 범위에 만들지 않는다.

## 2026-09-07 (codex, T-200a 구현·독립 리뷰 수정)

ESP32 C99 health/fixed pool·safe BSP·IDF TWDT/PSRAM/heap/USB adapter와 단일 owner app을 구현했다. Windows89/89, SDK fixture·app 실패 단계·동시 pool·coverage와 clean6종 warning0을 확인했다. [검증 evidence](reviews/adversarial/evidence/2026-09-07-T-200a-validation.md)는 commit별 결과와 미실행 HIL을 분리한다.

`55c7801`의 독립 리뷰 원본을 모두 저장한 후 비교했다. panic HALT 허용(P2), uintptr_t strict API 실패(P1), bootloader factory-reset/NVS 삭제 config 허용(P1), 자기참조 금지 목록 시험(P2)을 수정했다. PRINT_REBOOT/지연0·비휘발성 변경 차단과 독립21개 삭제 변이를 추가하고 실제 SDK 정상 build 및 factory/HALT configure 거부를 확인했다. 원문 내부 fence를 보존하기 위해 문서 link validator의 fence 처리를 보강하고 회귀4개를 추가했다. 모든 delta는 post-fix 독립 재검토 대상이며 아직 merge 승인이 아니다.

## 2026-09-07 (codex, T-102a merge와 T-200a 시작)

최종 head `3fd2b86`의 Windows/GCC/Clang/sanitizer/target CI5개 SUCCESS와 clean6종 binary warning0을 확인하고 PR #21을 `db5ed19`로 merge했다. 최초 리뷰5건 및 GCC portability1건은 두 reviewer가 독립 재확인해 모두 FIXED다. [최종 merge evidence](reviews/adversarial/evidence/2026-09-07-T-102a-merge.md)에 정확한 hash·원격 artifact·전체 host74/74를 보존했다. 보드 flash/HIL·차량 gate는 닫지 않았다.

사용자 요청의 MCU/core 순서에 따라 `codex/t200a-esp32-core-bench`에서 T-200의 boot/health/watchdog·고정 pool·config 검증 소프트웨어를 T-200a로 분리한다. 기존 실제 PSRAM/heap/USB/GPIO/UART 계측 acceptance는 T-200에 유지한다. ESP-IDF SDK API와 생성 board 설정을 먼저 대조했고 RTOS·C·구조·문서·driver 지침을 적용한다. 전체 phase를 건너뛰어 radio/CAN/OTA/provisioning을 활성화하지 않는다.

## 2026-09-07 (codex, T-102a 구현과 적대적 리뷰 수정)

`ca1a299`의 최초 2인 리뷰에서 deadline miss 소거(P1), NMI/feed 경쟁(P2), callback API 문서 실패(P1), 필수 vote 회귀시험 공백(P2), 개별 .su 누락 허용(P2)을 확인했다. [원본·교차 확인·수정 기록](reviews/adversarial/2026-09-07-T-102a.md)에 severity를 보존했다. 실제 완료 us 기반 deadline 판정, terminal NMI reset, 함수 type typedef, 독립 worker/vote 변이시험과 compile database별 stack evidence 대조를 적용했다. board flash/provisioning은 하지 않았다.

초기 Windows Debug/Release·WSL ASan+UBSan은74/74 PASS였고 새 fixture·변이 검출·target gate를 재실행했다. `e928cf6` target 일괄 빌드는 리뷰 결함 수정으로 대체하므로 중단하고 부분 산출물을 최종 증거에서 제외했다. run34064143948의 Windows host는 Doxygen 다운로드 실패였으며 최종 CI로 다시 확인한다. 아직 T-102a DONE/merge가 아니다.

## 2026-09-07 (codex, T-004 merge와 T-102a 시작)

PR #20의 최종 head `182f975`에서 Windows/Linux host·sanitizer와 Windows target CI10건 SUCCESS를 확인하고 merge `caafc24`를 검증했다. 추가로 같은 clean head에서 STM32 Debug/Release·ESP32 네 프로젝트를 새 디렉터리에 모두 clean build해 warning/error0과 동일 ESP version을 확인했다. [merge evidence](reviews/adversarial/evidence/2026-09-07-T-004-merge.md)에 binary hash와 CI 링크를 보존하고 T-004를 DONE archive로 이동했다.

사용자의 재개·core/base 우선 요청에 따라 `codex/t102a-stm32-core-bench`를 만들었다. T-102의 최소 boot/fault image를 T-102a로 분리하되 G1/G2 실측 의무와 기존 후속 task의 선행은 유지한다. Windows PnP 조회에서 ST-LINK/STM32 시리얼 장비를 식별하지 못했으므로 장비 준비 정보를 사용자에게 질문했으며 자동 flash·option-byte 변경은 하지 않는다. embedded architecture/cstyle/documentation/driver/ISR 지침에 따라 SDK 독립 실패 정책·정적 queue·scheduler와 CMSIS backend를 분리한다.

## 2026-09-07 (codex, T-004 구현·검증 closure)

UART 1.0 schema/generator·semantic payload 검증·COBS/CRC stream·plan transaction·command cache·link/session/replay를 구현했다. 적대적 리뷰에서 발견한 stale heartbeat/cache, 방향·auth 혼동, enqueue 전 replay commit, borrowed payload 수명, quota와 staging timeout을 수정하고 재시험했다. 동기 queue copy와 단일 worker 소유권을 공개 계약에 명시했다. 최초·중간 finding과 2인 post-fix 원문은 [T-004 통합 기록](reviews/adversarial/2026-09-07-T-004.md)에 모두 보존했다.

- `3c6967a` 기준 Windows Debug/Release와 WSL Clang ASan+UBSan은 각각 68/68 PASS. UART function100%·line91.24%·branch79.12%, core coverage·schema·strict API docs도 PASS다.
- C `soak-24h`는 양방향 합계 69,120,000,130 byte를 실제 decoder에 입력했고 315,103회 손상 주입과 guard/resync/counter 검사를 801.87초에 통과했다. 이는 4Mbps·8-N-1의 방향당 24시간 byte budget이며 실제 벽시계 24시간·실물 UART 시험이 아니다. Python virtual-time 시험만으로 full-rate를 주장했던 근거는 대체했다.
- STM32 Debug/Release clean build와 ESP32 네 프로젝트 incremental build의 최종 바이너리·SHA-256·warning/error0을 [evidence](reviews/adversarial/evidence/2026-09-07-T-004-validation-final.md)에 기록했다. ESP 버전 문자열은 병행 문서 커밋 때문에 서로 다르며 단일 재현 bundle로 표시하지 않았다.
- B 최종 T-004 PASS, A 최종 조건부 PASS의 physical 문구 구분을 반영했다. 코드 P0/P1은 남지 않았으며 원격 CI 완료 전 merge하지 않는다. PR #20 merge 후 DONE 기록을 갱신한다.
- 최신 사용자 재개 요청에 따라 PR merge 후 후속 task를 계속한다. 실제 UART DMA/RTS/CTS·보드 flash·CAN/RF/HIL·production provisioning·차량은 NOT_RUN이다.

## 2026-09-07 (codex, T-004 UART schema/codec 시작)

T-003 PR #19가 `4ee017b`로 main에 merge된 것을 확인하고 `agent/codex-t004-uart-schema-codec`에서 T-004를 시작했다. T-004는 ESP-NOW tunnel이 아닌 Communicator ESP32↔STM32 내부 UART v1.0 semantic ABI, generated C header, fixed-buffer COBS/CRC codec/parser와 host fault simulation만 다룬다. UART DMA/실물 RTS/CTS, 보드 flash, CAN/HIL, recovery UART는 후속 task와 별도 gate다.

- 직접 설치한 SDK 기준은 유지한다: ESP-IDF `6.0.3` (`C:\cv\esp-idf-6.0.3`), STM32CubeG4 `1.6.3` (`C:\cv\STM32CubeG4-1.6.3`), Arm GNU `15.3.Rel1`, CMake `4.4.3`, Ninja `1.13.2`.
- 먼저 schema→generator→semantic codec→fault/negative test→host/target build 순으로 진행하고, draft PR에 작은 커밋을 원격 반영한다. 생성 ABI drift와 실제 target warning을 별도로 기록한다.

## 2026-09-07 (codex, T-003 ESP-NOW codec/session/QoS closure)

T-002 merge `c18a8a5` 이후 branch `agent/codex-t003-espnow-codec-session`에서 T-003을 구현했다. 사용자 변경은 보존했으며, target SDK를 직접 설치·검증했다: ESP-IDF `6.0.3` (`C:\cv\esp-idf-6.0.3`), STM32CubeG4 `1.6.3` (`C:\cv\STM32CubeG4-1.6.3`), Arm GNU `15.3.Rel1`, CMake `4.4.3`, Ninja `1.13.2`.

- generated ESP-NOW contract/TLV table, byte-safe C codec, session lifecycle cookie·secure binding·anti-replay·pairing/control adapter, fixed pool/rate limiter/QoS1 scheduler와 C/Python reference/fault tests를 추가했다. generated output check와 source/header byte stability gate를 유지했다.
- 최신 local 재검증: host Debug/Release 각각 CTest `49/49`, Coverage core `9/9`와 line/function `100%`, branch `99.63%`, Python unit `35`, generated/negative/budget/plan/link/API docs gate PASS. hash-locked docs dependency는 `tools/requirements-docs.lock` 경로로 strict install PASS했다.
- STM32 Debug/Release와 communicator/diagnostic bridge/controller/public IDF fixture ESP32-S3를 current source commit `6a076a3`에서 clean build했다. 모두 exit code `0`, strict `warning:|error:|CMake Error|ninja: error` scan `0`이다. 결과와 SHA-256은 [target evidence](reviews/adversarial/evidence/2026-09-07-T-003-target-final.md)에 둔다.
- 최초 B 리뷰의 P1은 stale target evidence(`1780e0a`)였다. 전 target image를 재빌드하고 evidence를 `6a076a3`에 bind한 commit `67ccee9`를 push한 뒤 A/B post-fix 재검토가 모두 PASS했다. 원본·disposition은 [T-003 review](reviews/adversarial/2026-09-07-T-003.md)에 보존했다.
- PR #19는 draft 상태에서 시작해 최종 gate 이후 merge한다. board flash/boot, reset/brownout, RF/CCMP·mbedTLS runtime, CAN/HIL·차량, production OTA signing/provisioning은 장비·승인 범위 밖이라 `NOT_RUN`으로 유지한다.

## 2026-09-06 (codex, C99 기반 코드·넓은 시험·생성 API)

기준선 b529a722fb813d5b60ab667675d73996895ea3fc에서 agent/codex-firmware-foundation을 만들었다. 사용자 dirty 변경 없이 시작했으며 기존 v1.2 prototype은 수정하지 않고 별도 host 회귀로 보존했다. embedded-architecture/cstyle/documentation 원칙으로 SDK-free codec/app, BSP/platform, caller ownership, API 오류·수명 계약을 분리했다.

- 세 장치·네 MCU startup/BSP/config와 생성 pin header, strict C99 envelope/CRC/COBS/classic CAN batch/sequence를 추가했다. radio/CAN/OTA 활성화 경로는 없다.
- 사용자가 Doxygen 허용을 명확히 한 뒤 Sphinx9.1.0/Breathe4.36.0/Doxygen1.18.0/Furo2025.12.19와 hash lock을 선택했다. 공개 함수14개 계약 검사 및 warning0 사이트 build를 실행했다.
- 독립 C 표준 자문 Boole(01a074a0-9ee0-7883-83da-4e4a79fbe3ea)은 C17을 권고했으나 사용자 C99 지시를 유지했다. SDK GNU23과 공용 C99를 분리하고 C99 정적 검사를 추가했다. 자문은 2인 코드 리뷰 PASS를 대체하지 않는다.
- 공식 archive SHA256 검증 후 .tools의 Clang23.1.0/CMake4.4.3/Ninja1.13.2/Doxygen1.18.0으로 Windows Debug/Release 각각31/31 CTest를 통과했다. core9그룹·독립 Python/C2195 vector·BSP4종 실패 주입·generator·legacy12회귀·문서/plan gate 포함.
- 새 profile coverage: core 실행line546/546(100%), function20/20(100%), branch295/296(99.66%). BSP/SDK/Python/legacy를 분모에 포함하지 않았다.
- tools/build_docs.py 실제 통과. 작성 중 Doxygen output parent 부재와 Sphinx 함수 pointer 표기 경고를 발견하고 builder 및 명시적인 함수 type typedef로 수정한 뒤 재검증했다.
- tools/environment/setup-windows.ps1 -VerifyOnly: 일반 shell CMake 부재, 고정 host shell에서는 arm-none-eabi-gcc 부재로 실패. target SDK build/실보드/단전/차량 gate는 NOT_RUN이다.
- T-001은 IN_PROGRESS이고 전체 ABI·SDK target CI·HIL acceptance는 열어 둔다. 새 2인 독립 리뷰와 remote CI 결과는 후속 review closure에 기록한다.
- 최초 remote CI에서 Windows cp1252 한글 출력 오류와 GCC 정수 승격 경고를 확인했다. CTest Python -X utf8/host PYTHONUTF8, byte version 손상의 XOR 복원과 기존 brightness 시험 비교 상수 한 곳을 수정했다. v1.2 runtime 동작은 변경하지 않았다. beae8a9의 [CI34009610099](https://github.com/digitie/canview/actions/runs/34009610099)는 Windows 전체 gate와 Linux GCC Release/Clang ASan+UBSan 모두 PASS다.
- 새 .tools/api-venv의 hash-locked 설치가 끝나기 전에 문서 생성을 실행한 한 차례는 Sphinx 미설치로 실패했다. 설치 완료 후 동일 명령의 strict build 및 pip check를 통과했으며 이 초기 순서 오류를 성공으로 집계하지 않았다. 기존 plan validator 부정 fixture35개도 별도로 통과했다.
- 59ac404의 독립 리뷰 A(P2 세 건)·B(P2 두 건) 원문을 각각 보존한 뒤 비교했다. GPIO 검증 finding은 중복이며, 오디오 PLAY16/REC14 교정·SoC/module allowlist·IDF GNU adapter 분리·startup4역할 C99 compile·CAN 전체 필드/독립208 golden을 반영했다. core runtime byte 알고리즘은 변경하지 않았다.
- 수정 후 Windows Debug/Release 각31/31, generator7그룹, envelope/COBS2195+CAN208 vector, coverage546/546·295/296, API strict14계약 PASS. bus 상수0·bus offset 오기·encode/decode 동시 delta endian 반전의 시험용 변이3종은 새 golden gate가 모두 검출했다. 원문에 포함된 임시 worktree 링크는 literal evidence로 보존했다. post-fix 재검토와 최종 CI는 closure report에서 추적한다.

## 2026-09-06 (codex, 전체 계획 두 차례 점검·UI 개선)

기준선 `4aeb2912da063c6fcb0d8715aa46f84c7d1d1b0f`의 사용자 변경을 보존하고 기존 PR16 branch에서 작업했다. 계획/자동화/LVGL/진단 웹을 분리한 작업자와 main의 2차 대조를 수행했다. [요구 추적표](architecture/requirements-coverage.md)에 42개 요구와 46개 상세 task, 남은 실제 gate를 연결했다. embedded-architecture/documentation/cstyle 원칙으로 의미 명령·상태 소유권·ISR/queue 경계·실행되지 않은 gate를 분리했다. Hallmark는 사용하지 않았다.

- 계획: 누락된 OTA8단계·PCB 제작·audio bench/SPORT source task11개, 순환 의존성 제거, schema phase/session/reason·설정 A/B 정본 충돌을 수정했다. 상세 task DONE 승격 없음.
- UI: 운전자/진단 웹 각5뷰, 4WD·순간연비·RPM 옆 보조값·FFT 차속/RPM·signed dBFS·날짜/60개 분·정차잠금·경고 touch-through·미수신/로컬 초안을 개선했다. DPF lamp OFF를 전체 정상으로 표시하지 않는다.
- 자동화: stale/idle 반복 감광·boost base 오염, volume pending·FFT invalid, SPORT stale/수동 mode·tick gap을 수정했다. 수정 전7개 failure의 재현과 수정 후 회귀를 확인했다.

작성자 실제 검증:

| 명령/환경 | 결과 |
|---|---|
| `python -B -X utf8 tools/validate_plan.py` | 상세46·metadata/선행 DAG 오류0 |
| `python -B -X utf8 -m unittest discover -s tests -p test_plan_validation.py -v` | 부정 fixture 포함35시험 PASS; 작성 중 최초0-test 결과는 통과로 집계하지 않음 |
| `node tools/ui/check-browser.cjs --screenshots` (Playwright+Edge, 외부망 차단) | 최초 운전자72검사·진단10그룹, 촬영 전환 상태 회귀 추가 뒤 운전자74검사; JS 오류/외부 요청0; `docs/images/` 재생성 |
| VS Developer PowerShell → `cmake -S tests/automation -B .tools/automation-main-build -G Ninja -DCMAKE_BUILD_TYPE=Debug`, build, CTest | MSVC19.50 `/W4 /WX /utf-8`, 최초11/11·SPORT gap 철회 교차 회귀 추가 뒤12/12 PASS |
| `./tools/ui/validate-lvgl.ps1` | 공식 LVGL8.4.0 `4495f42` 실제 C 링크·수명/상태 Debug 회귀 PASS. 같은 CMake를 `.tools/lvgl-release-build`/Release로 빌드한 assertion-enabled 회귀도 PASS. 최종 변경은 독립 review에서 재확인 |
| Python navigation/hardware unittest | 기존16+9시험 PASS; 실제 RF/전기적 HIL 아님 |
| KiCad10.0.6 Python `tools/hardware/validate_exports.py`, `check_margins.py` | 4보드 export/pad/BOM 정합성 PASS, 저장 ERC0 확인·정적 margin 재검산; PCB/아날로그 승인 아님 |
| `tools/validate_document_links.py`, `git diff --check` | 상대 링크·공백 검사 PASS, 최종 closure 후 재실행 |

일반 PowerShell의 `setup-windows.ps1 -VerifyOnly`는 CMake PATH 부재로 실패했다. 설치된 VS dev shell의 CMake4.2.3-msvc3/Ninja1.12.1은 host 시험에 사용했지만 잠금 target toolchain CMake4.4.3/Ninja1.13.2/Arm15.3.Rel1을 충족했다고 표시하지 않는다. ESP/STM target, OTA runtime·PCB·전원 차단/HIL·Android/iOS 실기기·최종 한글 font/LCD FPS/8시간 soak·실차 evidence는 미실행이다. CodeGraph 미초기화로 `rg`·정본 직접 읽기·compiler/test로 추적했다.

작업자와 별도의 독립 reviewer2명이 동일 immutable `d078437`을 검토했다. 최초 A는 PASS, B는 gap에서 SPORT 소유권 철회 누락(P1)·evidence enum 정본 충돌(P2)·경고 촬영 시점(P3)을 발견했다. 두 원문을 먼저 저장한 뒤 교차 검토했다. gap에서도 fresh 외부 mode 관찰은 권한을 철회하도록 수정하고, UNKNOWN 포함 evidence 등급과 REJECTED 심사 상태를 분리했다. 촬영 문제와 모든 LVGL 탭의 경고 면적도 보완했다.

새 `sport-gap-revocation`은 정상 ECO→SPORT 진입 후 4개 외부 mode×5개 elapsed 경계를 교차한다. 수정 전 assertion 실패가 Windows CRT dialog에서 대기해 해당 시험 process만 종료했고, headless stderr/abort 설정과 CTest 30초 timeout을 추가했다. 수정 뒤20경우와 전체12시험을 통과했다. 최종 post-fix 재검토·원문·finding disposition과 PR 결과는 [새 리뷰 기록](reviews/README.md)에 보존한다. 이 일지나 prototype으로 차량 CAN TX를 허용하지 않는다.

## 2026-09-06 (codex, 독립 OTA·N16R8 회로)

사용자의 N16R8 선택에 따라 WROOM-1-N16R8, 내부 bundle staging, 외장 SPI NOR 미실장을 채택했다. reset/BOOT0·복구 버튼·물리 CAN 차단을 실제 생성 입력과 KiCad 산출물에 반영했다. embedded-architecture/documentation 스킬을 적용해 플랫폼 경계와 전원 차단 수용 조건을 분리했다. 사용자 단일 MD 요청을 우선해 설계와 두 전문 리뷰어 원문·disposition·재검토를 [OTA 문서](architecture/ota.md)에 누적한다.

- 최초 리뷰 P1 두 건(인터록 GPIO 역구동, 승인 전 PREPARED 자동 설치), P2 한 건(영속 downgrade 정책 부재)을 회로/테스트/설계에서 수정했다. 최종 closure는 OTA 문서의 동일 immutable post-fix 재검토가 정본이다.
- KiCad10.0.6 전체 재생성:4보드 ERC0, 정합성·전원 margin·hardware9시험 PASS. CSV 접근 일시 실패 후 전체 재실행 성공.
- ESP target VerifyOnly는 CMake 부재로 실패. OTA target 구현·PCB·전원 차단 HIL·제작 및 차량 송신 승인은 미완료다.

## 2026-09-05 (codex, R1 독립 리뷰 수정)

동일 immutable `06bb51c72180f9c040db3ccf0b223a823c570409`를 전문 reviewer2명이 object-only 방식으로 검토했다. 두 원문을 상호 공개 전에 보존했다. P1 세 건(게이트 DC VGS 정격, PHY rail 소실 시 FT fail-open, USB CC 제어기 VDD 범위)을 BUK7Y12·active-high AHCT126·USB전용3.3V로 수정했다. B의 capability/UNAUTHORIZED payload/헤더 이름과 양쪽의 CRLF 해시 finding도 반영했다.

- Windows KiCad10.0.6 전체 export: ERC0/waiver0,343개 BOM item/1,235 named pad 정합성 PASS.
- Windows Python: navigation16시험, hardware net/Boolean5시험 PASS. 후자는 HIL/아날로그 과도 시뮬레이션이 아니다.
- PDF56개/1,745쪽/95,230,736byte의 크기·SHA·parse 오류0. 기존 원문/land 미확보 gate는 유지한다.
- 생성 text canonical LF와 immutable Git blob hash 검사 경로를 추가했다. 신규 USB회로/FET/FT enable은 기존 footprint 또는 소형 LDO만 사용하며 보드 소형화 우선을 유지한다.
- 수정 기준선의 원 reviewer 재검토와 최종 disposition은 별도 review report에 기록한다. 이 로그만으로 P1 closure나 제작 허용을 선언하지 않는다.

## 2026-09-05 (codex, R1 상세 회로·센서 확장)

**범위**: 사용자가 명확히 선택한 가격보다 소형화 우선 기준으로 네 보드 회로·local footprint·BOM·FW 핀맵과 센서 protocol을 작성했다. 이전 작업자의 KiCad version/Windows 문서·S3 footprint 수정을 유지하고 합쳤다. `embedded-architecture`와 `embedded-documentation` 원칙에 따라 센서 owner, wire 정본, 실측 gate와 후속 task를 분리했다.

**설계**: 자동차/USB-C 전원 mux, automotive-only PHY/GPS, MAX20040 adjustable5.0875V/외부 bootstrap diode, TCAN1046 DYY pin 수정, reset/rail/WD latch 차단, MAX3055 자체 rail을 따르는 TX/EN gate,24개 테스트 패드. MTi7 DR+BMP384 AUX SPI, cased GPS UART/PPS, LVDS 원격 T5848 mic, 기존 Waveshare RTC 재사용. 전원·센서 경계는 ADR-006이다.

**검증**:

- Windows KiCad10.0.6 `tools/hardware/export-review.ps1`: 네 보드 ERC0개, waiver0개, 총333개 BOM item(테스트 패드·DNP 포함)/1,209 named pad의 net·pad·BOM 정합성 PASS. source에서 XML/sexpr/PDF를 재생성했다.
- `tools/hardware/check_margins.py`:5V/supervisor/OV/WD 정적 계산 통과. FB 누설 가정과 빠른 collapse 지연·ripple·SOA 미포함을 명시했다.
- Windows Python `-m unittest discover -s tools/protocol -p test_navigation_codec.py -v`:12개 host 시험 통과. 실제 session allocator/cache·role 검사·RF/STM 통합 구현은 T-100b 후속이다.
- 제조사 PDF54개,1,709쪽,93,272,603byte의 전체 페이지 parse·SHA-256·크기 검사 오류0. 미확보2건과 최신판/land 미확보는 별도 기록했다.
- 회로 PDF의 diode 극성과 global label 방향을 눈으로 확인하고 preview를 실제 export에서 다시 만들었다. native exporter 순서 race는 `Start-Process -Wait`와 독립 netlist 검사로 수정했다.
- CodeGraph의 현재 연결은 다른 프로젝트이므로 사용하지 않았다. `rg`, source/schema 참조, local link 검사와 독립 export 검사로 영향 범위를 확인했다. WSL은 검색·patch·다운로드 보조, 생성/검증/Git은 Windows executable이다.

**남은 조건**: MAX20040 land90-0409 원본 overlay, 구판/미확보 원문, 구매 R/C·harness·PCB/열/loop/SOA·HIL은 미완료다. T-100은 IN_PROGRESS, T-100b는 BLOCKED를 유지한다. 실제 보드·오실로스코프·차량이 없는 상태를 시험 완료로 표시하지 않는다. 전문2인 immutable 적대적 리뷰 결과는 별도 archive에 기록한다.

## 2026-09-05 (codex)

**작업**: 최신 Windows EDA export와 Communicator 회로 산출물 정합성 보정

**변경**:

- KiCad `10.0.6`을 현재 안정 EDA baseline으로 manifest와 문서에 고정하고, `export-review.ps1`로 생성기·XML netlist·ERC JSON·PDF export를 한 번에 재현하게 했다.
- STM32 UFQFPN48 7번 패드 표기를 공식 `PG10-NRST` 이중 기능으로 맞췄다.
- ESP32-S3-MINI-1-N4R2에 S2 footprint를 사용하던 참조를 제거하고, Espressif 공식 S3 land pattern 기반 전용 footprint로 교체했다.
- `09_can_ft`와 `14_can_connectors`를 생성기 호출 순서와 동일하게 분리해 11개 hierarchical sheet, BOM, pinmap, connectivity, schematic, netlist가 같은 입력에서 나오도록 갱신했다.

**검증**:

- Windows KiCad bundled Python과 KiCad CLI `10.0.6`으로 생성·netlist·ERC·PDF export를 실제 실행했다.
- ERC는 23개 violation을 보고했다. 기존 power/isolated-label 및 PCB·SI·transient 미검증 gate가 남아 있어 제작·차량 연결 승인은 아니다.
- ESP-IDF/STM32 target compile은 현재 셸에 해당 host tool이 없어 미실행으로 유지했다.

## 2026-09-05 (codex)

**작업**: 최신 Windows 임베디드 개발환경과 target build bootstrap 구성

**결정**:

- ESP-IDF `v6.0.3`, STM32CubeG4 `v1.6.3`, CMake `4.4.3`, Ninja `1.13.2`, Arm GNU Toolchain `15.3.Rel1`을 manifest에 고정했다.
- ESP-IDF `v6.0.3` peeled commit `76f5dedd9950a3012fee8fb7d5586df21fc67802`, STM32CubeG4 `v1.6.3` peeled commit `d11b194a9f05d1b143d154771f3dbc282c8052a5`을 기록했다.
- 버전 선택과 upgrade 규칙을 [ADR-005](adr/005-latest-windows-embedded-toolchain.md)에 기록했다.

**변경**:

- `tools/environment/setup-windows.ps1`가 Windows host tool version, SDK checkout commit, ESP-IDF export와 핵심 SDK 파일을 검증한다.
- `firmware/controller/`와 `firmware/communicator/esp32/`에 독립 ESP-IDF project, `main`, 향후 public `canview_protocol` component, `sdkconfig.defaults`, partition table를 추가했다. T-002 전에는 incomplete v1.2 header를 application dependency로 연결하지 않는다.
- STM32 CMake minimum/preset/toolchain에서 CMake 4.4, Ninja, Arm GCC 15.3.x를 검증하고 memory usage report를 출력하도록 했다.
- `canview_can`의 private protocol include path를 public `REQUIRES canview_protocol` 경계로 바꿨다.

**검증**:

- `git diff --check` 통과.
- 현재 실행 셸에는 CMake, Ninja, Arm GNU compiler, ESP-IDF와 STM32CubeG4 checkout이 없어 실제 target configure/build는 미실행이다. 따라서 T-200/T-300/T-102 acceptance는 완료로 표시하지 않는다.
- Windows에서 실행할 전체 준비 명령은 [tools README](../tools/README.md)와 [장치별 toolchain](development/toolchains.md)에 기록했다.

## 2026-09-05 (codex)

**작업**: 문서 정보구조 재정립과 실행별 독립 적대적 리뷰 gate 도입

**변경**:

- `AGENTS.md`를 공통 정책·안전 경계·단계별 읽기 규칙의 짧은 정본으로 정리하고, 사용자가 보강한 Ruthless Review 원칙을 유지했다.
- `docs/README.md`를 중앙 router로 추가하고 상세 설계를 architecture·hardware·development·vehicle·UI 하위 디렉터리로 이동했다.
- `SKILL.md`는 정책 사본이 아닌 작업별 문서 router로 축소했다.
- 적대적 리뷰는 매 실행마다 새 report를 만들고, 서로 다른 전문 영역의 reviewer subagent 2명이 같은 immutable 기준선을 독립 검토하도록 workflow와 archive를 정의했다.
- 기존 기준선 리뷰는 `docs/reviews/adversarial/2026-09-04-baseline-design.md`로 이관하고 ADR-004에서 새 정본 관계를 기록했다.

**환경**: branch·status·commit은 Windows Git을 정본으로 사용했다. 대량 상대 링크 경로 수정에는 Windows Python을 찾지 못해 WSL `python3`를 일회성 보조 도구로 사용했으며, 이후 Windows Git diff와 별도 link 검증으로 결과를 확인했다.

**적대적 리뷰**: 서로 다른 전문 영역의 reviewer subagent 2명이 immutable commit `b6f523f`를 독립 검토해 6개 P1, 3개 P2, 2개 P3와 추가 관찰 1개를 보고했다. 수정 commit `ab613c8`에서 두 reviewer가 모든 항목의 해소와 신규 P0/P1 회귀 없음에 동의했다. 두 `CONDITIONAL` verdict의 유일한 조건인 post-fix 결과·disposition 기록은 [통합 report](reviews/adversarial/2026-09-05-document-information-architecture.md)와 별도 evidence로 종결했다.

**검증**: 1차에는 Markdown local link 459개와 fragment 8개를 확인했다. closure 포함 Markdown 89개, local link 476개, fragment 11개에서 오류 0개, 상세 task 파일·요약 link 34/34, 이동 전 경로 잔존 0개, Windows Node `prototype.js --check`, Windows Git `diff --check`를 통과했다. staged 보안 감사 결과는 PR에 남긴다.

## 2026-09-05 (codex)

**작업**: Windows 개발환경과 일회성 worktree 정책 반영 및 embedded-skills 설치

**변경 파일**:

- AGENTS.md, SKILL.md
- docs/development/windows.md
- docs/runbooks/agent-workflow.md, docs/runbooks/agent-failure-patterns.md
- docs/adr/003-windows-development-and-ephemeral-worktrees.md
- docs/adr/README.md, docs/decisions.md, CHANGELOG.md, docs/resume.md

**외부 설치**: `rovinax/embedded-skills` `master` (`022ce31b469b1a1d0c1261c2c8d0f3e07b2c0bbc`)에서 `embedded-architecture`, `embedded-cstyle`, `embedded-documentation`, `embedded-driver-design`, `embedded-isr-design`, `embedded-rtos-design`을 Codex skills 디렉터리에 설치했다.

**결정**: Windows PowerShell과 Windows native 도구를 정본 개발환경으로 삼고, worktree는 병렬·격리·독립 리뷰가 필요할 때만 생성하며 merge 또는 abandon 후 제거한다. WSL/Linux는 보조 환경으로만 취급한다.

**검증**: GitHub 설치 스크립트가 6개 스킬 설치를 완료했다. 저장소 문서의 경로·worktree 표현과 ADR 색인을 갱신했다. Markdown local link 78개, task 상세/요약 34개, host C automation, UI JavaScript syntax 검증을 통과했다. 현재 셸에는 CMake/Ninja가 없어 해당 build gate는 미실행이다.

## 2026-09-05 (codex)

**작업**: kor-travel-geo 문서 운영 구조를 canview에 적용 (문서 구조 task)

**변경 파일**:

- AGENTS.md, SKILL.md, CHANGELOG.md와 .gitignore
- README.md
- docs/architecture/README.md
- docs/architecture/system.md, docs/architecture/implementation-readiness.md, docs/reviews/adversarial/2026-09-04-baseline-design.md
- docs/development/windows.md, docs/runbooks/documentation-maintenance.md, docs/resume.md, docs/journal.md
- docs/adr/, docs/runbooks/
- docs/tasks.md, docs/tasks-rule.md, docs/tasks-done.md, docs/tasks/README.md

**결정**: 열린 task 요약은 docs/tasks.md에 두고, 상세 task는 docs/tasks/ 아래에 하나씩 유지한다. 완료 task는 docs/tasks-done.md로 이동한다.

**발견**: 기존 canview에는 docs/tasks/README.md에만 task 요약이 있었고 AGENTS.md·SKILL.md·ADR·runbook·resume·journal 정본이 없었다.

**다음**: T-001 host toolchain/CI와 T-100 KiCad 회로도·BOM을 병렬 착수한다.
## 2026-09-09 (codex, T-103 STM32 FDCAN capture-only C source)

사용자의 `G1 이전 fw 구현 허용`, `C로 작성`, `완주까지 진행` 지시에 따라 T-500과
T-102 source 선행을 기준으로 T-103을 `IN_PROGRESS`로 시작했다. `canview_stm_fdcan_capture`
module은 3 channel board PHY contract(TCAN1046/TCAN1046/MAX3055), 80 MHz nominal timing
table, classic CAN 0..8 byte validation, timestamp wrap/역행 보호, bounded 64-slot ring,
wire batch, filter reentry와 generic ID inventory를 구현한다. `fdcan_capture` CMSIS adapter는
FDCAN1/2/3 RX FIFO0를 monitor mode로 설정할 수 있지만 TX register/API는 없으며, ISR에서는
raw W1..W4와 TIM2 timestamp만 SPSC ring에 넣고 worker service가 decode/callback/drop 보고를
수행한다. raw ring 포화도 module drop counter로 합산한다.

검증은 pinned Windows Clang/CMake/Ninja에서 focused FDCAN test와 전체 CTest `117/117`,
pinned Arm GNU 15.3.Rel1/STM32CubeG4 1.6.3에서 STM32 Debug/Release ELF/MAP/HEX/BIN,
post-build memory/stack/source-TX gate와 warning/error `0`을 확인했다. 이전 WSL sanitizer
실행은 Windows worktree metadata를 WSL `git`가 해석하지 못해 `python-unit` 10건이
`firmware source digest is missing`으로 실패했으며 sanitizer PASS로 취급하지 않는다.
source commit 후 정상 `.git` clone에서 ASan/UBSan 전체 suite를 재실행한다. 실제 board/HIL,
FDCAN electrical/bitrate/IRQ latency, reset/brownout, CAN analyzer TX-zero, vehicle bus와
provisioning은 장비가 없어 `NOT_RUN`이며 CAN TX는 계속 `NO-GO`다.
