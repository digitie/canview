# Foundation bench 예산

이 표와 [machine manifest](foundation.yaml)는 같은 지표 집합을 표현한다. `foundation.yaml`은 외부 YAML 패키지를 요구하지 않도록 JSON 문법을 사용하는 YAML 파일이다. 실제 target map·`.su`·runtime 측정값으로 교체하기 전에는 아래 파일을 synthetic evidence로만 취급한다.

| 지표 | 예산 | 단위 |
|---|---:|---|
| `flash_used_bytes` | 262144 | bytes |
| `ram_used_bytes` | 65536 | bytes |
| `stack_max_bytes` | 8192 | bytes |
| `boot_to_safe_state_ms` | 250 | ms |
| `control_round_trip_ms` | 100 | ms |

검사는 `python -B tools/check_budgets.py`로 실행한다. 예산 초과, evidence 누락, 표와 manifest의 불일치는 실패한다.
