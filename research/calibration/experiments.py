"""Blocked holdout experiments for direct and skipped-segment tracker pairs."""
import argparse
import json

import numpy as np

from functional_fit import closure_error, fit_distance, metric, public_fit, shared_center, subset
from session_io import OUTPUT, RECORDINGS, load_session, pair_arrays, save_json

DIRECT = [("chest", "left_arm"), ("chest", "right_arm"),
          ("pelvis", "left_leg"), ("pelvis", "right_leg")]
SKIPPED = [("left_arm", "left_hand"), ("right_arm", "right_hand"),
           ("left_leg", "left_foot"), ("right_leg", "right_foot")]
DEFAULT = ["shouler_test", "test__02", "session_2026_06_06_023116",
           "session_2026_06_27_144642", "session_2026_06_27_165948"]


def test_direct(arrays):
    cutoff = arrays[0][0] + .65*(arrays[0][-1]-arrays[0][0])
    train, test = subset(arrays, arrays[0] <= cutoff), subset(arrays, arrays[0] > cutoff)
    if len(train[0]) < 30 or len(test[0]) < 15:
        return dict(error="Too few observations on one side of blocked split")
    baseline = shared_center(train)
    robust = shared_center(train, True)
    result = dict(ols=dict(fit=public_fit(baseline), test=metric(closure_error(test, baseline["x"]))),
                  robust=dict(fit=public_fit(robust), train=metric(closure_error(train, robust["x"])),
                              test=metric(closure_error(test, robust["x"]))))
    centres = []
    for block in np.array_split(np.arange(len(arrays[0])), 5):
        fit = shared_center(subset(arrays, block), True)
        centres.append(fit["x"])
    result["five_block_offset_sd_mm"] = (np.std(centres, axis=0)*1000).tolist()
    return result


def test_skipped(arrays, starts):
    cutoff = arrays[0][0] + .65*(arrays[0][-1]-arrays[0][0])
    train, test = subset(arrays, arrays[0] <= cutoff), subset(arrays, arrays[0] > cutoff)
    if len(train[0]) < 30 or len(test[0]) < 15:
        return dict(error="Too few observations on one side of blocked split")
    raw_length = np.median(np.linalg.norm(train[2]-train[4], axis=1))
    result = dict(raw_tracker_distance=dict(length_m=float(raw_length),
                  test=metric(np.linalg.norm(test[2]-test[4], axis=1)-raw_length)))
    for name, prior in [("free", None), ("weak_zero_prior", .15), ("strong_zero_prior", .04)]:
        fit = fit_distance(train, starts=starts, prior_sigma=prior)
        result[name] = dict(fit=public_fit(fit), train=metric(closure_error(train, fit["x"], True)),
                            test=metric(closure_error(test, fit["x"], True)))
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sessions", nargs="+", default=DEFAULT)
    parser.add_argument("--samples", type=int, default=1800)
    parser.add_argument("--starts", type=int, default=6)
    parser.add_argument("--all-rich-unique", action="store_true")
    parser.add_argument("--output-name", default="experiments.json")
    args = parser.parse_args()
    if args.all_rich_unique:
        catalog = json.loads((OUTPUT / "catalog.json").read_text())
        unique = {}
        for row in catalog:
            if len(row.get("roles", [])) >= 10 and row.get("duration_s", 0) > 10:
                unique.setdefault(row["frames_sha256"], row["session"])
        args.sessions = list(unique.values())
    results = []
    for name in args.sessions:
        session = load_session(RECORDINGS / name, args.samples)
        roles = session["role_columns"]
        result = dict(session=name, frames=session["total_frames"], sampled=len(session["times"]),
                      duration_s=float(np.ptp(session["times"])),
                      validity={role: float(np.mean(session["valid"][:, column])) for role, column in roles.items()},
                      direct={}, skipped={})
        for a, b in DIRECT + SKIPPED:
            if a not in roles or b not in roles:
                continue
            arrays = pair_arrays(session, a, b)
            if len(arrays[0]) < 50:
                result["direct" if (a, b) in DIRECT else "skipped"][a+":"+b] = dict(error="Insufficient valid data")
                continue
            destination = result["direct"] if (a, b) in DIRECT else result["skipped"]
            destination[a+":"+b] = test_direct(arrays) if (a, b) in DIRECT else test_skipped(arrays, args.starts)
        results.append(result)
        save_json(args.output_name, results)
        compact = dict(session=name, direct={key: round(value["robust"]["test"]["rms_mm"], 2)
                                            for key, value in result["direct"].items() if "robust" in value},
                       skipped={key: {method: [round(value[method]["test"]["rms_mm"], 2),
                                                round(value[method]["fit"]["x"][6]*1000, 1)]
                                      for method in ("free", "weak_zero_prior", "strong_zero_prior")}
                                for key, value in result["skipped"].items() if "free" in value})
        print(json.dumps(compact), flush=True)


if __name__ == "__main__":
    main()
