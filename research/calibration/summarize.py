"""Summarize research diagnostics without interpreting residual as anatomical accuracy."""
import json
from pathlib import Path

import numpy as np
import scipy

from session_io import OUTPUT, save_json


def main():
    catalog = json.loads((OUTPUT/"catalog.json").read_text())
    sweep = json.loads((OUTPUT/"sweep.json").read_text())
    selected = json.loads((OUTPUT/"experiments.json").read_text())
    stats = {}
    for pair in sweep[0]["skipped"]:
        rows = [row["skipped"][pair] for row in sweep if "free" in row["skipped"].get(pair, {})]
        fits = [row["free"]["fit"] for row in rows]
        stats[pair] = dict(
            cases=len(rows), scalar_holdout_rms_mm_median=float(np.median([row["free"]["test"]["rms_mm"] for row in rows])),
            raw_scalar_holdout_rms_mm_median=float(np.median([row["raw_tracker_distance"]["test"]["rms_mm"] for row in rows])),
            fitted_length_mm_range=[float(min(fit["x"][6] for fit in fits)*1000), float(max(fit["x"][6] for fit in fits)*1000)],
            bound_hits=sum(fit["bound_hit"] for fit in fits),
            fits_not_converged=sum(not fit["success"] for fit in fits),
            note="Broad numerical bounds and no anatomical ground truth; values are not accepted lengths.")
    result = dict(catalog_sessions=len(catalog), catalog_errors=[row for row in catalog if "error" in row],
                  rich_session_count=sum(len(row.get("roles", [])) >= 10 and row.get("duration_s", 0)>10 for row in catalog),
                  unique_rich=len(sweep), selected_sessions=[row["session"] for row in selected],
                  selected_source_frames=sum(row["frames"] for row in selected),
                  selected_sampled_frames=sum(row["sampled"] for row in selected),
                  selected_validity_range=[float(min(value for row in selected for value in row["validity"].values())),
                                           float(max(value for row in selected for value in row["validity"].values()))],
                  sweep_pairs=stats, software=dict(numpy=np.__version__, scipy=scipy.__version__),
                  accepted_calibrations=0, anatomical_ground_truth_available=False)
    save_json("summary.json", result)
    print(json.dumps(result, indent=2))


if __name__ == "__main__":
    main()
