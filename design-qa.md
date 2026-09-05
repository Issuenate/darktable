> Chronological visual-QA log. For current state, open questions and the
> traps to avoid when resuming, read `ESSENTIALS_STATUS.md` first.

# Essentials Library / Choose design QA

## Source of truth

- Selected concept: `/Users/issuenate/.codex/generated_images/01a06c46-9397-75a1-a9a3-a7b3dc057b47/exec-b786405e-a26d-41c6-9897-18b2ffaacfca.png`
- Source dimensions: 1487 x 1058
- State: Library / Choose, one photo selected

## Implementation capture

- Native GTK capture: `/Users/issuenate/Projects/darktable/build/design-qa/implementation.png`
- Capture dimensions: 2742 x 1830
- State: Library / Choose, one photo selected, isolated temporary catalog with four bundled sample photos
- Platform: macOS, native darktable shell

## Comparison evidence

- Full side-by-side comparison: `/Users/issuenate/Projects/darktable/build/design-qa/full-comparison.png`
- The implementation capture was proportionally scaled and padded beside the 1487 x 1058 source so the complete windows could be compared without cropping.

## Comparison history

### Iteration 1

- [P1] Essentials panels did not remain hidden after the main window's final `show_all`, exposing Advanced controls in the same state.
- [P1] The initial CSS triggered a GTK style-computation crash on macOS.
- [P2] The shared header was a dense single row and did not preserve the concept's step hierarchy.

Changes made: defer the Essentials visibility pass until the main window maps, scope the CSS to conservative supported properties, and split the header into a stage row plus a search/action row.

### Iteration 2

- [Resolved] The native Library state remains running with Advanced side panels hidden.
- [Resolved] Add Photos, Choose, Edit & Export, Find Anything, Advanced, Library navigation, the photo grid, selection details, rating, Open in Edit, and Export are present in the intended hierarchy.
- [Resolved] The header, navigation, inspector, and primary action use the selected graphite-and-blue visual direction.
- [P1] Albums and recent imports are absent from the Essentials navigation, so the implemented first-use organization flow is incomplete relative to the source.
- [P1] Label and Add to album are absent from the contextual inspector.
- [P2] Native darktable's bottom toolbar and panel revealers remain visible instead of the reduced status footer shown in the concept.
- [P2] The native macOS title bar and available test-display height produce different proportions from the concept.
- [Data-state] The reference shows a populated four-column catalog; the isolated verification catalog contains only four bundled photos. This is not treated as a layout defect.

## Final result

blocked

The foundational Library / Choose slice is functional and testable, but the P1 organization and inspector gaps above prevent claiming visual and functional parity with the selected concept.

# Essentials Edit / Lightroom design QA

## Source of truth

- Product reference: `/Users/issuenate/Projects/darktable/build/lightroom-reference/lightroom-edit-panel.png`
- State: desktop Edit view with histogram, Light controls, and a large central photo
- Interaction target: the complete right inspector scrolls under the pointer, with no requirement to grab a narrow scrollbar

## Implementation evidence

- Native GTK capture: `/Users/issuenate/Projects/darktable/build/interaction-qa/13-lightroom-edit-panel.png`
- Combined comparison: `/Users/issuenate/Projects/darktable/build/interaction-qa/14-reference-comparison.png`
- Code-level state: Essentials applies a dedicated quick-access preset made from the original darktable widgets; Advanced restores the user's saved module preset and selected group

## Comparison findings

- [Resolved] Export is a persistent blue action in the shared header.
- [Resolved] The Edit panel has a wider, color-neutral graphite surface, a clear Edit title, larger adjustment rows, and restrained dividers.
- [Resolved] Technical module links, preset menus, module-order controls, blend modes, masks, and instances are not part of the Essentials quick-access surface.
- [Resolved] Essentials controls operate on the original module widgets, so history, undo, XMP, and rendered output remain shared with Advanced mode.
- [Resolved] Mouse-wheel and trackpad scrolling work across the full right panel in Essentials; Advanced preserves its existing modifier preference.
- [Resolved] The user's Advanced module-group preset, selected group, and left-panel visibility are preserved when changing modes.
- [P1] The automated macOS session reports `Failed to initialize CVDisplayLink`; its captured window remains on a stale loading frame and therefore cannot prove the final repainted adjustment panel visually.
- [P1] A live visual pass on a normal interactive display is still required to tune spacing and confirm the final module order against the Lightroom reference.
- [P2] A Lightroom-like Auto control is intentionally absent because no safe shared capability/executor has been defined for it yet; a visual-only button would be misleading.

## Final result

blocked

The implementation compiles and the interaction architecture is in place, but the current automated display cannot provide a trustworthy final-state screenshot. Visual parity remains blocked until the final inspector is observed and compared on an interactive display.

# Essentials Empty Library design QA

## Source and implementation evidence

- Reported screen: `/Users/issuenate/Desktop/Screenshot 2026-09-04 at 20.41.27.png`
- Corrected native GTK capture: `/Users/issuenate/Projects/darktable/build/interaction-qa/16-empty-library-polished.png`
- Side-by-side comparison: `/Users/issuenate/Projects/darktable/build/interaction-qa/17-empty-library-comparison.png`
- State: empty isolated catalog in Essentials Library

## Comparison findings

- [Resolved] The legacy empty-collection help diagram and its overlapping instructional text are replaced by one short title and one next-step sentence.
- [Resolved] The native logo, filter strip, rating footer, and empty global bottom panel no longer compete with the Essentials workflow.
- [Resolved] Add Photos is now a prominent blue primary action in the Library panel.
- [Resolved] The contextual inspector uses the quiet placeholder `select a photo` until a selection exists.
- [Resolved] The graphite surfaces now fill the complete content area instead of exposing the native gray empty-panel background.
- [Resolved] Advanced mode restores the user's previous center and bottom panel visibility when leaving Essentials.

## Final result

pass

No P0, P1, or P2 visual issue remains in the empty Library state. The transient first-run style-import toast in the isolated test catalog is application status feedback, not part of the persistent layout.

# Essentials cross-screen alignment and USB workflow QA

## Evidence

- Reported inconsistent state: `/Users/issuenate/Desktop/Screenshot 2026-09-04 at 20.58.28.png`
- Corrected Library/culling state: `/Users/issuenate/Projects/darktable/build/interaction-qa/18-zh-library-culling-aligned.png`
- Corrected Edit facade: `/Users/issuenate/Projects/darktable/build/interaction-qa/23-edit-aligned-fixed.png`
- Reported-state comparison: `/Users/issuenate/Projects/darktable/build/interaction-qa/library-compare.png`
- Lightroom/Edit comparison: `/Users/issuenate/Projects/darktable/build/interaction-qa/edit-compare.png`

## Comparison findings

- [Resolved] View routing uses stable view flags rather than translated view names, so non-English Library no longer activates the Edit step or exposes technical panels.
- [Resolved] Culling and comparison layouts keep the same graphite Library shell, left navigation, contextual inspector, fixed header, and persistent Export action.
- [Resolved] Edit reapplies its facade after the darkroom finishes constructing widgets, preventing GTK's final show pass from revealing the technical module list.
- [Resolved] The Edit inspector now contains friendly adjustment sections backed by the original processing widgets, with a continuous full-panel scroll target.
- [Resolved] The Essentials module-layout identifier is stable and unlocalized; UI translations do not control preset selection.
- [Resolved] The primary Library action says `browse usb or folder`, searches for USB/external-drive language, and uses darktable's in-place add-to-library path without copying originals.
- [Resolved] The same graphite, border, type, spacing, and blue-action system is used in the shared header, Library panels, preview/culling surface, and Edit inspector.
- [Deferred] Profile/Auto and the focused export preset dialog remain executor-backed roadmap work; no visual-only controls were added.

## Final result

pass for the scoped Add Photos, Choose/culling, and Essentials Edit shell states

No P0, P1, or P2 consistency issue remains in these scoped states. The isolated macOS capture process can occasionally remain on a stale darkroom loading frame when backgrounded; the completed Edit capture above is the accepted interactive evidence.

# Essentials Library / Choose P1 follow-up

(This chapter was lost when design-qa.md was overwritten by an external editor
on 2026-09-05; restored from the session record. ESSENTIALS_STATUS.md is the
durable copy.)

## Scope

The two P1 items left open by "Essentials Library / Choose design QA" above,
plus the registered-but-unreachable `library.reject` capability.

## Changes made

- [P1 resolved] Albums are a first-class Library destination. An album is a
  darktable tag under the reserved `album|` hierarchy, so the destination is a
  collection rule on `DT_COLLECTION_PROP_TAG` with the text `album*`. Recent
  imports were already present as `library.recently_added`.
- [P1 resolved] The contextual inspector gained a color label row (five labels
  plus clear) and an album row (a combo with entry listing existing albums, plus
  add), so `Label` and `Add to album` are no longer absent.
- [Resolved] Reject is reachable, next to the stars.
- [Resolved] The stars now show the current rating. `dtgtk_cairo_paint_star()`
  fills only when handed a color as paint data, so the previous
  `dtgtk_button_set_active()` calls drew nothing; the rating widget gave no
  feedback at all.
- [Resolved] Confirmation is proportional. A dialog appears only when an action
  reaches two or more photos; a single click on a single photo applies
  immediately. Rating, reject, color label and album share `_confirmed()`.
- New capabilities `library.albums` and `library.label`; an `enum` argument now
  carries its accepted `values` through the MCP descriptor.

## Final result

blocked on visual confirmation only

Both P1 functional gaps are closed and every automated check passes. The visual
parity claim for these rows remains unproven until someone looks at them.

# Essentials Edit: unreachable tools

## Reported

"the cropping and other tab on the editing panel, and I cannot do the color
grading. There's a color panel here right now, but it's just adjusting the
saturation, warmth, or cold, not the color grading."

## Diagnosis

Correct on both counts, and the cause is structural rather than a missing
preset entry. The Essentials editor was built entirely on the Quick Access
Panel, which can only borrow individual bauhaus widgets from a module. That is
enough for a slider, and not enough for:

- crop and rotate & perspective, whose real interface is the canvas. The
  geometry section did contain `crop/aspect`, `flip/rotate 90 degrees CW/CCW`,
  `ashift/rotation` and `ashift/automatic cropping`, so an aspect ratio could be
  picked, but no crop box could ever be drawn.
- color balance rgb, whose grading lives in the 4 ways tab. Only
  `global vibrance`, `global saturation` and `contrast` were borrowed, which is
  exactly the "saturation, warmth or cold" that was reported.
- color equalizer, which was not exposed at all.
- the tone mapper's curve.

Hiding the quick-access "go to the full module" link, which Essentials does so
the technical panel cannot leak in, closed the only remaining escape route.

## Changes made

- Added Essentials tool mode. A section can carry rows that open a module's own
  complete interface in the panel, with a back arrow in the editor title. It
  uses darktable's existing `force_show_module` single-module filter, so the
  user gets the real module rather than an imitation of it.
- Tool rows: tone curve (light, whichever tone mapper the workflow enabled),
  color grading and color mixer (color), crop & straighten and rotate &
  perspective (geometry).
- New capabilities `edit.light.tone_curve`, `edit.color.grading` and
  `edit.color.mixer`; crop and perspective reuse the existing
  `edit.geometry.crop` and `edit.geometry.straighten`.
- Leaving a tool releases the module focus before rebuilding the panel, so
  crop's `gui_focus(self, FALSE)` commits the crop box instead of dropping it.
- A tool cannot survive a mode switch or a view change, since the Advanced panel
  has no back arrow.

## Verification

- Clean build of darktable, darktable-mcp, test_capabilities and all Essentials
  targets; no new warnings.
- Registry check and unit tests pass: 40 actions against 41 capabilities.
- Find Anything resolves the new goals by everyday phrase: "split tone the
  shadows" to `edit.color.grading`, "crop this to a square" to
  `edit.geometry.crop`, "make only the greens more saturated" to
  `edit.color.mixer`.
- darktable opens a photo straight into the Essentials darkroom against an
  isolated catalog with no criticals, GTK warnings or invalid casts, which
  exercises the tool row construction in `_basics_show()`.
- Not verified: clicking a tool row. Screen Recording permission is not granted
  to the desktop client on this machine, so the panel could not be driven or
  captured. The crop overlay, the back arrow's return path and the committed
  crop box still need an interactive pass.

## Final result

blocked on interactive confirmation

The unreachable-tool gap is closed in code and every automated check passes. No
claim is made yet that dragging a crop box in Essentials produces a crop.

# Code review follow-up

## Scope

Five-axis review of the complete Essentials changeset, then the fixes.

## Defects found and fixed

- [Critical] The reject button never showed its state. `_update()` read the
  rating with `dt_image_get_xmp_rating()`, which returns -1 for a rejected
  photo (`common/image.c:607`), and compared it against `DT_VIEW_REJECT` (6).
  The comparison could never be true. There are two rating accessors with
  different reject encodings; the write path uses `ratings.c`, so the read now
  does too.
- [Critical] An album name containing a comma silently created a second,
  unrelated top-level tag, because `dt_tag_attach_string_list()` splits on
  commas (`common/tags.c:548`). "Trip, 2026" produced `album|Trip` and a loose
  `2026`. Commas are now rejected with a toast.
- [Important] `CAPABILITY()` collided with the thread-safety annotation macro in
  `src/external/ThreadSafetyAnalysis.h`, reached through `common/dtpthread.h`.
  The registry only built because it included nothing that pulled that in;
  adding `control/conf.h` broke it immediately. Renamed to
  `DT_CAPABILITY_DESCRIPTOR`.
- [Important] `_is_essentials()` existed three times verbatim plus a fourth
  divergent copy in `gui/gtk.c` that did not resolve `auto`. Replaced by
  `dt_essentials_mode_is_active()` in `common/capabilities.c`.
- [Important] A group switch from outside Essentials - a module's show shortcut
  reaching `_show_module_callback()`, or a history entry - stranded the user in
  the technical module list, since Essentials hides the group buttons.
  `_lib_modulegroups_update_iop_visibility()` now snaps back to the sections
  unless a tool is open.
- [Important] The four new files used K&R braces and `if (` spacing; darktable
  is Allman with `if(` (715 to 3 across sampled libs). Converted mechanically,
  with the token stream verified unchanged.
- [Consider] The album dropdown called `dt_tag_get_with_usage()`, which counts
  every tagged image in the catalog to build a list that is then filtered down
  to a few rows. Now `dt_tag_get_tags_images()`, scoped to the album hierarchy.
- [Consider] `_view_changed` queued `_apply_experience` on an idle that could
  outlive `gui_cleanup()`'s `g_free(self->data)`. The source id is now tracked,
  coalesced to one pending pass, and removed at teardown.
- [Hygiene] Removed the write-only `label_clear` field; made `album_add` local.
- [Documented] `_set_module_widget_visible()` bypasses `dt_lib_set_visible()`
  on purpose: the supported call persists to conf and would rewrite the user's
  panel preferences. That reasoning is now in the code.

## Not fixed

- The changeset is roughly 3300 lines, well past a reviewable size. It has not
  been committed; the split is left to the human, as CONTRIBUTING and AGENTS.md
  require.
- No test coverage for the new UI logic. `_confirmed()`'s threshold, tool
  spec-to-module resolution and the album prefix round-trip are all testable
  without GTK and none of them are tested.

## Verification

- Clean build of darktable, darktable-cli, darktable-mcp, test_capabilities and
  all Essentials targets; no new warnings.
- Registry check and unit tests pass: 40 actions against 41 capabilities.
- Theme parses through a GtkCssProvider.
- darktable starts in Essentials against an isolated catalog with four photos,
  no criticals, GTK warnings or invalid casts.
- Not verified: any of it under the pointer. Both Critical defects were
  state-display and data-integrity bugs that no automated check caught and that
  one interactive pass would have.

# Simplification pass

- Registry descriptors converted from two positional macros to designated
  initializers. The call sites previously ended in five consecutive unnamed
  booleans. Verified behaviour-identical by diffing the full MCP serialization
  of all 41 descriptors before and after: byte-identical.
- `_apply_experience()` split by responsibility, 96 lines to 39.
- One compressed guard in `_essentials_section_name()` split into two guard
  clauses.
- Rejected as churn: converting the `_essentials_*_name()` chains to tables, and
  splitting the inspector's `gui_init()`.

Found in passing, not fixed: `_tooltip_reposition()` in
`src/gui/accelerators.c:1081` passes the result of
`gdk_display_get_monitor_at_window()` to `gdk_monitor_get_workarea()` without a
NULL check, so a tooltip shown before its toplevel is on a monitor logs a
Gdk-CRITICAL. Reproduces identically in Advanced mode, so it is pre-existing and
unrelated to this work.
