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

#include "dtgtk/colorwheel.h"

#include "bauhaus/bauhaus.h"
#include "common/darktable.h"
#include "common/math.h"
#include "dtgtk/drawingarea.h"
#include "gui/gtk.h"

#include <math.h>

typedef struct _colorwheel_t
{
  GtkWidget *hue;
  GtkWidget *chroma;
  float chroma_range;
  dtgtk_color_wheel_rgb_fn rgb_fn;
  gpointer user_data;

  /* the disc is expensive to compute per pixel, so keep it until the size or
   * the color function changes */
  cairo_surface_t *disc;
  int disc_size;

  gboolean dragging;
} _colorwheel_t;

static void _free(gpointer data)
{
  _colorwheel_t *w = data;
  if(w->disc) cairo_surface_destroy(w->disc);
  g_free(w);
}

static _colorwheel_t *_get(GtkWidget *widget)
{
  return g_object_get_data(G_OBJECT(widget), "colorwheel");
}

void dtgtk_color_wheel_invalidate(GtkWidget *wheel)
{
  _colorwheel_t *w = _get(wheel);
  if(!w) return;
  if(w->disc)
  {
    cairo_surface_destroy(w->disc);
    w->disc = NULL;
  }
  gtk_widget_queue_draw(wheel);
}

static void _build_disc(_colorwheel_t *w, const int size)
{
  if(w->disc) cairo_surface_destroy(w->disc);
  w->disc = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, size, size);
  w->disc_size = size;

  const int stride = cairo_image_surface_get_stride(w->disc);
  uint8_t *const pixels = cairo_image_surface_get_data(w->disc);
  const float radius = size * 0.5f;

  for(int y = 0; y < size; y++)
  {
    uint32_t *row = (uint32_t *)(pixels + (size_t)y * stride);
    for(int x = 0; x < size; x++)
    {
      const float dx = (x + 0.5f) - radius;
      const float dy = (y + 0.5f) - radius;
      const float distance = hypotf(dx, dy);

      /* one pixel of coverage feathering, so the rim is not stair-stepped */
      const float coverage = CLAMPF(radius - distance, 0.0f, 1.0f);
      if(coverage <= 0.0f)
      {
        row[x] = 0;
        continue;
      }

      /* screen y grows downwards; negate so hue turns counter-clockwise, and
       * put 0 degrees at the right, matching the hue slider's gradient */
      float hue = atan2f(-dy, dx) * (180.0f / M_PI_F);
      if(hue < 0.0f) hue += 360.0f;

      float rgb[3] = { 0.0f, 0.0f, 0.0f };
      w->rgb_fn(hue, distance / radius, rgb, w->user_data);

      const uint8_t a = (uint8_t)(coverage * 255.0f + 0.5f);
      const uint8_t r = (uint8_t)(CLAMPF(rgb[0], 0.0f, 1.0f) * coverage * 255.0f + 0.5f);
      const uint8_t g = (uint8_t)(CLAMPF(rgb[1], 0.0f, 1.0f) * coverage * 255.0f + 0.5f);
      const uint8_t b = (uint8_t)(CLAMPF(rgb[2], 0.0f, 1.0f) * coverage * 255.0f + 0.5f);
      row[x] = ((uint32_t)a << 24) | ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
    }
  }
  cairo_surface_mark_dirty(w->disc);
}

static gboolean _draw(GtkWidget *widget, cairo_t *cr, gpointer user_data)
{
  _colorwheel_t *w = _get(widget);
  GtkAllocation allocation;
  gtk_widget_get_allocation(widget, &allocation);

  const int size = MIN(allocation.width, allocation.height);
  if(size < 8) return TRUE;
  if(!w->disc || w->disc_size != size) _build_disc(w, size);

  const double ox = (allocation.width - size) * 0.5;
  const double oy = (allocation.height - size) * 0.5;
  const double radius = size * 0.5;

  cairo_set_source_surface(cr, w->disc, ox, oy);
  cairo_paint(cr);

  /* rim */
  cairo_set_line_width(cr, DT_PIXEL_APPLY_DPI(1.0));
  cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.22);
  cairo_arc(cr, ox + radius, oy + radius, radius - 0.5, 0.0, 2.0 * M_PI);
  cairo_stroke(cr);

  /* center mark, so "no grading" is readable at a glance */
  cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.35);
  cairo_arc(cr, ox + radius, oy + radius, DT_PIXEL_APPLY_DPI(1.6), 0.0, 2.0 * M_PI);
  cairo_fill(cr);

  const float hue = dt_bauhaus_slider_get(w->hue);
  const float chroma = dt_bauhaus_slider_get(w->chroma);
  const float reach = w->chroma_range > 0.0f
    ? CLAMPF(chroma / w->chroma_range, 0.0f, 1.0f)
    : 0.0f;
  const double angle = hue * (M_PI / 180.0);
  const double px = ox + radius + cos(angle) * reach * radius;
  const double py = oy + radius - sin(angle) * reach * radius;

  /* a line back to the center, so a small offset is still visible */
  cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.45);
  cairo_set_line_width(cr, DT_PIXEL_APPLY_DPI(1.0));
  cairo_move_to(cr, ox + radius, oy + radius);
  cairo_line_to(cr, px, py);
  cairo_stroke(cr);

  const double puck = DT_PIXEL_APPLY_DPI(5.5);
  cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.55);
  cairo_arc(cr, px, py, puck + DT_PIXEL_APPLY_DPI(1.0), 0.0, 2.0 * M_PI);
  cairo_fill(cr);
  cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
  cairo_set_line_width(cr, DT_PIXEL_APPLY_DPI(2.0));
  cairo_arc(cr, px, py, puck, 0.0, 2.0 * M_PI);
  cairo_stroke(cr);

  return TRUE;
}

/* map a pointer position to the two sliders */
static void _set_from_pointer(GtkWidget *widget, const double x, const double y)
{
  _colorwheel_t *w = _get(widget);
  GtkAllocation allocation;
  gtk_widget_get_allocation(widget, &allocation);

  const double size = MIN(allocation.width, allocation.height);
  const double radius = size * 0.5;
  if(radius <= 0.0) return;

  const double dx = x - ((allocation.width - size) * 0.5 + radius);
  const double dy = y - ((allocation.height - size) * 0.5 + radius);

  float hue = atan2(-dy, dx) * (180.0 / M_PI);
  if(hue < 0.0f) hue += 360.0f;
  const float chroma = CLAMPF(hypot(dx, dy) / radius, 0.0f, 1.0f) * w->chroma_range;

  dt_bauhaus_slider_set(w->hue, hue);
  dt_bauhaus_slider_set(w->chroma, chroma);
}

static void _reset(GtkWidget *widget)
{
  _colorwheel_t *w = _get(widget);
  /* only the chroma returns to zero: with no chroma the hue does not matter,
   * and keeping it lets the user come straight back out along the same line */
  dt_bauhaus_slider_set(w->chroma, 0.0f);
}

static void _pressed(GtkGestureSingle *gesture,
                     const int n_press,
                     const double x,
                     const double y,
                     GtkWidget *widget)
{
  _colorwheel_t *w = _get(widget);
  if(gtk_gesture_single_get_current_button(gesture) != GDK_BUTTON_PRIMARY) return;

  if(n_press >= 2)
  {
    _reset(widget);
    return;
  }
  w->dragging = TRUE;
  _set_from_pointer(widget, x, y);
}

static void _released(GtkGestureSingle *gesture,
                      const int n_press,
                      const double x,
                      const double y,
                      GtkWidget *widget)
{
  _get(widget)->dragging = FALSE;
}

static void _motion(GtkEventControllerMotion *controller,
                    const double x,
                    const double y,
                    GtkWidget *widget)
{
  if(_get(widget)->dragging) _set_from_pointer(widget, x, y);
}

/* scrolling over the disc moves outwards and inwards, which is the axis a
 * wheel cannot express as precisely as a slider */
static void _scrolled(GtkEventControllerScroll *controller,
                      const double dx,
                      const double dy,
                      GtkWidget *widget)
{
  _colorwheel_t *w = _get(widget);
  if(dy == 0.0) return;

  const float step = w->chroma_range / 50.0f;
  const float chroma = CLAMPF(dt_bauhaus_slider_get(w->chroma) - dy * step,
                              0.0f, w->chroma_range);
  dt_bauhaus_slider_set(w->chroma, chroma);
}

static void _slider_changed(GtkWidget *slider, GtkWidget *widget)
{
  gtk_widget_queue_draw(widget);
}

GtkWidget *dtgtk_color_wheel_new(GtkWidget *hue_slider,
                                 GtkWidget *chroma_slider,
                                 const float chroma_range,
                                 dtgtk_color_wheel_rgb_fn rgb_fn,
                                 gpointer user_data)
{
  g_return_val_if_fail(hue_slider && chroma_slider && rgb_fn, NULL);

  _colorwheel_t *w = g_malloc0(sizeof(*w));
  w->hue = hue_slider;
  w->chroma = chroma_slider;
  w->chroma_range = chroma_range > 0.0f ? chroma_range : 1.0f;
  w->rgb_fn = rgb_fn;
  w->user_data = user_data;

  GtkWidget *widget = dtgtk_drawing_area_new_with_aspect_ratio(1.0);
  gtk_widget_set_name(widget, "color-wheel");
  g_object_set_data_full(G_OBJECT(widget), "colorwheel", w, _free);

  gtk_widget_add_events(widget, GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK
                                | GDK_POINTER_MOTION_MASK | GDK_SMOOTH_SCROLL_MASK);
  g_signal_connect(widget, "draw", G_CALLBACK(_draw), NULL);
  dt_gui_connect_click(widget, _pressed, _released, widget);
  dt_gui_connect_motion(widget, _motion, NULL, NULL, widget);
  dt_gui_connect_scroll(widget, GTK_EVENT_CONTROLLER_SCROLL_VERTICAL
                                | GTK_EVENT_CONTROLLER_SCROLL_DISCRETE,
                        _scrolled, widget);

  /* follow the sliders, so the puck is right however the value was changed */
  g_signal_connect(hue_slider, "value-changed", G_CALLBACK(_slider_changed), widget);
  g_signal_connect(chroma_slider, "value-changed", G_CALLBACK(_slider_changed), widget);

  return widget;
}

// clang-format off
// modelines: These editor modelines have been set for all relevant files by tools/update_modelines.py
// vim: shiftwidth=2 expandtab tabstop=2 cindent
// kate: tab-indents: off; indent-width 2; replace-tabs on; indent-mode cstyle; remove-trailing-spaces modified;
// clang-format on
