# CANView 문서 유지보수 Runbook

이 문서는 문서 종류별 책임, 갱신 조건, 이동 검증을 정의한다. 작업 시작 순서와 branch·review·PR 절차는 [agent workflow](agent-workflow.md), 분야별 문서 선택은 [문서 지도](../README.md)를 따른다.

## 1. 문서 간 책임

| 문서 종류 | 답해야 하는 질문 | 넣지 않는 내용 |
|---|---|---|
| `AGENTS.md` | 모든 에이전트가 반드시 지킬 짧은 정책과 금지선은 무엇인가 | 장치별 설치 명령, 긴 설계 근거, 과거 작업 로그 |
| `docs/README.md` | 필요한 상세 문서를 어디서 찾는가 | 상세 설계의 복제 |
| `docs/architecture/` | 현재 구현이 따라야 할 구조·책임·계약은 무엇인가 | 작업 순서, 시간순 변경 로그 |
| `docs/adr/` | 구조적 선택을 왜 했고 무엇을 대체했는가 | 현재 설계 전체의 중복 설명 |
| `docs/tasks/T-NNN-*.md` | 한 작업의 scope, 선행 조건, 산출물, acceptance, gate는 무엇인가 | 제품 전체 설계 재정의 |
| `docs/runbooks/` | 반복 가능한 작업 절차는 무엇인가 | 제품 기능·wire contract 결정 |
| `docs/reviews/` | 특정 immutable 기준선에서 무엇을 검토했고 어떻게 disposition했는가 | 현재 요구사항의 정본 |
| `docs/resume.md` | 지금 상태와 다음 한 작업은 무엇인가 | 긴 구현 이력과 상세 설계 |
| `docs/journal.md` | 언제 어떤 작업을 어떤 근거·명령으로 수행했는가 | 현재 정책을 대신하는 지침 |
| `CHANGELOG.md` | 사용자에게 보이는 변경은 무엇인가 | 내부 구현 일지 |

같은 규범을 여러 문서에 복제하지 않는다. 상위 문서는 한두 문장으로 책임을 요약하고 상세 정본에 링크한다.

## 2. 갱신 판단

변경과 직접 관련된 문서만 갱신한다.

| 변경 결과 | 필수 갱신 |
|---|---|
| 구조·권한·wire contract 결정 | 관련 architecture, 새 ADR, ADR index |
| 구현 범위·상태·acceptance 변화 | 해당 상세 task와 `tasks.md` |
| 현재 차단 조건·다음 작업 변화 | `resume.md` |
| 재현 명령·실패·handoff 정보 발생 | `journal.md` 최신 항목 |
| 사용자 가시 기능 변화 | `CHANGELOG.md` |
| 적대적 리뷰 실행 | 새 review report와 review index |
| 파일 이동·정본 재분류 | `docs/README.md`, incoming link, 관련 ADR |

코드·protocol·회로·UI가 바뀌었는데 architecture나 task가 현재 상태와 다르면 구현은 완료되지 않은 것이다. 반대로 사용자 가시성이나 현재 상태를 바꾸지 않은 내부 수정 때문에 모든 기록 파일을 형식적으로 수정하지 않는다.

## 3. Architecture와 ADR

- architecture 문서는 현재형으로 유지하고 이전 설계의 긴 연혁을 쌓지 않는다.
- 되돌리기 어렵거나 여러 subsystem에 영향을 주는 결정은 `docs/adr/NNN-<slug>.md`에 하나씩 기록한다.
- 결정을 뒤집을 때 옛 ADR을 삭제하지 않고 새 ADR에서 `Supersedes`를 명시하며 옛 ADR은 `Superseded`로 표시한다.
- `docs/adr/README.md`와 `docs/decisions.md`의 번호·상태·다음 번호를 함께 갱신한다.
- 세부 수치나 필드가 machine-readable schema에서 생성되면 schema를 정본으로 두고 architecture에는 의미와 불변 조건만 남긴다.

## 4. Task, resume와 journal

- 상세 task는 [task 규칙](../tasks-rule.md)에 따라 파일 하나가 독립적으로 구현 가능한 수준이어야 한다.
- `tasks.md`는 요약·의존성·상태만 두고 상세 acceptance를 복제하지 않는다.
- 완료된 task는 `tasks-done.md`로 이동하되 evidence와 최종 gate 결과를 보존한다.
- `resume.md`는 현재 상태, 다음 한 작업, 시작 파일, 검증 명령, 차단 조건만 짧게 유지한다.
- `journal.md`는 최신 항목을 위에 추가한다. 기존 기록은 사실 오류 correction 외에는 다시 쓰지 않는다.
- journal에는 도구 fallback, 미실행 검증, 사용자 변경 보존 여부처럼 다음 작업자가 재현에 필요한 사실을 남긴다.

## 5. 누적 적대적 리뷰

적대적 리뷰는 실행마다 `docs/reviews/adversarial/YYYY-MM-DD-<scope>.md`를 새로 만든다. 기존 report에 다음 리뷰 결과를 추가하지 않는다.

1. 두 전문 리뷰어의 독립 finding과 기준 commit을 새 report에 기록한다.
2. 수정·ADR·task·기각 근거와 재검증 결과를 같은 report의 disposition에 연결한다.
3. `docs/reviews/README.md` 표 맨 위에 report를 추가한다.
4. 과거 report의 링크나 사실 오류를 고칠 때는 correction note와 날짜를 남긴다.

리뷰 운영과 merge gate는 [agent workflow](agent-workflow.md), 형식은 [review archive](../reviews/README.md)와 [template](../reviews/adversarial/TEMPLATE.md)이 정본이다.

## 6. 문서 이동과 이름 변경

1. 목적 디렉터리가 문서 지도에서 의미가 명확한지 확인한다.
2. Git 이력을 보존하도록 `git mv`를 사용한다.
3. Markdown link뿐 아니라 backtick·plain text로 적힌 옛 경로를 전체 저장소에서 검색한다.
4. 이동한 문서 서두에 상위 인덱스와 자신의 책임을 짧게 밝힌다.
5. 상대 링크는 새 파일 위치를 기준으로 다시 계산한다.
6. local link 검사와 `git diff --check`를 실행한다.
7. 정본 관계가 바뀌었다면 ADR로 남긴다.

검색 예시는 다음과 같다.

```powershell
rg -n "old-name\.md|docs/old/path" -g "*.md"
git diff --check
```

링크 label도 실제 파일명과 역할에 맞게 고친다. target만 새 경로이고 label은 옛 경로인 상태를 남기지 않는다.

## 7. 작성 원칙과 최종 점검

- Markdown/RST 본문은 한글로 쓰고 공식 필드명, 코드 식별자, 명령어, URL은 필요한 경우 원문을 유지한다.
- 사실, 후보, 추정, 미검증 상태를 구분한다.
- 긴 문서에 목차를 중복 생성하기보다 상위 인덱스에서 필요한 문서로 라우팅한다.
- 외부 원문은 공식 URL과 version, commit 또는 document revision을 기록한다.
- 비밀, VIN, raw 위치정보, 개인 capture와 내부 접속정보를 문서에 넣지 않는다.

완료 전 다음을 확인한다.

- 문서 지도와 실제 트리가 일치한다.
- 이동 전 경로와 깨진 local link가 없다.
- architecture, ADR, task, review의 역할이 뒤섞이지 않았다.
- `tasks.md`의 링크 수와 실제 상세 task 수가 일치한다.
- staged diff에 의도하지 않은 user 파일이나 민감정보가 없다.
