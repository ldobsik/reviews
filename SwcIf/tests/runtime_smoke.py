from __future__ import annotations

import argparse
import shutil
import subprocess
import sys
from pathlib import Path

import yaml


def rel(repo: Path, p: Path) -> str:
    return str(p.resolve().relative_to(repo.resolve()))


def run(cmd, cwd: Path | None = None):
    print("$", " ".join(str(x) for x in cmd))
    subprocess.run(cmd, cwd=str(cwd) if cwd else None, check=True)


def gather_build_inputs(repo: Path, ecu_yaml: Path):
    ecu = yaml.safe_load(ecu_yaml.read_text(encoding="utf-8"))
    ecu_name = ecu["name"]

    include_dirs = [
        repo,
        repo / "core",
        repo / "manual" / ecu_name,
    ]

    sources = [repo / "core" / "SwcIf.c"]

    for swc in ecu.get("swcs", []):
        swc_yaml = (repo / swc["yaml"]).resolve()
        swc_root = swc_yaml.parent
        inc = swc_root / "include"
        if inc.exists():
            include_dirs.append(inc)
        sources.extend(sorted((swc_root / "src").glob("*.c")))

    manual_dir = repo / "manual" / ecu_name
    if manual_dir.exists():
        sources.extend(sorted(p for p in manual_dir.glob("*.c") if not p.name.startswith("SwcIf_Bind_")))

    return ecu, ecu_name, include_dirs, sources, manual_dir


def make_test_main(ecu_name: str) -> str:
    if ecu_name == "HeartbeatECU":
        return """#include <stdint.h>\n#include \"SwcIf.h\"\n#include \"SwcIf_GenSrv.h\"\n\nint main(void)\n{\n    SwcIf_Init();\n    if (SwcIf_GetInstanceCount() != 1u) { return 10; }\n    if (SWCIF_SERVER(HEARTBEAT_0, HEARTBEAT_GetTicks)() != 0u) { return 11; }\n    SwcIf_MainFunction();\n    if (SWCIF_SERVER(HEARTBEAT_0, HEARTBEAT_GetTicks)() != 1u) { return 12; }\n    return 0;\n}\n"""
    if ecu_name == "RearLampECU":
        return """#include <stdint.h>\n#include \"SwcIf.h\"\n#include \"SwcIf_GenSrv.h\"\n#include \"DRLPOS.h\"\n#include \"LAMPDRV.h\"\n\nint main(void)\n{\n    const LAMPDRV_Out_type * lamp_out = (const LAMPDRV_Out_type *)0;\n    SwcIf_Init();\n    if (SwcIf_GetInstanceCount() != 3u) { return 20; }\n    lamp_out = SwcIf_OutputCPtr_LAMPDRV_0();\n    if (lamp_out == (const LAMPDRV_Out_type *)0) { return 21; }\n    if (SWCIF_SERVER(DRLPOS_0, DRLPOS_GetStatus)() != DRLPOS_CONTROL_OFF) { return 22; }\n    SwcIf_MainFunction();\n    SwcIf_MainFunction();\n    if (SWCIF_SERVER(DRLPOS_0, DRLPOS_GetStatus)() != DRLPOS_CONTROL_OFF) { return 23; }\n    SWCIF_SERVER(DRLPOS_0, DRLPOS_ForceOff)();\n    if (SWCIF_SERVER(DRLPOS_0, DRLPOS_GetStatus)() != DRLPOS_CONTROL_OFF) { return 24; }\n    return 0;\n}\n"""
    if ecu_name == "SingletonDemoECU":
        return """#include <stdint.h>\n#include \"SwcIf.h\"\n#include \"FILTER.h\"\n#include \"SENSORBRIDGE.h\"\n\nint main(void)\n{\n    SwcIf_Init();\n    if (SwcIf_GetInstanceCount() != 3u) { return 30; }\n    if (FILTER_cfg == (const FILTER_CfgType *)0) { return 31; }\n    if (SwcIf_OutputCPtr_FILTER() != &tx) { return 32; }\n    if (SwcIf_OutputCPtr_SENSORBRIDGE() != &IO) { return 33; }\n    SwcIf_MainFunction();\n    SwcIf_MainFunction();\n    FILTER_Api.Reset();\n    if (tx.y != 0.0f) { return 34; }\n    return 0;\n}\n"""
    return """#include <stdint.h>\n#include \"SwcIf.h\"\n\nint main(void)\n{\n    SwcIf_Init();\n    SwcIf_MainFunction();\n    return 0;\n}\n"""


def write_text_crlf(path: Path, content: str) -> None:
    content = content.replace("\r\n", "\n").replace("\r", "\n")
    path.write_text(content, encoding="utf-8", newline="\r\n")


def runtime_ecu(repo: Path, ecu_yaml: Path, build_root: Path) -> None:
    ecu, ecu_name, include_dirs, sources, manual_dir = gather_build_inputs(repo, ecu_yaml)
    ecu_build = build_root / ecu_name
    out_dir = ecu_build / "generated"
    obj_dir = ecu_build / "obj"
    shutil.rmtree(ecu_build, ignore_errors=True)
    obj_dir.mkdir(parents=True, exist_ok=True)

    run([
        sys.executable,
        str(repo / "generator" / "generate.py"),
        "--root", str(repo),
        "--ecu", rel(repo, ecu_yaml),
        "--out", str(out_dir),
    ], cwd=repo)

    include_dirs.extend([out_dir])
    sources.extend([out_dir / "SwcIf_Gen.c", out_dir / "SwcIf_GenMeta.c"])

    for inst in ecu.get("instances", []):
        manual_binder = manual_dir / f"SwcIf_Bind_{inst['name']}.c"
        gen_binder = out_dir / f"SwcIf_Bind_{inst['name']}.stub.c"
        if manual_binder.exists():
            sources.append(manual_binder)
        elif gen_binder.exists():
            sources.append(gen_binder)

    test_main = ecu_build / f"runtime_test_{ecu_name}.c"
    write_text_crlf(test_main, make_test_main(ecu_name))
    sources.append(test_main)

    seen = set()
    unique_sources = []
    for s in sources:
        s = s.resolve()
        if s.exists() and s not in seen:
            seen.add(s)
            unique_sources.append(s)

    seen_inc = set()
    unique_includes = []
    for inc in include_dirs:
        inc = inc.resolve()
        if inc.exists() and inc not in seen_inc:
            seen_inc.add(inc)
            unique_includes.append(inc)

    common_compile = [
        "gcc",
        "-std=c11",
        "-Wall",
        "-Wextra",
        "-Werror=implicit-function-declaration",
        "-Werror=incompatible-pointer-types",
        "-Werror=int-conversion",
        "-Werror=return-type",
        "-c",
    ]
    for inc in unique_includes:
        common_compile.append(f"-I{inc}")

    objects = []
    for src in unique_sources:
        obj = obj_dir / (src.name + ".o")
        cpp = obj_dir / (src.name + ".i")
        run(common_compile + [str(src), "-o", str(obj)], cwd=repo)
        run(common_compile + [str(src), "-E", "-o", str(cpp)], cwd=repo)
        objects.append(obj)

    exe = ecu_build / f"runtime_smoke_{ecu_name}"
    run(["gcc", *[str(o) for o in objects], "-o", str(exe)], cwd=repo)
    run([str(exe)], cwd=repo)
    print(f"[OK] {ecu_name}: runtime smoke passed")


def main() -> int:
    ap = argparse.ArgumentParser(description="Runtime smoke harness for SwcIf examples using system gcc")
    ap.add_argument("--repo-root", default=None, help="Repository root (default: parent of this script's directory)")
    ap.add_argument("--build-root", default=None, help="Build/output directory (default: <repo>/.build/runtime_smoke)")
    ap.add_argument("--ecu", action="append", default=[], help="Run only matching ECU name(s); may be repeated")
    args = ap.parse_args()

    repo = Path(args.repo_root).resolve() if args.repo_root else Path(__file__).resolve().parents[1]
    build_root = Path(args.build_root).resolve() if args.build_root else (repo / ".build" / "runtime_smoke")
    build_root.mkdir(parents=True, exist_ok=True)

    ecu_yamls = sorted((repo / "ecus").glob("*/*.yaml"))
    if args.ecu:
        wanted = set(args.ecu)
        ecu_yamls = [p for p in ecu_yamls if p.stem in wanted]

    if not ecu_yamls:
        raise SystemExit("No ECU YAML files selected")

    for ecu_yaml in ecu_yamls:
        runtime_ecu(repo, ecu_yaml, build_root)

    print("All selected ECUs passed runtime smoke.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
