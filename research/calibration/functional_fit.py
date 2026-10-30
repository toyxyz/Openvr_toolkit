"""Functional centre/distance hypotheses; estimates are not accepted anatomy."""
import numpy as np
from scipy.optimize import least_squares


def transformed(rotations, positions, point):
    return positions + np.einsum("nij,j->ni", rotations, point)


def closure_error(arrays, x, distance=False):
    _, ra, pa, rb, pb = arrays
    delta = transformed(ra, pa, x[:3]) - transformed(rb, pb, x[3:6])
    return np.linalg.norm(delta, axis=1) - x[6] if distance else delta


def metric(error):
    values = np.linalg.norm(error, axis=1) if error.ndim == 2 else np.abs(error)
    return dict(rms_mm=float(np.sqrt(np.mean(values**2))*1000),
                median_mm=float(np.median(values)*1000), p95_mm=float(np.percentile(values, 95)*1000))


def shared_center(arrays, robust=False):
    _, ra, pa, rb, pb = arrays
    matrix = np.concatenate([ra, -rb], axis=2).reshape(-1, 6)
    rhs = (pb-pa).reshape(-1)
    x, _, rank, singular = np.linalg.lstsq(matrix, rhs, rcond=None)
    if robust:
        # SciPy scales the Jacobian in place for robust losses; preserve the model matrix.
        x = least_squares(lambda point: matrix@point-rhs, x, jac=lambda _: matrix.copy(),
                          loss="soft_l1", f_scale=.01, max_nfev=200).x
    return dict(x=x, rank=int(rank), singular_values=singular,
                condition=float(singular[0]/max(singular[-1], 1e-15)))


def distance_jacobian(arrays, x):
    _, ra, pa, rb, pb = arrays
    delta = transformed(ra, pa, x[:3]) - transformed(rb, pb, x[3:6])
    unit = delta / np.maximum(np.linalg.norm(delta, axis=1, keepdims=True), 1e-12)
    return np.c_[np.einsum("ni,nij->nj", unit, ra),
                 -np.einsum("ni,nij->nj", unit, rb), -np.ones(len(delta))]


def fit_distance(arrays, length_range=(.10, .70), starts=6, prior_sigma=None, seed=40):
    _, ra, pa, rb, pb = arrays
    raw_length = np.median(np.linalg.norm(pa-pb, axis=1))
    lo = np.r_[np.full(6, -.35), length_range[0]]
    hi = np.r_[np.full(6, .35), length_range[1]]
    rng, solutions = np.random.default_rng(seed), []

    def residual(x):
        error = closure_error(arrays, x, True)
        # Equivalent to data sigma=5mm and a zero-offset sigma prior, in metres.
        # Both are sensitivity assumptions; neither is measured sensor/anatomical noise.
        return np.r_[error, .005*x[:6]/prior_sigma] if prior_sigma else error

    def jacobian(x):
        jac = distance_jacobian(arrays, x)
        return np.r_[jac, np.c_[.005*np.eye(6)/prior_sigma, np.zeros(6)]] if prior_sigma else jac

    for i in range(starts):
        initial = np.r_[np.zeros(6) if i == 0 else rng.normal(0, .08, 6), raw_length]
        initial = np.clip(initial, lo+1e-6, hi-1e-6)
        fit = least_squares(residual, initial, jac=jacobian, bounds=(lo, hi),
                            loss="soft_l1", f_scale=.01, max_nfev=250, ftol=1e-9, xtol=1e-9, gtol=1e-9)
        solutions.append(fit)
    best = min(solutions, key=lambda fit: fit.cost)
    singular = np.linalg.svd(distance_jacobian(arrays, best.x), compute_uv=False)
    near = [fit.x for fit in solutions if fit.cost <= best.cost*1.05+1e-8]
    spread = np.ptp(np.array(near), axis=0)
    return dict(x=best.x, cost=float(best.cost), success=bool(best.success),
                condition=float(singular[0]/max(singular[-1], 1e-15)), singular_values=singular,
                effective_rank=int(np.sum(singular > singular[0]*1e-3)),
                bound_hit=bool(np.any(np.minimum(best.x-lo, hi-best.x) < .001)),
                near_solution_spread_mm=(spread*1000), prior_sigma_m=prior_sigma)


def subset(arrays, selection):
    return tuple(item[selection] for item in arrays)


def public_fit(fit):
    return {key: value.tolist() if isinstance(value, np.ndarray) else value for key, value in fit.items()}
