"""Catalog available mapped recordings without exposing device serials."""
import json
import hashlib

from session_io import RECORDINGS, index_entries, metadata, save_json


def main():
    rows = []
    for folder in sorted(RECORDINGS.iterdir()):
        if not (folder / "manifest.json").exists():
            continue
        try:
            manifest, config, bindings, fingerprint = metadata(folder)
            entries = index_entries(folder)
            with (folder / "frames.bin").open("rb") as file:
                content_hash = hashlib.file_digest(file, "sha256").hexdigest()
            rows.append(dict(session=folder.name, created=manifest.get("createdAtUtc"),
                             frames=len(entries), duration_s=float((int(entries[-1, 1])-int(entries[0, 1]))/1e9) if len(entries) else 0,
                             actors=int(config.get("actor_count", 0)), roles=list(bindings),
                             binding_fingerprint=fingerprint,
                             frames_sha256=content_hash,
                             note="Same bindings do not establish unchanged tracker attachments."))
        except (ValueError, KeyError, OSError) as error:
            rows.append(dict(session=folder.name, error=str(error)))
    save_json("catalog.json", rows)
    useful = [row for row in rows if len(row.get("roles", [])) >= 10 and row.get("duration_s", 0) > 10]
    unique = len({row["frames_sha256"] for row in useful})
    print(json.dumps(dict(total=len(rows), mapped_rich=len(useful), unique_rich=unique,
                         candidates=[dict(session=row["session"], frames=row["frames"],
                                          duration_s=row["duration_s"]) for row in useful]), indent=2))


if __name__ == "__main__":
    main()
