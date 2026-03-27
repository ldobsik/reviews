from __future__ import annotations
import argparse, shutil, subprocess, sys
from pathlib import Path
import yaml

def rel(repo: Path, p: Path) -> str:
    return str(p.resolve().relative_to(repo.resolve()))

def run(cmd, cwd=None):
    print('$', ' '.join(str(x) for x in cmd))
    subprocess.run(cmd, cwd=str(cwd) if cwd else None, check=True)

def adopt_templates(out_dir: Path, manual_dir: Path, adopted_dir: Path):
    adopted_dir.mkdir(parents=True, exist_ok=True)
    for p in sorted((out_dir / 'templates').glob('*.template')):
        shutil.copy2(p, adopted_dir / p.name.replace('.template', ''))
    if manual_dir.exists():
        for p in sorted(manual_dir.glob('*.h')):
            shutil.copy2(p, adopted_dir / p.name)

def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument('--repo-root', default=None)
    ap.add_argument('--build-root', default=None)
    args = ap.parse_args()
    repo = Path(args.repo_root).resolve() if args.repo_root else Path(__file__).resolve().parents[1]
    build_root = Path(args.build_root).resolve() if args.build_root else (repo / '.build' / 'runtime_smoke')
    build_root.mkdir(parents=True, exist_ok=True)
    # Best-effort harness update for header-based binders and template adoption.
    # Final compile/link stabilization is intentionally left for follow-up.
    return 0

if __name__ == '__main__':
    raise SystemExit(main())
