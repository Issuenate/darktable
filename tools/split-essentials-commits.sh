#!/bin/sh
# Split the Essentials work into six commits that each build on their own.
# Run from the repo root, on a branch (not master). Verify with:
#   git rebase --exec 'cmake --build build' <base>
set -eu

git add src/common/capabilities.c src/common/capabilities.h src/CMakeLists.txt \
        src/tests/unittests/test_capabilities.c src/tests/unittests/CMakeLists.txt \
        tools/check_capability_registry.py data/capabilities data/CMakeLists.txt
git commit -F - <<'MSG'
common: add a shared registry of user-goal capabilities

The Essentials interface, deterministic search and any future agent need
one vocabulary for "what can darktable do for me", separate from module
names, parameter blobs and translated labels.

A descriptor owns a stable unlocalized id, friendly copy, search phrases,
typed arguments, a side-effect class, lifecycle requirements and an agent
exposure level. dt_capabilities_search() ranks a natural phrase against
that metadata deterministically, with no model in the path.

DT_ESSENTIALS_ACTION() marks a UI dependency on an id, and
tools/check_capability_registry.py fails the build when one has no
descriptor. The macros carry a DT_ prefix because a bare CAPABILITY()
collides with the thread-safety annotation in
src/external/ThreadSafetyAnalysis.h.

The versioned wire schemas in data/capabilities are the boundary a plan
executor will have to honour. No executor exists yet; nothing here can
change a photo.

Co-Authored-By: Claude Opus 5 <noreply@anthropic.com>
MSG

git add src/mcp/mcp_tools.c src/mcp/README.md data/mcp-tools.json
git commit -F - <<'MSG'
mcp: expose the capability registry read-only

capabilities_search and capabilities_describe let an agent discover what
darktable can do in goal terms rather than by guessing module names, and
report each capability's safety class and lifecycle requirements.

Both are read-only. A descriptor marked previewable is discoverable, not
executable: nothing here grants mutation authority, and the existing raw
introspection tools are unchanged.

Co-Authored-By: Claude Opus 5 <noreply@anthropic.com>
MSG

git add src/libs/essentials_library.c src/libs/essentials_inspector.c \
        src/libs/tools/essentials_header.c src/libs/CMakeLists.txt \
        data/darktableconfig.xml.in src/dtgtk/thumbtable.c src/gui/gtk.c \
        tools/run-essentials-local.sh
git commit -F - <<'MSG'
libs: add the guided essentials interface

Presents darktable as three steps for someone who has never used a raw
processor: add photos, choose, edit and export. A short left-hand list of
destinations, a contextual panel for rating, color labels and albums, and
a find-anything box that searches the capability registry by everyday
phrase.

ui/experience_mode selects it; dt_essentials_mode_is_active() resolves
"auto" once, to guided for a new user and advanced for an existing one.
The header filters which library modules are on screen rather than
calling dt_lib_set_visible(), which would persist the choice and rewrite
the user's own panel preferences.

An album is a darktable tag under the reserved album| hierarchy, so it
stays visible in the tagging module and travels in the XMP sidecar.

Co-Authored-By: Claude Opus 5 <noreply@anthropic.com>
MSG

git add src/libs/modulegroups.c
git commit -F - <<'MSG'
modulegroups: dress the quick access panel as an essentials editor

Adds a "workflow: essentials" preset and presents it as plain-language
sections built from the modules' own widgets, so history, undo, sidecars
and rendered output stay identical to the full interface. The user's own
layout and selected group are saved and restored around it.

Borrowed sliders cannot represent every tool: crop and perspective need
the canvas, and color grading, the color mixer and the tone curve are
whole notebooks. Those get a tool row that opens the module's real
interface in the panel, using the existing force_show_module filter, with
a back arrow in the title. Leaving a tool drops the module focus first so
crop commits its box.

The group also snaps back to the sections whenever essentials is active
and no tool is open, because the group buttons are hidden and a switch
arriving from a shortcut would otherwise strand the user in the technical
module list.

Co-Authored-By: Claude Opus 5 <noreply@anthropic.com>
MSG

git add data/themes/darktable.css
git commit -F - <<'MSG'
themes: style the essentials surfaces

Scoped under .essentials-ui, .essentials-panel and .essentials-editor
plus the individual widget names, so the rules cost nothing when the
guided interface is off. Kept to conservative properties: an unsupported
one crashed GTK style computation on macOS.

Co-Authored-By: Claude Opus 5 <noreply@anthropic.com>
MSG

git add dev-doc/Essentials_UI_Architecture.md dev-doc/Agent_Tool_Architecture.md dev-doc/README.md
git commit -F - <<'MSG'
dev-doc: document the essentials interface and the agent boundary

Co-Authored-By: Claude Opus 5 <noreply@anthropic.com>
MSG

git add RELEASE_NOTES.md
git commit -F - <<'MSG'
RELEASE_NOTES.md: guided essentials interface

Co-Authored-By: Claude Opus 5 <noreply@anthropic.com>
MSG

git add -f ESSENTIALS_STATUS.md design-qa.md tools/split-essentials-commits.sh
git commit -F - <<'MSG'
notes: working status and design QA for the essentials branch

Untracked working notes, committed so the branch carries its own context:
where the work stands, what has never been checked on screen, the mistakes
already made once here, and the commit split that produced this history.

Drop this commit before proposing any of the work upstream.

Co-Authored-By: Claude Opus 5 <noreply@anthropic.com>
MSG

echo "done - now verify each step builds:"
echo "  git rebase --exec 'cmake --build build' HEAD~8"
