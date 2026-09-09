STM32 FDCAN capture-only
========================

이 API는 세 채널 classic CAN 관찰용 profile, timestamp, bounded ring, wire
batch와 generic ID inventory 계약을 제공한다. FDCAN TX와 차량 제어 권한은
정의하지 않는다. G474 CMSIS register adapter는 IRQ raw snapshot과 worker
decode 경계를 별도 계약으로 노출한다.

.. _canview__stm__fdcan__capture_8h:

.. doxygenfile:: canview_stm_fdcan_capture.h
   :project: canview

.. _fdcan__capture_8h:

.. doxygenfile:: fdcan_capture.h
   :project: canview
