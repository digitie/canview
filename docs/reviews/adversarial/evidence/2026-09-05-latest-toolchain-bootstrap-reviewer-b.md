# Reviewer B 원본 결과

- Review ID: `CV-20260905-ENV-01-B`
- 실행 ID: `01a06f14-9cc5-7391-8f10-951301a6d511`
- 전문 영역: protocol·재현성·문서·보안
- 기준 commit: `bfdd2c2d9242840261c85abfa73f051acfc54c97`
- parent: `783a36f31c080687377bf5844ea3c3f0983df8bf`
- 실행일: 2026-09-05 (세부 wall-clock은 orchestrator가 제공하지 않음)
- 작업 방식: read-only commit review, 파일 변경·commit 없음

## 전달한 요청

`bfdd2c2`와 parent를 비교해 protocol/component integration, reproducibility, version-source evidence, PowerShell verification, CMake preset semantics, partition/config drift, documentation links/latest claims와 secret/security leakage를 적대적으로 검토하고 P0–P3 finding을 file/line, failure scenario, impact, disposition과 함께 제출한다.

## 결과

### P1 — pinned SDK commits are tag-object hashes

manifest의 ESP-IDF `06e31f0c...`와 STM32CubeG4 `64d78dd7...`는 tag object이고 checkout commit은 각각 `76f5dedd...`, `d11b194a...`다. clean setup가 실패한다.

### P2 — `-VerifyOnly`가 SDK commit을 검증하지 않음

최초 script는 verify-only에서 `idf.py`와 STM32 header 존재만 확인했다. 임의 remote/commit이 통과할 수 있으므로 remote, exact commit, tag와 submodule을 항상 검증해야 한다.

### P2 — CMake/tool selection drift

export 전 도구만 확인하고 preset은 Ninja를 PATH에서 찾았다. export 후 revalidation과 executable path pinning, compiler exact version 검사가 필요하다.

### P2 — incomplete v1.2 protocol header의 public application 연결

현재 header는 T-002 전의 incomplete v1.2인데 scaffold main이 이를 소비해 v1.3 통합처럼 보일 수 있다. v1.3 schema/generated artifact가 준비될 때까지 component를 application dependency로 연결하지 않아야 한다.

### P3 — target build 문서 불일치

Windows 정책인데 Communicator/Diagnostic Bridge 절에 Bash와 `/dev/ttyACM*`가 남았고 Diagnostic Bridge directory는 아직 없다. PowerShell 명령과 future-only 표기가 필요하다.

## 확인 항목

- 기준 commit과 parent ancestry 확인
- local Markdown link 506개, 오류 0개
- partition arithmetic와 4 MiB/16 MiB 경계 확인
- secret, VIN, private key, credential, DSN, API token 미발견
- 공식 release/version page와 문서 URL 확인
- target build는 review 환경 도구 부재로 미실행
