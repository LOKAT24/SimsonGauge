# Pre-build: bake the firmware version (git tag) into the build as FW_VERSION.
#
# Runs automatically via  extra_scripts = pre:scripts/git_version.py  in
# platformio.ini. scripts/release.ps1 creates the tag *before* building, so a
# release build gets a clean "vX.Y.Z". A normal dev build gets the latest tag +
# commits since + short hash (e.g. "v1.0.0-3-gabc1234"), "-dirty" when the tree
# has uncommitted changes, or "dev" if git is unavailable.

import subprocess

Import("env")  # noqa: F821  (injected by PlatformIO)


def get_fw_version():
    try:
        out = subprocess.check_output(
            ["git", "describe", "--tags", "--always", "--dirty", "--abbrev=7"],
            stderr=subprocess.DEVNULL,
        )
        return out.strip().decode("utf-8")
    except Exception:
        return "dev"


version = get_fw_version()
print("FW_VERSION = %s" % version)

env.Append(CPPDEFINES=[("FW_VERSION", env.StringifyMacro(version))])
