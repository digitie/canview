"""Deterministic host adapter and fail-closed lab-rig contract."""

from __future__ import annotations

from dataclasses import dataclass, field
import hashlib
import random
from typing import Any

from .events import EventLog
from .scenario import Scenario


HOST_ADAPTER_VERSION = "host-sim-v1"
LAB_ADAPTER_VERSION = "lab-contract-v1"


@dataclass
class SimulationResult:
    """adapter가 scenario 실행 후 반환하는 측정값."""

    metrics: dict[str, int] = field(default_factory=lambda: {
        "map_bytes": 8192,
        "stack_bytes": 512,
        "heap_free_bytes": 8192,
        "queue_depth": 64,
        "wcet_us": 300,
        "latency_us": 2000,
    })


def _scenario_seed(seed: int, scenario_id: str) -> int:
    material = f"{seed}:{scenario_id}".encode("utf-8")
    return int.from_bytes(hashlib.sha256(material).digest()[:8], "little")


class HostAdapter:
    """실제 CAN/UART/radio 없이 같은 scenario를 결정적으로 실행한다."""

    name = HOST_ADAPTER_VERSION
    physical = False

    def execute(self, scenario: Scenario, seed: int,
                log: EventLog) -> SimulationResult:
        result = SimulationResult()
        rng = random.Random(_scenario_seed(seed, scenario.scenario_id))
        monotonic_ns = 1_000_000

        def emit(kind: str, **fields: Any) -> None:
            nonlocal monotonic_ns
            log.append(monotonic_ns, "host-simulator", kind, **fields)
            monotonic_ns += 1_000

        def emit_map(kind: str, fields: dict[str, Any]) -> None:
            nonlocal monotonic_ns
            log.append_fields(monotonic_ns, "host-simulator", kind, fields)
            monotonic_ns += 1_000

        seen_tokens: set[str] = set()
        for action in scenario.actions:
            action_type = str(action["type"])
            if action_type == "radio_loss":
                for rate in action.get("rates", []):
                    packets = int(action.get("packets", 64))
                    dropped = sum(1 for _ in range(packets)
                                  if rng.randrange(100) < int(rate))
                    emit("RADIO_SUMMARY", loss_percent=int(rate), sent=packets,
                         delivered=packets - dropped, dropped=dropped,
                         duplicate_deliveries=int(action.get("duplicate_deliveries", 0)),
                         reordered=int(action.get("reordered", 0)))
            elif action_type == "radio_delay":
                emit("RADIO_DELAY_SUMMARY", max_delay_ms=int(action["max_ms"]))
            elif action_type == "reset":
                targets = action.get("targets", [])
                for target in targets:
                    emit("RESET_REQUEST", target=str(target), reason="scenario")
                    emit("BOOT_EPOCH", target=str(target), epoch=1)
            elif action_type == "uart_fault":
                for fault in action.get("faults", []):
                    emit("UART_FAULT_REJECTED", fault=str(fault),
                         parser_state="RESYNC")
            elif action_type == "can_load":
                for channel in action.get("channels", []):
                    channel_id = int(channel["channel"])
                    rx_frames = int(channel.get("rx_frames", 128))
                    emit("CAN_CHANNEL_SUMMARY", channel=channel_id,
                         rx_frames=rx_frames, tx_frames=0, ack_frames=0,
                         bus_state=str(channel.get("bus_state", "ERROR_PASSIVE")),
                         error_counter=int(channel.get("error_counter", 0)))
            elif action_type == "resource":
                queue_depth = int(action.get("queue_depth", 64))
                result.metrics["queue_depth"] = max(result.metrics["queue_depth"],
                                                     queue_depth)
                result.metrics["heap_free_bytes"] = min(
                    result.metrics["heap_free_bytes"],
                    int(action.get("heap_free_bytes", 8192)))
                emit("RESOURCE_SUMMARY", pool=str(action.get("pool", "unknown")),
                     queue_depth=queue_depth,
                     heap_free_bytes=int(action.get("heap_free_bytes", 8192)),
                     rejected=int(action.get("rejected", 0)),
                     observer_drops=int(action.get("observer_drops", 0)))
            elif action_type == "safety_gates":
                for item in action.get("checks", []):
                    emit("SAFETY_DECISION", check=str(item), decision="DENY",
                         vehicle_tx=False, reason="capture-only")
            elif action_type == "duplicate_command":
                for token in action.get("tokens", []):
                    token_text = str(token)
                    duplicate = token_text in seen_tokens
                    seen_tokens.add(token_text)
                    emit("COMMAND_REPLAY", request_token=str(token),
                         executed=False,
                         result="DUPLICATE" if duplicate else "ACCEPTED")
            elif action_type == "feedback":
                for case in action.get("cases", []):
                    case_text = str(case)
                    if case_text == "result-before-ack":
                        emit("FEEDBACK_SEQUENCE", case=case_text,
                             sequence=["RESULT", "ACK"])
                        feedback_result = "RESULT_BEFORE_ACK"
                    elif "mismatch" in case_text:
                        feedback_result = "MISMATCH"
                    elif "success" in case_text:
                        feedback_result = "SUCCESS"
                    elif "override" in case_text:
                        feedback_result = "MANUAL_OVERRIDE"
                    else:
                        feedback_result = "TIMEOUT"
                    emit("FEEDBACK_RESULT", case=str(case),
                         result=feedback_result,
                         tx_permitted=False)
            elif action_type == "power":
                for stage in action.get("stages", []):
                    emit("POWER_EVENT", stage=str(stage), tx_gate="OFF",
                         capture_state="RECOVERING")
            elif action_type == "security":
                for vector in action.get("vectors", []):
                    emit("SECURITY_REJECT", vector=str(vector), accepted=False)
            elif action_type == "guardian":
                for guardian in action.get("guardians", []):
                    emit("GUARDIAN_TIMEOUT", guardian=str(guardian),
                         tx_gate="OFF", reset_requested=True)
            elif action_type == "radio_pressure":
                emit("RADIO_BUDGET", softap_kbps=int(action.get("softap_kbps", 0)),
                     observer_kbps=int(action.get("observer_kbps", 0)),
                     control_kbps=int(action.get("control_kbps", 0)),
                     rssi_dbm=int(action.get("rssi_dbm", -60)),
                     overflow_policy="DROP_OBSERVER")
            elif action_type == "capture":
                for channel in action.get("channels", []):
                    emit("CAN_RX", channel=int(channel), frame_count=1,
                         capture_only=True)
            elif action_type == "budget":
                for metric, value in action.get("values", {}).items():
                    metric_name = str(metric)
                    metric_value = int(value)
                    if metric_name == "heap_free_bytes":
                        result.metrics[metric_name] = min(
                            result.metrics.get(metric_name, metric_value),
                            metric_value)
                    else:
                        result.metrics[metric_name] = max(
                            result.metrics.get(metric_name, metric_value),
                            metric_value)
                emit("BUDGET_SAMPLE", metrics=dict(result.metrics))
            elif action_type == "event":
                emit_map(str(action.get("kind", "SCENARIO_EVENT")),
                         dict(action.get("fields", {})))
            else:
                emit("UNKNOWN_ACTION_REJECTED", action_type=action_type)

        emit("TX_GATE_STATE", vehicle_tx=False, mode="CAPTURE_ONLY")
        emit("HARNESS_COMPLETE", scenario=scenario.scenario_id,
             firmware_mode="CAPTURE_ONLY")
        return result


@dataclass(frozen=True)
class RigConfig:
    """private rig 설정의 공개·비밀 없는 최소 contract."""

    available: bool
    adapter: str
    channels: tuple[int, ...]
    device_ids: tuple[str, ...]


class RigConfigError(ValueError):
    """rig 설정이 없거나 contract가 잘못된 경우."""


def parse_rig_config(raw: dict[str, Any]) -> RigConfig:
    if raw.get("schema_version") != 1:
        raise RigConfigError("rig schema_version must be 1")
    adapter = raw.get("adapter")
    if adapter != LAB_ADAPTER_VERSION:
        raise RigConfigError(f"unsupported rig adapter: {adapter!r}")
    available = raw.get("available")
    if not isinstance(available, bool):
        raise RigConfigError("rig available must be boolean")
    channels = raw.get("can_channels", [1, 2, 3])
    device_ids = raw.get("device_ids", [])
    if not isinstance(channels, list) or any(not isinstance(item, int)
                                              for item in channels):
        raise RigConfigError("can_channels must be integer list")
    if (not channels or any(isinstance(item, bool) or item not in {1, 2, 3}
                            for item in channels)
            or len(channels) != len(set(channels))):
        raise RigConfigError("can_channels must be unique values from 1..3")
    if not isinstance(device_ids, list) or any(not isinstance(item, str)
                                               for item in device_ids):
        raise RigConfigError("device_ids must be string list")
    if any(not item or len(item) > 128 for item in device_ids):
        raise RigConfigError("device_ids must be non-empty strings up to 128 chars")
    return RigConfig(available, adapter, tuple(channels), tuple(device_ids))
