# T-007 최종 소프트웨어 수용 감사와 증거 closure

- Review ID: 2026-09-19-T-007-final-acceptance
- 종류: 전문 리뷰어 2인 독립 전체 수용 감사
- Candidate: `77b84cf07cd868f0f0b858a00a0ae8c9b3eb9ef4`
- Base: `6cf1b8e57840a27b83c407d1325a92f869cf2f5d`
- Coordinator: 주 작업 에이전트
- 범위: T-007의 다섯 AC, schema/parser/packager/signing/streaming/예산, 실제 target 증거
- 범위 밖: T-204/T-107 Flash writer, T-205 영속 정책, provisioning, physical/HIL
- 결과: 두 정적 verdict CONDITIONAL 원문 유지. 남은77b CI/artifact 조건은 작성자가 확인.
  T-007 소프트웨어 수용 근거 충족, review closure commit의 CI 및 merge 대기.

## 1. 독립 실행과 동일 manifest

| 항목 | A / Maxwell | B / Huygens |
|---|---|---|
| 전문 | C 수명·target·권한 경계 | schema·native·시험·CI |
| Agent | 01a0b784-5d6a-7540-97e8-ac159eb3a828 | 01a0b784-5e5f-7d42-8953-968159b85914 |
| Execution UUID | 4852ebbb-c56c-4ef5-b2f1-eb4e81bcc1e3 | d8856588-8ebd-4b0e-bf86-403566ed842f |
| UTC 시작 | 2026-09-19T06:19:01.2667771Z | 2026-09-19T06:19:03.7447900Z |
| UTC 종료 | 2026-09-19T06:26:47.1203082Z | 2026-09-19T06:28:02.1725099Z |
| Raw | [A](evidence/2026-09-19-T-007-final-acceptance-a.md) | [B](evidence/2026-09-19-T-007-final-acceptance-b.md) |
| Verdict | CONDITIONAL | CONDITIONAL |

양쪽 모두 시작·종료 git object hash를 확인하고 고정 show/diff로만 읽었다.
Detached worktree clean 검증이나 reviewer의 시험 실행을 주장하지 않는다.
핵심 미열람 incomplete는 없다. 파일·행·명령·미검토 범위는 raw에 보존했다.
원본을 각각 저장하고 CRLF 정규화·끝 공백 제외 완전 일치 확인 뒤 교차 비교했다.

```text
T-007 최종 전체 소프트웨어 수용 감사(완료 선언 아님). repo F:/dev/canview-wt/t007-ota-container; PRbase6cf1b8e57840a27b83c407d1325a92f869cf2f5d; currentcandidate77b84cf07cd868f0f0b858a00a0ae8c9b3eb9ef4. 새UUID시작종료UTC/시작종료catfile/revparse,고정object-only show/diff. 파일수정/시험/compile/다른agent/상대새finding조회금지. A C계약/target/소유권 B schema/packager/native/시험·CI증거 관점 독립. 공통동일manifest.
이번목적은하위deltaPASS반복이아니라T007상세의5개수용기준+구현범위각항목(컨테이너schema/parser/CLI/enum,map,bounds,signing연결/golden,streaming소유권/부분입력reset/예산)과사용자actualtargetbuild gate를현재증거에일대일대응해 남은소프트웨어작업의유무를판정하는것. 특정PASS유도금지. 실제빠진요구있으면정확한source/API/실패시나리오/최소수정제시. 없음도구체근거제시. 아직TaskAC미체크이며물리미실행은NOT_RUN허용하지만필수소프트웨어기준축소불가.
현재T007상세의소유권표는architecture§12와T204/T107실제writer·T205영속policy·T508물리qualification을대응했고원A/B가scope충돌FIXED확인. 후속완성품을T007선행으로순환요구하지않되 AC3의사전검증실패/금지target/본문native실패연결증거유지. 새로운stage.c actualC가bodyauth/preflight/floor전begin금지·bodyfeedOK뒤write·전체body뒤nativeverify를강제. storage/nativecallbacks는모형이며실제Flashmap은후속owner. 정상begin/write양성대조와실패0회,정확한targetenum검사,5Cmutant(earlybegin/writeafterreject/nativefailure/hashcontextguard/identityguard). PREPARED/selector API없음이무조건0회PASS증거는아님;정확한이task계약구분검토.
현재경로: schema/cvota-v2.schema.json protocol/schema/ota-container-v2.yaml;shared/ota/src CBOR/envelope/manifest/body/floor/native_stm/native_metadata/stage;tools/ota manifest_json/container/native;tests/ota/* C/Python교차/actualCNG/golden/변이;firmware/platform/esp32s3/ota_crypto,ota_image;commBSPota;IDFcomponent+tests/fixtures/idf-ota-image actualPSAfullreceiverread-only와stage4NULLlink;정상4target회귀(정상OTAowner완료아님).
일반CLI --native는고정officialespsecure5.4/MCUboot2.4cleanpin에outer뒤native검증/metadata-u64대조. ESP sameblockkey/schemeCRCbinding검증도완료. 공개키external/개인키저장없음. 합성golden394310B hash68eb18e10bf35d351c1604500bf85f6e95aa41c6b49477ffbbdacd9477902655,실제ESP SDKBINsigned+syntheticSTM+P256outer. CLI동작직접재실행77b에서NATIVE_SIGNATURES_AND_METADATA_MATCHED/localpolicyNOT_VERIFIED.
작성자current77b최종Debug150/15027.52s Release150/15023.42s,stageASanUBSan27groups. Native/generalCNG/SDKnegative/strictdocsmatrix기존등록유지. stage.c기존cov6/6func91/91lines90/92branches(productionC이후변경없음),현재전체CI35426085834진행중. 569CI35424935438=6/6+target21byteshash/source7hash/logs28warningerror0 audited,manifest707b25dc3d9234cea85e581e4418e91e591309950d628db257d6718cfdadf30b. 82cCI35425622423진행,어느이전CI도77b최종증거로재사용않음. 현재CI/산출물은메인agent가별도확인예정. 따라서판정에서소프트웨어수용충족과최신CI/증거귀속완료를분리.
정적예산sharedREADME323이후 실제SDKDWARFprefix16488/body856/stage896/PSA108, .su자체frames;상한연산과NOT_RUN인전체SDKheap/stack/WCET·후속gateowner분리. 이전A-AC04/B-AUD03정적기록부족원A/BFIXED. 실제runtime/provisioning/Flash/HILNOT_RUN TXNO-GO.
결과한국어완전raw UUID시각/hash/isolation/실제읽은filescommands/미검토범위,5개AC와구현범위각행에근거·충족/미충족/미검증판정,잔여findingP0..P3exactfilelineimpactrecommendation,oldA-AC03(전체parserPSAread-onlytarget조합)및AC3현재판정,최소다음작업,verdictBLOCK/CONDITIONAL/PASS. 기존과거raw를광범위재독하지말고현재정본과요청범위source근거로판단. 핵심범위못읽으면incomplete명시. 단순현재source존재/hostgreen/문서주장만으로전체DONE하지말것.
```

진행 중 양쪽에 같은 추가 정보(77b Linux139/13918.72초, sanitizer 옵션 확인,
strict docs71API/Sphinx PASS)를 전달했다. 상대 finding은 공유하지 않았다.
이후 wait가 짧은 추가 정보 응답만 반환해 이미 작성한 전체 원문을 재전송받았다.
새 실행·판정으로 만들거나 최초 UUID/시각을 바꾸지 않았다.

## 2. Finding과 수용 판단

A/B 모두 신규 P0/P1/P2/P3 없음. 다섯 AC와 구현 범위의 소프트웨어 구성은 충족,
현 candidate의 CI/artifact 귀속만 별도 미검증이라는 독립 결론이다.

| Finding | 상태 | 근거·원 reviewer 확인 |
|---|---|---|
| A-AC-01 / B-T007-AUD-02 P2 | FIXED | 일반 native CLI와 실제 공식 verifier·metadata 연결, 양쪽 최종 확인 |
| A-AC-02 / B-T007-AUD-01 P2 | FIXED | 정본과 후속 writer/policy 책임 대응, AC3 문구 유지, 양쪽 최종 확인 |
| A-AC-03 P2 | FIXED | full parser/body/PSA read-only receiver 연결을 원 A 확인. 아래77b 실제 target 산출물 감사 추가 |
| A-AC-04 / B-T007-AUD-03 P2 | FIXED | 정적 메모리/연산/frame 기록, 전체 실측과 분리, 양쪽 확인 |
| B-T007-AUD-04 P3 | FIXED | [receiver 원 B 확인](2026-09-19-T-007-receiver.md), 현재 구현 구분 |
| B-STAGE-01 P2 | FIXED | [post2 원 A/B 확인](2026-09-19-T-007-stage-post2.md), 양성 대조·reset 전 상태·5개 실제 C mutant |

앞선 최초 BLOCK 보고서를 수정하지 않는다. 개별 native/PSA/receiver finding의
post-fix closure도 기존 report에 보존돼 있으며 이번 전체 감사는 잔여 결함을 찾지 못했다.
P0/P1 미해결 없음. 물리 gate를 finding closure로 대체하지 않는다.

| AC | 소프트웨어 실행 근거 | 판정 경계 |
|---|---|---|
| AC1 | C/Python CBOR·typed·envelope/CNG, 잘못된 identity·서명·깊이·중복·절단 시험 | 충족 |
| AC2 | C/Python differential·body 배치/길이/정렬·overflow·target 시험 | 충족 |
| AC3 | 실제 C stage 정상 callback 양성 대조·거절 후 차단·native 실패·5개 변이, 실제 SDK compile/link | T-007 계약·호출 순서 충족. 실제 Flash 보호/영속 승인 enforcement 아님 |
| AC4 | C uint64·native metadata·일반 native CLI의2^53/u64최대/불일치 시험 | 충족 |
| AC5 | 고정 공식 도구·dependency lock·공개키만 보존한 golden·clean venv/native CLI/CI | 충족. 제품 signing/provisioning 아님 |

## 3. 작성자의 최신 실행·산출물 감사

77b의 [CI35426085834](https://github.com/digitie/canview/actions/runs/35426085834)는6/6 성공이다.
이 run의 target-firmware-images 및 target-firmware-logs를 직접 내려받았다.

- sourceRevision와 expectedSourceRevision:77b 전체 hash 일치.
- ciRun: digitie/canview#35426085834 일치.
- STM32 Debug/Release, ESP Communicator/Bridge/Controller, public/OTA SDK fixture:
  ELF/MAP/BIN 합계21개 길이와 SHA256 모두 일치.
- sourceProvenance7개: 해당 commit 내용의 Windows CRLF byte hash 모두 일치.
- target 로그28개: compiler/linker/CMake warning/error 패턴0건.
- artifact manifest SHA256:
  `4730a7a1b8b361ab4771465e14eb00e4e15028df101e90a962784fa87556a787`.
- 로컬 경로: build/ci-35426085834-target-images 및
  build/ci-35426085834-target-logs/target-artifacts.json.
- Windows Debug150/15027.52초, Release150/15023.42초.
- 별도 Linux Git clone에서 최신77b ASan/UBSan139/13918.72초.
  compile_commands의 address,undefined 옵션 확인. 로그는
  /tmp/canview-stage-repo-Lk67Ca/post2-build.log 및 post2-test.log.
- Strict Doxygen/Sphinx71API PASS: build/t007-final-docs.log.
- 일반 CLI --native 직접 실행:
  NATIVE_SIGNATURES_AND_METADATA_MATCHED394310B; local policy/install NOT_VERIFIED.
- stage 기존 production coverage 함수6/6·행91/91·분기90/92.
  이후77b는 시험만 수정했으며 production C는 동일하다.
  CI의 기존 공용 coverage·SDK config negative·generated drift gate도 통과했다.

감사 명령은 gh run view/download, Get-FileHash/파일 길이 비교,
git show source→UTF8 CRLF SHA256, Select-String warning/error scan이다.
일반 Git/Node 안내 경고까지 모든 CI 로그가 무경고라고 확대하지 않는다.

## 4. 최종 disposition와 남은 절차

두 reviewer의 CONDITIONAL을 PASS로 바꾸지 않는다. 남은 조건이었던77b CI/산출물
귀속을 위 실행으로 충족했다는 것은 coordinator의 증거 결합 판단이다.
신규 코드 수정은 필요하지 않다. 이 closure 기록 commit의 CI를 다시 확인한 후
PR36을 ready/merge하고 origin/main과 merge commit을 확인한다. 그전 Task는 IN_PROGRESS다.

실제 Flash·read-back·보호 map은 T-204/T-107, 영속 floor/journal/selector는 T-205,
총 stack/heap/WCET·전원/HIL은 해당 target 및 T-508에서 검증한다.
Physical/device/Flash/HIL/provisioning/실차는 NOT_RUN, 차량 TX는 NO-GO다.
T-007 수용은 제품 전체 OTA 설치 완료나 차량 release 승인이 아니다.
