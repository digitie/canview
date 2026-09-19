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

Boot runtime
------------

이 계약의 구현은 G474 boot 전용이다. 앱 core와 동시에 사용하지 않으며
SDK startup과 BSP safe output 이후 main 단일 owner에서만 호출한다.
부팅 승인·최종 loader·실제 watchdog 계측을 대신하지 않는다.

.. _canview__boot__runtime_8h:

.. doxygenfile:: canview_boot_runtime.h
   :project: canview

Boot handoff
------------

서명과 영속 정책 승인 뒤 고정 primary로 진입하는 MCU 의존 경계다.
임의 주소·secondary 진입은 받지 않으며 이 API 자체는 부팅 승인 수단이 아니다.
실제 앱 진입과 NMI/reset timing은 NOT_RUN이다.

.. _canview__boot__handoff_8h:

.. doxygenfile:: canview_boot_handoff.h
   :project: canview
