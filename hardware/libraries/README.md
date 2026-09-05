# 로컬 symbol·footprint

KiCad10.0.6 설치본에서 사용한 패턴을 `CANView.pretty/`에 저장했다. `Family__Name`은 원본 library/name을 보존하며 변경은 library ID 이름뿐이다. 선택 부품에 부족한 TI DYY/DRB, MTi, BMP384, T5848, Murata 패턴은 `tools/hardware/custom_footprints.py`, S3 MINI 전용 패턴은 로컬 파일과 Espressif 도면을 사용한다. S2 패턴으로 대체하지 않는다.

KiCad community library 사본에는 [원문 라이선스](KiCad-LICENSE.md), CC-BY-SA4.0와 전자설계 산출물 예외가 적용된다. 원 저작자는 KiCad community이며 [upstream](https://gitlab.com/kicad/libraries/kicad-footprints)을 참조한다. 이 library collection의 복제본은 프로젝트 자체 라이선스로 재허가하지 않는다. 직접 생성한 package 패턴도 해당 collection에서 같은 조건으로 제공한다. 제조사 PDF의 저작권은 별개다.

`CANView.kicad_sym`은 제조사/설치 library 핀 번호와 electrical type을 기반으로 생성한 기능별 심벌이다. 각 symbol pin을 물리 named pad와 대조하고, global net label로 sheet 사이를 연결한다. 단순 parse/ERC 통과는 제조사 land/paste/공정 검증을 대신하지 않는다. 특히 **MAX20040 U8은 PROVISIONAL**이며 제조사 land90-0409 원본 overlay 전 PCB 제작 금지다.

pad geometry는 모두 로컬에 있지만 선택적3D model 경로는 설치 KiCad 경로를 참조한다. 3D model을 저장했다고 주장하지 않는다. `MP`는 JST 기계 고정 패드 NC, USB 실드 이름은 `SH`, LM74800 EP는 FLOAT, STM32 EP는 GND다. 이 차이는 [validator](../../tools/hardware/validate_exports.py)가 확인한다.
