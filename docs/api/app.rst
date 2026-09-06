부팅과 platform 포트
====================

.. _canview__app_8h:

.. _canview__platform__port_8h:

포트의 context 수명은 app보다 길어야 한다. 포트 함수 포인터는 시작 시 복사하며 caller가 나중에 포트 구조체를 바꿔도 app은 따라가지 않는다.
안전 초기화 실패는 FAULT에 고정되고 자동 재시작하지 않는다.

.. doxygenfile:: canview_app.h
   :project: canview

.. doxygenfile:: canview_platform_port.h
   :project: canview
