#
# Force PlatformIO to compile LVGL's demos/ folder.
#
# The lvgl package keeps demos/ (and examples/) as siblings of src/, and its
# library.json defines no srcFilter, so PlatformIO only builds src/. As a
# result lv_demo_widgets() (used by this example) is never compiled and the
# linker fails with "undefined reference to lv_demo_widgets".
#
# We locate the installed lvgl inside this environment's libdeps, add its root
# to the include path (so the demo .c files find "lvgl.h"), and feed demos/ to
# the compiler. The individual demos stay guarded by their LV_USE_DEMO_* macros
# in lv_conf.h, so only the ones you enable produce code.
#
import os
import glob

Import("env")  # noqa: F821  (injected by PlatformIO/SCons)


def find_lvgl():
    libdeps = env.subst("$PROJECT_LIBDEPS_DIR")
    pioenv = env.subst("$PIOENV")
    candidates = [os.path.join(libdeps, pioenv, "lvgl")]
    candidates += glob.glob(os.path.join(libdeps, "*", "lvgl"))
    for path in candidates:
        if os.path.isdir(os.path.join(path, "demos")):
            return path
    return None


lvgl_dir = find_lvgl()

if lvgl_dir:
    # demo sources include "lvgl.h" (LV_LVGL_H_INCLUDE_SIMPLE) -> need lvgl root
    env.Append(CPPPATH=[lvgl_dir])
    demos_dir = os.path.join(lvgl_dir, "demos")
    env.BuildSources(
        os.path.join("$BUILD_DIR", "lvgl_demos"),
        demos_dir,
    )
    print("[lvgl_build_demos] compiling LVGL demos from: %s" % demos_dir)
else:
    print(
        "[lvgl_build_demos] WARNING: lvgl not found in libdeps yet - "
        "demos will not be compiled (lv_demo_widgets may fail to link)."
    )
