# Diagnostic Bridge 기반 프로젝트

ESP32-S3-WROOM-1-N8R2, 8 MiB Flash/2 MiB Quad PSRAM 기준이다.
BSP는 LED5를 끄고 GPIO4 버튼을 입력으로 설정한다. 외부 R14 10k pull-up을 사용하며 버튼은 진단 입력일 뿐 복구/페어링 권한이 아니다.
`main/app_main.c`는 read-only NVS credential이 있을 때만 `esp_http_server` 기반 local SoftAP/DNS/web shell을 시작한다. GPIO4를 3초 hold해야 service window가 열리며, web API의 capability는 항상 `control_scope=0`, `vehicle_tx=false`다. ESP-NOW observer·capture·Signal Lab과 차량 송신은 아직 구현하지 않는다.

고정 ESP-IDF v6.0.3 환경에서 이 디렉터리의 `idf.py build`를 사용한다.
T-400a에서 [공용 bench core](../docs/esp-core-bench.md)의 boot/health·고정 pool·단일 owner TWDT2초·100ms 주기·USB 진단을 연결한다. Flash8MiB/Quad PSRAM2MiB·ECC 없음의 고정 계약을 검사하고 실패 시 feed를 중단한다. credential은 NVS에서 읽기만 하며 기본 PIN/password를 만들지 않는다. web shell의 owner·주기·상한·인증·남은 gate는 [web-shell 문서](docs/web-shell.md), 공용 절차와 남은 기반 작업은 [펌웨어 기반 문서](../../docs/architecture/firmware-foundation.md)를 따른다.

현재 target `idf.py build`는 BIN/ELF/MAP 생성까지 확인했지만 실제 보드 flash/HIL은 검증하지 않았다. G1 물리 gate, AP association, phone browser, production security provisioning과 차량 CAN evidence는 `NOT_RUN`이다.

partitions.csv는 bench factory layout이며 OTA 제품 partition과 호환되지 않는다.
partitions.ota-template.csv는 설계 검토용이고 기본 빌드에서 사용하지 않는다.
