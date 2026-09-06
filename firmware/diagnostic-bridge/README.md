# Diagnostic Bridge 기반 프로젝트

ESP32-S3-WROOM-1-N8R2, 8 MiB Flash/2 MiB Quad PSRAM 기준이다.
BSP는 LED를 끄고 GPIO4 복구 버튼을 입력으로 설정한다.
Wi-Fi/ESP-NOW/웹서버/차량 송신은 시작하지 않는다.

고정 ESP-IDF v6.0.3 환경에서 이 디렉터리의 `idf.py build`를 사용한다.
foundation ESP-IDF image binary 생성과 warning/error scan은 통과했지만 실제 보드 flash/HIL은 아직 검증하지 않았다. 공용 절차와 남은 작업은
[펌웨어 기반 문서](../../docs/architecture/firmware-foundation.md)를 따른다.

partitions.csv는 bench factory layout이며 OTA 제품 partition과 호환되지 않는다.
partitions.ota-template.csv는 설계 검토용이고 기본 빌드에서 사용하지 않는다.
