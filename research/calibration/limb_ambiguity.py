"""Exact full-limb counterexample: ball shoulder, hinge elbow, two-axis wrist."""
import json

import numpy as np
from scipy.spatial.transform import Rotation

from session_io import save_json


def main():
    t = np.linspace(0, 12, 400)
    upper_rotation = Rotation.from_euler("xyz", np.c_[.4*np.sin(t), .6*np.sin(.8*t), .3*np.cos(t)]).as_matrix()
    elbow_angle = .9+.7*np.sin(.7*t)
    wrist_x, wrist_z = .3*np.sin(1.3*t), .25*np.cos(1.7*t)
    rx = lambda angle: Rotation.from_euler("x", angle).as_matrix()
    rz = lambda angle: Rotation.from_euler("z", angle).as_matrix()
    forearm_rotation = upper_rotation@rz(elbow_angle)
    hand_rotation = forearm_rotation@rx(wrist_x)@rz(wrist_z)
    elbow = np.c_[.2*np.sin(.2*t), 1.2+.1*np.cos(t), .1*np.cos(.3*t)]
    shoulder = elbow + np.einsum("nij,j->ni", upper_rotation, [0, .31, 0])
    wrist = elbow + np.einsum("nij,j->ni", forearm_rotation, [0, -.26, 0])
    upper_local_elbow, hand_local_wrist = np.array([.02, .08, -.01]), np.array([.01, -.07, .02])
    sensor_upper = elbow - np.einsum("nij,j->ni", upper_rotation, upper_local_elbow)
    sensor_hand = wrist - np.einsum("nij,j->ni", hand_rotation, hand_local_wrist)
    rows = []
    for shift in (0, .02, .05, .10):
        new_elbow = elbow + np.einsum("nij,j->ni", upper_rotation, [0, 0, shift])
        new_forearm_length = np.hypot(.26, shift)
        fixed_tilt = np.arctan2(shift, .26)
        new_forearm_rotation = forearm_rotation@rx(fixed_tilt)
        new_hand_rotation = new_forearm_rotation@rx(wrist_x-fixed_tilt)@rz(wrist_z)
        new_wrist = new_elbow + np.einsum("nij,j->ni", new_forearm_rotation, [0, -new_forearm_length, 0])
        new_upper_sensor = new_elbow - np.einsum("nij,j->ni", upper_rotation, upper_local_elbow+[0, 0, shift])
        new_hand_sensor = new_wrist - np.einsum("nij,j->ni", new_hand_rotation, hand_local_wrist)
        sensor_position_error = max(np.max(np.abs(new_upper_sensor-sensor_upper)),
                                    np.max(np.abs(new_hand_sensor-sensor_hand)))
        hand_rotation_error = np.max(Rotation.from_matrix(np.swapaxes(hand_rotation, 1, 2)@new_hand_rotation).magnitude())
        upper_lengths = np.linalg.norm(new_elbow-shoulder, axis=1)
        assert sensor_position_error < 1e-12 and hand_rotation_error < 1e-12
        assert np.ptp(upper_lengths) < 1e-12
        rows.append(dict(elbow_shift_mm=shift*1000,
                         upper_length_mm=float(upper_lengths[0]*1000),
                         forearm_length_mm=float(new_forearm_length*1000),
                         wrist_zero_change_deg=float(np.degrees(fixed_tilt)),
                         max_tracker_position_error_m=float(sensor_position_error),
                         max_hand_rotation_error_rad=float(hand_rotation_error)))
    save_json("full_limb_ambiguity.json", dict(
        model="Ball shoulder, fixed upper geometry, elbow hinge, two-axis wrist; unknown joint anatomy/zero conventions",
        conclusion="Upstream shoulder continuity and a two-axis wrist alone do not eliminate the family.",
        limitation="This is a specific noiseless model counterexample, not a proof against every anatomical model/reference constraint.",
        cases=rows))
    print(json.dumps(rows, indent=2))


if __name__ == "__main__":
    main()
