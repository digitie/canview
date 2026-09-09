STM32 UART DMA와 semantic runtime
=================================

이 API는 Communicator STM32의 USART2 4 Mbps DMA adapter와 portable semantic
runtime의 경계를 정의한다. RX DMA ring, ISR event latch, bounded worker
service의 소유권을 분리하며, UART 명령을 차량 CAN frame으로 변환하거나
vehicle-TX 권한을 부여하지 않는다.

.. _canview__stm__uart_8h:

.. doxygenfile:: canview_stm_uart.h
   :project: canview

.. _uart__dma_8h:

.. doxygenfile:: uart_dma.h
   :project: canview
