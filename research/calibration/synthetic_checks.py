"""Known-truth checks and counterexamples for sparse functional calibration."""
import json

import numpy as np
from scipy.optimize._numdiff import approx_derivative
from scipy.spatial.transform import Rotation

from functional_fit import closure_error, distance_jacobian, fit_distance, metric, shared_center
from hinge_fit import fit_circle
from session_io import save_json


def hinge_sequence(n=400, wrist_motion=True, noise=0, outliers=False):
    rng = np.random.default_rng(20261031)
    t = np.linspace(0, 6*np.pi, n)
    ra = Rotation.from_euler("xyz", np.c_[.5*np.sin(t), .3*np.cos(t*.7), .4*np.sin(t*.4)]).as_matrix()
    relative = Rotation.from_euler("z", .9+.8*np.sin(t*.8)).as_matrix()
    rf = ra @ relative
    wrist = Rotation.from_euler("xyz", np.c_[.4*np.sin(t*1.3), .6*np.cos(t*.9), .3*np.sin(t*1.7)]).as_matrix()
    rb = rf @ wrist if wrist_motion else rf
    a, b, length = np.array([.02, .08, -.01]), np.array([.01, -.07, .02]), .26
    center = np.c_[.2*np.sin(t*.2), 1.2+.1*np.sin(t*.4), .1*np.cos(t*.3)]
    pa = center - np.einsum("nij,j->ni", ra, a)
    end = center + np.einsum("nij,j->ni", rf, np.array([0, -length, 0]))
    pb = end - np.einsum("nij,j->ni", rb, b)
    pa += rng.normal(0, noise, pa.shape)
    pb += rng.normal(0, noise, pb.shape)
    if outliers:
        pb[::20] += rng.normal(0, .10, pb[::20].shape)
    return (t, ra, pa, rb, pb), np.r_[a, b, length]


def check_distance_gauge():
    arrays, truth = hinge_sequence()
    errors = {}
    for shift in (0, .05, .10, .20):
        equivalent = truth.copy()
        equivalent[2] += shift
        equivalent[6] = np.sqrt(truth[6]**2+shift**2)
        residual = closure_error(arrays, equivalent, True)
        assert np.max(np.abs(residual)) < 1e-12
        errors[str(shift)] = dict(length_m=float(equivalent[6]), residual=metric(residual))
    numerical = approx_derivative(lambda x: closure_error(arrays, x, True), truth)
    assert np.max(np.abs(numerical-distance_jacobian(arrays, truth))) < 1e-7
    return errors


def check_joint_centres():
    rng = np.random.default_rng(123)
    ra = Rotation.random(200, random_state=rng).as_matrix()
    rb = Rotation.random(200, random_state=rng).as_matrix()
    a, b = np.array([.1, .2, .03]), np.array([-.03, -.2, .02])
    center = rng.normal(0, .4, (200, 3))
    pa = center - np.einsum("nij,j->ni", ra, a)
    pb = center - np.einsum("nij,j->ni", rb, b)
    arrays = np.arange(200), ra, pa, rb, pb
    fit = shared_center(arrays)
    error = np.max(np.abs(fit["x"]-np.r_[a, b]))
    assert error < 1e-12 and fit["rank"] == 6
    transform = Rotation.from_euler("xyz", [.3, .2, -.7]).as_matrix()
    translation = np.array([3, -2, 5])
    moved = (arrays[0], transform@ra, pa@transform.T+translation,
             transform@rb, pb@transform.T+translation)
    assert np.max(np.abs(shared_center(moved)["x"]-fit["x"])) < 1e-12
    # Relative calibration also tolerates a common time-varying world transform;
    # world motion/contact/dynamics still need the reset history.
    world = Rotation.random(200, random_state=rng).as_matrix()
    moved_each_frame = (arrays[0], world@ra, np.einsum("nij,nj->ni", world, pa)+translation,
                        world@rb, np.einsum("nij,nj->ni", world, pb)+translation)
    assert np.max(np.abs(shared_center(moved_each_frame)["x"]-fit["x"])) < 1e-12
    contaminated = list(arrays)
    contaminated[4] = pb.copy()
    contaminated[4][::10] += rng.normal(0, .15, (20, 3))
    ols = shared_center(contaminated)
    robust = shared_center(contaminated, True)
    ols_error = np.linalg.norm(ols["x"]-np.r_[a, b])
    robust_error = np.linalg.norm(robust["x"]-np.r_[a, b])
    assert robust_error < ols_error
    return dict(exact_error_m=float(error), outlier_ols_offset_error_mm=float(ols_error*1000),
                outlier_robust_offset_error_mm=float(robust_error*1000), world_transform_invariant=True)


def check_unconstrained_distance():
    # A general constant-distance mechanism tests the solver, not human anatomy.
    rng = np.random.default_rng(71)
    n = 300
    ra = Rotation.random(n, random_state=rng).as_matrix()
    rb = Rotation.random(n, random_state=rng).as_matrix()
    unit = rng.normal(size=(n, 3))
    unit /= np.linalg.norm(unit, axis=1, keepdims=True)
    a, b, length = np.array([.02, .08, -.01]), np.array([.01, -.07, .02]), .26
    pa = rng.normal(0, .4, (n, 3))
    pb = pa + np.einsum("nij,j->ni", ra, a) - np.einsum("nij,j->ni", rb, b) - unit*length
    arrays = np.arange(n), ra, pa, rb, pb
    fit = fit_distance(arrays, starts=5)
    error = np.max(np.abs(fit["x"]-np.r_[a, b, length]))
    assert error < 1e-6
    return dict(offset_length_error_m=float(error), effective_rank=fit["effective_rank"])


def check_hinge_selection():
    arrays, truth = hinge_sequence()
    proximal = truth[:3]+np.array([0, .31, 0])
    fit = fit_circle(arrays, starts=6, proximal_center=proximal)
    assert abs(fit["selected_length_m"]-truth[6]) < 1e-5
    assert np.linalg.norm(np.array(fit["selected_center"])-truth[:3]) < 1e-5
    assert abs(fit["proximal_bone_length_m"]-.31) < 1e-5
    locked, truth = hinge_sequence(wrist_motion=False)
    singular = np.linalg.svd(distance_jacobian(locked, truth), compute_uv=False)
    rank = int(np.sum(singular > singular[0]*1e-8))
    assert rank <= 3
    return dict(ideal_hinge_with_perpendicular_prior_recovers_geometry=True,
                locked_distal_distance_rank=rank,
                caveat="Perpendicular-axis selection is a modelling convention, not an anatomical measurement.")


def noise_probe():
    rows = []
    rng = np.random.default_rng(220)
    for position_noise, rotation_noise_deg in [(0, 0), (.002, .3), (.005, 1)]:
        arrays, truth = hinge_sequence(noise=position_noise)
        arrays = list(arrays)
        for index in (1, 3):
            perturb = Rotation.from_rotvec(rng.normal(0, np.radians(rotation_noise_deg), (len(arrays[0]), 3))).as_matrix()
            arrays[index] = arrays[index]@perturb
        fit = fit_circle(arrays, starts=5, proximal_center=truth[:3]+np.array([0, .31, 0]))
        rows.append(dict(position_noise_mm=position_noise*1000, rotation_noise_deg=rotation_noise_deg,
                         length_error_mm=(fit["selected_length_m"]-truth[6])*1000,
                         proximal_center_error_mm=float(np.linalg.norm(np.array(fit["selected_center"])-truth[:3])*1000)))
    return rows


def remount_probe():
    rng, n = np.random.default_rng(809), 250
    rotations = [Rotation.random(n, random_state=rng).as_matrix() for _ in range(3)]
    shoulder = rng.normal(0, .1, (n, 3))
    elbow = shoulder + np.einsum("nij,j->ni", rotations[1], [0, -.31, 0])
    origins = [shoulder, shoulder, elbow]
    base_mounts = [(np.eye(3), np.array([.03, -.04, .02])) for _ in range(3)]
    new_mounts = [(Rotation.from_euler("xyz", [.4, -.3, .2]).as_matrix(),
                   np.array([-.01, .02, .05])) for _ in range(3)]

    def capture(mounts):
        return [(r@mount[0], p+np.einsum("nij,j->ni", r, mount[1]))
                for r, p, mount in zip(rotations, origins, mounts)]

    results, first_offsets = [], None
    for mounts in (base_mounts, new_mounts):
        poses = capture(mounts)
        first = shared_center((np.arange(n), *poses[0], *poses[1]))["x"]
        second = shared_center((np.arange(n), *poses[1], *poses[2]))["x"]
        length = np.linalg.norm(second[:3]-first[3:6])
        assert abs(length-.31) < 1e-12
        if first_offsets is None:
            first_offsets = first
            wrong_error = 0.0
        else:
            wrong_joint = poses[1][1]+np.einsum("nij,j->ni", poses[1][0], first_offsets[3:6])
            wrong_error = float(np.mean(np.linalg.norm(wrong_joint-shoulder, axis=1))*1000)
            assert wrong_error > 20
        results.append(dict(recovered_length_mm=float(length*1000),
                            reused_old_mount_joint_error_mm=wrong_error))
    return dict(cases=results, assumption="Three instrumented adjacent rigid bodies, rich ball-joint motion; not the current missing-segment topology.")


def main():
    result = dict(shared_centres=check_joint_centres(),
                  analytic_distance_jacobian_and_hinge_counterexample=check_distance_gauge(),
                  general_distance_solver=check_unconstrained_distance(),
                  hinge_selection=check_hinge_selection(), ideal_model_noise_probe=noise_probe(),
                  remount_probe=remount_probe())
    save_json("synthetic_checks.json", result)
    print(json.dumps(result, indent=2))


if __name__ == "__main__":
    main()
