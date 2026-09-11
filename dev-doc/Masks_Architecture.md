# Mask subsystem architecture

How masks attach to modules, how the fork's AI object masks and the Essentials
Lightroom-style local masks build on that, and the specific things an agent gets
wrong when changing any of it.

This page is not a full reference for darktable's upstream mask system. It gives
the mental model, then documents the two fork features layered on top. For the
segmentation model I/O (tensors, preprocessing, `config.json`) see
[AI_Tasks.md](AI_Tasks.md); for the blend GUI and pixelpipe generally see
[GUI.md](GUI.md) and [pixelpipe_architecture.md](pixelpipe_architecture.md).

## Three layers, and their state

| Layer | What it is | Where | Status |
|-------|------------|-------|--------|
| Native masks + blend | drawn shapes, parametric masks, raster reuse, the blend GUI | upstream | committed |
| AI object masks | `DT_MASKS_OBJECT` type + automatic subject/sky/background selection | fork | committed (on `master`), `HAVE_AI` only |
| Essentials local masks | mask-first "create a mask, then adjust only that area" editor workflow | fork | **working-tree WIP** on `local/essentials-and-remove`, mostly unstaged |

Line numbers below refer to the `local/essentials-and-remove` working tree on
2026-09-11. The Essentials pieces are an uncommitted snapshot, not APIs available
on `master`; locate the named functions with `rg` before relying on these lines.

## Mental model: `mask_mode` is a per-module bitmask

Each module opts into masking through `dt_develop_blend_params_t.mask_mode`, a
`dt_develop_mask_mode_t` bitmask (`src/develop/blend.h:93`):

```c
DEVELOP_MASK_DISABLED    = 0,       // off
DEVELOP_MASK_ENABLED     = 1,       // uniform (blend the whole frame)
DEVELOP_MASK_MASK        = 1 << 1,  // drawn mask
DEVELOP_MASK_CONDITIONAL = 1 << 2,  // parametric mask
DEVELOP_MASK_RASTER      = 1 << 3,  // raster mask (reuse another module's mask)
```

Two facts that are easy to miss:

- **The modes are a bitmask but the UI is radio-style.** Exactly one mode toggle
  (`selected_mask_mode`) is active at a time. Drawn/parametric and raster are
  **mutually exclusive in the supported GUI modes**: raster is not combined
  with `MASK`/`CONDITIONAL`, while drawn and parametric can be combined.
  At render time the `if(uniform) / else if(raster) / else (drawn+parametric)`
  chain in `dt_develop_blend_process` (`src/develop/blend.c:526`) gives raster
  precedence rather than composing it with drawn/parametric masks. The GUI
  prevents adding a drawn/parametric mode to raster with the
  guard at `src/develop/blend_gui.c:1447`:
  `if(module->blend_params->mask_mode & (mask_mode | DEVELOP_MASK_RASTER)) return FALSE;`
  This is the AGENTS.md pitfall "raster masks are mutually exclusive with drawn
  and parametric ones."

- **A drawn mask is a group of shapes.** Shapes are `dt_masks_form_t`
  (`dt_masks_type_t`: circle, ellipse, path, brush, gradient, group, and
  `object` under `HAVE_AI`). A module's shapes are collected into one GROUP form
  whose id lives in `blend_params.mask_id`; group members
  (`dt_masks_point_group_t`) carry a combine op (union/intersection/difference/
  sum/exclusion) and per-shape opacity.

### The one rule agents break

**When starting a mask from a GUI action, use the blend GUI's mode transition
rather than only assigning `blend_params->mask_mode`.** The
full sequence that keeps the panel consistent (the mode toggle, the radio
`selected_mask_mode`, the header mask indicator, showmask/suppress visibility,
box visibility, and a history item) lives only in `_blendop_masks_modes_toggle`
(`src/develop/blend_gui.c:1435`) and its `_blendop_masks_mode_callback`. Writing
`mask_mode` directly leaves the blend GUI half-initialized; completing the first
shape then crashes on the inconsistent module. This exact bug was hit and fixed
during the Essentials work, which is why `dt_iop_gui_blend_start_mask` (below)
exists.

### The single shared creation state

There is exactly one in-progress mask creation for the whole app:
`darktable.develop->form_gui` (`dt_masks_form_gui_t`) plus
`darktable.develop->form_visible`. On-canvas drawing reads these from the
darkroom center view and dispatches to `form->functions->button_pressed` etc.

The drawn-mask creation flow:

```
shape button click
  -> _blendop_masks_create_shape        (blend_gui.c:1629)
       -> _blendop_masks_modes_toggle(NULL, self, DEVELOP_MASK_MASK)   // enable drawn mode the sanctioned way
       -> masks_shown = DT_MASKS_EDIT_FULL, activate the shape toggle
       -> dt_masks_create(type)          (masks/masks.c:895)
       -> dt_masks_change_form_gui(form) (masks/masks.c:1456)          // resets form_gui, sets form_visible
       -> form_gui->creation_module = self;  form_gui->object_selection = ...
  ... user draws on canvas ...
  -> dt_masks_gui_form_save_creation     (masks/masks.c:382)           // appends form, _group_create sets blend_params.mask_id
```

**`creation_module` and `object_selection` must be set AFTER
`dt_masks_change_form_gui`**, never before: that call runs
`dt_masks_clear_form_gui`, which resets `creation_module = NULL` and
`object_selection = DT_MASKS_OBJECT_MANUAL`.

**`masks_type[]` index order is not the enum bit order.** `gui_init` fills it as
`[0]=GRADIENT, [1]=PATH, [2]=ELLIPSE, [3]=CIRCLE, [4]=BRUSH`, and `[5]=OBJECT`
only under `HAVE_AI` (`src/develop/blend_gui.c:2888` onward;
`DEVELOP_MASKS_NB_SHAPES` is 6 with `HAVE_AI`, else 5, `src/develop/blend.h:286`).
Anything mapping a shape enum to a button must search `masks_type[]`; do not
assume `index == bit`.

## Raster masks: reusing one module's mask in another

Raster masks are the mechanism for driving a second module from a mask computed
by a first. A source module publishes its computed blend mask into its
pipe-piece under the well-known key `BLEND_RASTER_ID`; a consumer binds to it and
fetches it at blend time.

```
source module                                  consumer module
  dt_iop_advertise_rastermask (imageop.c)         raster combo pick
  publishes mask under BLEND_RASTER_ID    <----   _raster_value_changed_callback (blend_gui.c:2998)
  (gated by dt_iop_piece_is_raster_mask_used)     writes blend_params.raster_mask_source/instance/id
                                                  dt_iop_commit_blend_params (imageop.c:2060)
                                                    materializes module->raster_mask.sink
  blend time (blend.c:617 raster branch): dt_dev_get_raster_mask(piece, sink.source, sink.id, ...)
```

- `dt_dev_get_raster_mask` (`src/develop/pixelpipe_hb.c:3792`) **enforces pipe
  order**: the source must be earlier in the pipe than the consumer, otherwise it
  logs "processed later in the pixel pipe" and returns NULL.
  `blend_params.raster_mask_invert` flips the fetched mask.
- Reordering or renaming instances must fix up consumers:
  `dt_iop_update_multi_priority` rewrites `raster_mask_instance` in every user's
  blend params and history.

**Do not confuse raster masks with `dt_masks_iop_use_same_as`.**
`dt_masks_iop_use_same_as` (`src/develop/masks/masks.c:1681`) copies a source
module's **drawn group members** into another module's own group (shared shape
references, independent opacity). It never touches the `raster_mask_*` fields.
Both are "reuse a mask" mechanisms but they are different; the Essentials feature
and its forward plan use the drawn-group one, not raster (see the misnomer note
below).

## AI object masks (committed, `HAVE_AI`)

Interactive object masking (SAM/SAM2/SegNext) is exposed as a mask shape,
`DT_MASKS_OBJECT` (`1 << 8`, `src/develop/masks.h:47`, `HAVE_AI` only). On top of
the click-to-segment tool, the fork adds one-click **subject / sky / background**
selection. Commit `12272d1112`.

### Build gating

`src/develop/masks/object.c` carries **no in-file `HAVE_AI` guards**: the whole
file is compiled only when `USE_AI` is on (`src/CMakeLists.txt`, which adds
`-DHAVE_AI`). By contrast `masks.c`, `blend_gui.c` and `libs/masks.c` wrap their
object hooks in explicit `#ifdef HAVE_AI`. Note the asymmetry:

- **unconditional** (present even in non-AI builds): the enum
  `dt_masks_object_selection_t` (`MANUAL=0, SUBJECT, SKY, BACKGROUND`,
  `src/develop/masks.h:403`) and the `form_gui->object_selection` field.
- **`HAVE_AI` only**: `DT_MASKS_OBJECT`, `dt_masks_functions_object`,
  `dt_masks_object_available()` and `dt_masks_object_selectors()`.

### Availability and the shared selector row

`dt_masks_object_available()` (`src/develop/masks/object.c:2154`) returns true
only when the AI registry is enabled and the active model for task `"mask"` is
downloaded. Both UI surfaces (the mask manager and each module's blend controls)
gate the buttons on it, logging "AI model is not available. Check preferences >
AI" otherwise.

`dt_masks_object_selectors()` (`src/develop/masks/object.c:2166`) builds the one
"select | subject | sky | background" button row used by both surfaces. Each
button carries its value as `GINT_TO_POINTER(DT_MASKS_OBJECT_SUBJECT + i)`, so a
button **never** emits `MANUAL` (0). `blend_gui.c` relies on this:
`if(object_selection)` treats any non-zero as "this is an object selection" and
forces the object shape index.

### Automatic selection is heuristic seeding, not classification

The model returns unlabeled regions. Automatic subject/sky/background just seeds
the same decoder with sampled points and ranks the candidates by photographic
priors:

1. On the encode worker, after encode + decoder warmup, `_automatic_prompt`
   (`src/develop/masks/object.c:314`) samples a fixed grid of independent
   foreground points: **5x5 = 25** over the whole frame for subject/background,
   **5x2 = 10** in the upper image for sky. It decodes a candidate per point
   (`dt_seg_reset_prev_mask` between them so they do not inherit each other),
   scores each with `_automatic_score` (`src/develop/masks/object.c:256`), and
   keeps the best point.
2. `ENCODE_READY` is published **after** warmup and `_automatic_prompt` in both
   the cache-hit and fresh-encode paths, so readiness implies inference is done.
3. On the main thread, `_automatic_apply` (via `g_idle_add`, not from the draw
   handler) replays the best point as a single label-1 prompt, sets
   `invert_selection` for background, runs the decoder, and produces an editable
   preview. The user refines with clicks; right-click commits as path shapes.

Differences worth knowing: **subject and background pick the same point**;
background just inverts the refined mask. **Sky** uses a separate ranking (top
contact, blue or bright-neutral color, low texture) and **keeps disconnected
components** (sky behind branches). Under an inverted (background) selection the
foreground/background click labels are swapped at the prompt level so click and
shift-click still add to and subtract from the displayed selection.

### Threading gotchas (why this code looks the way it does)

- **No nested GTK events during a decode.** `_run_decoder` uses
  `dt_control_set_temp_cursor("wait")`, not `dt_gui_cursor_set_busy()`, on
  purpose: pumping GTK events inside a synchronous decode could deliver a
  tool-change that frees `gui` and its scratchpad underneath the decoder.
- **Cancellation is an atomic int.** `_free_data` sets `d->canceled`, and
  `_automatic_prompt` polls it at the top of every iteration, so closing the
  tool mid-inference aborts the candidate loop.

These behaviors are locked by `src/tests/unittests/ai/test_object_selection.c`
(candidate ranking, cancellation, inversion, disconnected sky/background
exclusions, and that no idle callback runs during a decode). It `#include`s
`object.c` directly and stubs the `dt_seg_*` API, so it needs no real model;
build with `-DBUILD_TESTING=ON` and `USE_AI`.

See [AI_Tasks.md](AI_Tasks.md) ("Object Mask") for the segmentation model I/O and
`src/common/ai/segmentation.h`.

## Essentials local masks (working-tree WIP, unstaged)

The Essentials editor (`dt_essentials_mode_is_active()`, config
`ui/experience_mode == "essentials"`) is gaining a Lightroom-style workflow:
**create a mask (brush / radial / linear, or AI subject / sky / background), then
adjust only that area.** This inverts darktable's native model, where masking
lives inside each module's blend UI. See
[Essentials_UI_Architecture.md](Essentials_UI_Architecture.md) for the editor
shell this plugs into.

### The carrier model

Rather than adding a pixelpipe module, each mask is carried by a **dedicated
no-op `exposure` instance** (EV 0), duplicated from the real exposure module,
that owns the mask. The design and the code live in `src/libs/modulegroups.c`:

- `_essentials_create_mask` (`src/libs/modulegroups.c:1133`) finds the real
  exposure with `_essentials_find_module` (which now **skips carriers**,
  `src/libs/modulegroups.c:1018`), duplicates it with
  `dt_iop_gui_duplicate(base, FALSE)`, names it, reveals it via
  `_essentials_set_tool`, and starts the mask.
- `copy_params = FALSE` is load-bearing: that path skips the params/blend copy
  and the `dt_masks_iop_use_same_as` call (`src/develop/imageop.c:814`), so the
  carrier is a genuine default (EV 0, default blend, no shared mask).
- **A carrier is identified only by its `multi_name` prefix**
  `ESSENTIALS_MASK_PREFIX` (a diamond, `"\xe2\x97\x86 "`,
  `src/libs/modulegroups.c:64`). There is no dedicated flag.
  `_essentials_create_mask` sets `multi_name_hand_edited = TRUE` so darktable's
  auto-rename cannot silently un-mark it.
- `_essentials_find_module` must skip carriers in both roles: so the light
  section's exposure slider stays bound to the real exposure, and so
  `_essentials_create_mask` duplicates the real exposure rather than a carrier.

### `dt_iop_gui_blend_start_mask` (the reason this feature has a public API)

`_blendop_masks_create_shape` is static and keyed off a clicked shape button, so
it cannot be called from `modulegroups.c`. The WIP adds a public wrapper keyed
off the shape type instead:

```c
void dt_iop_gui_blend_start_mask(dt_iop_module_t *module,
                                 dt_masks_type_t shape,
                                 dt_masks_object_selection_t object);
```

Declared `src/develop/blend.h:510`, defined `src/develop/blend_gui.c:1700` (both
unstaged). It maps `shape` to a button via `masks_type[]`, refuses
`DT_MASKS_OBJECT` when the AI model is unavailable, then runs the same sanctioned
sequence as `_blendop_masks_create_shape`: `_blendop_masks_modes_toggle(NULL,
module, DEVELOP_MASK_MASK)`, activate the shape button, `dt_masks_create` +
`dt_masks_change_form_gui`, set `creation_module` and `object_selection`. It
exists precisely so an out-of-blend-UI caller cannot half-initialize the blend
GUI (the crash described in "The one rule agents break").

### The masks section

`_essentials_add_masks_section` (`src/libs/modulegroups.c:1183`) builds a "masks"
expander packed **first** in the editor (`_basics_show`), so masks read as the
top-level step. It lists existing carriers (open/delete rows) and an add-source
row from `_essentials_mask_sources[]` (`src/libs/modulegroups.c:1121`); the three
AI sources are skipped when no model is available.

### Gotchas specific to this WIP

- **Builds break with `USE_AI=OFF`.** `_essentials_mask_sources[]` names
  `DT_MASKS_OBJECT` unconditionally, but that enumerator only exists under
  `HAVE_AI`. Every other AI reference in the feature is guarded; this array is
  not. Wrap the AI rows in `#ifdef HAVE_AI` (or gate them another way) before
  this lands, and build with `USE_AI` both on and off per AGENTS.md.
- **"Raster" in the early design notes is a misnomer.** The carrier owns a
  **drawn** mask (`DEVELOP_MASK_MASK`), not a raster mask. Nothing in the feature
  touches `DEVELOP_MASK_RASTER`. The named Increment-2 reuse primitive
  `dt_masks_iop_use_same_as` also shares **drawn** groups. If a future change
  wants a single computed mask (including feathering) to drive several modules,
  raster masks are the alternative; pick deliberately and update the wording.
- **Delete needs a sibling.** `dt_iop_gui_delete` -> `_gui_delete_callback`
  returns early unless another instance shares the carrier's base id; the real
  exposure provides that, so deleting a carrier is safe only while the base
  exposure is in the pipe.
- **Increment 2 is not built.** Current code reveals the whole carrier exposure
  module for the adjustment. The planned per-mask slider stack of masked module
  instances bound via `dt_masks_iop_use_same_as` is intent only; nothing in the
  diff creates additional bound instances.

### Related, separately staged changes on this branch

Do not fold these into the mask feature; they ride along in the same working
tree but are unrelated:

- `_basics_hide` clears window focus before reparenting/destroying the focused
  tool button (`src/libs/modulegroups.c:585`, staged). A real crash fix, but part
  of the editor shell, not the masks.
- `src/iop/colorbalancergb.c` reorders the color-wheel row (staged), and
  `src/views/darkroom.c` adds a "cannot open photo for editing" dialog (staged).
  Neither is mask-related.

## Cross-cutting pitfalls

- Raster masks are mutually exclusive with drawn and parametric masks; drawn
  and parametric masks can be combined. See `blend.c` and the
  `blend_gui.c:1447` guard.
- GUI mask creation must go through the blend mode transition, not just assign
  `mask_mode`.
- `masks_type[]` index is not the `dt_masks_type_t` bit order.
- `synch_all` starts every pipeline node at its module's `default_enabled`
  before replaying history, so auto-applied modules (and their masks) run
  whether or not history asked for them (AGENTS.md).
- Suspected, unverified: `src/libs/masks.c:174` reaches `bd->masks_shown` under a
  guard (`mod && mod->enabled && mask_mode & DEVELOP_MASK_MASK`) that does not
  imply `blend_data != NULL`; a module can carry `mask_mode` from history before
  its blend GUI is built. Treat as a hazard to confirm, not a known live bug.

## Not covered here

For safety when touching these, read the code directly: mask **persistence**
(the `dev->forms` -> `pipe->forms` handoff and XMP/DB serialization), **blend
versioning and legacy migration** (`DEVELOP_BLEND_VERSION`,
`dt_develop_blend_legacy_params`, per-form `version`), **mask distortion** across
crop/rotate/scale, and the **OpenCL** blend path
(`dt_develop_blend_process_cl`, which must stay in sync with the CPU path).
Changing a `blend_params` or form field without updating the legacy converters
corrupts every saved edit.

## Testing obligations

- Touching the pixelpipe or any `src/iop/` module: run
  `src/tests/integration/` (pixel output within dE < 2, OpenCL off **and** on).
- AI object masks: `src/tests/unittests/ai/test_object_selection.c`
  (`-DBUILD_TESTING=ON` + `USE_AI`).
- Build with `USE_AI` both on and off; the disabled path is where the broken
  stubs hide.

## Where to change what

| I want to... | Touch |
|--------------|-------|
| Add a mask shape type | `dt_masks_type_t` (masks.h), `masks_type[]` in `gui_init`, a `dt_masks_functions_*` table |
| Change how drawn masks enable/commit | `_blendop_masks_modes_toggle` / `_blendop_masks_create_shape` (blend_gui.c) |
| Change automatic subject/sky/background | `_automatic_prompt` / `_automatic_score` / `_run_decoder` (masks/object.c) |
| Change the AI availability gate | `dt_masks_object_available` (object.c) |
| Change the Essentials mask sources or UI | `_essentials_mask_sources[]` / `_essentials_add_masks_section` (modulegroups.c) |
| Change how an Essentials mask is started | `dt_iop_gui_blend_start_mask` (blend_gui.c) |
| Reuse one module's mask in another | raster: `raster_mask_*` + `dt_dev_get_raster_mask`; drawn group: `dt_masks_iop_use_same_as` |
