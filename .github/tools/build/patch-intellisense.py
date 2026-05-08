#!/usr/bin/env python3
"""
Patch .vscode/c_cpp_properties.json with dynamic defines that
'pio project init --ide vscode' cannot capture, because they are
generated at build time by bin/platformio-custom.py.

Specifically injects:
  - APP_VERSION, APP_VERSION_SHORT, APP_ENV, APP_REPO, BUILD_EPOCH
  - All USERPREFS_* from userPrefs.jsonc

Usage:
    python3 bin/patch-intellisense.py <project_root> <variant>
"""

import configparser
import json
import re
import subprocess
import sys
from datetime import datetime
from pathlib import Path


def read_version(root: Path) -> dict:
    props = configparser.ConfigParser()
    props.read(root / "version.properties")
    major = props.get("VERSION", "major").strip()
    minor = props.get("VERSION", "minor").strip()
    build = props.get("VERSION", "build").strip()
    try:
        commit = (
            subprocess.check_output(
                ["git", "-C", str(root), "rev-parse", "--short", "HEAD"],
                stderr=subprocess.DEVNULL,
            )
            .decode()
            .strip()
        )
    except Exception:
        commit = "unknown"
    return {
        "long": f"{major}.{minor}.{build}.{commit}",
        "short": f"{major}.{minor}.{build}",
    }


def get_repo_owner(root: Path) -> str:
    try:
        url = (
            subprocess.check_output(
                ["git", "-C", str(root), "config", "--get", "remote.origin.url"],
                stderr=subprocess.DEVNULL,
            )
            .decode()
            .strip()
        )
        parts = url.split("/")
        return parts[-2] + "/" + parts[-1].replace(".git", "")
    except Exception:
        return "unknown/unknown"


def read_userprefs(root: Path) -> dict:
    jsonc_path = root / "userPrefs.jsonc"
    with open(jsonc_path) as f:
        text = re.sub(r"//.*", "", f.read(), flags=re.MULTILINE)
    return json.loads(text)


def make_define(key: str, value: str) -> str:
    r"""
    Convert a userPrefs key/value pair into the string that belongs in
    c_cpp_properties.json's "defines" array, matching the same rules used
    by platformio-custom.py (env.StringifyMacro for string values).

    IntelliSense define format examples:
      - numeric/bool/enum : "MY_MACRO=1"
      - string            : 'MY_MACRO="hello world"'  → JSON: "MY_MACRO=\"hello world\""
      - array initializer : "MY_MACRO={ 0x01, 0x02 }"
    """
    if value.startswith("{"):
        # Array initializer — keep as-is (IntelliSense will at least see the key defined)
        return f"{key}={value}"
    if value.lstrip("-").replace(".", "").isdigit():
        return f"{key}={value}"
    if value in ("true", "false"):
        return f"{key}={value}"
    if value.startswith("meshtastic_"):
        return f"{key}={value}"
    # String value: wrap in double-quotes so IntelliSense sees it as a string literal
    return f'{key}="{value}"'


def build_dynamic_defines(root: Path, variant: str) -> list:
    ver = read_version(root)
    repo = get_repo_owner(root)
    build_epoch = int(
        datetime.now().replace(hour=0, minute=0, second=0, microsecond=0).timestamp()
    )

    defines = [
        f"APP_VERSION={ver['long']}",
        f"APP_VERSION_SHORT={ver['short']}",
        f"APP_ENV={variant}",
        f"APP_REPO={repo}",
        f"BUILD_EPOCH={build_epoch}",
    ]

    for key, val in read_userprefs(root).items():
        defines.append(make_define(key, val))

    return defines


def patch(root: Path, variant: str) -> None:
    props_path = root / ".vscode" / "c_cpp_properties.json"
    if not props_path.exists():
        print(f"[patch-intellisense] {props_path} not found, skipping.")
        return

    with open(props_path) as f:
        raw = re.sub(r"//.*", "", f.read(), flags=re.MULTILINE)
    data = json.loads(raw)

    dynamic = build_dynamic_defines(root, variant)

    for config in data.get("configurations", []):
        existing = config.get("defines", [])
        # Strip the trailing empty-string sentinel PlatformIO always appends
        clean = [d for d in existing if d != ""]
        # Index existing keys to avoid duplicates (last writer wins on conflict)
        existing_keys = {(d.split("=")[0] if "=" in d else d) for d in clean}
        for d in dynamic:
            key = d.split("=")[0] if "=" in d else d
            if key not in existing_keys:
                clean.append(d)
                existing_keys.add(key)
        # Restore PlatformIO's trailing empty-string sentinel
        clean.append("")
        config["defines"] = clean

    with open(props_path, "w") as f:
        json.dump(data, f, indent=4)

    added = len(dynamic)
    print(f"[patch-intellisense] Injected {added} dynamic defines into {props_path}")


if __name__ == "__main__":
    if len(sys.argv) < 3:
        print(f"Usage: {sys.argv[0]} <project_root> <variant>")
        sys.exit(1)
    patch(Path(sys.argv[1]), sys.argv[2])
