"""Profile-length and temporal-block stability probes; no anatomy acceptance."""
import json

import numpy as np
from scipy.optimize import least_squares

from functional_fit import closure_error, distance_jacobian, fit_distance, metric, public_fit, subset
from session_io import RECORDINGS, load_session, pair_arrays, save_json


def profile(arrays, candidates, start):
    cutoff = arrays[0][0]+.65*np.ptp(arrays[0])
    train, test = subset(arrays, arrays[0] <= cutoff), subset(arrays, arrays[0] > cutoff)
    rows = []
    for length in candidates:
        def residual(offsets):
            return closure_error(train, np.r_[offsets, length], True)

        fit = least_squares(residual, start[:6], jac=lambda x: distance_jacobian(train, np.r_[x, length])[:, :6],
                            bounds=(-.35, .35), loss="soft_l1", f_scale=.01, max_nfev=300)
        x = np.r_[fit.x, length]
        rows.append(dict(length_mm=float(length*1000), train=metric(residual(fit.x)),
                         test=metric(closure_error(test, x, True)),
                         parameters=x.tolist(),
                         max_offset_mm=float(np.max(np.abs(fit.x))*1000)))
    return rows


def main():
    checks = [("shouler_test", "left_arm", "left_hand"),
              ("shouler_test", "left_leg", "left_foot"),
              ("session_2026_06_27_165948", "left_leg", "left_foot")]
    rows = []
    for name, a, b in checks:
        session = load_session(RECORDINGS/name, 1500)
        arrays = pair_arrays(session, a, b)
        whole = fit_distance(arrays, starts=8)
        blocks = []
        for block in np.array_split(np.arange(len(arrays[0])), 5):
            fit = fit_distance(subset(arrays, block), starts=5)
            blocks.append(dict(start_s=float(arrays[0][block[0]]), end_s=float(arrays[0][block[-1]]),
                               fit=public_fit(fit), within_block=metric(closure_error(subset(arrays, block), fit["x"], True)),
                               full_clip=metric(closure_error(arrays, fit["x"], True))))
        lengths = np.linspace(.16, .38, 12) if "arm" in a else np.linspace(.28, .64, 13)
        rows.append(dict(session=name, pair=a+":"+b, whole=public_fit(whole), blocks=blocks,
                         length_profile=profile(arrays, lengths, whole["x"])))
        save_json("sensitivity.json", rows)
        print(json.dumps(dict(session=name, pair=a+":"+b,
                              block_lengths_mm=[round(block["fit"]["x"][6]*1000, 1) for block in blocks],
                              condition=whole["condition"], profile=rows[-1]["length_profile"])), flush=True)
    family = rows[2]["length_profile"]
    short, long = [min(family, key=lambda row: abs(row["length_mm"]-target)) for target in (400, 640)]
    xshort, xlong = np.array(short["parameters"]), np.array(long["parameters"])
    comparison = dict(lengths_mm=[short["length_mm"], long["length_mm"]],
                      test_rms_mm=[short["test"]["rms_mm"], long["test"]["rms_mm"]],
                      knee_candidate_separation_mm=float(np.linalg.norm(xshort[:3]-xlong[:3])*1000),
                      ankle_candidate_separation_mm=float(np.linalg.norm(xshort[3:6]-xlong[3:6])*1000),
                      note="Separation is constant under each sensor's rigid transform; not anatomical error.")
    save_json("equivalent_candidates.json", comparison)
    print(json.dumps(comparison), flush=True)


if __name__ == "__main__":
    main()
