from __future__ import annotations

import argparse
import re
import shutil
import subprocess
from datetime import datetime
from pathlib import Path


def parse_int(value: str) -> int:
    value = value.strip()
    if not value or value == "-":
        return 0
    return int(value, 16)


def parse_dwarf_report(path: Path) -> list[dict[str, object]]:
    lines = path.read_text(encoding="utf-8", errors="ignore").splitlines()
    entries: list[dict[str, object]] = []
    in_table = False
    for line in lines:
        if line.strip().startswith("---"):
            in_table = True
            continue
        if not in_table:
            continue
        if not line.strip():
            break
        m = re.match(r"^\s*(\d+)\s+(\d+)\s+([\d\.]+)\s+(.+?)\s{2,}(.+)$", line)
        if not m:
            continue
        no = int(m.group(1))
        size_b = int(m.group(2))
        size_kb = m.group(3)
        file_line = m.group(4).strip()
        name = m.group(5).strip()
        entries.append(
            {
                "no": no,
                "size_b": size_b,
                "size_kb": size_kb,
                "file_line": file_line,
                "name": name,
            }
        )
    return entries


def parse_map(path: Path) -> dict[str, object]:
    lines = path.read_text(encoding="utf-8", errors="ignore").splitlines()
    current = ""
    section_sizes: dict[str, int] = {}
    entries: list[dict[str, object]] = []
    last_key = None
    for line in lines:
        if not line.strip():
            continue
        if re.match(r"^\s*Addr\s+Off\s+Size\s+", line):
            continue
        m = re.match(r"^\s*(\S+)\s+(\S+)\s+(\S+)\s+(.*)$", line)
        if not m:
            continue
        off_s = m.group(2)
        size_s = m.group(3)
        rest = m.group(4).strip()
        off = parse_int(off_s)
        size = parse_int(size_s)
        if re.match(
            r"^(TYPE|IMPORT|FUNCTION|TABLE|MEMORY|GLOBAL|EXPORT|ELEM|CODE|DATA|\.rodata|\.data|\.bss|CUSTOM\([^\)]+\))$",
            rest,
        ):
            current = rest
            section_sizes[current] = section_sizes.get(current, 0) + size
            continue
        if not current:
            continue
        key = f"{current}|{off_s}|{size_s}"
        if ":(" not in rest and last_key == key:
            last = entries[-1] if entries else None
            if last and last["off_s"] == off_s and last["size_s"] == size_s:
                name2 = rest.strip()
                if name2:
                    last["name"] = name2
            continue
        file_part = ""
        name = rest
        m2 = re.match(r"^(.*):\((.*)\)$", rest)
        if m2:
            file_part = m2.group(1)
            name = m2.group(2)
        else:
            m3 = re.match(r"^(<internal>):\((.*)\)$", rest)
            if m3:
                file_part = m3.group(1)
                name = m3.group(2)
        entries.append(
            {
                "section": current,
                "off_s": off_s,
                "size_s": size_s,
                "size": size,
                "file": file_part,
                "name": name,
            }
        )
        last_key = key
    return {"section_sizes": section_sizes, "entries": entries}


def bytes_str(n: int) -> str:
    return f"{n} B"


def kb_str(n: int) -> str:
    return f"{n / 1024.0:.2f} KB"


def group_sum(entries: list[dict[str, object]], key_fn, label_fn) -> list[dict[str, object]]:
    table: dict[str, dict[str, object]] = {}
    for e in entries:
        key = key_fn(e)
        if not key:
            continue
        if key not in table:
            table[key] = {"key": key, "size": 0, "label": label_fn(e)}
        table[key]["size"] = int(table[key]["size"]) + int(e["size"])
    return list(table.values())


def normalize_filename(value: str) -> str:
    if not value:
        return ""
    if value.startswith("<"):
        return value
    try:
        return Path(value).name
    except Exception:
        return value


def top_list(items: list[dict[str, object]], top_n: int) -> list[dict[str, object]]:
    return sorted(items, key=lambda x: int(x["size"]), reverse=True)[:top_n]


def write_section(sb: list[str], title: str) -> None:
    sb.append(title)
    sb.append("=" * len(title))


def parse_wat(path: Path) -> dict[str, int]:
    text = path.read_text(encoding="utf-8", errors="ignore")
    return {
        "types": len(re.findall(r"\(\s*type\b", text)),
        "imports": len(re.findall(r"\(\s*import\b", text)),
        "exports": len(re.findall(r"\(\s*export\b", text)),
        "funcs": len(re.findall(r"\(\s*func\b", text)),
        "data": len(re.findall(r"\(\s*data\b", text)),
        "elem": len(re.findall(r"\(\s*elem\b", text)),
        "tables": len(re.findall(r"\(\s*table\b", text)),
        "memories": len(re.findall(r"\(\s*memory\b", text)),
        "globals": len(re.findall(r"\(\s*global\b", text)),
    }


def decode_wat_name(value: str) -> str:
    def repl(match: re.Match) -> str:
        return chr(int(match.group(1), 16))

    return re.sub(r"\\([0-9a-fA-F]{2})", repl, value)


def normalize_wat_name(value: str) -> str:
    value = decode_wat_name(value)
    if value.startswith("$"):
        value = value[1:]
    return value


def parse_wat_funcs(path: Path, max_lines: int) -> list[dict[str, object]]:
    lines = path.read_text(encoding="utf-8", errors="ignore").splitlines()
    funcs: list[dict[str, object]] = []
    current = None
    for line in lines:
        m = re.match(r"^\s*\(func\s+(\$[^\s\)]+)", line)
        if m:
            if current:
                funcs.append(current)
            name = m.group(1)
            current = {
                "name": name,
                "norm": normalize_wat_name(name),
                "snippet": [line],
            }
            continue
        if current and len(current["snippet"]) < max_lines:
            current["snippet"].append(line)
    if current:
        funcs.append(current)
    return funcs


def find_wat_snippet(dwarf_name: str, wat_funcs: list[dict[str, object]]) -> list[str] | None:
    target = dwarf_name.strip()
    if not target:
        return None
    target_norm = normalize_wat_name(target)
    for fn in wat_funcs:
        if target_norm and target_norm in fn["norm"]:
            return fn["snippet"]
        if fn["norm"] and fn["norm"] in target_norm:
            return fn["snippet"]
    return None


def run_wasm2wat(wasm_path: Path, wat_path: Path) -> tuple[bool, str]:
    exe = shutil.which("wasm2wat") or shutil.which("wasm2wat.exe")
    if exe:
        result = subprocess.run(
            [exe, str(wasm_path), "-o", str(wat_path)],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
        )
    else:
        try:
            result = subprocess.run(
                f'wasm2wat "{wasm_path}" -o "{wat_path}"',
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True,
                shell=True,
            )
        except FileNotFoundError:
            return False, "wasm2wat not found"
    if result.returncode != 0:
        return False, result.stderr.strip() or "wasm2wat failed"
    return True, "generated"


def build_report(
    dwarf_report_path: Path,
    map_path: Path,
    wasm_path: Path | None,
    wat_path: Path | None,
    wat_status: str | None,
    out_path: Path,
    top_n: int,
) -> None:
    dwarf_entries = parse_dwarf_report(dwarf_report_path)
    map_info = parse_map(map_path)
    section_sizes = map_info["section_sizes"]
    map_entries = map_info["entries"]

    sb: list[str] = []
    sb.append("smallwasm size report")
    sb.append(f"Time: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
    sb.append(f"DWARF report: {dwarf_report_path}")
    sb.append(f"Map: {map_path}")
    if wat_path:
        sb.append(f"WAT: {wat_path}")
    sb.append("")

    write_section(sb, "DWARF Top Functions")
    for e in dwarf_entries[:top_n]:
        line = (
            f"{int(e['no']):>3}. {int(e['size_b']):>7}  {str(e['size_kb']):>9}  "
            f"{e['file_line']}  {e['name']}"
        )
        sb.append(line)
    if not dwarf_entries:
        sb.append("(no entries)")
    sb.append("")

    write_section(sb, "Map Section Summary")
    want = [
        "CODE",
        ".rodata",
        ".data",
        ".bss",
        "CUSTOM(name)",
        "CUSTOM(producers)",
        "CUSTOM(target_features)",
        "TYPE",
        "IMPORT",
        "EXPORT",
        "ELEM",
        "FUNCTION",
        "TABLE",
        "MEMORY",
        "GLOBAL",
        "DATA",
    ]
    for k in want:
        if k in section_sizes:
            n = int(section_sizes[k])
            sb.append(f"{k:<22} {bytes_str(n):>8}  {kb_str(n):>9}")
    sb.append("")

    code = [e for e in map_entries if e["section"] == "CODE"]
    rodata = [e for e in map_entries if e["section"] == ".rodata"]
    data = [e for e in map_entries if e["section"] == ".data"]
    bss = [e for e in map_entries if e["section"] == ".bss"]

    write_section(sb, "Map Top CODE Symbols")
    for i, it in enumerate(
        top_list(group_sum(code, lambda e: e["name"], lambda e: e["name"]), top_n), 1
    ):
        n = int(it["size"])
        sb.append(f"{i:>2}. {bytes_str(n):>7}  {kb_str(n):>9}  {it['label']}")
    if not code:
        sb.append("(no entries)")
    sb.append("")

    write_section(sb, "Map Top .rodata Symbols")
    for i, it in enumerate(
        top_list(group_sum(rodata, lambda e: e["name"], lambda e: e["name"]), top_n), 1
    ):
        n = int(it["size"])
        sb.append(f"{i:>2}. {bytes_str(n):>7}  {kb_str(n):>9}  {it['label']}")
    if not rodata:
        sb.append("(no entries)")
    sb.append("")

    write_section(sb, "Map Top .data Symbols")
    for i, it in enumerate(
        top_list(group_sum(data, lambda e: e["name"], lambda e: e["name"]), top_n), 1
    ):
        n = int(it["size"])
        sb.append(f"{i:>2}. {bytes_str(n):>7}  {kb_str(n):>9}  {it['label']}")
    if not data:
        sb.append("(no entries)")
    sb.append("")

    write_section(sb, "Map Top .bss Symbols")
    for i, it in enumerate(
        top_list(group_sum(bss, lambda e: e["name"], lambda e: e["name"]), top_n), 1
    ):
        n = int(it["size"])
        sb.append(f"{i:>2}. {bytes_str(n):>7}  {kb_str(n):>9}  {it['label']}")
    if not bss:
        sb.append("(no entries)")
    sb.append("")

    write_section(sb, "Map CODE By Object File")
    code_by_obj = group_sum(
        code,
        lambda e: normalize_filename(str(e["file"])),
        lambda e: normalize_filename(str(e["file"])),
    )
    for i, it in enumerate(top_list(code_by_obj, top_n), 1):
        n = int(it["size"])
        sb.append(f"{i:>2}. {bytes_str(n):>7}  {kb_str(n):>9}  {it['label']}")
    if not code_by_obj:
        sb.append("(no entries)")
    sb.append("")

    if wat_path and wat_path.exists():
        wat_info = parse_wat(wat_path)
        wat_funcs = parse_wat_funcs(wat_path, 12)
        write_section(sb, "WAT Summary")
        sb.append(f"types    : {wat_info['types']}")
        sb.append(f"imports  : {wat_info['imports']}")
        sb.append(f"exports  : {wat_info['exports']}")
        sb.append(f"funcs    : {wat_info['funcs']}")
        sb.append(f"data     : {wat_info['data']}")
        sb.append(f"elem     : {wat_info['elem']}")
        sb.append(f"tables   : {wat_info['tables']}")
        sb.append(f"memories : {wat_info['memories']}")
        sb.append(f"globals  : {wat_info['globals']}")
        sb.append("")
        write_section(sb, "WAT Snippets (Top Functions)")
        for e in dwarf_entries[:top_n]:
            name = str(e["name"])
            snippet = find_wat_snippet(name, wat_funcs)
            if not snippet:
                continue
            sb.append(f"{name}")
            sb.extend([f"  {line}" for line in snippet])
            sb.append("")
    else:
        write_section(sb, "WAT Summary")
        status = wat_status or "missing"
        sb.append(f"status   : {status}")
        sb.append("")

    out_path.write_text("\n".join(sb) + "\n", encoding="utf-8")


def main() -> int:
    root_dir = Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--dwarf-report",
        default=str(root_dir / "output.gl.dwarf.report.txt"),
    )
    parser.add_argument(
        "--map",
        default=str(root_dir / "output.gl.map"),
    )
    parser.add_argument(
        "--wasm",
        default=str(root_dir / "web" / "dist" / "output.gl.wasm"),
    )
    parser.add_argument(
        "--wat",
        default=str(root_dir / "output.gl.wat"),
    )
    parser.add_argument(
        "--out",
        default=str(root_dir / "output.gl.size.report.txt"),
    )
    parser.add_argument("--top-n", type=int, default=20)
    parser.add_argument("--skip-wat", action="store_true")
    args = parser.parse_args()

    dwarf_report_path = Path(args.dwarf_report)
    map_path = Path(args.map)
    wasm_path = Path(args.wasm) if args.wasm else None
    wat_path = Path(args.wat) if args.wat else None
    out_path = Path(args.out)

    if not dwarf_report_path.exists():
        raise SystemExit(f"missing dwarf report: {dwarf_report_path}")
    if not map_path.exists():
        raise SystemExit(f"missing map: {map_path}")

    wat_status = None
    if not args.skip_wat and wat_path:
        if wat_path.exists():
            wat_status = "existing"
        elif wasm_path and wasm_path.exists():
            ok, status = run_wasm2wat(wasm_path, wat_path)
            wat_status = status
            if not ok:
                wat_status = status
        else:
            wat_status = "missing wasm"

    build_report(
        dwarf_report_path,
        map_path,
        wasm_path,
        wat_path,
        wat_status,
        out_path,
        args.top_n,
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
