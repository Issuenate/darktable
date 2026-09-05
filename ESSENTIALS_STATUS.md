# Essentials interface - working status

Handoff note for whoever picks this up next, human or agent. Untracked on
purpose: it is a status file for this checkout, not project documentation.

- **Architecture**: [`dev-doc/Essentials_UI_Architecture.md`](dev-doc/Essentials_UI_Architecture.md) (in-tree, committed with the work)
- **Agent boundary**: [`dev-doc/Agent_Tool_Architecture.md`](dev-doc/Agent_Tool_Architecture.md)
- **Visual QA log**: [`design-qa.md`](design-qa.md) (chronological, has been overwritten at least once)
- **Commit split**: not committed yet. `tools/split-essentials-commits.sh` is a
  ready-to-run script with drafted messages, seven commits that each build on
  their own. It is excluded from commits itself.

Last updated after a code review and a simplification pass over the whole
changeset.

## Where it stands

Nothing is committed. About 1320 changed lines in 14 tracked files plus 2570
lines of new source, across 25 paths, all in the working tree.

| area | state |
|------|-------|
| Capability registry, search, CI check, unit tests | working |
| MCP `capabilities_search` / `capabilities_describe` | working, read-only |
| Library: destinations, add photos, empty state | working |
| Inspector: rating, reject, color labels, albums | working, **never seen on screen** |
| Editor: friendly sections from borrowed widgets | working |
| Editor: tool mode (crop, perspective, grading, mixer, tone curve) | working, **never clicked** |
| Mode switch, panel save/restore, theming | working |
| Plan executor, preview, receipts, undo groups | **not started** |
| Essentials export surface | **not started**, falls back to the Advanced module |

41 capabilities, 40 `DT_ESSENTIALS_ACTION` references, enforced by
`tools/check_capability_registry.py` (ctest `test_essentials_capability_registration`).

## Read this before writing code here

These are mistakes already made once in this feature. Do not reintroduce them.

**Braces and spacing.** darktable is Allman with `if(`, not K&R with `if (`.
Measured 715 to 3 across sampled libs, 373 to 0 on braces. The four Essentials
files were originally written K&R and have been converted. Match the tree.

**`CAPABILITY()` is taken.** `src/external/ThreadSafetyAnalysis.h` defines it,
and it arrives through `common/dtpthread.h`, so nothing here may define a macro
by that name. The descriptors are plain designated initializers now; the only
macro left is `DT_CAPABILITY_ARGS()`, which keeps an argument array and its
count in step. `tools/check_capability_registry.py` finds descriptors by
grepping for `.id = "..."`.

**Two rating accessors disagree about reject.** `dt_ratings_get()` returns
`DT_VIEW_REJECT` (6); `dt_image_get_xmp_rating()` returns **-1**
(`common/image.c:607`). Mixing them silently breaks state display. The write
path is `ratings.c`, so read with `dt_ratings_get()`.

**`dt_tag_attach_string_list()` splits on commas** (`common/tags.c:548`). Any
user-entered name reaching it needs validating first, or one album becomes two
tags.

**Do not call `dt_lib_set_visible()`** to hide modules for Essentials. It
persists the choice under `<view>/<module>_visible`, so it would rewrite the
user's own panel preferences and leave modules hidden in Advanced. Essentials
changes what is on screen, never what the user asked for.

**One reader for the mode.** `dt_essentials_mode_is_active()` in
`common/capabilities.c`. It resolves `auto` and writes the answer back. Do not
add a fourth local copy; there used to be three plus a divergent one.

**`_lib_modulegroups_switch_to()` clears `force_show_module`.** Tool mode
therefore sets the group by hand and refreshes through
`_lib_modulegroups_update_visibility_proxy()`, which leaves it alone.

**Leaving a tool must drop the module focus first.** Crop commits its box from
`gui_focus(self, FALSE)`; rebuilding the panel without releasing focus throws
the crop away.

**CSS: conservative properties only.** An unsupported one crashed GTK style
computation on macOS. Check with a `GtkCssProvider` before assuming it parses.

**`FILE(COPY ...)` runs at configure time**, so editing `data/themes/darktable.css`
does not refresh the build tree. `tools/run-essentials-local.sh` re-copies it.

## What changed most recently

Two rounds after the original Library/Choose and Edit slices.

**Round 1 - closed the P1 gaps in the QA log.**

- Albums became a Library destination. An album is a tag under the reserved
  `album|` hierarchy (`DT_ESSENTIALS_ALBUM_PREFIX` in `common/capabilities.h`),
  so it stays visible in the Advanced tagging module and travels in the XMP.
  The destination is a collection rule on `DT_COLLECTION_PROP_TAG` with the text
  `album*`; the trailing `*` selects the hierarchy, see `get_query_string()` in
  `common/collection.c:1677`.
- Inspector gained color labels (five plus clear) and an album row.
- Reject became reachable, next to the stars.
- The stars never showed the rating at all: `dtgtk_cairo_paint_star()` fills
  only when handed a color as paint data, so `dtgtk_button_set_active()` drew
  nothing. The fill colour is read from the style context at update time, not in
  `gui_init()`, where the widget is not styled yet.
- Confirmation became proportional. A dialog only appears when an action reaches
  two or more photos; one click on one photo applies immediately.
  `requires_confirmation` in a descriptor governs the *agent* lifecycle, not a
  human's click.
- New capabilities `library.albums`, `library.label`; enum arguments now carry
  their accepted `values` through the MCP descriptor.

**Round 2 - unreachable tools.** Reported: "the cropping and other tab on the
editing panel, and I cannot do the color grading."

Correct, and structural. The editor was built entirely on the Quick Access
Panel, which can only borrow individual bauhaus widgets. Crop and perspective
need the canvas; color balance rgb's grading lives in a notebook tab; the color
equalizer was not exposed at all. Hiding the quick-access "go to the full
module" link, which Essentials does deliberately, closed the last escape route.

Added **tool mode**: a section can carry rows that open a module's own complete
interface in the panel, with a back arrow in the title, built on darktable's
existing `force_show_module` filter. Rows: tone curve (light, whichever tone
mapper the workflow enabled), color grading and color mixer (color), crop &
straighten and rotate & perspective (geometry). New capabilities
`edit.light.tone_curve`, `edit.color.grading`, `edit.color.mixer`.

**Round 3 - code review.** Two Critical defects fixed (the reject-state read and
the comma-in-album-name tag split, both described above), plus the macro
collision, the duplicated mode predicate, a stranding hole where a group switch
from a shortcut left the user in the technical module list, the brace style, a
full-catalog tag query on every album dropdown, and an idle callback that could
outlive `gui_cleanup()`.

**Round 4 - simplification.** The registry's 41 descriptors were built by two
positional macros, so every call site ended in five unnamed booleans
(`TRUE, TRUE, TRUE, FALSE, TRUE`) that nobody could read without counting
commas. They are now designated initializers, with one small
`DT_CAPABILITY_ARGS()` macro left to keep an argument array and its count in
step. Verified by diffing the complete MCP serialization of all 41 descriptors
before and after: byte-identical. `_apply_experience()` went from 96 lines to 39
by extracting the panel-override and module-visibility passes.

Deliberately *not* simplified: the `_essentials_*_name()` lookup chains in
`modulegroups.c` (flat, one mapping per line, a table would add a struct and a
loop for nothing) and the inspector's `gui_init()` (131 lines, where existing
darktable libs run 153 to 376).

## What is verified, and what is not

Verified: clean build of darktable, darktable-cli, darktable-mcp,
test_capabilities and every Essentials target, no warnings.
`ctest -R capabilit` passes both tests. The theme parses through a
`GtkCssProvider`. darktable starts in Essentials against an isolated catalog and
reaches the darkroom with no criticals, GTK warnings or invalid casts. Find
Anything resolves natural phrases to the right capabilities over MCP.

**Not verified: anything under the pointer.** No screenshot and no click has
happened in several sessions, because Screen Recording is not granted to the
desktop client on this machine. Both Critical defects found in review were
state-display and data-integrity bugs that no automated check caught and that a
single interactive pass would have. Treat every visual and interaction claim in
`design-qa.md` after the "cross-screen alignment" entry as unproven.

To look at it:

```bash
./tools/run-essentials-local.sh ~/Pictures/some-folder
```

Worth trying first: the reject button's state, an album name containing a comma,
clicking a tool row and dragging a crop, and the back arrow.

## Verifying a change to the registry

The descriptors are data, so a change to them can be proved rather than argued.
Dump the whole registry through MCP before and after and diff it; the two search
calls below carry the complete serialization of all 41 descriptors between them:

```sh
dump() {
  printf '%s\n' \
   '{"jsonrpc":"2.0","id":1,"method":"initialize","params":{"protocolVersion":"2024-11-05","capabilities":{},"clientInfo":{"name":"t","version":"1"}}}' \
   '{"jsonrpc":"2.0","id":2,"method":"tools/call","params":{"name":"capabilities_search","arguments":{"limit":32}}}' \
   '{"jsonrpc":"2.0","id":3,"method":"tools/call","params":{"name":"capabilities_search","arguments":{"query":"","context":"edit","limit":32}}}' \
   | ./build/bin/darktable-mcp 2>/dev/null | tail -2
}
dump > before.json      # ... make the change, then rebuild ...
dump > after.json
diff before.json after.json
```

**Check that the build actually succeeded before trusting that diff.** A failed
build leaves the previous `darktable-mcp` in place, and the comparison then
proves nothing. That mistake has already been made here once.

The theme can be checked the same way, by loading
`data/themes/darktable.css` through a `GtkCssProvider` in a throwaway program
and failing on any `parsing-error`.

## Pre-existing bugs noticed here, not fixed

Neither is caused by this work; both were confirmed to reproduce without it.

- `_tooltip_reposition()` in `src/gui/accelerators.c:1081` passes the result of
  `gdk_display_get_monitor_at_window()` straight to `gdk_monitor_get_workarea()`
  with no NULL check, so a tooltip shown before its toplevel is on a monitor
  logs a Gdk-CRITICAL. Reproduces identically in Advanced mode.
- `cmake --build build` with no target fails on `test_sample`, which links a
  bare `cmocka` that this Homebrew setup does not resolve. Build named targets.

## Known gaps and next steps

Roughly in order of value.

1. **An interactive pass.** Everything above is blocked on it.
2. **Tests for the UI logic.** `_confirmed()`'s threshold, tool spec to module
   resolution, and the album prefix round-trip are all testable without GTK and
   none of them are tested.
3. **Split the work into commits** before it goes near a PR, with
   `tools/split-essentials-commits.sh`. Each must build on its own; darktable's
   history is bisected, so check with
   `git rebase --exec 'cmake --build build' HEAD~7`.
4. **Essentials export.** Currently opens the Advanced export module and says
   so. A focused surface needs the plan executor first.
5. **The plan executor** from `Agent_Tool_Architecture.md`: validation, state
   hashes, preview isolation, receipts, undo groups. Nothing is `executable`
   until it exists.
6. **Masks**, and any other canvas module, have no tool row. Adding one is a
   table entry plus a capability, but masks need a friendly vocabulary first.
7. **`people` and `tags`** both open the tag browser unfiltered; only `albums`
   has a hierarchy of its own.
8. **Auto/Profile controls** are absent on purpose. A visually plausible button
   with no capability behind it is worse than no button.

## Map

```
src/common/capabilities.{c,h}      registry, search, dt_essentials_mode_is_active()
src/libs/tools/essentials_header.c stages, Find Anything, Advanced switch, facade
src/libs/essentials_library.c      left panel: add photos, destinations
src/libs/essentials_inspector.c    right panel: rating, reject, labels, album
src/libs/modulegroups.c            editor sections + tool mode
src/dtgtk/thumbtable.c             Essentials empty-library painting
src/gui/gtk.c                      side panel scrolls without a modifier
data/capabilities/*.schema.json    versioned wire contracts
data/themes/darktable.css          scoped under .essentials-ui / .essentials-panel
tools/check_capability_registry.py CI: every action needs a descriptor
tools/run-essentials-local.sh      run against an isolated catalog (untracked, macOS)
tools/split-essentials-commits.sh  the commit split, drafted (untracked)
```
