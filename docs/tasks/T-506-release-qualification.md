# T-506 fault, security, soak와 release manifest

- 상태: `BLOCKED`
- 우선순위: `P0`
- Gate: `G6`
- 선행: 기능별 G5 완료
- 후속: 설치 후보 release

## 목표

hardware revision, 세 firmware, protocol, profile, configuration과 evidence를 하나의 재현 가능한 release로 묶고 차량 전원·통신·보안 fault에서 안전 상태를 검증한다.

이 gate는 현재 요청 범위인 engineering prototype/폐쇄시험 설치 적합성까지다. AEC-Q 부품 qualification, PPAP, 법규·EMC 인증과 양산 traceability는 포함하지 않으며, 이를 통과해도 양산품 또는 OEM-grade라고 표시하지 않는다. 양산을 목표로 바꾸면 별도 production qualification program과 task를 먼저 만든다.

## qualification matrix

- slow/fast power cycle, brownout, cold crank, OV/reverse, RF burst rail droop
- STM/ESP/Controller/Bridge reset과 boot loop
- CAN short/bus-off/error passive/TXD stuck, hard gate off
- ESP-NOW loss/replay/spoof/channel mismatch/queue exhaustion
- UART corruption/CTS stall/partial packet
- NVS power loss, key rotation, config migration, RTC invalid
- 24/72시간 soak, temperature 범위와 memory/stack watermark
- UI frame/latency, stale/fault, touch-through warning
- audio/SPORT 기능별 manual override와 rollback

## release manifest

```text
Git commit and dirty=false
hardware revision + schematic/netlist/BOM digest
Controller/Communicator ESP/STM/Bridge image SHA-256
ESP-IDF/Arm/CMake/compiler versions and sdkconfig digest
ESP-NOW/UART protocol version and schema digest
vehicle profile/DBC/evidence manifest digest
build mode and enabled control scopes
test suite IDs, rig revision, pass artifact URL/digest
known limitations and rollback image
```

## 수용 기준

- [ ] 모든 G0–G5 evidence가 manifest digest로 연결된다.
- [ ] production key가 image/repo/log에 없다.
- [ ] Secure Boot, Flash/NVS encryption provisioning과 recovery 절차가 실제 board에서 검증된다.
- [ ] STM32 boot authenticity, protected control-root page와 production SWD/readout lock이 검증되고 service unlock은 root erase+gate-off로 수렴한다.
- [ ] 72시간 soak에 reset loop, unbounded heap loss, control queue loss가 없다.
- [ ] 모든 injected fault에서 unintended CAN frame 0건이다.
- [ ] release image가 승인 profile/hardware에서만 `VEHICLE_TX`로 시작한다.
- [ ] rollback image가 control disabled 상태로 정상 boot한다.
- [ ] 설치·제거·고장 시 사용자 절차가 문서화됐다.

## 검증

```bash
python tests/release/build_manifest.py --check
python tests/hil/run.py --suite release --rig-config private/rig.yaml
python tests/release/verify_artifacts.py release/manifest.json
python tests/security/scan_release.py release/
```

## 실패 처리

한 qualification 항목이라도 실패하면 기능만 조용히 제외해 같은 version을 재사용하지 않는다. 새 manifest와 version을 만들고 관련 gate를 다시 실행한다. 안전 관련 실패는 default build를 `CAPTURE_ONLY`로 되돌린다.
