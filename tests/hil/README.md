# T-500 fault bench와 HIL harness

T-500은 protocol loss, reset, UART fault, CAN load와 safety/resource 경계를
같은 scenario 형식으로 반복 실행하기 위한 공용 harness다. 이 구현은 먼저
하드웨어와 분리된 deterministic host subset과 fail-closed lab adapter 계약을
제공한다. host PASS는 board/HIL PASS가 아니며, 실제 rig가 없으면 `SKIPPED` 또는
`BLOCKED`로 끝난다.

## Scenario 형식

`scenarios/*.yaml`은 외부 YAML dependency 없이 읽을 수 있는 YAML 1.2
JSON-compatible subset이다. JSON은 YAML 1.2의 유효한 표현이므로 Windows와 CI의
parser 차이를 없애면서 `schema_version`, `id`, `suites`, `mode`, `actions`,
`expect`를 동일하게 유지한다. scenario digest는 canonical 내용에서 계산한다.

현재 inventory는 ESP-NOW loss/delay, 독립 reset, UART byte/CTS fault, 세 CAN
channel load, pool/queue exhaustion, stale/revision/profile/lease/hard-gate deny,
duplicate command, result/feedback timeout, brownout, injection/cache overflow,
guardian timeout, SoftAP/observer radio pressure의 12개다.

## 실행

```powershell
python -B tests/hil/run.py --suite host --seed 1 --output build/hil-host
python -B tests/hil/validate_evidence.py build/hil-host --expect-status PASS

# 선택 실행은 validator에 trusted selection을 별도로 전달한다.
python -B tests/hil/validate_evidence.py build/hil-selected `
  --expect-status PASS --expected-scenario brownout

# local rig가 없을 때는 성공으로 승격되지 않는다.
python -B tests/hil/run.py --suite g2-readonly `
  --rig-config tests/hil/rig.example.yaml --output build/hil-g2
python -B tests/hil/validate_evidence.py build/hil-g2 --expect-status SKIPPED
```

실제 lab 설정은 `lab-contract-v1` adapter, 세 CAN channel과 장치 식별자만
공개 metadata로 둔다. serial/port/secret은 private 파일에서만 공급한다.
현재 연결된 lab backend가 없으므로 `available: true`도 실행을 가장하지 않고
`BLOCKED`로 반환한다. 실제 hardware adapter와 G2/G4 결과는 T-101, T-103,
T-104, T-201, T-501, T-503a, T-505, T-508이 각각 소유한다.

## Evidence contract

runner는 report JSON과 scenario별 JSONL을 생성한다. report에는 seed, firmware와
harness source digest, scenario digest, adapter, metrics, physical/HIL 상태가 들어간다.
각 JSONL event에는 monotonic timestamp, 연속 sequence와 byte `log_offset`이
있다. analyzer는 다음 순서로 첫 위반을 보존한다.

JSONL record delimiter는 ASCII LF(`\n`) 하나로 고정한다. JSON 문자열 안의
U+0085/U+2028/U+2029 같은 Unicode line separator는 record를 나누지 않으며,
writer·reader·byte offset 검증이 같은 delimiter 규칙을 사용한다.

1. event 존재·sequence·monotonic timeline
2. `CAPTURE_ONLY` CAN TX 0 및 command allow-list
3. expected event/field
4. map/stack/heap/queue/WCET/latency budget

scenario가 `ordered_events`를 선언하면 action event의 순서와 field subset을
앞에서부터 검증하고, 모든 실행은 `CAPTURE_ONLY` `TX_GATE_STATE`와
`HARNESS_COMPLETE`로 끝나야 한다. `validate_evidence.py`는 trusted host
scenario의 PASS report에 대해 현재 inventory를 deterministic replay하여 event,
metric, seed와 byte length를 report와 대조한다. 따라서 report에 적힌 metric만으로
예산 통과를 주장할 수 없다. selection을 생략한 PASS 검증은 trusted 전체 host
inventory를 요구하며, 선택 실행은 `--expected-scenario` 또는 API의
`expected_scenarios`로 caller가 지정한 trusted 부분집합과 정확히 일치해야 한다.
trusted PASS는 `trusted-replay`, custom FAIL은 구조적 analyzer 결과만 재계산하는
`structural-only`로 표시하며, 후자는 임의 scenario assertion의 증거로 승격하지
않는다. custom scenario의 `expect`는 report에 반영하지 않고 구조적 검사에는
빈 assertion contract를 사용한다. report와 event log는 각각 bounded writer와
validator의 공통 8 MiB report/event 상한을 적용하며, 상한을 넘으면 compact
`BLOCKED` report를 남긴다.

실패 report는 반드시 `first_violation.invariant`와 `log_offset`을 포함한다.
`fixtures/forbidden-can-tx.jsonl`은 capture-only TX 판정이 실제로 실패해야 하는
negative fixture다. positive host scenario에서는 CAN TX event를 생성하지 않는다.

`--suite g2-readonly`는 같은 scenario inventory를 소비하지만 rig 설정이 없거나
하드웨어가 없으면 실행을 거부한다. 따라서 host simulator를 physical evidence로
재사용하지 않는다.
