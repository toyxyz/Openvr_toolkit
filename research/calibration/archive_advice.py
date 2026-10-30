"""Archive completed authorized adviser reports and measured usage, without credentials."""
from decimal import Decimal
import json
from pathlib import Path

from session_io import OUTPUT, save_json


def main():
    state = Path("C:/Users/toyxy/Documents/Codex/astra-parallel-state")
    reports = []
    for label, job in [("design", "908e4d17218b4876af7c6485e113f9fa"),
                       ("review", "ec8cc3052e294e4f8bd77e598260a900")]:
        result = json.loads((state/(job+".json")).read_text(encoding="utf-8"))
        if result["status"] != "completed" or not result.get("text"):
            raise RuntimeError("Adviser report is not complete: " + label)
        OUTPUT.mkdir(parents=True, exist_ok=True)
        (OUTPUT/("astra_"+label+".txt")).write_text(result["text"], encoding="utf-8")
        reports.append(dict(report=label, job_id=job, model=result["model"], usage=result["usage"],
                            accounted_cost_usd_upper_estimate=result["accounted_cost_usd_upper_estimate"]))
    total = sum(Decimal(str(report["accounted_cost_usd_upper_estimate"])) for report in reports)
    summary = dict(reports=reports, total_tokens=sum(report["usage"]["total_tokens"] for report in reports),
                   total_accounted_cost_usd_upper_estimate=str(total),
                   note="Costs are the helper's conservative usage accounting, not an independently retrieved invoice.")
    save_json("astra_usage.json", summary)
    print(json.dumps(summary, indent=2))


if __name__ == "__main__":
    main()
