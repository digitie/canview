# Communicator 회로 산출물

이 디렉터리는 `R1-review` 제안 회로의 검토 산출물을 보관한다. 현재 상태는 **제작 정본이 아니며 차량 CAN 연결·송신 승인도 아니다**.

## 포함 범위

- `kicad/`: STM32, ESP32, CAN PHY, TX gate, PHY mode, watchdog latch, arm switch 계층 회로도
- `bom.csv`: MPN·footprint·source·DNP·sheet가 포함된 검토용 BOM
- `netlist.xml`: 회로도에서 내보낸 검토용 netlist
- `pinmap.csv`: KiCad symbol pin과 net을 펼친 기계 판독 표
- `connectivity.json`: sheet·component·net 연결 입력과 provenance
- `erc.json`: KiCad ERC 결과
- `preview.png`, `schematic.pdf`: 사람이 검토할 수 있는 export
- `../libraries/`: 이 회로에서 사용하는 symbol·footprint 사본
- `../references/manifest.json`: 부품 원문과 SHA-256 기록. 현재 manifest에는 `stm32g4-hardware` 원문 fetch timeout이 기록되어 있어 reference evidence가 완전하지 않다.

## 현재 gate

`erc.json`에는 아직 power pin 및 isolated label 경고/오류가 남아 있고, PCB·배선·열·SI·전원 transient 검증이 없다. 그러므로 이 산출물의 BOM, netlist, pinmap은 구현 입력과 리뷰 자료일 뿐이다. `T-100`의 ERC waiver, PCB constraints, 전원/보호 계산, 독립 hard TX gate 검증을 통과하기 전에는 주문·조립·차량 연결에 사용하지 않는다.

특히 다음 안전 경계는 회로와 firmware 양쪽에서 다시 확인해야 한다.

- TX permit이 없거나 MCU·rail·watchdog·물리 arm 조건이 사라지면 세 CAN TXD가 recessive/tri-state로 수렴해야 한다.
- CAN1/2의 split termination과 CAN3의 RTH/RTL은 차량 topology를 확인하기 전 DNP variant다.
- ESP32 USB/SWD는 차량 전원과 분리된 service 경로이며, USB·SWD만 연결해도 CAN PHY가 활성화되면 안 된다.

## 재생성 원칙

생성 입력은 `tools/hardware/`의 Python script와 `connectivity.json`에 두며, 회로도·netlist·BOM·pinmap·ERC/PDF를 한 revision으로 갱신한다. 생성 후에는 KiCad 10 GUI/CLI에서 ERC와 netlist를 다시 내보내고 `connectivity.json`, `bom.csv`, `pinmap.csv`와 대조한다. 생성된 산출물을 손으로 고치지 않으며, vendor PDF는 reference evidence로만 사용한다.
