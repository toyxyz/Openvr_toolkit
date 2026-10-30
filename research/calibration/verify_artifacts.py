"""Verify immutable recordings, code size, and a candidate-only result contract."""
import hashlib
import json
from pathlib import Path

from session_io import OUTPUT, RECORDINGS, ROOT, save_json


def main():
    catalog = json.loads((OUTPUT/"catalog.json").read_text())
    verified = 0
    for row in catalog:
        if "frames_sha256" not in row:
            continue
        with (RECORDINGS/row["session"]/"frames.bin").open("rb") as file:
            digest = hashlib.file_digest(file, "sha256").hexdigest()
        if digest != row["frames_sha256"]:
            raise RuntimeError("Source recording changed since catalog: " + row["session"])
        verified += 1
    sizes = {}
    for file in Path(__file__).resolve().parent.glob("*.py"):
        size = len(file.read_text(encoding="utf-8").splitlines())
        if size > 300:
            raise RuntimeError(f"Research code exceeds 300 lines: {file.name} ({size})")
        sizes[file.name] = size
    source = {row["session"]: row for row in catalog}
    candidates = []
    for session in json.loads((OUTPUT/"experiments.json").read_text()):
        for pair, result in session["skipped"].items():
            if "free" not in result:
                continue
            candidates.append(dict(
                session=session["session"], frames_sha256=source[session["session"]]["frames_sha256"],
                pair=pair, model="constant_distance_across_untracked_segment",
                offset_and_length_parameters_si=result["free"]["fit"]["x"],
                parameter_provenance="model_conditioned_candidate_not_anatomically_identified",
                heldout_scalar_distance_residual=result["free"]["test"],
                accepted_for_reconstruction=False, anatomically_validated=False,
                rejection_reasons=["Distance-only axial/length ambiguity",
                                   "No independent anatomical reference",
                                   "Mount episode and model mismatch not resolved"] ))
    save_json("calibration_candidates.json", dict(schema="calibration_research_candidate_v1", candidates=candidates))
    # Only move our four new task files, keeping financial evidence out of tracked sources.
    original = (ROOT/"work/calibration_research").resolve()
    destination = (OUTPUT/"work").resolve()
    if not original.is_relative_to(ROOT.resolve()) or not destination.is_relative_to(ROOT.resolve()):
        raise RuntimeError("Task-file archive escapes workspace")
    destination.mkdir(parents=True, exist_ok=True)
    for name in ("astra-design.json", "astra-review.json", "credit-evidence-design.json", "credit-evidence-review.json"):
        file = original/name
        target = destination/name
        if file.exists():
            if target.exists():
                raise RuntimeError("Task archive already exists: " + name)
            file.rename(target)
    audit = dict(source_recordings_rehashed=verified, recording_hashes_unchanged=True,
                 research_code_lines=sizes, candidates_exported=len(candidates), accepted_candidates=0,
                 output_scope="Research artifacts only; product source changes are not inferred by this verifier",
                 note="This audit checks experiment inputs/artifacts, not anatomical accuracy.")
    save_json("verification.json", audit)
    print(json.dumps(audit, indent=2))


if __name__ == "__main__":
    main()
