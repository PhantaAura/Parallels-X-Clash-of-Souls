#!/usr/bin/env python3
"""Repair Rrvvfo's low-poly torso and garment skin weights.

The source mesh deliberately uses disconnected cuboid faces, so automatic
envelope weighting gives the torso panels arm and pelvis contamination.  This
tool assigns weights from bind-space height and connected garment regions while
leaving geometry, materials, the skeleton, and every unrelated vertex intact.
"""

from __future__ import annotations

import argparse
import importlib.util
import math
import struct
from pathlib import Path


JOINT_COMPONENT_UNSIGNED_BYTE = 5121
WEIGHT_COMPONENT_FLOAT = 5126


def load_cooker(project_root: Path):
    path = project_root / "scripts" / "cook-character-glb.py"
    spec = importlib.util.spec_from_file_location("px_character_cooker", path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"Could not load {path}")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def clamp(value: float, low: float = 0.0, high: float = 1.0) -> float:
    return max(low, min(high, value))


def normalize(weights: dict[str, float]) -> dict[str, float]:
    cleaned = {name: value for name, value in weights.items() if value > 1.0e-6}
    total = sum(cleaned.values())
    if total <= 1.0e-8:
        raise ValueError("A repaired vertex received no skin weight")
    return {name: value / total for name, value in cleaned.items()}


def torso_chain(y: float, neck_limit: float = 0.20) -> dict[str, float]:
    """Return a smooth, four-bone torso blend at bind-space height ``y``."""
    centers = {
        "spine": 0.831,
        "spine.001": 0.982,
        "spine.002": 1.120,
        "spine.003": 1.297,
        "spine.004": 1.493,
    }
    sigma = 0.165
    weights = {
        name: math.exp(-((y - center) ** 2) / (2.0 * sigma * sigma))
        for name, center in centers.items()
    }
    weights = normalize(weights)

    neck = weights.get("spine.004", 0.0)
    if neck > neck_limit:
        excess = neck - neck_limit
        weights["spine.004"] = neck_limit
        recipients = ["spine.002", "spine.003"]
        recipient_total = sum(weights[name] for name in recipients)
        for name in recipients:
            weights[name] += excess * weights[name] / recipient_total

    # The runtime stores four influences.  Discard only the least relevant
    # Gaussian tail and renormalize deterministically.
    strongest = sorted(weights.items(), key=lambda item: (-item[1], item[0]))[:4]
    return normalize(dict(strongest))


def blend_in(base: dict[str, float], additions: dict[str, float], amount: float) -> dict[str, float]:
    amount = clamp(amount)
    result = {name: value * (1.0 - amount) for name, value in base.items()}
    for name, value in additions.items():
        result[name] = result.get(name, 0.0) + amount * value
    strongest = sorted(result.items(), key=lambda item: (-item[1], item[0]))[:4]
    return normalize(dict(strongest))


def side_name(x: float) -> str:
    # This rig uses positive bind X for Blender's .L bones.
    return "L" if x >= 0.0 else "R"


def body_torso_weights(position: tuple[float, ...]) -> dict[str, float]:
    x, y, _ = position
    weights = torso_chain(y, neck_limit=0.16)

    pelvis_amount = 0.45 * clamp((0.980 - y) / 0.220)
    if pelvis_amount > 0.0:
        left_share = clamp(0.5 + x / 0.30)
        pelvis = {"pelvis.L": left_share, "pelvis.R": 1.0 - left_share}
        weights = blend_in(weights, pelvis, pelvis_amount)

    if y > 1.42 and abs(x) > 0.07:
        height = clamp((y - 1.42) / 0.095)
        width = clamp((abs(x) - 0.07) / 0.16)
        weights = blend_in(weights, {f"shoulder.{side_name(x)}": 1.0}, 0.14 * height * width)
    return weights


def garment_torso_weights(position: tuple[float, ...], shoulder_limit: float) -> dict[str, float]:
    x, y, _ = position
    weights = torso_chain(y)
    if y > 1.40 and abs(x) > 0.075:
        height = clamp((y - 1.40) / 0.115)
        width = clamp((abs(x) - 0.075) / 0.17)
        weights = blend_in(
            weights,
            {f"shoulder.{side_name(x)}": 1.0},
            shoulder_limit * height * width,
        )
    return weights


def shirt_sleeve_weights(position: tuple[float, ...]) -> dict[str, float]:
    x, _, _ = position
    side = side_name(x)
    distance = abs(x)
    shoulder = f"shoulder.{side}"
    upper = f"upper_arm.{side}"
    forearm = f"forearm.{side}"
    # Follow the actual bind-pose shoulder -> upper-arm -> elbow chain.  The
    # former distance buckets never gave the shirt cuff any forearm influence,
    # so it stayed behind when the elbow bent.
    if distance <= 0.24:
        t = clamp((distance - 0.15) / 0.09)
        return normalize({shoulder: 1.0 - t, upper: t})
    t = clamp((distance - 0.24) / 0.22)
    return normalize({upper: 1.0 - t, forearm: t})


def jacket_sleeve_weights(position: tuple[float, ...]) -> dict[str, float]:
    x, _, _ = position
    side = side_name(x)
    distance = abs(x)
    shoulder = f"shoulder.{side}"
    upper = f"upper_arm.{side}"
    forearm = f"forearm.{side}"
    hand = f"hand.{side}"

    # Blend continuously along the real bind-pose arm chain.  This retains the
    # artist's intended joint ownership while removing the severe neighboring-
    # vertex discontinuities in the source sleeve export.
    if distance <= 0.24:
        t = clamp((distance - 0.15) / 0.09)
        return normalize({shoulder: 1.0 - t, upper: t})
    if distance <= 0.46:
        t = clamp((distance - 0.24) / 0.22)
        return normalize({upper: 1.0 - t, forearm: t})
    t = clamp((distance - 0.46) / 0.20)
    return normalize({forearm: 1.0 - t, hand: t})


def jacket_cap_weights(position: tuple[float, ...]) -> dict[str, float]:
    x, _, _ = position
    side = side_name(x)
    # Each cap is one small rigid armor/cloth island.  Per-vertex gradients made
    # its corners fan apart under a shoulder turn; one shared blend keeps the
    # silhouette closed while following the arm instead of hovering on torso.
    return {
        "spine.003": 0.38,
        "spine.004": 0.12,
        f"shoulder.{side}": 0.42,
        f"upper_arm.{side}": 0.08,
    }


def connected_components(indices: list[int], vertex_count: int) -> list[list[int]]:
    adjacency = [set() for _ in range(vertex_count)]
    for offset in range(0, len(indices), 3):
        a, b, c = indices[offset : offset + 3]
        for first, second in ((a, b), (b, c), (c, a)):
            adjacency[first].add(second)
            adjacency[second].add(first)

    components: list[list[int]] = []
    visited: set[int] = set()
    for vertex in range(vertex_count):
        if vertex in visited:
            continue
        stack = [vertex]
        visited.add(vertex)
        component: list[int] = []
        while stack:
            current = stack.pop()
            component.append(current)
            for neighbor in adjacency[current]:
                if neighbor not in visited:
                    visited.add(neighbor)
                    stack.append(neighbor)
        components.append(component)
    return components


def component_bounds(component: list[int], positions: list[tuple[float, ...]]) -> tuple[list[float], list[float]]:
    low = [min(positions[index][axis] for index in component) for axis in range(3)]
    high = [max(positions[index][axis] for index in component) for axis in range(3)]
    return low, high


def accessor_offset(document: dict, accessor_index: int, item: int) -> int:
    accessor = document["accessors"][accessor_index]
    view = document["bufferViews"][accessor["bufferView"]]
    component_size = {5121: 1, 5126: 4}[accessor["componentType"]]
    component_count = {"VEC4": 4}[accessor["type"]]
    packed_size = component_size * component_count
    stride = view.get("byteStride", packed_size)
    return view.get("byteOffset", 0) + accessor.get("byteOffset", 0) + item * stride


def write_weights(
    data: bytearray,
    binary_start: int,
    document: dict,
    joint_accessor: int,
    weight_accessor: int,
    vertex: int,
    weights: dict[str, float],
    joint_lookup: dict[str, int],
) -> None:
    joints_definition = document["accessors"][joint_accessor]
    weights_definition = document["accessors"][weight_accessor]
    if joints_definition["componentType"] != JOINT_COMPONENT_UNSIGNED_BYTE:
        raise ValueError("Rrvvfo repair expects unsigned-byte JOINTS_0")
    if weights_definition["componentType"] != WEIGHT_COMPONENT_FLOAT:
        raise ValueError("Rrvvfo repair expects float WEIGHTS_0")

    normalized = normalize(weights)
    cleaned = {name: value for name, value in normalized.items() if value >= 0.01}
    strongest = sorted(normalize(cleaned).items(), key=lambda item: (-item[1], item[0]))[:4]
    strongest = list(normalize(dict(strongest)).items())
    joints = [joint_lookup[name] for name, _ in strongest]
    values = [value for _, value in strongest]
    while len(joints) < 4:
        joints.append(0)
        values.append(0.0)

    joint_offset = binary_start + accessor_offset(document, joint_accessor, vertex)
    weight_offset = binary_start + accessor_offset(document, weight_accessor, vertex)
    struct.pack_into("<4B", data, joint_offset, *joints)
    struct.pack_into("<4f", data, weight_offset, *values)


def find_binary_start(data: bytearray) -> int:
    offset = 12
    while offset < len(data):
        length, kind = struct.unpack_from("<II", data, offset)
        start = offset + 8
        if kind == 0x004E4942:
            return start
        offset = start + length
    raise ValueError("GLB has no binary chunk")


def primitive_for(document: dict, node_name: str) -> dict:
    node = next((node for node in document["nodes"] if node.get("name") == node_name), None)
    if node is None or "mesh" not in node:
        raise ValueError(f"Missing skinned node {node_name!r}")
    primitives = document["meshes"][node["mesh"]].get("primitives", [])
    if not primitives:
        raise ValueError(f"Node {node_name!r} has no primitives")
    return primitives[0]


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("source", type=Path)
    parser.add_argument("output", type=Path)
    arguments = parser.parse_args()
    if arguments.source.resolve() == arguments.output.resolve():
        raise ValueError("Use a separate output path so the artist source remains recoverable")

    project_root = Path(__file__).resolve().parents[1]
    cooker = load_cooker(project_root)
    document, binary = cooker.parse_glb(arguments.source)
    data = bytearray(arguments.source.read_bytes())
    binary_start = find_binary_start(data)

    skin = document.get("skins", [])
    if len(skin) != 1 or len(skin[0].get("joints", [])) != 39:
        raise ValueError("Expected Rrvvfo's single 39-joint skin")
    joint_names = [document["nodes"][node].get("name", "") for node in skin[0]["joints"]]
    joint_lookup = {name: index for index, name in enumerate(joint_names)}
    required = {
        "spine", "spine.001", "spine.002", "spine.003", "spine.004",
        "pelvis.L", "pelvis.R", "shoulder.L", "shoulder.R",
        "upper_arm.L", "upper_arm.R", "forearm.L", "forearm.R", "hand.L", "hand.R",
    }
    if not required.issubset(joint_lookup):
        raise ValueError("Rrvvfo skeleton names do not match the repair profile")

    repaired: dict[str, int] = {"body_torso": 0, "shirt_torso": 0, "shirt_sleeves": 0,
                                "jacket_torso": 0, "jacket_caps": 0, "jacket_sleeves": 0}

    # Underlying low-poly torso: repair only the disconnected cuboid faces below
    # the neck, leaving arms, hands, legs, and head completely untouched.
    body = primitive_for(document, "Rrvvfos Body")
    body_attributes = body["attributes"]
    body_positions = cooker.read_accessor(document, binary, body_attributes["POSITION"])
    body_indices = [int(item[0]) for item in cooker.read_accessor(document, binary, body["indices"])]
    for component in connected_components(body_indices, len(body_positions)):
        low, high = component_bounds(component, body_positions)
        is_torso_face = (
            low[0] >= -0.205 and high[0] <= 0.237 and
            low[1] >= 0.755 and high[1] <= 1.516 and
            low[2] >= 1.508 and high[2] <= 1.685
        )
        if not is_torso_face:
            continue
        for vertex in component:
            write_weights(data, binary_start, document, body_attributes["JOINTS_0"],
                          body_attributes["WEIGHTS_0"], vertex,
                          body_torso_weights(body_positions[vertex]), joint_lookup)
            repaired["body_torso"] += 1

    shirt = primitive_for(document, "Shirt")
    shirt_attributes = shirt["attributes"]
    shirt_positions = cooker.read_accessor(document, binary, shirt_attributes["POSITION"])
    shirt_indices = [int(item[0]) for item in cooker.read_accessor(document, binary, shirt["indices"])]
    for component in connected_components(shirt_indices, len(shirt_positions)):
        low, high = component_bounds(component, shirt_positions)
        is_sleeve = low[1] >= 1.36 and (low[0] >= 0.16 or high[0] <= -0.16)
        for vertex in component:
            weights = shirt_sleeve_weights(shirt_positions[vertex]) if is_sleeve else garment_torso_weights(
                shirt_positions[vertex], shoulder_limit=0.30)
            write_weights(data, binary_start, document, shirt_attributes["JOINTS_0"],
                          shirt_attributes["WEIGHTS_0"], vertex, weights, joint_lookup)
            repaired["shirt_sleeves" if is_sleeve else "shirt_torso"] += 1

    jacket = primitive_for(document, "Jacket")
    jacket_attributes = jacket["attributes"]
    jacket_positions = cooker.read_accessor(document, binary, jacket_attributes["POSITION"])
    jacket_indices = [int(item[0]) for item in cooker.read_accessor(document, binary, jacket["indices"])]
    for component in connected_components(jacket_indices, len(jacket_positions)):
        low, high = component_bounds(component, jacket_positions)
        is_sleeve = low[1] >= 1.30 and max(abs(low[0]), abs(high[0])) > 0.40
        is_cap = low[1] >= 1.50 and not is_sleeve
        category = "jacket_sleeves" if is_sleeve else "jacket_caps" if is_cap else "jacket_torso"
        for vertex in component:
            if is_sleeve:
                weights = jacket_sleeve_weights(jacket_positions[vertex])
            elif is_cap:
                weights = jacket_cap_weights(jacket_positions[vertex])
            else:
                weights = garment_torso_weights(jacket_positions[vertex], shoulder_limit=0.22)
            write_weights(data, binary_start, document, jacket_attributes["JOINTS_0"],
                          jacket_attributes["WEIGHTS_0"], vertex, weights, joint_lookup)
            repaired[category] += 1

    expected = {
        "body_torso": 47,
        "shirt_torso": 28,
        "shirt_sleeves": 28,
        "jacket_torso": 102,
        "jacket_caps": 16,
        "jacket_sleeves": 96,
    }
    if repaired != expected:
        raise ValueError(f"Unexpected topology classification: {repaired!r}; expected {expected!r}")

    arguments.output.parent.mkdir(parents=True, exist_ok=True)
    arguments.output.write_bytes(data)
    print(f"Repaired {arguments.source.name} -> {arguments.output}")
    print("Regions: " + ", ".join(f"{name}={count}" for name, count in repaired.items()))


if __name__ == "__main__":
    main()
