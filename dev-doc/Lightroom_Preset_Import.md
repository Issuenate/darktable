# Lightroom preset import

darktable can import a Lightroom develop preset (`.xmp`) as an **approximate**
darktable style. The whole importer is file-local `static` `_lightroom_*` helpers
in `src/common/styles.c`; it adds no new public function and reuses the existing
style storage path, so an imported preset is indistinguishable from any other
style once saved.

"Approximate" is the load-bearing word: only a handful of Lightroom settings map
to a darktable module, the transfer curves are empirical, and everything else is
dropped and merely listed in the style description. This page is about what maps,
what does not, and what an agent breaks by changing it.

## Where it hooks in

There is no new entry point. Two existing public functions gain a branch that
fires when the filename ends in `.xmp` (`_is_lightroom_preset`,
`src/common/styles.c:1440`, a pure suffix test):

- `dt_styles_import_from_file()` (`src/common/styles.c:2086`) runs the
  conversion, `dt_style_save()`s it, and raises `DT_SIGNAL_STYLE_CHANGED`, then
  returns before the native `.dtstyle` GMarkup path.
- `dt_get_style_name()` (`src/common/styles.c:2239`, declared in
  `src/gui/styles.h`) runs the *full* conversion just to return the resulting
  style name, or `NULL` on failure. The GUI uses this to **validate before it
  offers to overwrite/delete** an existing style.

UI: `_import_clicked()` in `src/libs/styles.c` accepts `*.dtstyle` and `*.xmp` in
the same file chooser. A preset that fails to convert (`dt_get_style_name`
returns `NULL`) is **silently skipped** in the loop; only a `dt_control_log`
fires, no dialog.

Both public functions parse the file, and the overwrite path deletes the old
style before re-importing. Validation therefore precedes deletion, but this is
not an atomic replacement: a file change or a failure on the second read can
still prevent re-import after deletion. Preserve the validation order if you
add caching.

## What it parses

`_lightroom_style_read()` (`src/common/styles.c:1702`) uses **libxml2** (not the
GMarkup parser the native path uses), and is deliberately strict:

- `xmlReadFile(..., XML_PARSE_NONET)`, and any DTD (`intSubset`/`extSubset`) is
  rejected outright, blocking entity-expansion / XXE.
- exactly **one top-level** `rdf:Description` is required under `rdf:RDF`,
  optionally wrapped in `x:xmpmeta`. Nested descriptions in embedded looks are
  not counted (`src/common/styles.c:1751`).
- settings are read from the Adobe camera-raw-settings namespace
  `crs = "http://ns.adobe.com/camera-raw-settings/1.0/"` (`:1704`), in either
  form: as RDF **attributes** on the Description, or as crs **child elements**. A
  key present as both is a duplicate and rejected.
- **only top-level settings** are read; embedded looks and masks (which have
  their own parameters) are not descended into.
- `PresetType='Normal'` is required (`src/common/styles.c:1798`). This is the
  preset-type gate separating a develop preset from a same-extension image
  sidecar; a `Look` profile or a sidecar without `PresetType` is rejected. A
  preset also needs at least one convertible setting. **Do not loosen this
  gate.**

## The approximation mapping

Three mechanisms, all writing through **introspection** (`module->so->get_p` /
field setters by name), never by casting to a `dt_iop_*_params_t` struct.

### Scalar mappings

A `mappings[]` table (`src/common/styles.c:1710`) sets
`param = value * scale + offset` after range-checking the raw Lightroom value:

| Lightroom `crs` key | darktable module | param | transform |
|---|---|---|---|
| `Exposure2012` | exposure | exposure | x1 (EV) |
| `GrainAmount` | grain | strength | x0.8 |
| `GrainFrequency` | grain | scale | empirical table |
| `SplitToningShadowHue` / `...Saturation` | splittoning | shadow_hue / shadow_saturation | /360 ; x0.01 |
| `SplitToningHighlightHue` / `...Saturation` | splittoning | highlight_hue / highlight_saturation | /360 ; x0.01 |
| `SplitToningBalance` | splittoning | balance | x0.005 + 0.5 |
| `Clarity2012` | bilat (local contrast) | detail | x0.0065 |
| `PostCropVignetteAmount` | vignette | brightness | x0.01 |
| `PostCropVignetteMidpoint` | vignette | scale | empirical table |
| `PostCropVignetteFeather` | vignette | falloff_scale | x1 |

A module is only instantiated when its **trigger** key is present (a `groups[]`
table just after `mappings[]`): e.g. `GrainFrequency` alone will not create a
grain instance, and the split-toning balance/hues alone will not create
splittoning. Each module also gets fixed setup so it behaves like the Lightroom
control (e.g. exposure's bias compensation off, bilat in local-contrast mode).

The `GrainFrequency` and `PostCropVignetteMidpoint` transforms are non-linear,
empirical curves **copied from the image-sidecar importer** in
`src/develop/lightroom.c` (`lr2dt_grain_frequency`, `lr2dt_vignette_midpoint`).
The comments mark the coupling deliberately: if you "fix" one importer's tables,
the other diverges.

### Tone curves

`_lightroom_tonecurves()` (`src/common/styles.c:1570`) reads the point curves
(`ToneCurvePV2012[Red|Green|Blue]`, falling back to the legacy `ToneCurve*`) and
synthesizes a 6-node parametric curve from `ParametricShadows/Darks/Lights/
Highlights` plus the three split points (also using the sidecar importer's
midpoint model). Each fires as a **separate `rgbcurve` instance** with a fixed
`multi_priority` order (parametric, composite, channels) so composite and channel
edits do not overwrite each other. Curves are `DT_S_SCALE_MANUAL_RGB`,
monotone-Hermite, capped at `MAX_ANCHORS` (20) nodes.

### HSL to color zones

`_lightroom_hsl()` (`src/common/styles.c:1653`) maps Lightroom's 8 fixed HSL
bands to a `colorzones` curve's 8 nodes. It positions each node by the band's
**Lab hue angle** (sRGB -> XYZ -> Lab), because color zones selects by Lab hue,
not by an evenly spaced RGB wheel. Lightroom's Luminance/Saturation/Hue
adjustments map to the three color-zones channels with hand-tuned per-channel
factors.

### Why "approximate", and what is dropped

The style description is set to "approximate Lightroom preset conversion"
(`src/common/styles.c:1808`). Most Lightroom settings have **no mapping at all**
(temperature, tint, contrast, whites, blacks, highlights, shadows, texture,
dehaze, sharpening, noise reduction, ...). Every unmapped, non-metadata key is
appended to the description as a sorted `omitted settings: ...` list. Conversion
is **all-or-nothing for attempted mappings**: an out-of-range or malformed value
read by a conversion pass discards the whole style. Scalar groups without a
trigger are skipped before their values are checked (`src/common/styles.c:1812`);
for example, `GrainFrequency` without `GrainAmount` is omitted rather than
validated. Process-version differences are ignored beyond preferring the
PV2012 curve keys.

## How a style is built

Each mapped module is loaded fresh (`dt_iop_load_module_by_so`, defaults
`memcpy`'d in, introspection setters applied, then `dt_iop_cleanup_module`), and
its params + `default_blendop_params` are XMP-encoded into the style item. This
matters:

- **The style is current-version by construction.** The stored blob is exactly
  the current struct layout, with `module = module->version()` and
  `blendop_version = DEVELOP_BLEND_VERSION`. This is *because* it uses
  introspection rather than a hardcoded params struct. Do not "optimize" by
  casting to `dt_iop_*_params_t`: that reintroduces version fragility and defeats
  the design.
- Persistence goes through the same `dt_style_save` / `dt_style_plugin_save` path
  as a native style (XMP-encode into a `GString`, then decode back to a blob on
  save), so an imported Lightroom style round-trips through
  `dt_styles_save_to_file` like any other.

## Gotchas for modifying this

- **Introspection field names are load-bearing string literals.** Setters look
  modules and fields up by name (`"exposure"`, `"curve_nodes"`,
  `"curve_autoscale"`, `"compensate_exposure_bias"`, ...). If an IOP renames or
  removes such a field, the setter returns false and the whole import silently
  fails (`goto error`); presets that used to convert start being rejected. This
  is only caught if that module is in the test's `add_dependencies`.
- **`MAX_ANCHORS` is 20**, matching `DT_IOP_RGBCURVE_MAXNODES` and
  `DT_IOP_COLORZONES_MAXNODES`. The setter re-checks the live array count, so it
  fails safe, but changing `MAX_ANCHORS` independently breaks the assumption.
- **The rgbcurve `multi_priority` order is meaningful** and pinned by the tests;
  reordering the passes renumbers existing users' expectations.
- **XMP handling checks namespace URIs, not prefix spelling** (exact `crs`/`rdf`
  hrefs, one top-level Description, no DTD). Alternate prefixes are supported:
  the test fixtures use `c:` and `r:`. Multiple top-level Descriptions are
  rejected; descriptions inside embedded looks are ignored.
- **GUI failure is silent** (`continue`, no dialog); keep that in mind if you
  change error semantics.

## Testing

`src/tests/lightroom_presets.c` (target `darktable-test-lightroom-presets`, a
`ctest`, independent of the cmocka unit tests) boots an in-memory darktable,
builds XMP fixtures, imports them, and reads the resulting `style_items` back
from SQLite. It covers the scalar mappings and their numeric transforms, the
`crs:Name` `rdf:Alt` selection, the omitted-settings list, attribute vs element
forms, the tone-curve instances and priorities, the HSL Lab-hue mapping, a large
battery of rejections (bad numbers, structured-where-scalar, DTD/entities,
`PresetType='Look'`, sidecars, oversized curves or non-increasing input
coordinates), and that importing
a style touches neither `main.images` nor `main.history`. It builds a real
darktable, so it depends on the `exposure grain splittoning bilat vignette
rgbcurve colorzones` module `.so`s.

It does **not** cover the GUI path (file chooser, overwrite dialog), signal
emission, or the actual pixel fidelity of the approximation (no pipeline render).
