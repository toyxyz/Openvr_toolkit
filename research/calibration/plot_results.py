"""Export standalone figures illustrating nonidentifiability and temporal instability."""
import json

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np

from session_io import OUTPUT


def main():
    sensitivity = json.loads((OUTPUT/"sensitivity.json").read_text())
    plt.rcParams.update({"font.size": 11, "axes.spines.top": False, "axes.spines.right": False})
    fig, axes = plt.subplots(1, 3, figsize=(15, 4.9), layout="constrained")
    shifts = np.linspace(0, .20, 100)
    axes[0].plot(shifts*100, np.sqrt(.26**2+shifts**2)*100, color="#4c78a8", lw=2.5)
    axes[0].set(xlabel="Joint-centre shift along hinge axis (cm)", ylabel="Equally fitting segment length (cm)",
                title="Synthetic: different geometry, identical data")
    axes[0].text(.04, .85, "All solutions have ~zero residual", transform=axes[0].transAxes, fontsize=10)
    profile = sensitivity[2]["length_profile"]
    axes[1].plot([row["length_mm"]/10 for row in profile], [row["train"]["rms_mm"] for row in profile],
                 marker="o", label="Training distance RMS", color="#4c78a8")
    axes[1].plot([row["length_mm"]/10 for row in profile], [row["test"]["rms_mm"] for row in profile],
                 marker="o", label="Held-out distance RMS", color="#e68a2e")
    axes[1].set(xlabel="Assumed shank length (cm)", ylabel="Scalar distance residual (mm)",
                title="Recorded motion: a broad, flat length profile", ylim=(0, 8))
    axes[1].legend(fontsize=9)
    for row, color, label in [(sensitivity[0], "#4c78a8", "Forearm candidate"),
                              (sensitivity[1], "#e68a2e", "Shank candidate")]:
        axes[2].plot(np.arange(1, 6), [block["fit"]["x"][6]*100 for block in row["blocks"]],
                     marker="o", color=color, label=label)
    axes[2].set(xlabel="Contiguous block in the same recording", ylabel="Fitted length (cm)",
                title="Recorded motion: length varies by window", xticks=np.arange(1, 6))
    axes[2].legend(fontsize=9)
    for ax in axes:
        ax.grid(alpha=.2)
    fig.suptitle("A small tracker residual does not establish anatomical accuracy", fontsize=16)
    fig.savefig(OUTPUT/"identifiability.png", dpi=160)
    fig.savefig(OUTPUT/"identifiability.svg")
    plt.close(fig)
    print(str(OUTPUT/"identifiability.png"))


if __name__ == "__main__":
    main()
