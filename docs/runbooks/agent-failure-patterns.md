# CANView 반복 실패 패턴과 복구

| 증상 | 먼저 확인할 것 | 복구 |
|------|----------------|------|
| CMake가 target을 찾지 못함 | CMake version, Ninja, STM32CubeG4_ROOT | `docs/development/windows.md` 기준으로 도구와 pinned dependency 확인 |
| STM32 build가 헤더를 찾지 못함 | CubeG4 경로와 device package | 환경변수만 고치고 성공 처리하지 말고 미실행 gate 기록 |
| ESP-IDF target 설정이 Controller와 섞임 | esp32s3, Flash/PSRAM, sdkconfig | Controller와 Communicator 설정을 별도 유지 |
| KiCad ERC가 power pin을 경고함 | 실제 symbol pin, PWR_FLAG, rail ownership | 데이터시트·symbol library 대조 후 waiver 근거 작성 |
| netlist와 BOM이 달라짐 | schematic revision, MPN, DNP/variant field | schematic에서 BOM을 재생성하고 consistency script 실행 |
| CAN3가 이상 동작함 | MAX3055 bus type/bitrate/WAKE/ERR | 확인 전 channel disabled, CAPTURE_ONLY 유지 |
| UART frame이 간헐적으로 깨짐 | 4 Mbps timing, RTS/CTS polarity, DMA lifetime | logic analyzer와 malformed-frame test를 함께 확인 |
| CAN 신호가 그럴듯하지만 불확실함 | DBC commit, capture provenance, freshness | candidate/evidence로 유지하고 정상 UI·safety 승격 금지 |
| UI가 오래된 값을 표시함 | signal age, boot epoch, pending 상태 | stale 표시와 fail-safe default를 적용 |
| protocol 변경 후 일부 장치만 동작함 | schema version, generated header, golden vector | 수동 header 수정 대신 generator와 모든 codec을 함께 갱신 |
| queue가 가득 참 | priority별 quota와 backpressure | raw observer부터 drop, safety/control은 BUSY/fail-closed |
| branch merge 후 문서 링크가 깨짐 | docs/tasks.md 이동에 따른 상대 경로 | Markdown local-link 검사와 root README 링크를 함께 확인 |
| Windows와 WSL에서 결과가 다름 | `F:/...`와 `/mnt/f/...` 경로·도구·개행 차이 | Windows checkout을 정본으로 삼고 표준 검증은 Windows native 도구로 재실행 |
| 임시 worktree가 남아 있음 | merge/abandon 후 cleanup 누락 | 활성 프로세스와 미커밋 변경을 확인한 뒤 `git worktree remove`와 `git worktree prune` 실행 |

같은 실패가 반복되면 새 task로 분리하고, 원인·재현 명령·복구·남은 위험을 이 문서 또는 task에 추가한다.
