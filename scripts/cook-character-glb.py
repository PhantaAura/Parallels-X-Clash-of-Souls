#!/usr/bin/env python3
"""Cook the subset of glTF 2.0 used by Parallels X character assets.

The source GLB remains authoritative.  This deterministic desktop cook keeps
geometry, material slots, joint names, hierarchy, inverse bind matrices, skin
weights, and optional authored animation clips in a small engine-owned format.
"""

from __future__ import annotations

import argparse
import json
import math
import struct
from pathlib import Path


MAGIC = b"PXSKEL1\0"
VERSION = 2
COMPONENTS = {
    5120: ("b", 1),
    5121: ("B", 1),
    5122: ("h", 2),
    5123: ("H", 2),
    5125: ("I", 4),
    5126: ("f", 4),
}
TYPE_COMPONENTS = {"SCALAR": 1, "VEC2": 2, "VEC3": 3, "VEC4": 4, "MAT4": 16}


def identity() -> list[list[float]]:
    return [[1.0 if row == column else 0.0 for column in range(4)] for row in range(4)]


def multiply(a: list[list[float]], b: list[list[float]]) -> list[list[float]]:
    return [[sum(a[row][k] * b[k][column] for k in range(4)) for column in range(4)] for row in range(4)]


def transform_point(matrix: list[list[float]], value: tuple[float, float, float]) -> tuple[float, float, float]:
    result = [sum(matrix[row][column] * (*value, 1.0)[column] for column in range(4)) for row in range(4)]
    divisor = result[3] if abs(result[3]) > 1.0e-8 else 1.0
    return result[0] / divisor, result[1] / divisor, result[2] / divisor


def transform_direction(matrix: list[list[float]], value: tuple[float, float, float]) -> tuple[float, float, float]:
    result = [sum(matrix[row][column] * value[column] for column in range(3)) for row in range(3)]
    length = math.sqrt(sum(component * component for component in result))
    if length <= 1.0e-8:
        return 0.0, 1.0, 0.0
    return result[0] / length, result[1] / length, result[2] / length


def node_matrix(node: dict) -> list[list[float]]:
    if "matrix" in node:
        values = node["matrix"]
        return [[float(values[column * 4 + row]) for column in range(4)] for row in range(4)]
    tx, ty, tz = node.get("translation", [0.0, 0.0, 0.0])
    x, y, z, w = node.get("rotation", [0.0, 0.0, 0.0, 1.0])
    sx, sy, sz = node.get("scale", [1.0, 1.0, 1.0])
    rotation = [
        [1 - 2 * (y * y + z * z), 2 * (x * y - z * w), 2 * (x * z + y * w), 0.0],
        [2 * (x * y + z * w), 1 - 2 * (x * x + z * z), 2 * (y * z - x * w), 0.0],
        [2 * (x * z - y * w), 2 * (y * z + x * w), 1 - 2 * (x * x + y * y), 0.0],
        [0.0, 0.0, 0.0, 1.0],
    ]
    scale = identity()
    scale[0][0], scale[1][1], scale[2][2] = sx, sy, sz
    translation = identity()
    translation[0][3], translation[1][3], translation[2][3] = tx, ty, tz
    return multiply(translation, multiply(rotation, scale))


def column_major(matrix: list[list[float]]) -> list[float]:
    return [matrix[row][column] for column in range(4) for row in range(4)]


def parse_glb(path: Path) -> tuple[dict, bytes]:
    data = path.read_bytes()
    if len(data) < 20:
        raise ValueError("GLB is truncated")
    magic, version, declared_length = struct.unpack_from("<4sII", data, 0)
    if magic != b"glTF" or version != 2 or declared_length != len(data):
        raise ValueError("Expected a complete glTF 2.0 binary")
    offset = 12
    document = None
    binary = None
    while offset < len(data):
        chunk_length, chunk_type = struct.unpack_from("<II", data, offset)
        offset += 8
        chunk = data[offset : offset + chunk_length]
        offset += chunk_length
        if chunk_type == 0x4E4F534A:
            document = json.loads(chunk.decode("utf-8"))
        elif chunk_type == 0x004E4942:
            binary = chunk
    if document is None or binary is None:
        raise ValueError("GLB must contain JSON and BIN chunks")
    if len(document.get("buffers", [])) != 1:
        raise ValueError("The character cooker supports one embedded GLB buffer")
    return document, binary


def read_accessor(document: dict, binary: bytes, index: int) -> list[tuple[float | int, ...]]:
    accessor = document["accessors"][index]
    if "sparse" in accessor:
        raise ValueError("Sparse accessors are not supported")
    view = document["bufferViews"][accessor["bufferView"]]
    component_format, component_size = COMPONENTS[accessor["componentType"]]
    component_count = TYPE_COMPONENTS[accessor["type"]]
    packed_size = component_size * component_count
    stride = view.get("byteStride", packed_size)
    start = view.get("byteOffset", 0) + accessor.get("byteOffset", 0)
    result = []
    for item in range(accessor["count"]):
        result.append(struct.unpack_from("<" + component_format * component_count, binary, start + item * stride))
    return result


def descendants(document: dict, root_index: int) -> set[int]:
    result: set[int] = set()
    stack = [root_index]
    while stack:
        index = stack.pop()
        if index in result:
            continue
        result.add(index)
        stack.extend(document["nodes"][index].get("children", []))
    return result


def write_string(output, value: str) -> None:
    encoded = value.encode("utf-8")
    if len(encoded) > 65535:
        raise ValueError("Asset name is too long")
    output.write(struct.pack("<H", len(encoded)))
    output.write(encoded)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("source", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--settings", type=Path, required=True)
    parser.add_argument("--animations", type=Path)
    arguments = parser.parse_args()

    document, binary = parse_glb(arguments.source)
    settings = json.loads(arguments.settings.read_text(encoding="utf-8"))
    nodes = document.get("nodes", [])
    root_name = settings["root_node"]
    roots = [index for index, node in enumerate(nodes) if node.get("name") == root_name]
    if len(roots) != 1:
        raise ValueError(f"Expected exactly one character root named {root_name!r}")
    included_nodes = descendants(document, roots[0])

    parents = [-1] * len(nodes)
    for parent_index, node in enumerate(nodes):
        for child in node.get("children", []):
            parents[child] = parent_index
    local_matrices = [node_matrix(node) for node in nodes]
    world_matrices: list[list[list[float]] | None] = [None] * len(nodes)

    def world(index: int) -> list[list[float]]:
        cached = world_matrices[index]
        if cached is not None:
            return cached
        parent = parents[index]
        cached = local_matrices[index] if parent < 0 else multiply(world(parent), local_matrices[index])
        world_matrices[index] = cached
        return cached

    skins = document.get("skins", [])
    if len(skins) != 1:
        raise ValueError("The DEV character must contain exactly one skin")
    skin = skins[0]
    joint_nodes = skin["joints"]
    joint_lookup = {node_index: joint_index for joint_index, node_index in enumerate(joint_nodes)}
    inverse_bind_values = read_accessor(document, binary, skin["inverseBindMatrices"])
    if len(inverse_bind_values) != len(joint_nodes):
        raise ValueError("Inverse bind matrix count does not match joint count")

    joints = []
    for joint_index, node_index in enumerate(joint_nodes):
        parent_node = parents[node_index]
        while parent_node >= 0 and parent_node not in joint_lookup:
            parent_node = parents[parent_node]
        joints.append({
            "name": nodes[node_index].get("name", f"joint_{joint_index}"),
            "parent": joint_lookup.get(parent_node, -1),
            "local": column_major(local_matrices[node_index]),
            "inverse_bind": [float(value) for value in inverse_bind_values[joint_index]],
        })

    animations = []
    if arguments.animations:
        authored = json.loads(arguments.animations.read_text(encoding="utf-8"))
        if authored.get("version") not in (1, 2) or not isinstance(authored.get("clips"), list):
            raise ValueError("Animation source must use version 1 and contain a clips list")
        named_joints = {joint["name"]: index for index, joint in enumerate(joints)}
        used_names: set[str] = set()
        for source_clip in authored["clips"]:
            name = source_clip.get("name", "")
            duration = float(source_clip.get("duration", 0.0))
            if not name or name in used_names or not math.isfinite(duration) or duration <= 0.0:
                raise ValueError("Animation clips require unique names and positive durations")
            used_names.add(name)
            clip = {"name": name, "duration": duration, "loop": bool(source_clip.get("loop", False)), "tracks": []}
            used_joints: set[int] = set()
            for source_track in source_clip.get("tracks", []):
                joint_name = source_track.get("joint", "")
                if joint_name not in named_joints:
                    raise ValueError(f"Animation {name!r} references unknown joint {joint_name!r}")
                joint_index = named_joints[joint_name]
                if joint_index in used_joints:
                    raise ValueError(f"Animation {name!r} repeats joint {joint_name!r}")
                used_joints.add(joint_index)
                interpolation_name = source_track.get("interpolation", "LINEAR")
                if interpolation_name not in ("LINEAR", "STEP"):
                    raise ValueError("Only LINEAR and STEP animation interpolation are supported")
                interpolation = 0 if interpolation_name == "LINEAR" else 1
                channels = {}
                for channel, width in (("translation", 3), ("rotation", 4), ("scale", 3)):
                    keys = []
                    previous_time = -1.0
                    for key in source_track.get(channel, []):
                        time = float(key["time"])
                        value = [float(component) for component in key["value"]]
                        if len(value) != width or not math.isfinite(time) or time < 0.0 or time > duration or time <= previous_time:
                            raise ValueError(f"Invalid {channel} keys for {name!r}/{joint_name!r}")
                        if not all(math.isfinite(component) for component in value):
                            raise ValueError(f"Non-finite {channel} value for {name!r}/{joint_name!r}")
                        if channel == "rotation":
                            length = math.sqrt(sum(component * component for component in value))
                            if length <= 1.0e-8:
                                raise ValueError("Animation quaternion cannot have zero length")
                            value = [component / length for component in value]
                        if channel == "scale" and any(component <= 0.0 for component in value):
                            raise ValueError("Animation scale components must be positive")
                        keys.append((time, value))
                        previous_time = time
                    channels[channel] = keys
                if not any(channels.values()):
                    raise ValueError(f"Animation track {name!r}/{joint_name!r} has no keys")
                clip["tracks"].append({"joint": joint_index, "interpolation": interpolation, **channels})
            animations.append(clip)

    palette = settings.get("material_fallbacks", {})
    materials = []
    for index, material in enumerate(document.get("materials", [])):
        name = material.get("name", f"material_{index}")
        pbr = material.get("pbrMetallicRoughness", {})
        authored_factor = pbr.get("baseColorFactor")
        if authored_factor is None:
            if name not in palette:
                raise ValueError(f"Material {name!r} has no exported base color or configured fallback")
            authored_factor = palette[name]
        if len(authored_factor) != 4:
            raise ValueError(f"Material {name!r} color must be RGBA")
        materials.append({
            "name": name,
            "color": [float(value) for value in authored_factor],
            "unlit": "KHR_materials_unlit" in material.get("extensions", {}),
            "double_sided": bool(material.get("doubleSided", False)),
        })

    vertices = []
    indices: list[int] = []
    submeshes = []
    for node_index in sorted(included_nodes):
        node = nodes[node_index]
        if "mesh" not in node:
            continue
        node_world = world(node_index)
        for primitive in document["meshes"][node["mesh"]].get("primitives", []):
            if primitive.get("mode", 4) != 4:
                raise ValueError("Only triangle primitives are supported")
            attributes = primitive["attributes"]
            positions = read_accessor(document, binary, attributes["POSITION"])
            normals = read_accessor(document, binary, attributes["NORMAL"])
            if len(positions) != len(normals):
                raise ValueError("Position and normal counts do not match")
            joint_values = read_accessor(document, binary, attributes["JOINTS_0"]) if "JOINTS_0" in attributes else None
            weight_values = read_accessor(document, binary, attributes["WEIGHTS_0"]) if "WEIGHTS_0" in attributes else None
            if (joint_values is None) != (weight_values is None):
                raise ValueError("JOINTS_0 and WEIGHTS_0 must be supplied together")
            first_vertex = len(vertices)
            for vertex_index, (position, normal) in enumerate(zip(positions, normals)):
                baked_position = transform_point(node_world, tuple(float(value) for value in position))
                baked_normal = transform_direction(node_world, tuple(float(value) for value in normal))
                source_joints = joint_values[vertex_index] if joint_values else (0, 0, 0, 0)
                source_weights = weight_values[vertex_index] if weight_values else (0.0, 0.0, 0.0, 0.0)
                weight_total = sum(float(value) for value in source_weights)
                normalized_weights = tuple(float(value) / weight_total for value in source_weights) if weight_total > 1.0e-8 else (0.0, 0.0, 0.0, 0.0)
                if any(int(value) >= len(joints) for value in source_joints):
                    raise ValueError("Vertex references a joint outside the skin")
                vertices.append((baked_position, baked_normal, tuple(int(value) for value in source_joints), normalized_weights))
            primitive_indices = read_accessor(document, binary, primitive["indices"])
            if len(primitive_indices) % 3:
                raise ValueError("Triangle index count is not divisible by three")
            first_index = len(indices)
            indices.extend(first_vertex + int(value[0]) for value in primitive_indices)
            submeshes.append((first_index, len(primitive_indices), int(primitive.get("material", 0))))

    if not vertices or not indices:
        raise ValueError("No character geometry was found beneath the selected root")
    bounds_min = [min(vertex[0][axis] for vertex in vertices) for axis in range(3)]
    bounds_max = [max(vertex[0][axis] for vertex in vertices) for axis in range(3)]

    arguments.output.parent.mkdir(parents=True, exist_ok=True)
    with arguments.output.open("wb") as output:
        output.write(MAGIC)
        output.write(struct.pack("<6I", VERSION, len(materials), len(joints), len(vertices), len(indices), len(submeshes)))
        output.write(struct.pack("<6f", *bounds_min, *bounds_max))
        for material in materials:
            write_string(output, material["name"])
            output.write(struct.pack("<4fBB", *material["color"], material["unlit"], material["double_sided"]))
        for joint in joints:
            write_string(output, joint["name"])
            output.write(struct.pack("<i32f", joint["parent"], *joint["local"], *joint["inverse_bind"]))
        for position, normal, vertex_joints, weights in vertices:
            output.write(struct.pack("<6f4H4f", *position, *normal, *vertex_joints, *weights))
        output.write(struct.pack("<" + "I" * len(indices), *indices))
        for first_index, index_count, material_index in submeshes:
            output.write(struct.pack("<3I", first_index, index_count, material_index))
        output.write(struct.pack("<I", len(animations)))
        for clip in animations:
            write_string(output, clip["name"])
            output.write(struct.pack("<fBI", clip["duration"], clip["loop"], len(clip["tracks"])))
            for track in clip["tracks"]:
                output.write(struct.pack(
                    "<HBBBIII", track["joint"], track["interpolation"], track["interpolation"],
                    track["interpolation"], len(track["translation"]), len(track["rotation"]), len(track["scale"])))
                for channel in ("translation", "rotation", "scale"):
                    for time, value in track[channel]:
                        output.write(struct.pack("<" + "f" * (len(value) + 1), time, *value))

    print(
        f"Cooked {arguments.source.name}: {len(vertices)} vertices, {len(indices)//3} triangles, "
        f"{len(joints)} joints, {len(materials)} materials, {len(animations)} clips -> {arguments.output}"
    )


if __name__ == "__main__":
    main()
