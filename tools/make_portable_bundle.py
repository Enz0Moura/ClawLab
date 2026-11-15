#!/usr/bin/env python3
"""Create a relocatable Linux bundle of the LaClaw Lab applications."""

from __future__ import annotations

import argparse
import os
import platform
import re
import shutil
import subprocess
import sys
import tarfile
from pathlib import Path


SYSTEM_PROVIDED_PREFIXES = (
    "ld-linux",
    "libc.so",
    "libm.so",
    "libpthread.so",
    "libdl.so",
    "librt.so",
    "libresolv.so",
    "libutil.so",
    "libgcc_s.so",
    "libstdc++.so",
    # OpenGL loads the host's vendor driver at runtime. Bundling GL dispatch or
    # vendor libraries can break hardware acceleration on another machine.
    "libGL.so",
    "libGLX.so",
    "libOpenGL.so",
    "libGLdispatch.so",
    "libEGL.so",
    "libGLES",
    "libgbm.so",
    "libdrm.so",
    "libglapi.so",
    "libvulkan.so",
)

DEPENDENCY_RE = re.compile(r"^\s*(\S+)\s+=>\s+(\S+)")
LOADER_RE = re.compile(r"^\s*(/\S+)\s+\(")


def is_host_dependency(soname: str) -> bool:
    name = Path(soname).name
    return name == "linux-vdso.so.1" or name.startswith(SYSTEM_PROVIDED_PREFIXES)


def dependencies(binary: Path) -> list[tuple[str, Path]]:
    result = subprocess.run(["ldd", str(binary)], text=True, capture_output=True, check=False)
    output = result.stdout + result.stderr
    if result.returncode != 0 or "not a dynamic executable" in output:
        raise RuntimeError(f"ldd não conseguiu analisar {binary}:\n{output.strip()}")

    found: dict[str, Path] = {}
    for line in output.splitlines():
        if "not found" in line:
            raise RuntimeError(f"Dependência ausente de {binary}: {line.strip()}")
        match = DEPENDENCY_RE.match(line)
        if match:
            soname, resolved = match.groups()
            found.setdefault(soname, Path(resolved))
            continue
        match = LOADER_RE.match(line)
        if match:
            loader = Path(match.group(1))
            found.setdefault(loader.name, loader)
    return list(found.items())


def stage_bundle(root: Path, arch: str) -> Path:
    if not sys.platform.startswith("linux"):
        raise RuntimeError("O pacote portátil é destinado a Linux.")
    if platform.machine() != arch:
        raise RuntimeError(f"Arquitetura solicitada {arch} difere do host {platform.machine()}.")

    stage = root / "portable" / f"laclawlab-linux-{arch}-portable"
    if stage.exists():
        shutil.rmtree(stage)
    bin_dir, lib_dir = stage / "bin", stage / "lib"
    bin_dir.mkdir(parents=True)
    lib_dir.mkdir()

    executables = ("prize_machine", "library_demo")
    for name in executables:
        source = root / name
        if not source.is_file():
            raise RuntimeError(f"Executável não encontrado: {source}; rode make all primeiro.")
        shutil.copy2(source, bin_dir / name)

    copied: set[str] = set()
    for name in executables:
        for soname, resolved in dependencies(bin_dir / name):
            if is_host_dependency(soname) or soname in copied:
                continue
            destination = lib_dir / soname
            shutil.copy2(resolved, destination)
            copied.add(soname)

    for name in executables:
        env = os.environ.copy()
        env["LD_LIBRARY_PATH"] = str(lib_dir)
        result = subprocess.run(
            ["ldd", str(bin_dir / name)], env=env, text=True, capture_output=True, check=False
        )
        output = result.stdout + result.stderr
        if result.returncode != 0 or "not found" in output:
            raise RuntimeError(f"O pacote ainda tem dependências ausentes para {name}:\n{output}")
        for line in output.splitlines():
            match = DEPENDENCY_RE.match(line)
            if not match:
                continue
            soname, resolved = match.groups()
            if not is_host_dependency(soname) and not Path(resolved).is_relative_to(lib_dir):
                raise RuntimeError(f"{soname} não foi incluída no pacote ({resolved}).")

    (stage / "run.sh").write_text(
        "#!/bin/sh\n"
        'APP_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd) || exit 1\n'
        'export LD_LIBRARY_PATH="$APP_DIR/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"\n'
        'exec "$APP_DIR/bin/prize_machine" "$@"\n',
        encoding="utf-8",
    )
    (stage / "run-demo.sh").write_text(
        "#!/bin/sh\n"
        'APP_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd) || exit 1\n'
        'export LD_LIBRARY_PATH="$APP_DIR/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"\n'
        'exec "$APP_DIR/bin/library_demo" "$@"\n',
        encoding="utf-8",
    )
    for launcher in (stage / "run.sh", stage / "run-demo.sh"):
        launcher.chmod(0o755)

    (stage / "README.txt").write_text(
        f"LaClaw Lab — Linux {arch} portable bundle\n\n"
        "Run ./run.sh for the simulator or ./run-demo.sh for the library example.\n"
        "This package includes FreeGLUT and its non-system user-space dependencies.\n"
        f"It still requires a compatible Linux {arch} system, glibc, an X11 display,\n"
        "and the system OpenGL implementation/driver for the destination machine.\n",
        encoding="utf-8",
    )
    return stage


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-dir", required=True, type=Path)
    parser.add_argument("--arch", required=True)
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()

    try:
        stage = stage_bundle(args.build_dir, args.arch)
        args.output.parent.mkdir(parents=True, exist_ok=True)
        with tarfile.open(args.output, "w:gz") as archive:
            archive.add(stage, arcname=stage.name)
    except (OSError, RuntimeError, subprocess.SubprocessError) as error:
        print(f"portable bundle: {error}", file=sys.stderr)
        return 1

    print(f"Pacote portátil criado: {args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
