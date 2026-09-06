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
# C library types are supplied by the compiler, not duplicated API entities.
nitpick_ignore = [("c:identifier", name) for name in
                  ("uint8_t", "uint16_t", "uint32_t", "uint64_t", "size_t", "bool")]
exclude_patterns = []
