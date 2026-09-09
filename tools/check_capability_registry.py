#!/usr/bin/env python3
"""Fail when an Essentials UI action has no shared capability descriptor."""

from __future__ import annotations

import pathlib
import re
import sys


ROOT = pathlib.Path(__file__).resolve().parents[1]
REGISTRY = ROOT / "src/common/capabilities.c"
# Sources that only exist once the Essentials interface does. If none of these
# is present the interface has not landed yet and there is nothing to check.
ESSENTIALS_FILES = (
    ROOT / "src/libs/tools/essentials_header.c",
    ROOT / "src/libs/essentials_library.c",
    ROOT / "src/libs/essentials_inspector.c",
)
# modulegroups.c predates the interface, so its presence proves nothing.
UI_FILES = ESSENTIALS_FILES + (ROOT / "src/libs/modulegroups.c",)


def main() -> int:
    descriptor_pattern = re.compile(
        r'\.id = "([a-z0-9_.]+)"', re.MULTILINE
    )
    action_pattern = re.compile(r'DT_ESSENTIALS_ACTION\("([a-z0-9_.]+)"\)')
    registered = set(descriptor_pattern.findall(REGISTRY.read_text(encoding="utf-8")))
    referenced: dict[str, list[str]] = {}
    for source in UI_FILES:
        # the Essentials sources land in later commits; a file that is not here
        # yet has no actions to check, and must not fail the build
        if not source.exists():
            continue
        for capability_id in action_pattern.findall(source.read_text(encoding="utf-8")):
            referenced.setdefault(capability_id, []).append(str(source.relative_to(ROOT)))

    missing = {key: value for key, value in referenced.items() if key not in registered}
    if missing:
        for capability_id, sources in sorted(missing.items()):
            print(f"unregistered Essentials capability {capability_id}: {', '.join(sources)}")
        return 1
    if not any(source.exists() for source in ESSENTIALS_FILES):
        print("the Essentials interface is not present yet; nothing to check")
        return 0
    if not referenced:
        print("no Essentials actions were found in the Essentials sources")
        return 1
    print(f"verified {len(referenced)} Essentials actions against {len(registered)} capabilities")
    return 0


if __name__ == "__main__":
    sys.exit(main())
