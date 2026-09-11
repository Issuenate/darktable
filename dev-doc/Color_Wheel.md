# Color wheel widget (`dtgtk_color_wheel`)

A hue/chroma disc that drives two existing bauhaus sliders: the angle around the
disc is the hue slider, the distance from the center is the chroma slider. It is
a `dtgtk` widget added on this fork (`src/dtgtk/colorwheel.c`, `.h`), and the
first consumer is color balance rgb, whose *4 ways* tab carries one wheel per
tonal range.

The point of the widget is reach without a new parameter path. See
[imageop_gui.md](imageop_gui.md) and [sliders.md](sliders.md) for the bauhaus
sliders it drives, and [Essentials_UI_Architecture.md](Essentials_UI_Architecture.md)
(the color grading tool row) for why the guided interface wanted it.

## The one rule it keeps

**The wheel owns no parameter.** It writes only through
`dt_bauhaus_slider_set()` on the hue and chroma sliders the caller passes in, so
history, undo, XMP and the pixelpipe see an ordinary slider move; there is no
second parameter path and no wheel-specific serialization. It also connects to
the sliders' `value-changed`, so the puck tracks the value however it was changed
(a typed number, a preset, a reset). The numeric sliders remain the precise
input; the wheel is the fast, legible one.

## API

`src/dtgtk/colorwheel.h`:

```c
typedef void (*dtgtk_color_wheel_rgb_fn)(float hue, float radius,
                                         float rgb[3], gpointer user_data);

GtkWidget *dtgtk_color_wheel_new(GtkWidget *hue_slider,
                                 GtkWidget *chroma_slider,
                                 float chroma_range,
                                 dtgtk_color_wheel_rgb_fn rgb_fn,
                                 gpointer user_data);

void dtgtk_color_wheel_invalidate(GtkWidget *wheel);
```

- `hue_slider` / `chroma_slider` are the two bauhaus sliders the wheel reads and
  writes. The wheel does not take ownership; they must outlive it.
- `chroma_range` maps the rim to a slider value: a puck at the rim sets
  `chroma = chroma_range`, the center sets `0`. Pass the chroma slider's **soft**
  maximum, not its hard maximum (see the sizing note below). A non-positive
  value falls back to `1.0`.
- `rgb_fn` paints one point of the disc (below). It runs once per pixel while the
  disc is built, so it must be cheap and must not touch GTK.
- `dtgtk_color_wheel_invalidate()` drops the cached disc and redraws; call it
  when the color behind `rgb_fn` changes (for example the module's output
  profile).

### The paint callback

`rgb_fn(hue, radius, rgb, user_data)` is handed a `hue` in degrees `[0, 360)` and
a `radius` that is the **fraction of the way to the rim** (0 at the center, 1 at
the edge), and fills `rgb[3]` in `[0, 1]`. The radius is deliberately *not* in
the chroma slider's units: the puck must span a range that can be as narrow as
`0.01`, while the disc still has to look like a full color wheel. The caller owns
the mapping from that fraction to a displayable color, because a correct one
depends on the module's output profile.

## How it behaves

| Gesture | Effect |
|---------|--------|
| drag / click | `_set_from_pointer()` maps the pointer to hue (angle) and chroma (distance x `chroma_range`) and writes both sliders |
| double-click | `_reset()` zeros **chroma only** -- with no chroma the hue does not matter, and keeping it lets you come straight back out along the same line |
| scroll | nudges chroma in `chroma_range / 50` steps (the in/out axis a wheel reads less precisely than a slider) |

Geometry conventions (`_draw` / `_build_disc` in `colorwheel.c`): 0 degrees is at
the right and hue turns counter-clockwise, matching the hue slider's gradient
(screen y grows down, so `dy` is negated). The puck sits at
`reach = chroma / chroma_range` of the radius, with a line back to the center so
a small offset is still visible.

Input is wired with the GTK4-ready event-controller helpers
(`dt_gui_connect_click`, `dt_gui_connect_motion`, `dt_gui_connect_scroll`), not
raw `"button-press-event"` signals -- follow that when extending it.

## Painting and caching

The disc is expensive (one `rgb_fn` call per pixel), so `_build_disc()` renders
it to a cached `cairo_surface_t` and keeps it until the widget size changes or
`dtgtk_color_wheel_invalidate()` is called. The rim is feathered by one pixel of
coverage so it is not stair-stepped. The widget is a
`dtgtk_drawing_area_new_with_aspect_ratio(1.0)` named `#color-wheel`.

## The two surprises

Both come from the chroma slider's range being tiny, and both live on the
*consumer* side, not in the widget:

- **Reach is sized by the slider's soft maximum, not its hard one.** Every chroma
  parameter in color balance rgb tops out at `1.0` but is presented over a soft
  range as narrow as `0.01` to `0.5`. Using the hard maximum would squeeze all
  usable grading into a dot, so the consumer passes
  `dt_bauhaus_slider_get_soft_max(chroma)` as `chroma_range`.
- **The disc is painted over a fixed sweep, not the parameter's range.** Painting
  actual chroma over a range of `0.01` renders a near-gray disc, so the paint
  callback uses its own readable sweep. In color balance rgb `_colorwheel_rgb()`
  paints `MIN(radius * 0.25, max_chroma)` rather than the parameter value.

## Consumer: color balance rgb

`src/iop/colorbalancergb.c` gives the *4 ways* tab four wheels
(`global_wheel`, `shadows_wheel`, `highlights_wheel`, `midtones_wheel`), each
driving that range's hue (`*_H`) and chroma (`*_C`) sliders. Both the full and
the Essentials interface get them.

- `_colorwheel_rgb()` converts the disc point to RGB through the module's output
  profile (`g->sliders_output_profile`, `g->wheel_output_matrix`) and clips the
  chroma to `Ych_max_chroma_without_negatives()` so the rim does not band into a
  single clipped color. With no profile yet it paints flat gray.
- When the output profile changes, the module repaints the hue-slider gradients
  and calls `dtgtk_color_wheel_invalidate()` on all four wheels so their cached
  discs are rebuilt for the new profile.
- Each wheel is created with `chroma_range = dt_bauhaus_slider_get_soft_max()` of
  that range's chroma slider, so the four discs share code but map the rim to
  different soft maxima (global `0.01`, midtones `0.1`, highlights `0.2`,
  shadows `0.5`).

A staged refinement on `local/essentials-and-remove` passes the range's
luminance slider (`*_Y`) into `_add_color_wheel()` and reorders the wheel row to
sit directly above that slider, so the disc reads as the head of the range's
controls rather than after them.

## Adding a wheel to another module

1. Build the hue and chroma bauhaus sliders as usual.
2. Write an `rgb_fn` that maps `(hue, radius-fraction)` to a displayable `rgb`,
   using the module's own color knowledge. Keep it cheap and GTK-free.
3. `dtgtk_color_wheel_new(hue, chroma, dt_bauhaus_slider_get_soft_max(chroma),
   rgb_fn, self)` and pack the returned widget.
4. Call `dtgtk_color_wheel_invalidate()` whenever the meaning of the color
   changes (profile, working space).

Because the wheel only moves the two sliders, everything downstream -- history,
undo, presets, XMP, the pixelpipe -- keeps working with no further wiring.
