"""Thin launcher, including for Python distributions with isolated import paths."""
import importlib
from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent))
COMMANDS = ("catalog", "experiments", "synthetic_checks", "sensitivity", "hinge_experiments",
            "summarize", "plot_results", "verify_artifacts", "limb_ambiguity", "archive_advice")
if len(sys.argv) < 2 or sys.argv[1] not in COMMANDS:
    raise SystemExit("Usage: run.py " + "|".join(COMMANDS) + " [args]")
module_name = sys.argv.pop(1)
importlib.import_module(module_name).main()
