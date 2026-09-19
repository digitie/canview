# 합성 OTA 서명 golden

[T-007](../../../docs/tasks/T-007-ota-container.md)의 고정 byte열 교차 시험이다.
제품 firmware 배포물이나 설치 가능 판정이 아니다. Communicator 역할의 실제 SDK
ESP 이미지와 합성 STM payload를 함께 서명해 보존한다. 개인키는 메모리에서 생성한 뒤
폐기했으며 보존 파일은 공개키, 서명된 컨테이너, digest/provenance뿐이다.

## 생성과 재현

ESP 입력은 [SDK fixture](../idf-ota-image/README.md)의 실제 ESP-IDF6.0.3 빌드다.
현재 보존본은 `624848a`의 fixture 소스로 만든 로컬 BIN을 사용했다.
그 BIN에 espsecure5.4.0의 공식 Secure Boot v2 서명 생성·검사를 적용했다.
RSA3072/PSS salt32를 사용한다. STM은 공식 MCUboot imgtool v2.4.0의
512B header·protected metadata168B·P256 서명이며 payload는 `bytes(range(256))`다.
별도의 manifest P256 키로 정규 CBOR를 서명하고 기존 컨테이너 조립기를 사용한다.
정확한 SDK commit, 공개키/컨테이너 크기·SHA256와 입력 BIN SHA256은
[provenance.json](provenance.json)에 있다.

```powershell
# 기존 OTA 시험용 가상환경. 제품 SDK 환경을 변경하지 않는다.
python -m pip install --only-binary=:all: --require-hashes -r tools/requirements-ota.lock
python -m pip install esptool==5.4.0
$env:IDF_PATH = 'C:/cv/esp-idf-6.0.3'
$env:MCUBOOT_ROOT = 'C:/cv/mcuboot-2.4.0'
python -B tests/ota/generate_signed_golden.py build/idf-ota-image/canview_ota_image_sdk_probe.bin build/new-signed-golden
```

생성기는 SDK commit/clean 상태·고정 descriptor를 검사하고 새 디렉터리만 허용한다.
시험 개인키는 직렬화하지 않는다. 재실행하면 새 랜덤 키/RSA salt/ECDSA 서명 때문에
새 golden digest가 나오므로 이를 byte 재현으로 주장하지 않는다. 재현 gate는 **보존된
서명과 native image를 사용해 조립한 컨테이너의 정확한 byte 일치**다.
golden을 변경할 때는 binary·공개키·provenance를 함께 검토해야 한다.

## 검증

```powershell
python -B tests/ota/test_signed_golden.py --body-probe build/host-debug/canview-ota-body-crypto-probe.exe --stm-probe build/host-debug/canview-ota-native-stm-probe.exe
. C:/cv/esp-idf-6.0.3/export.ps1
python -B tests/ota/test_signed_golden.py --esp-sdk
```

Windows Debug/Release CTest의 `ota-signed-golden`은 공개키만으로 digest·outer P256·
u64·재조립, CNG C prefix/body12건, 공식 imgtool 정상 검증과 CNG native STM3건을
검사한다. STM 변이는 whole hash를 다시 계산해도 native 서명에서 거절돼야 한다.
target CI의 `ota-signed-golden-esp`는 공식 espsecure로 RSA 정상/잘못된 키/절단/
본문·descriptor·서명 변조10건을 검사한다. 빌드 job에서 실행하지만 **host 암호 실행**이다.

native STM payload는 부팅 코드가 아니다. 정상 OTA owner, 영속 policy, 제품 signing
CLI, 장치 native 실행·Flash·rollback·physical/HIL은 이 fixture로 완료되지 않는다.
physical/HIL은 `NOT_RUN`, 차량 TX는 `NO-GO`다.
