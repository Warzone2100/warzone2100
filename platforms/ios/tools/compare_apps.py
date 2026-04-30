#!/usr/bin/env python3

from __future__ import annotations

import argparse
import json
from pathlib import Path


def dir_size(path: Path) -> int:
    total = 0
    if not path.exists():
        return total
    for child in path.rglob("*"):
        if child.is_file():
            total += child.stat().st_size
    return total


def summarize(app_path: Path, resource_root: Path) -> dict:
    return {
        "app": str(app_path),
        "exists": app_path.exists(),
        "size_bytes": dir_size(app_path),
        "data_size_bytes": dir_size(resource_root / "data"),
        "docs_size_bytes": dir_size(resource_root / "docs"),
        "locale_size_bytes": dir_size(resource_root / "locale"),
    }


def resource_root(app_path: Path) -> Path:
    contents_resources = app_path / "Contents" / "Resources"
    if contents_resources.exists():
        return contents_resources
    return app_path


def main() -> int:
    parser = argparse.ArgumentParser(description="Compare the installed macOS app against the packaged iOS app.")
    parser.add_argument("--mac-app", required=True)
    parser.add_argument("--ios-app", required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()

    mac_app = Path(args.mac_app)
    ios_app = Path(args.ios_app)
    mac_summary = summarize(mac_app, resource_root(mac_app))
    ios_summary = summarize(ios_app, resource_root(ios_app))

    lines = [
        f"macOS app: {mac_app}",
        f"iOS app: {ios_app}",
        "",
        "macOS summary:",
        json.dumps(mac_summary, indent=2),
        "",
        "iOS summary:",
        json.dumps(ios_summary, indent=2),
        "",
        "Interpretation:",
    ]
    if mac_summary["data_size_bytes"] and ios_summary["data_size_bytes"]:
        ratio = ios_summary["data_size_bytes"] / mac_summary["data_size_bytes"]
        lines.append(f"- iOS bundle data payload is {ratio:.2%} of the installed macOS app data payload.")
    else:
        lines.append("- One of the app bundles is missing its data payload.")
    lines.append("- Matching data/docs/locale sizes is the quickest check that the packaged iOS app carries the real game assets.")

    output_path = Path(args.output)
    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text("\n".join(lines) + "\n", encoding="utf-8")
    print(output_path)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
