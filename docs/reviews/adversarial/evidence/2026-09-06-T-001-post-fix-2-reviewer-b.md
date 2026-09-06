# T-001 post-fix-2 재리뷰 B 원문 evidence

- 실행 ID: `01a0759c-c50a-7641-a6ae-ebda26e35742` (Lorentz)
- 전문 영역: CI·재현성·evidence integrity
- 기준선: `f447d86`
- 검토 commit: `7e0a8e1`
- 격리 방식: immutable Git object 읽기 전용; untracked evidence는 읽지 않음
- 파일 변경: 없음
- 최종 verdict: `BLOCK`

## 원문 findings

### B-001 — P1 — target CI의 Arm root/provenance 경로 불일치

위치: `.github/workflows/foundation.yml:103-131`

workflow는 archive를 `$root`에 직접 풀고 `$root\canview-arm-gnu-provenance.json`에 기록하지만 다음 단계는 `$root\arm-gnu-toolchain-15.3.rel1`을 `-ArmGnuRoot`로 전달한다. archive root listing은 `.version`, `bin/`, `lib/`, `arm-none-eabi/`이므로 setup이 존재하지 않는 root/marker를 찾다가 실패한다. target job은 STM32 configure/build에 도달하지 못한다.

### B-002 — P2 — provenance marker가 self-attested이며 toolchain 전체와 연결되지 않음

위치: `tools/environment/setup-windows.ps1:57-88`, `tools/environment/install-arm-gnu.ps1:19-30,48-60`

setup은 archive 자체를 재검증하지 않고 marker의 archiveSha256 및 gcc/objcopy/size hash만 신뢰한다. `ld.exe`, `cc1.exe` 등 나머지 파일은 hash 대상이 아니며 marker도 mutable JSON이다.

### B-003 — P2 — 실패한 target run에서 partial artifact가 업로드될 수 있음

위치: `.github/workflows/foundation.yml:175-230`

중간 실패 시 `target-artifacts.json` 전 partial `.bin/.elf/.map/log`가 `if: always()` wildcard로 업로드될 수 있다. `if-no-files-found: error`는 전체 18개 경로의 완전성을 보장하지 않는다. job 자체는 실패하므로 false green은 아니지만 target evidence 무결성 위험이다.

### B-004 — P3 — budget parser evidence 경로가 closed-world가 아님

위치: `tools/check_budgets.py:100-113`

metric/source/중복/malformed는 검증하지만 manifest evidence path와 CLI override를 repository 허용 root로 제한하지 않아 지정 파일이 tracked target artifact에서 왔음을 증명하지 않는다. 기본 evidence는 synthetic fixture다.

## Protocol bootstrap·검증 결과

Controller/Communicator main의 직접 `canview_protocol` dependency는 제거됐고 public fixture만 transitive 경계를 검증한다. protocol header는 v1.2 incomplete draft로 유지된다.

commit/parent, diff, budget/negative/generated/document/plan, PowerShell AST는 통과했다. document link 결과는 153 documents/1012 targets, unittest 35개였다. target build, 원격 CI, artifact upload/download, HIL, 차량 CAN은 미실행이었다.

P1 target archive root regression 때문에 `7e0a8e1`은 BLOCK이다.
