#!/usr/bin/env python3
"""Restore pinned harness inputs from the repository's offline bundles."""

from __future__ import annotations

import hashlib
import os
from pathlib import Path
import platform
import shutil
import subprocess
import sys
import tarfile


ROOT = Path(__file__).resolve().parents[1]
VENDOR = ROOT / "vendor" / "offline"
WORK = ROOT / "work" / "offline"
MGBA_COMMIT = "431935616c0e216d650b25b2ea0c0acccd359038"
DSSMM_COMMIT = "37c9e069c1f44c2d53ea633fa1bdcf97e788dcd4"
POCKETNES_COMMIT = "9c07f3d1d2a26b957769c5cde942060cc9ea6e64"
LIBGBA_COMMIT = "b8fa7f9c5c47289ec2e1e199d970042315bc1900"
MAXMOD_COMMIT = "ce8adb2a6bf1f29ec203b6c3410c1ddf8e270811"


def run(*args: str, cwd: Path | None = None) -> None:
    subprocess.run(args, cwd=cwd, check=True)


def verify_manifest() -> None:
    manifest = VENDOR / "SHA256SUMS"
    for line in manifest.read_text().splitlines():
        digest, relative = line.split(maxsplit=1)
        path = VENDOR / relative.strip()
        actual = hashlib.sha256(path.read_bytes()).hexdigest()
        if actual != digest:
            raise SystemExit(f"offline input hash mismatch: {path}")


def existing_commit(path: Path) -> str | None:
    if not (path / ".git").exists():
        return None
    result = subprocess.run(
        ["git", "-C", str(path), "rev-parse", "HEAD"],
        check=False,
        capture_output=True,
        text=True,
    )
    return result.stdout.strip() if result.returncode == 0 else None


def extract_mgba() -> None:
    destination = ROOT / "harnesses" / "third_party" / "mgba"
    commit = existing_commit(destination)
    if commit == MGBA_COMMIT:
        return
    marker = destination / ".offline-source-commit"
    if marker.is_file() and marker.read_text().strip() == MGBA_COMMIT:
        return
    if destination.exists():
        raise SystemExit(
            f"refusing to replace existing mGBA directory at {destination}; "
            "move it aside, then rerun"
        )
    destination.mkdir(parents=True)
    with tarfile.open(VENDOR / "sources" / f"mgba-{MGBA_COMMIT}.tar.gz", "r:gz") as archive:
        archive.extractall(destination, filter="data")
    marker.write_text(MGBA_COMMIT + "\n")


def restore_bundle(bundle: Path, destination: Path, commit: str) -> None:
    actual = existing_commit(destination)
    if actual == commit:
        return
    if destination.exists():
        raise SystemExit(
            f"refusing to replace existing checkout at {destination}; move it aside, then rerun"
        )
    destination.parent.mkdir(parents=True, exist_ok=True)
    run("git", "clone", str(bundle), str(destination))
    run("git", "-C", str(destination), "checkout", "--detach", commit)


def restore_toolchain() -> Path:
    toolchain_root = WORK / "devkitpro"
    archive_path = WORK / "devkitpro.tar.gz"
    shutil.rmtree(toolchain_root, ignore_errors=True)
    archive_path.parent.mkdir(parents=True, exist_ok=True)
    with archive_path.open("wb") as output:
        for part in sorted((VENDOR / "toolchain").glob("devkitpro-*.part")):
            with part.open("rb") as input_file:
                shutil.copyfileobj(input_file, output)
    with tarfile.open(archive_path, "r:gz") as archive:
        archive.extractall(toolchain_root, filter="data")
    archive_path.unlink()
    env_script = toolchain_root / "devkitarm.sh"
    env_script.write_text(
        "#!/usr/bin/env bash\n"
        "_offline_root=$(cd -- \"$(dirname -- \"${BASH_SOURCE[0]}\")\" && pwd)\n"
        "export DEVKITPRO=\"$_offline_root\"\n"
        "export DEVKITARM=\"$DEVKITPRO/devkitARM\"\n"
        "export PATH=\"$DEVKITARM/bin:$DEVKITPRO/tools/bin:$PATH\"\n"
    )
    env_script.chmod(0o755)
    return env_script


def main() -> None:
    if platform.system() != "Linux" or platform.machine() not in {"x86_64", "amd64"}:
        raise SystemExit("offline bundles currently target x86_64 Linux")
    if sys.version_info < (3, 12):
        raise SystemExit("offline setup targets Ubuntu 24.04 Python 3.12 or newer")
    required = ("git", "cc", "c++", "cmake", "ninja", "mcopy", "mkfs.vfat", "fsck.fat")
    missing = [name for name in required if shutil.which(name) is None]
    if missing:
        raise SystemExit(
            "missing host platform tools: " + ", ".join(missing)
            + "; offline bundle currently supplies source and devkitARM, not these host tools"
        )
    verify_manifest()
    extract_mgba()
    restore_bundle(
        VENDOR / "sources" / f"dssmm-{DSSMM_COMMIT}.bundle",
        ROOT / "harnesses" / "third_party" / "dssmm",
        DSSMM_COMMIT,
    )
    restore_bundle(
        VENDOR / "sources" / f"pocketnes-{POCKETNES_COMMIT}.bundle",
        ROOT / "work" / "PocketNES",
        POCKETNES_COMMIT,
    )
    env_script = restore_toolchain()
    restore_bundle(
        VENDOR / "sources" / f"libgba-{LIBGBA_COMMIT}.bundle",
        ROOT / "work" / "libgba",
        LIBGBA_COMMIT,
    )
    restore_bundle(
        VENDOR / "sources" / f"maxmod-{MAXMOD_COMMIT}.bundle",
        ROOT / "work" / "maxmod",
        MAXMOD_COMMIT,
    )
    gba_source = ROOT / "work" / "libgba"
    maxmod_source = ROOT / "work" / "maxmod"
    run(
        "bash",
        "-c",
        f"source '{env_script}' && make -C '{gba_source}' clean && "
        f"make -C '{gba_source}' -j2 && make -C '{gba_source}' install && "
        f"make -C '{maxmod_source}' clean && make -C '{maxmod_source}' gba -j2 && "
        f"make -C '{maxmod_source}' install-gba",
    )
    print("Offline source, devkitARM, libgba, and Maxmod inputs restored from this repository.")
    print("Next: source " + str(env_script.relative_to(ROOT)))
    print("Host platform tools are present. Core harness checks can now run without network access.")


if __name__ == "__main__":
    main()
