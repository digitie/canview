# Diagnostic Bridge 기반 프로젝트

ESP32-S3-WROOM-1-N8R2, 8 MiB Flash/2 MiB Quad PSRAM 기준이다.
BSP는 LED5를 끄고 GPIO4 버튼을 입력으로 설정한다. 외부 R14 10k pull-up을 사용하며 버튼은 진단 입력일 뿐 복구/페어링 권한이 아니다.
Wi-Fi/ESP-NOW/웹서버/차량 송신은 시작하지 않는다.

고정 ESP-IDF v6.0.3 환경에서 이 디렉터리의 `idf.py build`를 사용한다.
T-400a에서 [공용 bench core](../docs/esp-core-bench.md)의 boot/health·고정 pool·단일 owner TWDT2초·100ms 주기·USB 진단을 연결한다. Flash8MiB/Quad PSRAM2MiB·ECC 없음의 고정 계약을 검사하고 실패 시 feed를 중단한다. project와 입력 valid/level을 로그에 구분하며 NVS/OTA는 시작하지 않는다. 공용화 최종 검증·리뷰는 [T-400a](../../docs/tasks/T-400a-bridge-core-bench.md)에서 추적한다. 실제 보드 flash/HIL은 아직 검증하지 않았다. 공용 절차와 남은 작업은
[펌웨어 기반 문서](../../docs/architecture/firmware-foundation.md)를 따른다.

partitions.csv는 bench factory layout이며 OTA 제품 partition과 호환되지 않는다.
partitions.ota-template.csv는 설계 검토용이고 기본 빌드에서 사용하지 않는다.
