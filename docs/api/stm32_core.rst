STM32 최소 core와 고정 queue
============================

이 API는 SDK 독립 bench boot policy, cooperative worker 진척 감시와 고정 record queue를 제공한다.
보드 clock/핀/전원 계측이나 CAN·OTA·boot 인증을 대신하지 않는다.
worker와 callback context는 정적 수명이며 설정 후 단일 owner만 변경한다.
queue 통계의 직접 조회에도 동일 critical port가 필요하다.

.. _canview__stm__core_8h:

.. doxygenfile:: canview_stm_core.h
   :project: canview

.. _canview__stm__queue_8h:

.. doxygenfile:: canview_stm_queue.h
   :project: canview
