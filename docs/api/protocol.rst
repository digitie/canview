프레이밍과 수신 상태
====================

.. _canview__wire_8h:

.. _canview__status_8h:

ESP-NOW는 32바이트 header와 최대 208바이트 payload, UART는 32바이트 header와 최대 992바이트 payload를 받는다.
UART는 COBS와 마지막 0 delimiter를 포함해 최대 1030바이트 버퍼를 사용한다.
모든 multibyte 필드는 little-endian이다. host struct의 padding이나 sizeof를 wire에 사용하지 않는다.

.. c:macro:: CANVIEW_WIRE_CAN_MAX_RECORDS

   schema에서 생성되는 batch record 상한이다.

.. c:macro:: CANVIEW_WIRE_UART_MAX_ENCODED

   schema에서 생성되는 COBS packet 상한이다. delimiter는 포함하지 않는다.

.. c:macro:: CANVIEW_WIRE_UART_MAX_FRAME

   schema에서 생성되는 delimiter/COBS 적용 전 UART frame 상한이다.

.. doxygenfile:: canview_wire.h
   :project: canview

공용 상태
---------

.. doxygenfile:: canview_status.h
   :project: canview
