/*
    This file is part of darktable,
    Copyright (C) 2026 darktable developers.

    darktable is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    darktable is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with darktable.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include <gtk/gtk.h>

G_BEGIN_DECLS

/*
 * A hue/chroma disc that drives two existing bauhaus sliders: the angle around
 * the disc is the hue slider, the distance from the center is the chroma one.
 * The wheel owns no parameter of its own and writes only through
 * dt_bauhaus_slider_set(), so history, undo, XMP and the pixelpipe see an
 * ordinary slider move. It follows the sliders back, so the two stay in step
 * however the value was changed.
 *
 * The caller supplies the color used to paint the disc, because a correct one
 * depends on the module's output profile; see _colorwheel_rgb() in
 * iop/colorbalancergb.c.
 */

/*
 * Paints one point of the disc. The hue is in degrees [0,360) and the radius is
 * the fraction of the way to the rim, 0 at the center and 1 at the edge. The
 * radius is deliberately not in the chroma slider's units: the puck has to span
 * the slider's usable range, which can be as narrow as 0.01, while the disc
 * still has to look like a color wheel.
 */
typedef void (*dtgtk_color_wheel_rgb_fn)(const float hue,
                                         const float radius,
                                         float rgb[3],
                                         gpointer user_data);

GtkWidget *dtgtk_color_wheel_new(GtkWidget *hue_slider,
                                 GtkWidget *chroma_slider,
                                 const float chroma_range,
                                 dtgtk_color_wheel_rgb_fn rgb_fn,
                                 gpointer user_data);

/* discard the cached disc, for when the profile behind rgb_fn has changed */
void dtgtk_color_wheel_invalidate(GtkWidget *wheel);

G_END_DECLS

// clang-format off
// modelines: These editor modelines have been set for all relevant files by tools/update_modelines.py
// vim: shiftwidth=2 expandtab tabstop=2 cindent
// kate: tab-indents: off; indent-width 2; replace-tabs on; indent-mode cstyle; remove-trailing-spaces modified;
// clang-format on
