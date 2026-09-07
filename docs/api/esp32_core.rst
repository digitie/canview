Communicator ESP32 bench core
=============================

단일 main owner는 boot 뒤 100ms 고정 주기로 health를 확인한다.
성공해도 UART·무선·OTA·차량 TX 권한이 없으며, 실패 뒤 watchdog 갱신을 중단한다.
고정 pool은 task/callback 문맥만 허용하고 payload를 복사하여 token으로 소유권을 추적한다.
실제 SDK 어댑터의 host 모의시험과 target compile은 실물 watchdog·메모리·GPIO 검증을 대체하지 않는다.

.. _canview__esp__core_8h:

.. doxygenfile:: canview_esp_core.h
   :project: canview

.. _canview__esp__pool_8h:

.. doxygenfile:: canview_esp_pool.h
   :project: canview

.. _canview__esp__runtime_8h:

.. doxygenfile:: canview_esp_runtime.h
   :project: canview
