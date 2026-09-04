# T-302 Controller double-buffer UI model과 LVGL adapter

- 상태: `BLOCKED`
- 우선순위: `P1`
- Gate: `G2/G3`
- 선행: `T-005`, `T-301`
- 병렬 가능: `T-303`, `T-304`

## 목표

현재 global LVGL UI를 단일 UI thread와 immutable snapshot 계약에 연결하고 품질 불일치·unsigned FFT·이름 혼동을 제거한다.

## 구현 범위

- typed domain model→display snapshot exhaustive adapter
- two fixed buffers와 atomic revision handoff
- LVGL thread assertion와 stale snapshot drop
- protocol quality/evidence 표시 정책
- speed/RPM/limit/mode/audio/FFT metadata 추가
- `fft_peak/level` signed dBFS와 valid/clipped/calibrated
- `sport_monitor`를 `sport_automation`으로 rename
- 사용하지 않는 평균연비 field 제거
- theme YAML→LVGL/CSS token generator
- host LVGL screenshot/performance harness
- control별 visible/enabled/pending/applied/reason/request-token capability model
- explicit `canview_ui_t` instance create/destroy lifecycle과 duplicate-create 정책
- callback command/event의 borrowed-pointer 수명 명시 및 queue adapter의 즉시 by-value copy

## 수용 기준

- [ ] 4WD→순간연비→DPF→작은 속도/RPM 순서와 면적이 유지된다.
- [ ] Tucson TL 비율의 차량과 네 바퀴 gauge/TPMS가 320×480에서 겹치지 않는다.
- [ ] 메인 과속 overlay는 중앙 크고, 다른 화면은 반투명·touch-through다.
- [ ] SPORT red, NORMAL blue, ECO green이며 불필요한 상태 원이 없다.
- [ ] volume button·임의 sound position·평균연비가 없다.
- [ ] Cabin FFT와 상세 FFT 안에 peak/level이 있다.
- [ ] unverified/stale 값은 숫자로 보이지 않는다.
- [ ] callback/worker에서 LVGL API를 호출하면 test assertion이 실패한다.
- [ ] callback이 받은 stack-backed command pointer를 return 뒤 보관하지 않으며 poison-stack test가 통과한다.
- [ ] create/destroy 100회와 duplicate create에서 orphan LVGL object, callback, timer, heap leak이 없다.
- [ ] capability/scope/정지조건 부재는 제어를 숨기거나 비활성화하고 pending 중 중복 touch는 요청을 추가하지 않는다.
- [ ] ACK만으로 applied가 바뀌지 않고 matching terminal token+feedback에서만 표시가 확정된다.

## 수명 계약

UI public API는 process-global 암묵 singleton 대신 `canview_ui_t *` context를 반환한다. create 실패와 이미 사용 중인 display를 명시적 error로 돌려주며, destroy는 UI task에서 timer/event callback을 해제하고 queue를 drain한 뒤 object를 삭제한다. callback의 command pointer는 호출 중에만 유효하다. asynchronous consumer용 기본 adapter는 callback이 반환되기 전에 고정 queue slot으로 구조체 전체를 복사한다.

## 성능 기준

UI model publish는 20 Hz, LVGL animation은 60 Hz 목표다. frame p95 16.7 ms, p99 25 ms, 50 ms 초과 0.1% 미만이며 model burst 중 heap allocation이 증가하지 않아야 한다.

## 검증

```bash
cmake --build --preset host-debug
ctest --preset host-debug -R lvgl --output-on-failure
python tests/ui/capture_screens.py --all-states
python tests/ui/compare_snapshots.py tests/ui/baseline
python tests/ui/check_frame_budget.py evidence/latest/frame-times.csv
```

## 증거

경고 없음/과속/야간/stale/fault와 모든 화면 screenshot, frame-time histogram, heap watermark를 남긴다.
