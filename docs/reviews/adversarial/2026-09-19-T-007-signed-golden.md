# T-007 합성 signed golden 적대적 리뷰

- Review ID: `2026-09-19-T-007-signed-golden`
- 종류: 전문 리뷰어 서브에이전트 2인 독립 적대적 리뷰
- Candidate: `bd9a1a6212413674bf35078016356fb90f9f1728`
- Base: `dfe3d8a292442a565cfb9029c2c40860a74b5dff`
- Scope: 합성 golden 생성·검증 Python, 보존 binary/공개키/provenance, CTest/CI 연결과 관련 문서14파일
- 범위 밖: 제품 signing CLI, 정상 OTA owner·영속 policy·target 설치 통합, physical/HIL
- 상태: `CONDITIONAL`, 중복 P2 한 건 수정·원 reviewer 재확인 대기

## 1. 독립성·원문

두 reviewer는 동일한 request와 hash를 받아 commit object-only로 직접 파일을 읽었다.
Git blob의 길이/hash도 직접 확인했으나 compiler·SDK·CNG 시험을 실행하지 않았다.
서로의 finding을 공유하지 않은 상태에서 같은 공백을 발견했다. 원문은 그대로 보존한다.

| 구분 | A | B |
|---|---|---|
| 전문 영역 | 암호·키 경계, CNG wire·false PASS | SDK·provenance, 재현성·CI·근거 |
| Subagent | `01a0b784-5d6a-7540-97e8-ac159eb3a828` | `01a0b784-5e5f-7d42-8953-968159b85914` |
| Execution UUID | `0178e029-d7a6-4d76-a353-33cf34961c4f` | `ce8022d5-1f2d-4b3e-a89a-87c237145fa4` |
| 시작 UTC | `2026-09-19T03:13:05.472Z` | `2026-09-19T03:13:09.3076895Z` |
| 종료 UTC | `2026-09-19T03:16:50.375Z` | `2026-09-19T03:22:01.0168591Z` |
| 실제 hash | candidate/base 일치 | candidate/base 일치 |
| 격리 | object-only, worktree clean 주장 없음 | object-only, worktree clean 주장 없음 |
| 원본 | [A](evidence/2026-09-19-T-007-signed-golden-reviewer-a.md) | [B](evidence/2026-09-19-T-007-signed-golden-reviewer-b.md) |
| 최초 verdict | CONDITIONAL | CONDITIONAL |

## 2. Finding과 disposition

P0/P1/P3는 없다. A-SG-01과 B-P2-01은 동일한 P2다.

- 위치: candidate `tests/ota/test_signed_golden.py:101–115`, `:35–38`, fixture README`:42–44`
- 실패: payload 변이는 내부 SHA256 TLV를 갱신하지 않아 verify 전에 거절되고,
  wrong root hash도 verify 전에 거절된다. 실제 P256 서명이 항상 성공해도 새3사례는
  통과하므로 주석/README의 서명 거절 보장이 과장된다.
- 수정: 기존 hash/root 거절을 유지하고 payload+내부 SHA256 TLV+whole hash를
  갱신하되 원본 P256 서명을 유지한 네 번째 사례를 추가한다. 반환 status와 호출 수
  `3,2,0,3`을 함께 대조한다. 주석/README의 범위도 검사별로 구분했다.
- Owner: Codex / T-007 / 이 checkpoint의 review closure 전 / 2026-09-19.
- Disposition: 로컬 수정·검증 완료, 원 A/B post-fix 확인 전이므로 closure는 열어 둔다.
  defer 또는 risk acceptance로 처리하지 않는다.

## 3. Coordinator 실행 근거

- 최초 candidate 범위: Windows Debug/Release 각각143/143, compiler/linker/CMake build 진단 경고0.
- 수정 후 전체 Debug/Release 각각143/143 재통과. CNG prefix/body12건·native STM4건.
- C native verifier의 마지막 verify 결과를 무시하고 OK로 반환하는 임시 변이 실행파일을
  실제 빌드했다. 새 시험이 `[0,12,12,0] != [0,12,12,12]`로 실패하여 변이를 검출했다.
  repository verifier 자체는 변경하지 않았다. 변이 빌드 로그는 비어 있으며 경고0이다.
- `build/t007-golden-mutant-test.log` SHA256:
  `71351ea9c7637624b0cd917ecf94a9fea602cf28d2afd52cc5f3ae4667a5c4b8`.
- 공식 ESP host RSA10건의 로그 `build/t007-golden-esp-sdk.log` SHA256:
  `97e16161d46e38de92139e9b8a27d7ec7181bea8968914480328dfbc77a6a4b1`.
- 별도 clean checkout에서도 공개키만으로 CNG/SDK golden 검증·byte 재조립을 확인했다.
  CNG executable은 동일한 기존 C 소스의 로컬 빌드 재사용이며 clean 전체 재빌드는 아니다.
- bd9a1a6의 CI35417880529는 host/portability/sanitizer/browser5개 job 통과,
  target job 진행 중이다. 수정 후 candidate와 최종 artifact는 별도로 검증해야 한다.

## 4. 최종 범위와 다음 확인

원 A/B의 수정본 재검토, 새 CI/target artifact 확인을 기다린다. 합성 signed golden은
정상 firmware installation·제품 signing CLI·T-007 전체 수용 기준을 완료하지 않는다.
물리/HIL·device native execution·Flash는 `NOT_RUN`, 차량 TX는 `NO-GO`다.
PR36은 Draft를 유지하며 PR33 한정 waiver를 적용하지 않는다.
