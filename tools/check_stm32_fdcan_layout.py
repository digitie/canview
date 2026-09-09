"""STM32G4 vendor fixed FDCAN Message RAM과 adapter 계약을 비교한다."""
from __future__ import annotations

import argparse
import ast
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]

VENDOR_MACROS = {
    "SRAMCAN_FLS_NBR": 28,
    "SRAMCAN_FLE_NBR": 8,
    "SRAMCAN_RF0_NBR": 3,
    "SRAMCAN_RF1_NBR": 3,
    "SRAMCAN_TEF_NBR": 3,
    "SRAMCAN_TFQ_NBR": 3,
    "SRAMCAN_FLS_SIZE": 4,
    "SRAMCAN_FLE_SIZE": 8,
    "SRAMCAN_RF0_SIZE": 72,
    "SRAMCAN_RF1_SIZE": 72,
    "SRAMCAN_TEF_SIZE": 8,
    "SRAMCAN_TFQ_SIZE": 72,
}

CONTRACT_MACROS = {
    "CANVIEW_STM_FDCAN_MESSAGE_RAM_STANDARD_FILTER_COUNT": 28,
    "CANVIEW_STM_FDCAN_MESSAGE_RAM_EXTENDED_FILTER_COUNT": 8,
    "CANVIEW_STM_FDCAN_MESSAGE_RAM_RX_FIFO0_ELEMENT_COUNT": 3,
    "CANVIEW_STM_FDCAN_MESSAGE_RAM_RX_FIFO1_ELEMENT_COUNT": 3,
    "CANVIEW_STM_FDCAN_MESSAGE_RAM_TX_EVENT_FIFO_ELEMENT_COUNT": 3,
    "CANVIEW_STM_FDCAN_MESSAGE_RAM_TX_FIFO_ELEMENT_COUNT": 3,
    "CANVIEW_STM_FDCAN_MESSAGE_RAM_STANDARD_FILTER_ELEMENT_BYTES": 4,
    "CANVIEW_STM_FDCAN_MESSAGE_RAM_EXTENDED_FILTER_ELEMENT_BYTES": 8,
    "CANVIEW_STM_FDCAN_MESSAGE_RAM_RX_FIFO0_ELEMENT_BYTES": 72,
    "CANVIEW_STM_FDCAN_MESSAGE_RAM_RX_FIFO1_ELEMENT_BYTES": 72,
    "CANVIEW_STM_FDCAN_MESSAGE_RAM_TX_EVENT_FIFO_ELEMENT_BYTES": 8,
    "CANVIEW_STM_FDCAN_MESSAGE_RAM_TX_FIFO_ELEMENT_BYTES": 72,
    "CANVIEW_STM_FDCAN_MESSAGE_RAM_STANDARD_FILTER_OFFSET_BYTES": 0,
    "CANVIEW_STM_FDCAN_MESSAGE_RAM_EXTENDED_FILTER_OFFSET_BYTES": 112,
    "CANVIEW_STM_FDCAN_MESSAGE_RAM_RX_FIFO0_OFFSET_BYTES": 176,
    "CANVIEW_STM_FDCAN_MESSAGE_RAM_RX_FIFO1_OFFSET_BYTES": 392,
    "CANVIEW_STM_FDCAN_MESSAGE_RAM_TX_EVENT_FIFO_OFFSET_BYTES": 608,
    "CANVIEW_STM_FDCAN_MESSAGE_RAM_TX_FIFO_OFFSET_BYTES": 632,
    "CANVIEW_STM_FDCAN_MESSAGE_RAM_INSTANCE_BYTES": 848,
}


def _macro_definitions(text: str) -> dict[str, str]:
    logical_text = re.sub(r"\\\r?\n[ \t]*", " ", text)
    definitions: dict[str, str] = {}
    for line in logical_text.splitlines():
        match = re.match(r"[ \t]*#define[ \t]+([A-Za-z_][A-Za-z0-9_]*)[ \t]+(.+)", line)
        if match is not None:
            expression = match.group(2).split("/*", 1)[0].strip()
            definitions[match.group(1)] = expression
    return definitions


def _constant_expression(expression: str, name: str) -> int:
    expression = re.sub(r"\b(?:uint32_t|UINT32_C)\b", "", expression)
    expression = expression.replace("U", "").replace("u", "").strip()
    if re.fullmatch(r"[0-9 ()*+\-/]+", expression) is None:
        raise RuntimeError(f"{name} has unsupported expression: {expression}")

    def evaluate(node: ast.AST) -> int:
        if isinstance(node, ast.Constant) and isinstance(node.value, int):
            return node.value
        if isinstance(node, ast.UnaryOp) and isinstance(node.op, (ast.UAdd, ast.USub)):
            value = evaluate(node.operand)
            return value if isinstance(node.op, ast.UAdd) else -value
        if isinstance(node, ast.BinOp) and isinstance(node.op, (ast.Add, ast.Sub, ast.Mult)):
            left = evaluate(node.left)
            right = evaluate(node.right)
            if isinstance(node.op, ast.Add):
                return left + right
            if isinstance(node.op, ast.Sub):
                return left - right
            return left * right
        raise RuntimeError(f"{name} has unsupported expression tree: {expression}")

    try:
        value = evaluate(ast.parse(expression, mode="eval").body)
    except (SyntaxError, TypeError, ValueError, RuntimeError) as error:
        raise RuntimeError(f"{name} cannot be evaluated: {expression}") from error
    if value < 0:
        raise RuntimeError(f"{name} is negative: {value}")
    return value


def _read_constants(path: Path, expected: dict[str, int]) -> dict[str, int]:
    definitions = _macro_definitions(path.read_text(encoding="utf-8"))
    values: dict[str, int] = {}
    for name in expected:
        if name not in definitions:
            raise RuntimeError(f"{path}: missing {name}")
        values[name] = _constant_expression(definitions[name], name)
    return values


def _derived_layout(vendor: dict[str, int]) -> dict[str, int]:
    standard_offset = 0
    extended_offset = standard_offset + vendor["SRAMCAN_FLS_NBR"] * vendor["SRAMCAN_FLS_SIZE"]
    fifo0_offset = extended_offset + vendor["SRAMCAN_FLE_NBR"] * vendor["SRAMCAN_FLE_SIZE"]
    fifo1_offset = fifo0_offset + vendor["SRAMCAN_RF0_NBR"] * vendor["SRAMCAN_RF0_SIZE"]
    tx_event_offset = fifo1_offset + vendor["SRAMCAN_RF1_NBR"] * vendor["SRAMCAN_RF1_SIZE"]
    tx_fifo_offset = tx_event_offset + vendor["SRAMCAN_TEF_NBR"] * vendor["SRAMCAN_TEF_SIZE"]
    instance_bytes = tx_fifo_offset + vendor["SRAMCAN_TFQ_NBR"] * vendor["SRAMCAN_TFQ_SIZE"]
    return {
        "CANVIEW_STM_FDCAN_MESSAGE_RAM_STANDARD_FILTER_COUNT": vendor["SRAMCAN_FLS_NBR"],
        "CANVIEW_STM_FDCAN_MESSAGE_RAM_EXTENDED_FILTER_COUNT": vendor["SRAMCAN_FLE_NBR"],
        "CANVIEW_STM_FDCAN_MESSAGE_RAM_RX_FIFO0_ELEMENT_COUNT": vendor["SRAMCAN_RF0_NBR"],
        "CANVIEW_STM_FDCAN_MESSAGE_RAM_RX_FIFO1_ELEMENT_COUNT": vendor["SRAMCAN_RF1_NBR"],
        "CANVIEW_STM_FDCAN_MESSAGE_RAM_TX_EVENT_FIFO_ELEMENT_COUNT": vendor["SRAMCAN_TEF_NBR"],
        "CANVIEW_STM_FDCAN_MESSAGE_RAM_TX_FIFO_ELEMENT_COUNT": vendor["SRAMCAN_TFQ_NBR"],
        "CANVIEW_STM_FDCAN_MESSAGE_RAM_STANDARD_FILTER_ELEMENT_BYTES": vendor["SRAMCAN_FLS_SIZE"],
        "CANVIEW_STM_FDCAN_MESSAGE_RAM_EXTENDED_FILTER_ELEMENT_BYTES": vendor["SRAMCAN_FLE_SIZE"],
        "CANVIEW_STM_FDCAN_MESSAGE_RAM_RX_FIFO0_ELEMENT_BYTES": vendor["SRAMCAN_RF0_SIZE"],
        "CANVIEW_STM_FDCAN_MESSAGE_RAM_RX_FIFO1_ELEMENT_BYTES": vendor["SRAMCAN_RF1_SIZE"],
        "CANVIEW_STM_FDCAN_MESSAGE_RAM_TX_EVENT_FIFO_ELEMENT_BYTES": vendor["SRAMCAN_TEF_SIZE"],
        "CANVIEW_STM_FDCAN_MESSAGE_RAM_TX_FIFO_ELEMENT_BYTES": vendor["SRAMCAN_TFQ_SIZE"],
        "CANVIEW_STM_FDCAN_MESSAGE_RAM_STANDARD_FILTER_OFFSET_BYTES": standard_offset,
        "CANVIEW_STM_FDCAN_MESSAGE_RAM_EXTENDED_FILTER_OFFSET_BYTES": extended_offset,
        "CANVIEW_STM_FDCAN_MESSAGE_RAM_RX_FIFO0_OFFSET_BYTES": fifo0_offset,
        "CANVIEW_STM_FDCAN_MESSAGE_RAM_RX_FIFO1_OFFSET_BYTES": fifo1_offset,
        "CANVIEW_STM_FDCAN_MESSAGE_RAM_TX_EVENT_FIFO_OFFSET_BYTES": tx_event_offset,
        "CANVIEW_STM_FDCAN_MESSAGE_RAM_TX_FIFO_OFFSET_BYTES": tx_fifo_offset,
        "CANVIEW_STM_FDCAN_MESSAGE_RAM_INSTANCE_BYTES": instance_bytes,
    }


def validate(vendor_path: Path, cmsis_path: Path, contract_path: Path) -> dict[str, int]:
    vendor = _read_constants(vendor_path, VENDOR_MACROS)
    if vendor != VENDOR_MACROS:
        raise RuntimeError(f"unexpected STM32CubeG4 FDCAN layout: {vendor}")
    expected_contract = _derived_layout(vendor)
    contract = _read_constants(contract_path, CONTRACT_MACROS)
    if contract != expected_contract:
        mismatches = {
            name: (expected_contract[name], contract.get(name))
            for name in CONTRACT_MACROS
            if contract.get(name) != expected_contract[name]
        }
        raise RuntimeError(f"adapter Message RAM contract mismatch: {mismatches}")
    cmsis_text = cmsis_path.read_text(encoding="utf-8")
    for register in ("RXF0S", "RXF0A"):
        if register not in cmsis_text:
            raise RuntimeError(f"STM32G4 CMSIS map is missing {register}")
    for register in ("RXF0C", "RXESC"):
        if register in cmsis_text:
            raise RuntimeError(f"STM32G4 CMSIS map unexpectedly exposes {register}")
    return contract


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sdk", required=True, type=Path)
    parser.add_argument("--contract", type=Path,
                        default=ROOT / "firmware/communicator/stm32/platform/stm32g474/"
                        "fdcan_message_ram.h")
    args = parser.parse_args()
    sdk = args.sdk.resolve()
    vendor_path = sdk / "Drivers/STM32G4xx_HAL_Driver/Src/stm32g4xx_hal_fdcan.c"
    cmsis_path = sdk / "Drivers/CMSIS/Device/ST/STM32G4xx/Include/stm32g474xx.h"
    contract = validate(vendor_path, cmsis_path, args.contract.resolve())
    print("PASS: STM32G4 fixed FDCAN Message RAM "
          f"instance={contract['CANVIEW_STM_FDCAN_MESSAGE_RAM_INSTANCE_BYTES']} "
          f"fifo0_offset={contract['CANVIEW_STM_FDCAN_MESSAGE_RAM_RX_FIFO0_OFFSET_BYTES']} "
          f"element={contract['CANVIEW_STM_FDCAN_MESSAGE_RAM_RX_FIFO0_ELEMENT_BYTES']} "
          f"depth={contract['CANVIEW_STM_FDCAN_MESSAGE_RAM_RX_FIFO0_ELEMENT_COUNT']}; "
          "CMSIS has no RXF0C/RXESC")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
