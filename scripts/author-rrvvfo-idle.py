#!/usr/bin/env python3
"""Author Rrvvfo's six-pose Legacy idle on his unchanged GLB rig.

Legacy `idle_01` through `idle_06` are distinct silhouette keys, each held for
125 ms.  This script translates those keys to the existing deform joints and
stores animation data only; no Rrvvfo choreography lives in gameplay/renderers.
"""

from __future__ import annotations

import argparse
import json
import math
import struct
from pathlib import Path


FRAME_SECONDS = 0.125
IDLE_DURATION = 6 * FRAME_SECONDS
FRAME_TIMES = [frame * FRAME_SECONDS for frame in range(6)]
KEY_TIMES = FRAME_TIMES + [IDLE_DURATION]


def normalize(values):
    length = math.sqrt(sum(value * value for value in values))
    if length <= 1.0e-10:
        raise ValueError("Cannot normalize a zero-length value")
    return [value / length for value in values]


def multiply(a, b):
    ax, ay, az, aw = a
    bx, by, bz, bw = b
    return normalize([
        aw * bx + ax * bw + ay * bz - az * by,
        aw * by - ax * bz + ay * bw + az * bx,
        aw * bz + ax * by - ay * bx + az * bw,
        aw * bw - ax * bx - ay * by - az * bz,
    ])


def inverse(q):
    return [-q[0], -q[1], -q[2], q[3]]


def rotate(q, value):
    vector = [value[0], value[1], value[2], 0.0]

    def raw(a, b):
        ax, ay, az, aw = a
        bx, by, bz, bw = b
        return [
            aw * bx + ax * bw + ay * bz - az * by,
            aw * by - ax * bz + ay * bw + az * bx,
            aw * bz + ax * by - ay * bx + az * bw,
            aw * bw - ax * bx - ay * by - az * bz,
        ]

    return raw(raw(q, vector), inverse(q))[:3]


def from_to(source, target):
    source = normalize(source)
    target = normalize(target)
    dot = sum(a * b for a, b in zip(source, target))
    if dot < -0.9999:
        axis = normalize([0.0, -source[2], source[1]]) if abs(source[0]) < 0.9 else normalize([-source[1], source[0], 0.0])
        return [axis[0], axis[1], axis[2], 0.0]
    cross = [
        source[1] * target[2] - source[2] * target[1],
        source[2] * target[0] - source[0] * target[2],
        source[0] * target[1] - source[1] * target[0],
    ]
    return normalize([cross[0], cross[1], cross[2], 1.0 + dot])


def axis_angle(axis, degrees):
    axis = normalize(axis)
    half = math.radians(degrees) * 0.5
    sine = math.sin(half)
    return [axis[0] * sine, axis[1] * sine, axis[2] * sine, math.cos(half)]


def delta_rotation(*axis_degrees):
    result = [0.0, 0.0, 0.0, 1.0]
    for axis, degrees in axis_degrees:
        result = multiply(result, axis_angle(axis, degrees))
    return result


def parse_glb(path: Path):
    data = path.read_bytes()
    magic, version, length = struct.unpack_from("<4sII", data, 0)
    if magic != b"glTF" or version != 2 or length != len(data):
        raise ValueError("Expected a complete glTF 2.0 binary")
    chunk_length, chunk_type = struct.unpack_from("<II", data, 12)
    if chunk_type != 0x4E4F534A:
        raise ValueError("GLB JSON chunk is missing")
    return json.loads(data[20 : 20 + chunk_length].decode("utf-8"))


# Each row is one actual Legacy sprite silhouette, not a generated breathing
# curve.  Direction vectors are in the model's character space (+Y is up,
# +Z is forward).  Frames 1-4 carry the open front-facing stance; frames 5-6
# carry the recognizable guarded three-quarter attitude from the sheet.
POSES = [
    {
        "root_yaw": -7.0, "root_height": 0.000, "torso_lean": -2.0, "torso_pitch": -1.0, "head_yaw": 5.0,
        "left_upper": [0.18, -0.93, 0.32], "left_fore": [0.05, -0.97, 0.24],
        "right_upper": [-0.20, -0.91, 0.36], "right_fore": [-0.06, -0.96, 0.28],
        "left_thigh": [0.17, -0.97, 0.17], "left_shin": [0.09, -0.99, -0.06],
        "right_thigh": [-0.22, -0.96, 0.18], "right_shin": [-0.12, -0.99, -0.05],
    },
    {
        "root_yaw": -3.0, "root_height": 0.006, "torso_lean": -0.7, "torso_pitch": -0.3, "head_yaw": 2.0,
        "left_upper": [0.16, -0.94, 0.30], "left_fore": [0.04, -0.98, 0.19],
        "right_upper": [-0.18, -0.92, 0.35], "right_fore": [-0.05, -0.97, 0.24],
        "left_thigh": [0.15, -0.98, 0.15], "left_shin": [0.08, -0.99, -0.05],
        "right_thigh": [-0.20, -0.97, 0.17], "right_shin": [-0.11, -0.99, -0.04],
    },
    {
        "root_yaw": 2.0, "root_height": 0.000, "torso_lean": 1.2, "torso_pitch": 0.5, "head_yaw": -2.0,
        "left_upper": [0.20, -0.91, 0.36], "left_fore": [0.06, -0.96, 0.28],
        "right_upper": [-0.17, -0.94, 0.29], "right_fore": [-0.04, -0.98, 0.19],
        "left_thigh": [0.18, -0.96, 0.19], "left_shin": [0.10, -0.99, -0.06],
        "right_thigh": [-0.17, -0.98, 0.14], "right_shin": [-0.09, -0.99, -0.04],
    },
    {
        "root_yaw": 0.0, "root_height": -0.004, "torso_lean": 0.0, "torso_pitch": 0.8, "head_yaw": 0.0,
        "left_upper": [0.19, -0.92, 0.34], "left_fore": [0.05, -0.97, 0.24],
        "right_upper": [-0.19, -0.92, 0.34], "right_fore": [-0.05, -0.97, 0.24],
        "left_thigh": [0.18, -0.97, 0.16], "left_shin": [0.10, -0.99, -0.05],
        "right_thigh": [-0.18, -0.97, 0.16], "right_shin": [-0.10, -0.99, -0.05],
    },
    {
        "root_yaw": 17.0, "root_height": -0.012, "torso_lean": 2.8, "torso_pitch": 3.2, "head_yaw": -9.0,
        "left_upper": [0.12, -0.84, 0.53], "left_fore": [0.04, -0.15, 0.99],
        "right_upper": [-0.15, -0.91, 0.38], "right_fore": [-0.05, -0.97, 0.23],
        "left_thigh": [0.13, -0.91, 0.40], "left_shin": [0.07, -0.99, -0.09],
        "right_thigh": [-0.24, -0.96, 0.14], "right_shin": [-0.13, -0.99, -0.03],
    },
    {
        "root_yaw": 12.0, "root_height": -0.006, "torso_lean": 2.0, "torso_pitch": 2.2, "head_yaw": -6.0,
        "left_upper": [0.15, -0.88, 0.46], "left_fore": [0.05, -0.95, 0.31],
        "right_upper": [-0.16, -0.92, 0.36], "right_fore": [-0.05, -0.97, 0.23],
        "left_thigh": [0.14, -0.93, 0.34], "left_shin": [0.08, -0.99, -0.08],
        "right_thigh": [-0.22, -0.96, 0.16], "right_shin": [-0.12, -0.99, -0.04],
    },
]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("source", type=Path)
    parser.add_argument("output", type=Path)
    arguments = parser.parse_args()
    document = parse_glb(arguments.source)
    nodes = document["nodes"]
    joint_nodes = document["skins"][0]["joints"]
    joint_lookup = {node: index for index, node in enumerate(joint_nodes)}
    name_lookup = {nodes[node].get("name", f"joint_{index}"): index for index, node in enumerate(joint_nodes)}
    parents = [-1] * len(nodes)
    for parent, node in enumerate(nodes):
        for child in node.get("children", []):
            parents[child] = parent

    local_bind = [normalize(nodes[node].get("rotation", [0, 0, 0, 1])) for node in joint_nodes]
    translation_bind = [list(nodes[node].get("translation", [0, 0, 0])) for node in joint_nodes]
    joint_parents = []
    for node in joint_nodes:
        parent = parents[node]
        while parent >= 0 and parent not in joint_lookup:
            parent = parents[parent]
        joint_parents.append(joint_lookup.get(parent, -1))

    required = {
        "spine", "pelvis.L", "pelvis.R", "spine.001", "spine.002", "spine.003",
        "shoulder.L", "upper_arm.L", "forearm.L", "hand.L",
        "Middle.L", "Pinky.L", "Ring.L", "Thumb.L", "Upper.L",
        "shoulder.R", "upper_arm.R", "forearm.R", "hand.R",
        "hand.R.001", "hand.R.002", "Middle.R", "Pinky.R", "Ring.R", "Thumb.R", "Upper.R",
        "spine.004", "spine.005", "spine.006",
        "thigh.L", "shin.L", "foot.L", "thigh.R", "shin.R", "foot.R",
    }
    missing = sorted(required.difference(name_lookup))
    if missing:
        raise ValueError(f"Rrvvfo rig is missing required deform joints: {', '.join(missing)}")

    rotation_samples = {name: [] for name in required}
    translation_samples = {"spine": []}

    for pose in POSES:
        local = list(local_bind)
        world = [None] * len(joint_nodes)

        def refresh():
            for index in range(len(joint_nodes)):
                parent = joint_parents[index]
                world[index] = local[index] if parent < 0 else multiply(world[parent], local[index])

        def add_local(joint_name, *axis_degrees):
            index = name_lookup[joint_name]
            local[index] = multiply(local_bind[index], delta_rotation(*axis_degrees))
            refresh()

        def aim(joint_name, target):
            index = name_lookup[joint_name]
            parent = joint_parents[index]
            current = rotate(world[index], [0.0, 1.0, 0.0])
            desired_world = multiply(from_to(current, target), world[index])
            local[index] = desired_world if parent < 0 else multiply(inverse(world[parent]), desired_world)
            refresh()

        add_local("spine", ([0, 1, 0], pose["root_yaw"]), ([0, 0, 1], pose["torso_lean"] * 0.30))
        add_local("spine.001", ([0, 0, 1], pose["torso_lean"] * 0.36), ([1, 0, 0], pose["torso_pitch"] * 0.32))
        add_local("spine.002", ([0, 0, 1], pose["torso_lean"] * 0.42), ([1, 0, 0], pose["torso_pitch"] * 0.38))
        add_local("spine.003", ([0, 0, 1], pose["torso_lean"] * 0.24), ([1, 0, 0], pose["torso_pitch"] * 0.30))
        add_local("pelvis.L", ([0, 0, 1], -pose["torso_lean"] * 0.22),)
        add_local("pelvis.R", ([0, 0, 1], -pose["torso_lean"] * 0.22),)

        # Clavicle tilt carries the shoulder asymmetry visible in every frame.
        add_local("shoulder.L", ([0, 0, 1], -3.0 - pose["torso_lean"] * 0.55),)
        add_local("shoulder.R", ([0, 0, 1], 1.5 - pose["torso_lean"] * 0.30),)
        aim("upper_arm.L", pose["left_upper"])
        aim("forearm.L", pose["left_fore"])
        aim("hand.L", normalize([pose["left_fore"][0] * 0.45, pose["left_fore"][1], pose["left_fore"][2]]))
        for finger in ("Middle.L", "Pinky.L", "Ring.L", "Upper.L"):
            add_local(finger, ([1, 0, 0], 68.0),)
        add_local("Thumb.L", ([0, 0, 1], 42.0), ([1, 0, 0], 24.0))
        aim("upper_arm.R", pose["right_upper"])
        aim("forearm.R", pose["right_fore"])
        aim("hand.R", normalize([pose["right_fore"][0] * 0.45, pose["right_fore"][1], pose["right_fore"][2]]))
        for finger in ("hand.R.001", "hand.R.002", "Middle.R", "Pinky.R", "Ring.R", "Upper.R"):
            add_local(finger, ([1, 0, 0], 68.0),)
        add_local("Thumb.R", ([0, 0, 1], -42.0), ([1, 0, 0], 24.0))

        # A wide, slightly bent stance replaces the narrow bind-pose legs.
        aim("thigh.L", pose["left_thigh"])
        aim("shin.L", pose["left_shin"])
        aim("foot.L", [0.08, -0.52, 0.85])
        aim("thigh.R", pose["right_thigh"])
        aim("shin.R", pose["right_shin"])
        aim("foot.R", [-0.08, -0.52, 0.85])

        add_local("spine.004", ([0, 1, 0], pose["head_yaw"] * 0.42), ([1, 0, 0], -pose["torso_pitch"] * 0.18))
        add_local("spine.005", ([0, 1, 0], pose["head_yaw"] * 0.35), ([0, 0, 1], -pose["torso_lean"] * 0.16))
        add_local("spine.006", ([0, 1, 0], pose["head_yaw"] * 0.23),)

        for joint_name in required:
            rotation_samples[joint_name].append(local[name_lookup[joint_name]])
        root_translation = list(translation_bind[name_lookup["spine"]])
        root_translation[1] += pose["root_height"]
        translation_samples["spine"].append(root_translation)

    # A closure key repeats Legacy frame 1 at 0.750 s so linear interpolation
    # flows back to the first silhouette without inventing a seventh pose.
    for samples in rotation_samples.values():
        samples.append(samples[0])
    translation_samples["spine"].append(translation_samples["spine"][0])

    tracks = []
    ordered_joints = [
        "spine", "pelvis.L", "pelvis.R", "spine.001", "spine.002", "spine.003",
        "shoulder.L", "upper_arm.L", "forearm.L", "hand.L",
        "Middle.L", "Pinky.L", "Ring.L", "Thumb.L", "Upper.L",
        "shoulder.R", "upper_arm.R", "forearm.R", "hand.R",
        "hand.R.001", "hand.R.002", "Middle.R", "Pinky.R", "Ring.R", "Thumb.R", "Upper.R",
        "spine.004", "spine.005", "spine.006",
        "thigh.L", "shin.L", "foot.L", "thigh.R", "shin.R", "foot.R",
    ]
    for joint_name in ordered_joints:
        track = {
            "joint": joint_name,
            "interpolation": "LINEAR",
            "rotation": [
                {"time": time, "value": value}
                for time, value in zip(KEY_TIMES, rotation_samples[joint_name])
            ],
        }
        if joint_name in translation_samples:
            track["translation"] = [
                {"time": time, "value": value}
                for time, value in zip(KEY_TIMES, translation_samples[joint_name])
            ]
        tracks.append(track)

    payload = {
        "version": 1,
        "source": "Legacy rrvvfo idle_01..idle_06; six silhouette keys at 125 ms each",
        "clips": [{"name": "idle", "duration": IDLE_DURATION, "loop": True, "tracks": tracks}],
    }
    arguments.output.parent.mkdir(parents=True, exist_ok=True)
    arguments.output.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")
    print(
        f"Authored six-pose Legacy idle: {len(tracks)} joint tracks, "
        f"{IDLE_DURATION:.3f}s -> {arguments.output}"
    )


if __name__ == "__main__":
    main()
