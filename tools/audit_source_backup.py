"""Check the staged source snapshot before publishing; never print secret values."""
import json
import re
import subprocess
from pathlib import Path


def git(*args):
    return subprocess.check_output(["git", *args])


def main():
    staged = git("diff", "--cached", "--name-only", "-z").decode().split("\0")
    patterns = [rb"sk-(?:proj-|svcacct-)?[A-Za-z0-9_-]{30,}",
                rb"gh[pousr]_[A-Za-z0-9]{30,}",
                rb"-----BEGIN (?:RSA |EC |OPENSSH )?PRIVATE KEY-----"]
    rejected = []
    total = 0
    for name in filter(None, staged):
        if name.startswith(("build/", "recordings/")) or "__pycache__" in name:
            rejected.append({"path": name, "reason": "local data/cache"})
            continue
        content = git("show", ":" + name)
        total += len(content)
        if len(content) > 5_000_000:
            rejected.append({"path": name, "reason": "unexpected large file"})
        if any(re.search(pattern, content) for pattern in patterns):
            rejected.append({"path": name, "reason": "credential-like content"})
    result = {"files": len(list(filter(None, staged))), "bytes": total,
              "rejected": rejected,
              "scope": "Staged files only; pattern scan is not an exhaustive secret audit"}
    target = Path("build/source_backup_audit.json")
    target.parent.mkdir(exist_ok=True)
    target.write_text(json.dumps(result, indent=2), encoding="utf-8")
    print(json.dumps(result, indent=2))
    if rejected:
        raise SystemExit("Source backup audit failed; see paths/reasons above")


if __name__ == "__main__":
    main()
