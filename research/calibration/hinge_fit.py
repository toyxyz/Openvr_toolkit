"""Circle-locus hypothesis, with an explicit proximal-centre anatomical convention."""
import numpy as np
from scipy.optimize import least_squares

from functional_fit import metric, transformed


def local_endpoint(arrays, distal_point):
    _, ra, pa, rb, pb = arrays
    return np.einsum("nji,nj->ni", ra, transformed(rb, pb, distal_point)-pa)


def normal_from_angles(angles):
    azimuth, elevation = angles
    return np.array([np.cos(azimuth)*np.cos(elevation), np.sin(azimuth)*np.cos(elevation), np.sin(elevation)])


def circle_seed(points):
    mean = np.mean(points, axis=0)
    _, _, vh = np.linalg.svd(points-mean, full_matrices=False)
    normal = vh[-1]
    matrix = np.r_[2*(points-mean), normal.reshape(1, 3)]
    target = np.r_[np.sum(points**2, axis=1)-np.mean(np.sum(points**2, axis=1)), normal@mean]
    center = np.linalg.lstsq(matrix, target, rcond=None)[0]
    radius = np.mean(np.linalg.norm(points-center, axis=1))
    angles = np.array([np.arctan2(normal[1], normal[0]), np.arcsin(normal[2])])
    return center, radius, angles


def fit_circle(arrays, initial_distal=None, starts=4, proximal_center=None):
    rng, candidates = np.random.default_rng(518), []
    initial_distal = np.zeros(3) if initial_distal is None else initial_distal
    lo, hi = np.r_[np.full(6, -.35), .10, -np.pi, -np.pi/2], np.r_[np.full(6, .35), .70, np.pi, np.pi/2]

    def residual(x):
        points = local_endpoint(arrays, x[3:6])
        delta = points-x[:3]
        return np.r_[np.linalg.norm(delta, axis=1)-x[6], delta@normal_from_angles(x[7:])]

    for i in range(starts):
        distal = initial_distal.copy() if i == 0 else rng.normal(0, .06, 3)
        center, radius, angles = circle_seed(local_endpoint(arrays, distal))
        initial = np.clip(np.r_[center, distal, radius, angles], lo+1e-7, hi-1e-7)
        fit = least_squares(residual, initial, bounds=(lo, hi),
                            loss="soft_l1", f_scale=.01, max_nfev=200)
        candidates.append(fit)
    best = min(candidates, key=lambda fit: fit.cost)
    x = best.x
    normal = normal_from_angles(x[7:])
    center = x[:3].copy()
    # This is an explicit perpendicular-segment-axis prior, not a measured centre.
    axial_shift = 0 if proximal_center is None else float(normal@(proximal_center-center))
    anatomical = center + axial_shift*normal
    return dict(circle_parameters=x.tolist(), plane_center=center.tolist(), axis=normal.tolist(),
                selected_center=anatomical.tolist(), distal_center=x[3:6].tolist(),
                selected_length_m=float(np.sqrt(x[6]**2+axial_shift**2)),
                proximal_bone_length_m=None if proximal_center is None else float(np.linalg.norm(anatomical-proximal_center)),
                axial_selection="circle_plane" if proximal_center is None else "nearest_proximal_ball_centre_convention",
                success=bool(best.success), bound_hit=bool(np.any(np.minimum(x-lo, hi-x) < .001)))


def circle_metrics(arrays, fit):
    points = local_endpoint(arrays, np.array(fit["distal_center"]))
    delta = points-np.array(fit["plane_center"])
    normal = np.array(fit["axis"])
    selected = points-np.array(fit["selected_center"])
    return dict(distance=metric(np.linalg.norm(selected, axis=1)-fit["selected_length_m"]),
                plane=metric(delta@normal))
