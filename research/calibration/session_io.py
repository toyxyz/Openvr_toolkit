"""Read-only recorder session adapter for calibration experiments (SI units)."""
import hashlib
import json
from pathlib import Path
import struct

import numpy as np
from scipy.spatial.transform import Rotation

ROLES = ("head", "chest", "pelvis", "left_arm", "right_arm", "left_hand", "right_hand",
         "left_leg", "right_leg", "left_foot", "right_foot")
ROOT = Path(__file__).resolve().parents[2]
RECORDINGS = ROOT / "build/vs2022/Debug/recordings"
OUTPUT = ROOT / "build/calibration_research"


def settings(path):
    if not path.exists():
        return {}
    return dict(line.split("=", 1) for line in path.read_text(encoding="utf-8-sig").splitlines()
                if "=" in line and not line.startswith("#"))


def index_entries(folder):
    data = (folder / "frame_index.bin").read_bytes()
    if len(data) < 16:
        raise ValueError(f"Truncated index: {folder.name}")
    magic, version, entry_size = struct.unpack_from("<8sII", data)
    if magic != b"OVTRIDX\0" or version != 1 or entry_size != 24 or (len(data)-16) % 24:
        raise ValueError(f"Unsupported index: {folder.name}")
    return np.array(list(struct.iter_unpack("<QQQ", data[16:])), dtype=np.uint64).reshape(-1, 3)


def metadata(folder, actor=0):
    manifest = json.loads((folder / "manifest.json").read_text(encoding="utf-8-sig"))
    config = settings(folder / "mapping_snapshot.cfg")
    by_serial = {device["serial"]: device for device in manifest["devices"]}
    bindings, attachment = {}, []
    for slot, role in enumerate(ROLES):
        key = f"actor_{actor}_slot_{slot}_serial"
        serial = config.get(key, config.get(f"mapping_slot_{slot}_serial", ""))
        device = by_serial.get(serial)
        if device:
            bindings[role] = device["id"]
        attachment.append(serial)
    fingerprint = hashlib.sha256(json.dumps(attachment).encode()).hexdigest()[:12]
    return manifest, config, bindings, fingerprint


def load_session(folder, max_samples=3000, actor=0):
    folder = Path(folder)
    manifest, config, bindings, fingerprint = metadata(folder, actor)
    entries = index_entries(folder)
    if len(entries) == 0:
        raise ValueError(f"Empty session: {folder.name}")
    ids = sorted(set(bindings.values()))
    id_to_column = {device_id: column for column, device_id in enumerate(ids)}
    selected = entries[::max(1, int(np.ceil(len(entries)/max_samples)))] if max_samples else entries
    times = np.empty(len(selected))
    positions = np.full((len(selected), len(ids), 3), np.nan)
    quaternions = np.full((len(selected), len(ids), 4), np.nan)
    valid = np.zeros((len(selected), len(ids)), dtype=bool)
    with (folder / "frames.bin").open("rb") as file:
        header = file.read(24)
        if len(header) != 24 or struct.unpack("<8sIIII", header)[:4] != (
                b"OVTRBIN\0", 1, 0x01020304, 24):
            raise ValueError(f"Unsupported frame header: {folder.name}")
        for i, (_, _, offset) in enumerate(selected):
            file.seek(int(offset))
            _, _, times[i], count, _ = struct.unpack("<QQdII", file.read(32))
            if count > 256:
                raise ValueError(f"Unreasonable pose count: {folder.name}")
            for _ in range(count):
                row = struct.unpack("<II13fI", file.read(64))
                if row[0] not in id_to_column:
                    continue
                column = id_to_column[row[0]]
                position, quaternion = np.array(row[2:5]), np.array(row[5:9])
                norm = np.linalg.norm(quaternion)
                okay = bool(row[-1] & 2) and np.isfinite(position).all() and np.isfinite(quaternion).all() and norm > 1e-8
                if okay:
                    positions[i, column] = position
                    quaternions[i, column] = quaternion / norm
                    valid[i, column] = True
    if np.any(np.diff(times) <= 0):
        raise ValueError(f"Non-monotonic sampled times: {folder.name}")
    rotations = np.full((len(selected), len(ids), 3, 3), np.nan)
    rotations[valid] = Rotation.from_quat(quaternions[valid]).as_matrix()
    return dict(name=folder.name, folder=folder, times=times, positions=positions,
                rotations=rotations, valid=valid,
                role_columns={role: id_to_column[device_id] for role, device_id in bindings.items()},
                manifest=manifest, config=config, fingerprint=fingerprint, total_frames=len(entries))


def pair_arrays(session, a, b):
    ca, cb = session["role_columns"][a], session["role_columns"][b]
    mask = session["valid"][:, ca] & session["valid"][:, cb]
    return (session["times"][mask], session["rotations"][mask, ca], session["positions"][mask, ca],
            session["rotations"][mask, cb], session["positions"][mask, cb])


def save_json(name, value):
    OUTPUT.mkdir(parents=True, exist_ok=True)
    (OUTPUT / name).write_text(json.dumps(value, indent=2, allow_nan=False), encoding="utf-8")
