#!/usr/bin/env python3
"""Generate compile_commands.json from lesson Makefiles (matches lesson.mk flags)."""

from __future__ import annotations

import json
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
EXAMPLES = ROOT / "examples"
OUT = ROOT / "compile_commands.json"

GCC_LINE = re.compile(
    r"^(arm-none-eabi-(?:gcc|g\+\+))\s+(.+)\s+-c\s+(\S+)\s+-o\s+(\S+)$"
)


def make_compile_lines(lesson_dir: Path, board: str | None) -> list[str]:
    args = ["make", "-Bnwk", "all"]
    if board == "dkx":
        args[1:1] = ["BOARD=dkx"]
    proc = subprocess.run(
        args,
        cwd=lesson_dir,
        capture_output=True,
        text=True,
        check=False,
    )
    if proc.returncode != 0:
        return []
    lines: list[str] = []
    for raw in proc.stdout.splitlines():
        line = raw.strip()
        if line.startswith("arm-none-eabi-gcc") or line.startswith("arm-none-eabi-g++"):
            lines.append(line)
    return lines


def supports_dkx(makefile: Path) -> bool:
    text = makefile.read_text(encoding="utf-8")
    return "BOARD),dkx" in text or "BOARD=dkx" in text


def entries_from_lesson(lesson_dir: Path, board: str | None) -> list[dict]:
    out: list[dict] = []
    lesson_dir = lesson_dir.resolve()
    for line in make_compile_lines(lesson_dir, board):
        m = GCC_LINE.match(line)
        if not m:
            continue
        compiler, flags, src, _obj = m.groups()
        src_path = (lesson_dir / src).resolve()
        if not src_path.is_file():
            continue
        out.append(
            {
                "directory": str(lesson_dir),
                "command": f"{compiler} {flags} -c {src} -o /dev/null",
                "file": str(src_path),
            }
        )
    return out


def install_ide_flag_links() -> None:
    """Symlink compile_flags.txt so clangd finds flags without compile_commands.json."""
    disco_src = EXAMPLES / "common" / "compile_flags.disco"
    dkx_src = EXAMPLES / "common" / "dkx" / "compile_flags.txt"

    for lesson in sorted(EXAMPLES.iterdir()):
        if not lesson.is_dir():
            continue
        if (lesson / "main.c").exists() or (lesson / "main.cpp").exists():
            link = lesson / "compile_flags.txt"
            if link.is_symlink() or not link.exists():
                link.unlink(missing_ok=True)
                link.symlink_to("../common/compile_flags.disco")

    for dkx_dir in sorted(EXAMPLES.rglob("dkx")):
        if not dkx_dir.is_dir() or dkx_dir == EXAMPLES / "common" / "dkx":
            continue
        link = dkx_dir / "compile_flags.txt"
        if link.resolve() == dkx_src.resolve():
            continue
        link.unlink(missing_ok=True)
        link.symlink_to("../../common/dkx/compile_flags.txt")


def main() -> int:
    entries: list[dict] = []
    makefiles = sorted(EXAMPLES.rglob("Makefile"))
    for makefile in makefiles:
        lesson = makefile.parent
        entries.extend(entries_from_lesson(lesson, None))
        if supports_dkx(makefile):
            entries.extend(entries_from_lesson(lesson, "dkx"))

    # Last write wins if the same file were listed twice (should not happen).
    by_file: dict[str, dict] = {}
    for e in entries:
        by_file[e["file"]] = e

    install_ide_flag_links()
    OUT.write_text(json.dumps(list(by_file.values()), indent=2) + "\n", encoding="utf-8")
    print(f"Wrote {len(by_file)} entries to {OUT}", file=sys.stderr)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
