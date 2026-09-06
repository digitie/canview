CANView 기반 C API
==================

이 사이트는 현재 구현된 MCU 독립 코드의 호출 계약이다.
전체 제품 프로토콜, 실제 무선 연결, 차량 제어, OTA 구현 완료를 뜻하지 않는다.
설계 정본은 저장소의 `기반 구조 문서 <https://github.com/digitie/canview/blob/main/docs/architecture/firmware-foundation.md>`_ 이다.

호출 규칙
---------

* 모든 context와 버퍼는 호출자가 소유한다. heap 할당과 내부 전역 상태는 없다.
* codec과 stream parser는 단일 worker에서 호출한다. ISR은 후속 adapter의 queue에 바이트/완료 사실만 전달한다.
* encode 입력과 출력은 겹치면 안 된다. decode view는 복사본이 아니며 입력 또는 stream의 다음 수정까지만 유효하다.
* encode 실패 시 유효한 written은 0이다. 출력 바이트 일부는 변경될 수 있으므로 송신하면 안 된다.
* CRC는 전송 손상 검출이며 인증이 아니다. envelope 성공만으로 ACK, dispatch, lease 또는 CAN 송신을 허용하지 않는다.
* sequence window는 인증 및 queue admission 뒤 commit한다. 사전 확인에는 context 복사본을 사용하고 session 변경 때 초기화한다.

연동 순서와 예제
------------------------------

.. code-block:: c

   canview_uart_stream_t stream;
   canview_wire_view_t view;
   canview_status_t status = canview_uart_stream_reset(&stream);
   if (status != CANVIEW_OK) {
       /* 상위 owner가 fault로 전환한다. */
       return;
   }
   /* worker가 DMA ring에서 한 바이트를 가져온 뒤 */
   status = canview_uart_stream_feed(&stream, byte, &view);
   if (status == CANVIEW_OK) {
       /* 여기서는 전송 형식만 검증됨.
        * 미구현: message allowlist, peer/session, MAC, role, QoS/queue admission.
        * 검증된 payload를 queue 소유 버퍼로 복사하기 전 다음 feed 금지.
        * 이 기반 예제는 dispatch하거나 응답하지 않는다. */
   }

상세 코드 작성자는 먼저 다음 계약을 유지한다.

#. transport input은 신뢰하지 않는다. delimiter 재동기화 뒤에도 version/길이/reserved/CRC를 모두 검사한다.
#. 오류를 성공값 또는 자동 재시도로 바꾸지 않는다. BUSY 재시도와 의미 명령 결과 확인은 후속 message dispatcher의 책임이다.
#. 알려지지 않은 message는 상위 dispatcher에서 거부한다. 현재 codec의 opaque message_type은 범용 byte envelope를 위한 것이지 허용 목록이 아니다.
#. CAN batch는 classic CAN DLC 0..8, bus 0..2만 지원한다. FDCAN 하드웨어 보유가 FD payload 지원을 뜻하지 않는다.
#. app은 UNINITIALIZED에서 안전 GPIO 설정을 요청하고 SAFE_IDLE 또는 FAULT로 간다. CAN/radio/OTA 상태는 아직 없다.

.. toctree::
   :maxdepth: 1

   protocol
   app
