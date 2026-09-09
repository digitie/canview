"""기반 C API 사이트. 일반 설계 정본은 docs/architecture에 유지한다."""
from pathlib import Path

project = "CANView 기반 C API"
copyright = "2026, CANView 기여자"
language = "ko"
extensions = ["breathe"]
root = Path(__file__).resolve().parents[2]
breathe_projects = {"canview": str(root / "build/api/doxygen/xml")}
breathe_default_project = "canview"
breathe_domain_by_extension = {"h": "c"}
html_theme = "furo"
html_title = project
nitpicky = True
# C library and protocol dependency types are supplied by another API layer or
# the compiler; this site documents the STM32 UART boundary without duplicating
# those declarations.
nitpick_ignore = [("c:identifier", name) for name in
                  ("uint8_t", "uint16_t", "uint32_t", "uint64_t", "uintptr_t", "size_t", "bool",
                   "int64_t", "canview_uart_message_view_t", "canview_uart_command_handle_t",
                   "canview_uart_command_key_t", "canview_uart_codec_t", "canview_uart_link_t",
                   "canview_uart_plan_context_t", "canview_uart_command_cache_t",
                   "canview_uart_replay_context_t", "CANVIEW_UART_CONTROL_TAG_SIZE",
                   "CANVIEW_UART_MAX_FRAME_SIZE")]
exclude_patterns = []
