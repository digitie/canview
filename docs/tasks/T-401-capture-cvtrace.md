# T-401 observer, capture와 `.cvtrace` storage

- 상태: `BLOCKED`
- 우선순위: `P1`
- Gate: `G2/G3`
- 선행: `T-203`, `T-400`, `T-103`
- 후속: `T-402`, `T-501`

## 목표

휴대폰 사용자가 코드나 CAN tool 없이 inventory와 bounded 행동 전후 capture를 만들고 self-validating `.cvtrace`로 export하게 한다.

## 구현 범위

- diagnostic lease와 observer config state machine
- ID/DLC/rate/period/change-mask inventory
- pretrigger ring과 ARM/START/STOP/CANCEL lifecycle, 별도 EVENT_MARKER 경로
- marker local/remote time와 uncertainty
- accepted/drop/gap/part completeness tracking
- PSRAM ring, short internal storage, optional SD abstraction
- `.cvtrace` v1 schema, writer, reader, validator, corruption recovery
- safe importer: fixed path allow-list, traversal/symlink 금지, entry/size/ratio quota
- bundle digest를 참조하는 별도 maintainer-signed evidence approval
- storage quota와 oldest-complete capture cleanup
- 4 KiB `CVJB` append journal, `CVFRAME1`/24-byte record와 finalize-last ZIP transaction
- free-byte admission 공식, emergency reserve와 no-SD 200 record/s 180초 상한

## container 계약

zip 안에 `manifest.json`, `frames.bin`, `markers.jsonl`, `inventory.json`, `candidates.json`, `README.txt`를 둔다. manifest가 각 entry SHA-256, record count, device/boot/session/profile/DBC/filter revision, time uncertainty와 drop/gap을 가진다.

## 수용 기준

- [ ] HTTP 202/ACK/recording/complete가 별도 상태다.
- [ ] power loss 중 capture가 complete로 오인되지 않는다.
- [ ] missing/corrupt entry와 digest mismatch를 validator가 거부한다.
- [ ] zip slip, symlink, duplicate path, zip bomb/고압축 fixture가 제한 전에 extract되지 않고 거부된다.
- [ ] self-consistent하지만 승인 signature가 없는 bundle이 VERIFIED 입력으로 사용되지 않는다.
- [ ] pre/post window와 marker ordering이 monotonic/time uncertainty를 보존한다.
- [ ] marker는 `CAN_EVENT_MARKER` 한 경로로만 기록되고 duplicate token/marker ID는 멱등 처리된다.
- [ ] Bridge lease 만료/reboot에서 filter 변경 권한을 잃고 vehicle control에는 영향이 없다.
- [ ] raw flood에서 P0/P1과 Controller telemetry reserve가 유지된다.
- [ ] storage cleanup이 active 또는 pinned capture를 지우지 않는다.
- [ ] journal의 모든 block write 경계 power cut에서 last valid CRC block까지만 partial/GAP로 복구된다.
- [ ] required-byte preflight 실패 capture는 쓰기를 시작하지 않고 허용 capture는 metadata까지 완주한다.
- [ ] incomplete export는 `partial=true`이며 VERIFIED evidence로 승격되지 않는다.

## 검증

```bash
python -m pytest -q tests/cvtrace
python tools/validate_cvtrace.py tests/fixtures/cvtrace/valid.cvtrace
ctest --preset host-sanitize -R capture --output-on-failure
python tests/hil/capture_powerloss.py --iterations 500
```

## 개인정보·증거

public fixture는 VIN, 위치, MAC, installation ID를 제거한다. 원본 capture는 승인된 private storage에 두고 manifest의 opaque evidence ID와 SHA-256만 commit한다.
