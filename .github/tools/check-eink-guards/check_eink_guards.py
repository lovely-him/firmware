#!/usr/bin/env python3
"""
check_eink_guards.py — Validates guard-macro conventions in staged C/C++ files.

Checks applied to every staged C/C++ file under src/:
  1.  `#ifdef <MACRO>` must be `#if defined(<MACRO>)`.
  1b. `#ifndef <MACRO>` must be `#if !defined(<MACRO>)`.
  2.  Every `#endif` closing a <MACRO> guard must end with
      the comment `// <MACRO>`.
  4.  `#elif defined(<MACRO>)` occurrences are flagged for manual review
      (the tool cannot safely restructure an #elif chain).

Additional check for newly-added files (git status A):
  3. The file must contain exactly one top-level guard block.
     Only trivial preamble is allowed before it:
       - blank lines
       - comment lines (// … or /* … */)
       - #pragma once
       - at most one quoted #include (self-header)

Issues of types 1, 1b, and 2 are auto-fixable.
Issues of types 3 and 4 require manual intervention.

Usage:
  python3 check_eink_guards.py --macro EINK_RA01S_GP02
  python3 check_eink_guards.py --macro EINK_RA01S_GP02 --fix
"""

from __future__ import annotations

import argparse
import re
import subprocess
import sys
from collections import defaultdict
from pathlib import Path

# ---------------------------------------------------------------------------
# Constants
# ---------------------------------------------------------------------------

CPP_EXTENSIONS = {".c", ".cpp", ".cc", ".h", ".hpp"}

# ---------------------------------------------------------------------------
# Per-macro regex builder
# ---------------------------------------------------------------------------


def _build_regexes(macro: str) -> dict:
    """Compile all regexes that depend on the guard macro name."""
    return {
        # Wrong format: #ifdef <MACRO>
        "RE_IFDEF": re.compile(rf"^#\s*ifdef\s+{re.escape(macro)}\b"),
        # Wrong negation: #ifndef <MACRO>
        "RE_IFNDEF": re.compile(rf"^#\s*ifndef\s+{re.escape(macro)}\b"),
        # Correct format: #if defined(<MACRO>) [optional && ...]
        "RE_IF_DEFINED": re.compile(rf"^#\s*if\s+defined\(\s*{re.escape(macro)}\s*\)"),
        # Correct negation: #if !defined(<MACRO>) [optional && ...]
        "RE_IF_NOT_DEFINED": re.compile(
            rf"^#\s*if\s+!\s*defined\(\s*{re.escape(macro)}\s*\)"
        ),
        # Correctly commented #endif
        "RE_ENDIF_CORRECT": re.compile(rf"^#\s*endif\s+//\s*{re.escape(macro)}\s*$"),
        # Needs manual review: #elif defined(<MACRO>)
        "RE_ELIF_DEFINED": re.compile(
            rf"^#\s*elif\s+defined\(\s*{re.escape(macro)}\s*\)"
        ),
    }


# Static regexes (macro-independent)
RE_ANY_IF = re.compile(r"^#\s*if(?:n?def)?\b|^#\s*if\b")
RE_ENDIF = re.compile(r"^#\s*endif\b")
RE_PRAGMA_ONCE = re.compile(r"^#\s*pragma\s+once\b")
RE_QUOTED_INCLUDE = re.compile(r'^#\s*include\s+"[^"]*"\s*$')
RE_COMMENT_LINE = re.compile(r"^(?://|/\*|\*)")


# ---------------------------------------------------------------------------
# Git helpers
# ---------------------------------------------------------------------------


def _run(*args: str, cwd: Path | None = None) -> str:
    r = subprocess.run(list(args), capture_output=True, text=True, cwd=cwd)
    return r.stdout if r.returncode == 0 else ""


def repo_root() -> Path:
    raw = _run("git", "rev-parse", "--show-toplevel")
    if not raw.strip():
        sys.exit("ERROR: Not inside a git repository.")
    return Path(raw.strip())


def staged_cpp_files(root: Path) -> list[tuple[str, str]]:
    """Return [(status_char, rel_path), ...] for staged C/C++ files under src/."""
    raw = _run("git", "diff", "--cached", "--name-status", "--", "src/", cwd=root)
    result: list[tuple[str, str]] = []
    for line in raw.splitlines():
        line = line.strip()
        if not line:
            continue
        parts = line.split("\t", 1)
        if len(parts) < 2:
            continue
        status_char = parts[0][0]  # first char: A, M, D, R, …
        path = parts[1].strip()
        if status_char == "D":
            continue
        if Path(path).suffix in CPP_EXTENSIONS:
            result.append((status_char, path))
    return result


def staged_lines(root: Path, path: str) -> list[str] | None:
    """Return lines (with line endings) of the staged version of *path*."""
    r = subprocess.run(
        ["git", "show", f":{path}"],
        capture_output=True,
        text=True,
        cwd=root,
    )
    if r.returncode != 0:
        return None
    return r.stdout.splitlines(keepends=True)


# ---------------------------------------------------------------------------
# Core parser: match every #if opener to its #endif
# ---------------------------------------------------------------------------


def _is_guard_opener(stripped: str, rx: dict) -> bool:
    """Return True when the line opens a guard for the target macro (any format, positive or negative)."""
    return bool(
        rx["RE_IFDEF"].match(stripped)
        or rx["RE_IFNDEF"].match(stripped)
        or rx["RE_IF_DEFINED"].match(stripped)
        or rx["RE_IF_NOT_DEFINED"].match(stripped)
    )


def parse_guard_pairs(lines: list[str], rx: dict) -> list[tuple[int, int, bool]]:
    """
    Walk the preprocessor nesting and return one tuple per matched pair:
        (endif_lineno, open_lineno, is_guard)
    Line numbers are 1-based.
    """
    stack: list[tuple[int, bool]] = []  # (open_lineno, is_guard)
    pairs: list[tuple[int, int, bool]] = []

    for i, raw in enumerate(lines, 1):
        s = raw.strip()
        if _is_guard_opener(s, rx):
            stack.append((i, True))
        elif RE_ANY_IF.match(s):
            stack.append((i, False))
        elif RE_ENDIF.match(s):
            if stack:
                open_lineno, is_guard = stack.pop()
                pairs.append((i, open_lineno, is_guard))

    return pairs


# ---------------------------------------------------------------------------
# File-level checks
# ---------------------------------------------------------------------------


def check_file(
    status: str, path: str, lines: list[str], macro: str, rx: dict
) -> list[dict]:
    """
    Return a list of issue dicts:
      {
        "type":    str,          # see below
        "path":    str,
        "line":    int | None,
        "message": str,
        "fixable": bool,
        "fix":     {"lineno": int, "new_raw": str} | None,
      }
    Types: "wrong_format" | "wrong_format_neg" | "missing_comment" | "elif_guard" |
           "no_guard" | "multiple_guards" | "large_preamble".
    """
    issues: list[dict] = []

    # ------------------------------------------------------------------ #
    # Check 1: #ifdef <MACRO>  →  must be #if defined(<MACRO>)            #
    # ------------------------------------------------------------------ #
    for i, raw in enumerate(lines, 1):
        s = raw.strip()
        if rx["RE_IFDEF"].match(s):
            indent = raw[: len(raw) - len(raw.lstrip())]
            new_raw = f"{indent}#if defined({macro})\n"
            issues.append(
                {
                    "type": "wrong_format",
                    "path": path,
                    "line": i,
                    "message": (
                        f"  {path}:{i}: "
                        f"`#ifdef {macro}` → must be `#if defined({macro})`"
                    ),
                    "fixable": True,
                    "fix": {"lineno": i, "new_raw": new_raw},
                }
            )

    # ------------------------------------------------------------------ #
    # Check 1b: #ifndef <MACRO>  →  must be #if !defined(<MACRO>)         #
    # ------------------------------------------------------------------ #
    for i, raw in enumerate(lines, 1):
        s = raw.strip()
        if rx["RE_IFNDEF"].match(s):
            indent = raw[: len(raw) - len(raw.lstrip())]
            new_raw = f"{indent}#if !defined({macro})\n"
            issues.append(
                {
                    "type": "wrong_format_neg",
                    "path": path,
                    "line": i,
                    "message": (
                        f"  {path}:{i}: "
                        f"`#ifndef {macro}` → must be `#if !defined({macro})`"
                    ),
                    "fixable": True,
                    "fix": {"lineno": i, "new_raw": new_raw},
                }
            )

    # ------------------------------------------------------------------ #
    # Check 2: bare #endif (or wrong comment) closing a guard             #
    # ------------------------------------------------------------------ #
    for endif_lineno, open_lineno, is_guard in parse_guard_pairs(lines, rx):
        if not is_guard:
            continue
        raw = lines[endif_lineno - 1]
        if not rx["RE_ENDIF_CORRECT"].match(raw.strip()):
            indent = raw[: len(raw) - len(raw.lstrip())]
            new_raw = f"{indent}#endif // {macro}\n"
            issues.append(
                {
                    "type": "missing_comment",
                    "path": path,
                    "line": endif_lineno,
                    "message": (
                        f"  {path}:{endif_lineno}: "
                        f"`#endif` closing guard (opened line {open_lineno}) "
                        f"must have `// {macro}`"
                    ),
                    "fixable": True,
                    "fix": {"lineno": endif_lineno, "new_raw": new_raw},
                }
            )

    # ------------------------------------------------------------------ #
    # Check 4: #elif defined(<MACRO>) — flag for manual review            #
    # ------------------------------------------------------------------ #
    for i, raw in enumerate(lines, 1):
        s = raw.strip()
        if rx["RE_ELIF_DEFINED"].match(s):
            issues.append(
                {
                    "type": "elif_guard",
                    "path": path,
                    "line": i,
                    "message": (
                        f"  {path}:{i}: "
                        f"`#elif defined({macro})` found — "
                        f"consider extracting into a standalone "
                        f"`#if defined({macro})` block"
                    ),
                    "fixable": False,
                    "fix": None,
                }
            )

    # ------------------------------------------------------------------ #
    # Check 3 (new files only): single top-level guard wrapping the file  #
    # ------------------------------------------------------------------ #
    if status != "A":
        return issues

    # --- Collect top-level guard openings ---
    depth = 0
    guard_open_depths: list[int] = []  # depth at which each guard opened
    top_guard_opens: list[int] = []  # 1-based line numbers

    for i, raw in enumerate(lines, 1):
        s = raw.strip()
        if _is_guard_opener(s, rx):
            if depth == 0:
                top_guard_opens.append(i)
            guard_open_depths.append(depth)
            depth += 1
        elif RE_ANY_IF.match(s):
            depth += 1
        elif RE_ENDIF.match(s):
            depth = max(0, depth - 1)
            if guard_open_depths and guard_open_depths[-1] == depth:
                guard_open_depths.pop()

    if not top_guard_opens:
        issues.append(
            {
                "type": "no_guard",
                "path": path,
                "line": None,
                "message": (
                    f"  {path}: New file has no top-level "
                    f"`#if defined({macro})` guard block"
                ),
                "fixable": False,
                "fix": None,
            }
        )
        return issues

    if len(top_guard_opens) > 1:
        issues.append(
            {
                "type": "multiple_guards",
                "path": path,
                "line": top_guard_opens[1],
                "message": (
                    f"  {path}:{top_guard_opens[1]}: "
                    f"New file has {len(top_guard_opens)} top-level guard "
                    f"blocks (expected exactly 1)"
                ),
                "fixable": False,
                "fix": None,
            }
        )
        return issues

    # --- Single top-level guard: check preamble lines ---
    guard_line = top_guard_opens[0]
    self_include_seen = False
    non_trivial: list[int] = []

    for j, raw in enumerate(lines[: guard_line - 1], 1):
        s = raw.strip()
        if not s:  # blank line
            continue
        if RE_COMMENT_LINE.match(s):  # comment
            continue
        if RE_PRAGMA_ONCE.match(s):  # #pragma once
            continue
        if RE_QUOTED_INCLUDE.match(s) and not self_include_seen:
            self_include_seen = True  # first self-header
            continue
        non_trivial.append(j)

    if non_trivial:
        sample = ", ".join(str(ln) for ln in non_trivial[:3])
        suffix = "…" if len(non_trivial) > 3 else ""
        issues.append(
            {
                "type": "large_preamble",
                "path": path,
                "line": non_trivial[0],
                "message": (
                    f"  {path}: Non-trivial code before guard "
                    f"(line(s) {sample}{suffix}) — should be inside the guard"
                ),
                "fixable": False,
                "fix": None,
            }
        )

    return issues


# ---------------------------------------------------------------------------
# Fix application
# ---------------------------------------------------------------------------


def write_fixes(all_issues: list[dict], root: Path) -> list[str]:
    """Phase 1: Write fixed content to the working tree (do NOT git add yet).
    Returns the list of paths that were written."""
    file_fixes: dict[str, dict[int, str]] = defaultdict(dict)
    for issue in all_issues:
        if issue["fixable"] and issue["fix"]:
            path = issue["path"]
            fix = issue["fix"]
            file_fixes[path][fix["lineno"]] = fix["new_raw"]

    if not file_fixes:
        return []

    written: list[str] = []
    for path, replacements in sorted(file_fixes.items()):
        # Always apply to staged content so line numbers stay valid
        lines = staged_lines(root, path)
        if lines is None:
            print(f"  SKIP {path}: cannot read staged content")
            continue

        for lineno, new_raw in sorted(replacements.items()):
            if 1 <= lineno <= len(lines):
                lines[lineno - 1] = new_raw

        full_path = root / path
        try:
            full_path.write_text("".join(lines), encoding="utf-8")
        except OSError as exc:
            print(f"  ERROR writing {path}: {exc}")
            continue

        written.append(path)

    return written


def stage_files(paths: list[str], root: Path) -> None:
    """Phase 2: git add the given paths."""
    for path in paths:
        subprocess.run(["git", "add", "--", path], cwd=root, check=True)
        print(f"  Staged: {path}")


def show_diff(paths: list[str], root: Path) -> None:
    """Show git diff (staged vs working tree) for the given paths."""
    subprocess.run(
        ["git", "diff", "--"] + paths,
        cwd=root,
    )


# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Validate guard-macro conventions in staged C/C++ files."
    )
    parser.add_argument(
        "--macro",
        required=True,
        metavar="NAME",
        help="Guard macro name, e.g. EINK_RA01S_GP02",
    )
    parser.add_argument(
        "--fix",
        action="store_true",
        help="After reporting, offer to write and stage auto-fixes",
    )
    args = parser.parse_args()

    macro: str = args.macro
    fix_mode: bool = args.fix
    rx = _build_regexes(macro)

    root = repo_root()
    files = staged_cpp_files(root)

    if not files:
        print("No staged C/C++ files found in src/.")
        return

    print(f"Checking guard macro: {macro}\n")

    all_issues: list[dict] = []
    for status, path in sorted(files, key=lambda x: x[1]):
        lines = staged_lines(root, path)
        if lines is None:
            print(f"WARNING: Cannot read staged content for {path}")
            continue
        all_issues.extend(check_file(status, path, lines, macro, rx))

    # ---- Report ----
    if not all_issues:
        print(f"✓  All {len(files)} staged file(s) pass EINK guard checks.")
        sys.exit(0)

    by_file: dict[str, list[dict]] = defaultdict(list)
    for issue in all_issues:
        by_file[issue["path"]].append(issue)

    n_fixable = sum(1 for i in all_issues if i["fixable"])
    n_manual = len(all_issues) - n_fixable

    print(f"Found {len(all_issues)} issue(s) across {len(by_file)} file(s):\n")
    for path in sorted(by_file):
        print(f"  [{path}]")
        for issue in by_file[path]:
            tag = " [auto-fixable]" if issue["fixable"] else " [manual fix]"
            print(f"{issue['message']}{tag}")
        print()

    parts: list[str] = []
    if n_fixable:
        parts.append(f"{n_fixable} auto-fixable")
    if n_manual:
        parts.append(f"{n_manual} require manual fix")
    print("Summary: " + ", ".join(parts))

    # ---- Fix mode ----
    if fix_mode and n_fixable:
        print()
        try:
            answer = (
                input("Write fixes to working tree and preview diff? [y/N] ")
                .strip()
                .lower()
            )
        except (EOFError, KeyboardInterrupt):
            print("\nAborted.")
            sys.exit(1)
        if answer != "y":
            print("Fixes not applied.")
            sys.exit(1 if all_issues else 0)

        # Phase 1: write to working tree
        print()
        written = write_fixes(all_issues, root)
        if not written:
            print("No files were modified.")
            sys.exit(1 if all_issues else 0)

        # Phase 2: show diff (staged original vs working tree fixed)
        print(f"\nDiff preview ({len(written)} file(s) modified in working tree):\n")
        show_diff(written, root)

        # Phase 3: ask to stage
        print()
        try:
            answer2 = input("Stage the fixed files (git add)? [y/N] ").strip().lower()
        except (EOFError, KeyboardInterrupt):
            print("\nAborted. Working tree changes kept, nothing staged.")
            sys.exit(1)

        if answer2 == "y":
            print()
            stage_files(written, root)
            print("\nDone. Run the script again to verify.")
        else:
            print(
                "Files left in working tree (not staged). Run `git checkout -- <file>` to discard."
            )

    sys.exit(1 if all_issues else 0)


if __name__ == "__main__":
    main()
