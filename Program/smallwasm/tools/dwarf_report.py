from __future__ import annotations

import argparse
import re
import subprocess
import sys
from pathlib import Path


def run_dwarfdump(wasm_path: Path, output_path: Path) -> None:
    output_path.parent.mkdir(parents=True, exist_ok=True)
    try:
        with output_path.open("w", encoding="utf-8", newline="\n") as f:
            result = subprocess.run(
                ["llvm-dwarfdump", str(wasm_path)],
                stdout=f,
                stderr=subprocess.PIPE,
                text=True,
            )
    except FileNotFoundError:
        raise SystemExit("llvm-dwarfdump not found in PATH")
    if result.returncode != 0:
        sys.stderr.write(result.stderr)
        raise SystemExit(result.returncode)


def parse_int(value: str) -> int:
    value = value.strip()
    if value.startswith("0x") or value.startswith("0X"):
        return int(value, 16)
    return int(value)


def parse_dwarfdump(dwarf_path: Path) -> list[dict[str, object]]:
    die_start_re = re.compile(r"^\s*0x[0-9a-fA-F]+:\s+(DW_TAG_\w+)")
    name_re = re.compile(r'DW_AT_name\s+\("([^"]*)"\)')
    abstract_re = re.compile(r'DW_AT_abstract_origin\s+\(0x[0-9a-fA-F]+\s+"([^"]+)"\)')
    low_re = re.compile(r"DW_AT_low_pc\s+\((0x[0-9a-fA-F]+|\d+)\)")
    high_re = re.compile(r"DW_AT_high_pc\s+\((0x[0-9a-fA-F]+|\d+)\)")
    decl_file_re = re.compile(r'DW_AT_decl_file\s+\("([^"]+)"\)')
    decl_line_re = re.compile(r"DW_AT_decl_line\s+\((\d+)\)")
    call_file_re = re.compile(r'DW_AT_call_file\s+\("([^"]+)"\)')
    call_line_re = re.compile(r"DW_AT_call_line\s+\((\d+)\)")

    entries: list[dict[str, object]] = []
    current_tag: str | None = None
    current_cu_file: str | None = None
    current_die: dict[str, object] | None = None

    def flush_die() -> None:
        nonlocal current_die
        if not current_die:
            return
        tag = current_die.get("tag")
        if tag not in ("DW_TAG_subprogram", "DW_TAG_inlined_subroutine"):
            current_die = None
            return
        low = current_die.get("low_pc")
        high = current_die.get("high_pc")
        name = current_die.get("name")
        if low is None or high is None or not name:
            current_die = None
            return
        low_i = int(low)
        high_i = int(high)
        if high_i >= low_i:
            size = high_i - low_i
        else:
            size = high_i
        if size <= 0:
            current_die = None
            return
        file_path = (
            current_die.get("decl_file")
            or current_die.get("call_file")
            or current_cu_file
            or ""
        )
        line_no = current_die.get("decl_line") or current_die.get("call_line") or 0
        entries.append(
            {
                "size": size,
                "file": str(file_path),
                "line": int(line_no),
                "name": str(name),
                "tag": str(tag),
            }
        )
        current_die = None

    with dwarf_path.open("r", encoding="utf-8", errors="ignore") as f:
        for raw_line in f:
            line = raw_line.rstrip("\n")
            m_start = die_start_re.search(line)
            if m_start:
                flush_die()
                current_tag = m_start.group(1)
                if current_tag == "DW_TAG_compile_unit":
                    current_cu_file = None
                    current_die = None
                elif current_tag in ("DW_TAG_subprogram", "DW_TAG_inlined_subroutine"):
                    current_die = {"tag": current_tag}
                else:
                    current_die = None
                continue

            if current_tag == "DW_TAG_compile_unit":
                m_name = name_re.search(line)
                if m_name:
                    current_cu_file = m_name.group(1)

            if not current_die:
                continue

            m_name = name_re.search(line)
            if m_name and not current_die.get("name"):
                current_die["name"] = m_name.group(1)

            m_abs = abstract_re.search(line)
            if m_abs and not current_die.get("name"):
                current_die["name"] = m_abs.group(1)

            m_low = low_re.search(line)
            if m_low:
                current_die["low_pc"] = parse_int(m_low.group(1))

            m_high = high_re.search(line)
            if m_high:
                current_die["high_pc"] = parse_int(m_high.group(1))

            m_df = decl_file_re.search(line)
            if m_df:
                current_die["decl_file"] = m_df.group(1)

            m_dl = decl_line_re.search(line)
            if m_dl:
                current_die["decl_line"] = int(m_dl.group(1))

            m_cf = call_file_re.search(line)
            if m_cf:
                current_die["call_file"] = m_cf.group(1)

            m_cl = call_line_re.search(line)
            if m_cl:
                current_die["call_line"] = int(m_cl.group(1))

    flush_die()
    entries.sort(key=lambda x: (-int(x["size"]), str(x["name"])))
    return entries


def format_size_kb(value: int) -> str:
    return f"{value / 1024.0:.2f} KB"


def write_report(entries: list[dict[str, object]], report_path: Path, dwarf_path: Path) -> None:
    lines: list[str] = []
    lines.append("DWARF size report")
    lines.append(f"Input: {dwarf_path}")
    lines.append(f"Count: {len(entries)}")
    lines.append("")
    header = f"{'No.':>3} {'Size(B)':>10} {'Size(KB)':>10} {'File:Line':<60} Function"
    lines.append(header)
    lines.append("-" * len(header))
    for i, e in enumerate(entries, 1):
        size = int(e["size"])
        file_path = str(e.get("file") or "")
        line_no = int(e.get("line") or 0)
        if file_path:
            file_line = f"{file_path}:{line_no}" if line_no else file_path
        else:
            file_line = f"<unknown>:{line_no}" if line_no else "<unknown>"
        name = str(e.get("name") or "")
        line = f"{i:>3} {size:>10} {format_size_kb(size):>10} {file_line:<60} {name}"
        lines.append(line)
    report_path.parent.mkdir(parents=True, exist_ok=True)
    report_path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def main() -> int:
    root_dir = Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--wasm",
        default=str(root_dir / "web" / "dist" / "output.gl.wasm"),
    )
    parser.add_argument(
        "--dwarf",
        default=str(root_dir / "output.gl.dwarf.txt"),
    )
    parser.add_argument(
        "--report",
        default=str(root_dir / "output.gl.dwarf.report.txt"),
    )
    parser.add_argument(
        "--skip-dump",
        action="store_true",
    )
    args = parser.parse_args()

    wasm_path = Path(args.wasm)
    dwarf_path = Path(args.dwarf)
    report_path = Path(args.report)

    if not args.skip_dump:
        if not wasm_path.exists():
            raise SystemExit(f"wasm not found: {wasm_path}")
        run_dwarfdump(wasm_path, dwarf_path)
    if not dwarf_path.exists():
        raise SystemExit(f"dwarf text not found: {dwarf_path}")
    entries = parse_dwarfdump(dwarf_path)
    write_report(entries, report_path, dwarf_path)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
