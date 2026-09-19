/* MCUboot header 512B 뒤에서 실행하는 primary 앱.
 * signed image 180KiB에서 header 512B와 TLV 여유 512B를 제외한다.
 * 실제 서명 크기/trailer 검증은 imgtool 단계에서 별도로 수행한다. */
CANVIEW_FLASH_ORIGIN = 0x08010200;
CANVIEW_FLASH_LENGTH = 179K;
INCLUDE application-sections.ld
ASSERT(ADDR(.isr_vector) == 0x08010200, "MCUboot primary vector mismatch")
ASSERT(SIZEOF(.isr_vector) == 0x1D8, "STM32G474 vector size mismatch")
