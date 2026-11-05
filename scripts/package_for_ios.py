#!/usr/bin/env python3
"""
Packages the ESP32 BLE controller firmware binary, SPIFFS static web files,
and canonical settings.json schema into the iOS app bundle at:
    ios/Resources/FirmwarePackage/
"""

import hashlib
import json
import re
import shutil
import subprocess
import sys
from pathlib import Path

ACTIVE_STATIC_FILES = [
    "index.html",
    "configurations.html",
    "update.html",
    "fs.html",
    "about.html",
    "profile.jpg",
]


def parse_version_header(header_path: Path):
    text = header_path.read_text(encoding="utf-8")
    ver_match = re.search(r'#define\s+VERSION_SHORT\s+"([^"]+)"', text)
    build_match = re.search(r'#define\s+BUILD_NUMBER\s+"([^"]+)"', text)
    full_match = re.search(r'#define\s+VERSION\s+"([^"]+)"', text)

    version_short = ver_match.group(1) if ver_match else "v1.0.0_0"
    build_number = int(build_match.group(1)) if build_match and build_match.group(1).isdigit() else 0
    version_full = full_match.group(1) if full_match else version_short
    return version_short, build_number, version_full


def extract_embedded_version(firmware_bin: Path):
    if not firmware_bin.exists():
        return None
    data = firmware_bin.read_bytes()
    matches = re.findall(rb"v\d+\.\d+\.\d+_\d+", data)
    if matches:
        return matches[0].decode("ascii", errors="ignore")
    return None


def sha256_file(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(65536), b""):
            h.update(chunk)
    return h.hexdigest()


def find_pio() -> str | None:
    pio_candidates = [
        shutil.which("pio"),
        shutil.which("platformio"),
        str(Path.home() / ".platformio" / "penv" / "bin" / "pio"),
    ]
    return next((c for c in pio_candidates if c and Path(c).exists()), None)


def ensure_firmware_built(ble_dir: Path, firmware_bin: Path, version_header: Path, force_build: bool = False):
    header_ver, _, _ = parse_version_header(version_header)
    embedded_ver = extract_embedded_version(firmware_bin)

    needs_build = force_build or not firmware_bin.exists() or (embedded_ver != header_ver)
    if not needs_build:
        return

    pio_cmd = find_pio()
    if pio_cmd:
        print(
            f"[package_for_ios] Building ESP32 firmware with {pio_cmd} "
            f"(embedded={embedded_ver}, header={header_ver})..."
        )
        res = subprocess.run([pio_cmd, "run", "-e", "esp32"], cwd=str(ble_dir))
        if res.returncode != 0 and not firmware_bin.exists():
            raise RuntimeError("PlatformIO build failed and no firmware.bin exists.")
    elif not firmware_bin.exists():
        raise FileNotFoundError(
            f"Firmware binary not found at {firmware_bin} and PlatformIO ('pio') is not installed."
        )


def copy_package_to(target_dir: Path, firmware_bin: Path, data_dir: Path, manifest: dict):
    if target_dir.exists():
        shutil.rmtree(target_dir)
    spiffs_out_dir = target_dir / "spiffs"
    spiffs_out_dir.mkdir(parents=True, exist_ok=True)

    shutil.copy2(firmware_bin, target_dir / "firmware.bin")
    for f in manifest["staticFiles"]:
        shutil.copy2(data_dir / f["name"], spiffs_out_dir / f["name"])
    shutil.copy2(data_dir / "settings.json", target_dir / "default_settings.json")

    manifest_path = target_dir / "manifest.json"
    with manifest_path.open("w", encoding="utf-8") as f:
        json.dump(manifest, f, indent=2)
        f.write("\n")


def main():
    ble_dir = Path(__file__).resolve().parent.parent
    repo_root = ble_dir.parent
    ios_pkg_dir = repo_root / "ios" / "Resources" / "FirmwarePackage"
    android_pkg_dir = repo_root / "android" / "app" / "src" / "main" / "assets" / "FirmwarePackage"

    firmware_bin = ble_dir / ".pio" / "build" / "esp32" / "firmware.bin"
    version_header = ble_dir / "include" / "version.h"

    force_build = "--build" in sys.argv
    ensure_firmware_built(ble_dir, firmware_bin, version_header, force_build=force_build)

    version_short, build_number, version_full = parse_version_header(version_header)
    embedded_ver = extract_embedded_version(firmware_bin)
    if embedded_ver and embedded_ver != version_short:
        print(
            f"[package_for_ios] Using embedded binary version {embedded_ver} "
            f"(header had {version_short})"
        )
        version_short = embedded_ver
        if "_" in embedded_ver:
            try:
                build_number = int(embedded_ver.split("_")[-1])
            except ValueError:
                pass

    data_dir = ble_dir / "data"
    static_files = []
    for fname in ACTIVE_STATIC_FILES:
        item = data_dir / fname
        if not item.exists():
            continue
        static_files.append(
            {
                "name": item.name,
                "size": item.stat().st_size,
                "sha256": sha256_file(item),
            }
        )

    settings_src = data_dir / "settings.json"
    with settings_src.open("r", encoding="utf-8") as f:
        default_settings_obj = json.load(f)

    required_keys = list(default_settings_obj.keys())

    manifest = {
        "version": version_short,
        "versionFull": version_full,
        "buildNumber": build_number,
        "firmwareFile": "firmware.bin",
        "firmwareSize": firmware_bin.stat().st_size,
        "firmwareSha256": sha256_file(firmware_bin),
        "defaultSettingsFile": "default_settings.json",
        "requiredSettingsKeys": required_keys,
        "deprecatedSettingsKeys": [
            "BTPinCode",
            "BLESecurityEnabled",
            "AutoStart",
        ],
        "staticFiles": static_files,
    }

    copy_package_to(ios_pkg_dir, firmware_bin, data_dir, manifest)
    copy_package_to(android_pkg_dir, firmware_bin, data_dir, manifest)

    offline_updater_script = ble_dir / "tools" / "build_offline_updater.py"
    if offline_updater_script.exists():
        subprocess.run([sys.executable, str(offline_updater_script)], cwd=str(ble_dir), check=False)

    print(f"[package_for_ios] Packaged firmware {version_short} (build {build_number}) -> {ios_pkg_dir}")
    print(f"[package_for_ios] Packaged firmware {version_short} (build {build_number}) -> {android_pkg_dir}")
    print(f"[package_for_ios] Embedded version in firmware.bin: {extract_embedded_version(firmware_bin)}")
    print(f"[package_for_ios] firmware.bin: {manifest['firmwareSize']} bytes")
    print(f"[package_for_ios] static files: {len(static_files)} files")
    print(f"[package_for_ios] settings keys: {len(required_keys)} keys")


if __name__ == "__main__":
    main()
