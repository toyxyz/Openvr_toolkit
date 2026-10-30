"""Test the extra hinge-locus assumption without claiming anatomical ground truth."""
import json

import numpy as np

from functional_fit import shared_center, subset
from hinge_fit import circle_metrics, fit_circle
from session_io import RECORDINGS, load_session, pair_arrays, save_json


def main():
    rows = []
    for name in ("shouler_test", "session_2026_06_27_165948"):
        session = load_session(RECORDINGS/name, 1000)
        for proximal, a, b in [("chest", "left_arm", "left_hand"), ("pelvis", "left_leg", "left_foot")]:
            arrays = pair_arrays(session, a, b)
            cutoff = arrays[0][0]+.65*np.ptp(arrays[0])
            train, test = subset(arrays, arrays[0] <= cutoff), subset(arrays, arrays[0] > cutoff)
            direct = pair_arrays(session, proximal, a)
            direct = subset(direct, direct[0] <= cutoff)
            upstream = shared_center(direct, True)["x"][3:6]
            fit = fit_circle(train, starts=6, proximal_center=upstream)
            row = dict(session=name, pair=a+":"+b, fit=fit,
                       train=circle_metrics(train, fit), test=circle_metrics(test, fit), blocks=[])
            for block in np.array_split(np.arange(len(arrays[0])), 5):
                block_fit = fit_circle(subset(arrays, block), starts=3, proximal_center=upstream)
                row["blocks"].append(dict(length_mm=block_fit["selected_length_m"]*1000,
                                          proximal_mm=block_fit["proximal_bone_length_m"]*1000,
                                          bound_hit=block_fit["bound_hit"]))
            rows.append(row)
            save_json("hinge_experiments.json", rows)
            print(json.dumps(dict(session=name, pair=a+":"+b,
                                  length_mm=fit["selected_length_m"]*1000,
                                  proximal_mm=fit["proximal_bone_length_m"]*1000,
                                  test=row["test"], blocks=row["blocks"])), flush=True)


if __name__ == "__main__":
    main()
