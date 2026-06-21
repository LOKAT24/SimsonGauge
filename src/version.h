#ifndef VERSION_H
#define VERSION_H

// FW_VERSION is injected at build time from the git tag by
// scripts/git_version.py (see platformio.ini -> extra_scripts). When building
// without git, or outside that script, fall back to "dev".
#ifndef FW_VERSION
#define FW_VERSION "dev"
#endif

#endif // VERSION_H
