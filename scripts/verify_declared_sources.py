#!/usr/bin/env python3
import argparse, glob, os, re, sys, subprocess
from pathlib import Path


def parse_depfile(p):
    s = Path(p).read_text(errors='ignore').replace('\\\n', ' ')
    i = s.find(':')
    if i < 0:
        return []
    rhs = s[i + 1 :]
    out, cur, esc = [], '', False
    for ch in rhs:
        if esc:
            cur += ch
            esc = False
            continue
        if ch == '\\':
            esc = True
            continue
        if ch in ' \n\r\t':
            if cur:
                out.append(cur)
                cur = ''
        else:
            cur += ch
    if cur:
        out.append(cur)
    return [str(Path(x).resolve()) for x in out]


def parse_dependinfo(p):
    txt = Path(p).read_text(errors='ignore')
    deps = []
    for m in re.finditer(r'set\(CMAKE_DEPENDS_DEPENDENCY_FILES\s*"([^"]*)"\)', txt):
        deps.extend(m.group(1).split(';'))
    for m in re.finditer(r'set\([A-Z_]+_DEPENDS_DEPENDENCY_FILES\s*"([^"]*)"\)', txt):
        deps.extend(m.group(1).split(';'))
    return [str(Path(x).resolve()) for x in deps if x]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--build-dir', required=True)
    ap.add_argument('--target', required=True)
    ap.add_argument('--target-root', required=True)
    ap.add_argument('--declared', required=True)
    ap.add_argument('--exclude', action='append', default=[])
    ap.add_argument('--ninja', default=None, help='Path to ninja executable (optional)')
    args = ap.parse_args()

    root = str(Path(args.target_root).resolve()) + os.sep
    excludes = [str(Path(x).resolve()) + os.sep for x in args.exclude]
    declared = set(
        x.strip() for x in Path(args.declared).read_text().splitlines() if x.strip()
    )

    used = set()
    for d in glob.glob(
        os.path.join(args.build_dir, f'**/CMakeFiles/**/*.d'), recursive=True
    ):
        for dep in parse_depfile(d):
            used.add(dep)
    for d in glob.glob(
        os.path.join(args.build_dir, f'**/CMakeFiles/**/DependInfo.cmake'),
        recursive=True,
    ):
        for dep in parse_dependinfo(d):
            used.add(dep)

    if args.ninja:
        try:
            proc = subprocess.run(
                [args.ninja, '-C', args.build_dir, '-t', 'deps'],
                stdout=subprocess.PIPE,
                stderr=subprocess.DEVNULL,
                text=True,
                check=False,
            )
            lines = proc.stdout.splitlines()
            n = len(lines)
            i = 0
            while i < n:
                line = lines[i]
                if ':' not in line:
                    i += 1
                    continue
                _, rhs = line.split(':', 1)
                rhs = rhs.split('#', 1)[0].strip()
                tokens = rhs.split()
                j = i + 1
                while j < n:
                    cont = lines[j]
                    if not cont or (cont[0] not in ' \t'):
                        break
                    cont = cont.split('#', 1)[0].strip()
                    if cont:
                        tokens.extend(cont.split())
                    j += 1
                for tok in tokens:
                    p = Path(tok)
                    if not p.is_absolute():
                        p = (Path(args.build_dir) / p).resolve()
                    used.add(str(p))
                i = j
        except Exception:
            pass

    filtered = set()
    for dep in used:
        if not dep.startswith(root):
            continue
        if any(dep.startswith(ex) for ex in excludes):
            continue
        if not (dep.endswith('.hpp') or dep.endswith('.cpp') or dep.endswith('.h')):
            continue
        filtered.add(dep)

    missing = sorted(x for x in filtered if x not in declared)
    if missing:
        sys.stderr.write(
            f"\n[verify:{args.target}] files used but not in sources.cmake (count={len(missing)}):\n"
        )
        for m in missing:
            sys.stderr.write(f"  {m}\n")
        return 1
    return 0


if __name__ == '__main__':
    raise SystemExit(main())


