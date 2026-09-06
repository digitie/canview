# Communicator R1 회로 산출물

이 디렉터리는 `R1-review` 제안 회로의 검토 산출물을 보관한다. 현재 상태는 **제작 정본이 아니며 차량 CAN 연결·송신 승인도 아니다**.

## 포함 범위

- [kicad/communicator.kicad_sch](kicad/communicator.kicad_sch): 전원/USB, STM32/ESP32, CAN PHY/gate, MTi7/BMP384, GPS, 24개 테스트 패드의 28개 상세 sheet와 root
- `bom.csv`: MPN·footprint·source·DNP·sheet가 포함된 검토용 BOM
- [netlist.xml](netlist.xml), [communicator.net](communicator.net): KiCad가 실제 내보낸 XML/sexpr netlist
- `pinmap.csv`: KiCad symbol pin과 net을 펼친 기계 판독 표
- `connectivity.json`: source에서 생성된 sheet·component·net 연결 모델과 provenance
- `erc.json`: KiCad ERC 결과
- `preview.png`, `schematic.pdf`: 사람이 검토할 수 있는 export
- `../libraries/`: 이 회로에서 사용하는 symbol·footprint 사본
- `../references/manifest.json`: 부품 원문과 SHA-256 기록. 현재 manifest에는 `stm32g4-hardware` 원문 fetch timeout이 기록되어 있어 reference evidence가 완전하지 않다.

## 현재 gate

KiCad10.0.6에서 **ERC0개와 named-pad/net/BOM 정합성 검사 통과**다. [검증 JSON](../validation.json)을 참조한다. MAX20040 U8 footprint는 제조사 land 원본 대조가 미완료인 PROVISIONAL이며, PCB·열·SI·전원 transient/SOA·HIL 검증도 없다. [제작 전 조건](../../docs/hardware/r1/verification.md)을 닫기 전 주문·조립·차량 연결에 사용하지 않는다.

특히 다음 안전 경계는 회로와 firmware 양쪽에서 다시 확인해야 한다.

- TX permit이 없거나 MCU·rail·watchdog·물리 arm 조건이 사라지면 세 CAN TXD가 recessive/tri-state로 수렴해야 한다.
- CAN1/2 split termination은 DNP, CAN3 RTH/RTL4.7k는 FIT 출발값이다. 검증된 ISO11898-3 버스에서만 사용할 수 있다.
- ESP32 USB/SWD는 차량 전원과 분리된 service 경로이며, USB·SWD만 연결해도 CAN PHY가 활성화되면 안 된다.

## 재생성 원칙

정본 입력은 `tools/hardware/*_circuits.py`다. `connectivity.json`도 생성물이므로 수동 편집하지 않는다. 네 보드의 회로도·netlist·BOM·pinmap·ERC/PDF를 함께 갱신한다. [FW 핀맵](../../docs/hardware/r1/firmware-pinmap.md)과 [전원/CAN 설명](../../docs/hardware/r1/communicator-circuit.md)을 사용하고, [이전 pin proposal](history/pinmap-pre-r1.csv)은 오류가 포함된 이력이므로 구현에 사용하지 않는다. Windows 저장소 루트에서 실행한다.

```powershell
. .\tools\hardware\export-review.ps1
```

이 스크립트가 `build_schematics.py`를 KiCad `10.0.6` bundled Python으로 실행한 뒤 `kicad-cli`로 XML netlist, ERC JSON, PDF를 내보낸다. 생성된 산출물을 손으로 고치지 않으며, vendor PDF는 reference evidence로만 사용한다. `ESP32-S3-MINI-1`은 S2 footprint를 재사용하지 않고 공식 S3 land pattern을 vendored footprint로 사용한다. STM32 UFQFPN48의 7번 패드는 `PG10-NRST` 이중 기능이며 `SYS_RESET_N`에 연결한다.
