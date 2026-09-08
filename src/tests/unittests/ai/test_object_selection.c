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

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>

#define dt_gui_cursor_set_busy _test_busy
#define dt_seg_is_encoded _test_encoded
#define dt_seg_get_encoded_rgb _test_rgb
#define dt_seg_compute_mask _test_decode
#define dt_seg_reset_prev_mask _test_reset
#define dt_seg_supports_box _test_supports_box
#include "develop/masks/object.c"
#include <cmocka.h>

#ifdef _WIN32
#include "win/main_wrapper.h"
#endif

static uint8_t _rgb[100 * 100 * 3];
static float _mask[100 * 100];
static int _decodes, _resets;
static gboolean _decode_error, _event_dispatched;
static int _expected_points = 1;

// preserve the production busy helper's nested event dispatch without a display
void _test_busy(void)
{
  dt_gui_process_events();
}

static gboolean _pending_tool_change(gpointer data)
{
  _event_dispatched = TRUE;
  return G_SOURCE_REMOVE;
}

gboolean _test_encoded(dt_seg_context_t *ctx)
{
  return TRUE;
}

const uint8_t *_test_rgb(const dt_seg_context_t *ctx, int *w, int *h)
{
  if(w) *w = 100;
  if(h) *h = 100;
  return _rgb;
}

void _test_reset(dt_seg_context_t *ctx)
{
  _resets++;
}

// the tests hand _run_decoder a stand-in context, so the real accessor would
// read a dt_seg_model_type_t out of an unrelated struct
gboolean _test_supports_box(dt_seg_context_t *ctx)
{
  return FALSE;
}

float *_test_decode(dt_seg_context_t *ctx, const dt_seg_point_t *points,
                    const int n, int *w, int *h)
{
  assert_false(_event_dispatched);
  assert_int_equal(n, _expected_points);
  assert_int_equal(points[0].label, 1);
  assert_true(points[0].x >= 0 && points[0].x < 100);
  assert_true(points[0].y >= 0 && points[0].y < 100);
  // every candidate must start without another candidate's refinement state
  if(_expected_points == 1)
    assert_int_equal(_resets, _decodes + 1);
  _decodes++;
  if(_decode_error) return NULL;
  *w = *h = 100;
  float *mask = g_malloc(sizeof(_mask));
  memcpy(mask, _mask, sizeof(_mask));
  return mask;
}

static void _rectangle(int x0, int y0, int x1, int y1)
{
  memset(_mask, 0, sizeof(_mask));
  for(int y = y0; y < y1; y++)
    for(int x = x0; x < x1; x++)
      _mask[y * 100 + x] = 1.0f;
}

static void _test_subject_ranking(void **state)
{
  memset(_rgb, 120, sizeof(_rgb));
  _rectangle(25, 25, 75, 75);
  const float center = _automatic_score(_mask, _rgb, 100, 100, FALSE);
  assert_true(center > 0);
  _rectangle(0, 0, 50, 50);
  assert_true(center > _automatic_score(_mask, _rgb, 100, 100, FALSE));
  _rectangle(0, 0, 100, 100);
  assert_float_equal(_automatic_score(_mask, _rgb, 100, 100, FALSE), 0, 0);
  _rectangle(0, 0, 0, 0);
  assert_float_equal(_automatic_score(_mask, _rgb, 100, 100, FALSE), 0, 0);
  _mask[0] = NAN;
  assert_float_equal(_automatic_score(_mask, _rgb, 100, 100, FALSE), 0, 0);
  assert_float_equal(_automatic_score(NULL, _rgb, 100, 100, FALSE), 0, 0);
}

static void _test_sky_rejection(void **state)
{
  for(int i = 0; i < 10000; i++)
  {
    _rgb[3*i] = 90; _rgb[3*i+1] = 150; _rgb[3*i+2] = 220;
  }
  _rectangle(0, 0, 100, 35);
  assert_true(_automatic_score(_mask, _rgb, 100, 100, TRUE) > 0);
  // blue water at the bottom must not be selected as sky
  _rectangle(0, 65, 100, 100);
  assert_float_equal(_automatic_score(_mask, _rgb, 100, 100, TRUE), 0, 0);
  _rectangle(0, 0, 100, 35);
  for(int i = 0; i < 10000; i++)
  {
    _rgb[3*i] = 40; _rgb[3*i+1] = 110; _rgb[3*i+2] = 30;
  }
  assert_float_equal(_automatic_score(_mask, _rgb, 100, 100, TRUE), 0, 0);
  memset(_rgb, 180, sizeof(_rgb));
  assert_true(_automatic_score(_mask, _rgb, 100, 100, TRUE) > 0);
  // a pale textured facade is not sky, even when it reaches the upper edge
  for(int y = 0; y < 100; y++)
    for(int x = 0; x < 100; x++)
      memset(_rgb + 3*(y*100+x), (x/2 + y/2) % 2 ? 120 : 240, 3);
  assert_float_equal(_automatic_score(_mask, _rgb, 100, 100, TRUE), 0, 0);
}

static void _test_prompt_lifecycle(void **state)
{
  _object_data_t d = { .automatic = DT_MASKS_OBJECT_SUBJECT };
  memset(_rgb, 150, sizeof(_rgb));
  _rectangle(25, 25, 75, 75);
  _decodes = _resets = 0;
  _automatic_prompt(&d);
  assert_true(d.automatic_found);
  assert_int_equal(_decodes, 25);
  assert_int_equal(_resets, 26);
  const dt_seg_point_t subject = d.automatic_point;
  d.automatic = DT_MASKS_OBJECT_BACKGROUND;
  _decodes = _resets = 0;
  _automatic_prompt(&d);
  assert_true(d.automatic_found);
  assert_float_equal(subject.x, d.automatic_point.x, 0);
  assert_float_equal(subject.y, d.automatic_point.y, 0);
  d.canceled = TRUE;
  _decodes = _resets = 0;
  _automatic_prompt(&d);
  assert_false(d.automatic_found);
  assert_int_equal(_decodes, 0);
  d.canceled = FALSE;
  _decode_error = TRUE;
  _automatic_prompt(&d);
  assert_false(d.automatic_found);
  _decode_error = FALSE;
}

static void _test_decoder_inversion_and_sky(void **state)
{
  dt_conf_t conf = {0};
  conf.table = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
  conf.override_entries = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
  conf.x_confgen = g_hash_table_new(g_str_hash, g_str_equal);
  dt_pthread_mutex_init(&conf.mutex, NULL);
  darktable.conf = &conf;
  dt_conf_set_int(CONF_OBJECT_REFINE_PASSES_KEY, 1);
  dt_conf_set_float(CONF_OBJECT_THRESHOLD_KEY, 0.5f);
  dt_dev_pixelpipe_t preview = { .processed_width = 100, .processed_height = 100,
                                 .iwidth = 100, .iheight = 100, .iscale = 1.0f };
  dt_develop_t dev = { .preview_pipe = &preview };
  darktable.develop = &dev;
  _object_data_t d = { .encode_w = 100, .encode_h = 100 };
  d.seg = (dt_seg_context_t *)&d;
  dt_masks_form_gui_t gui = { .scratchpad = &d, .guipoints_count = 1 };
  gui.guipoints = dt_masks_dynbuf_init(20, "test points");
  gui.guipoints_payload = dt_masks_dynbuf_init(10, "test labels");
  dt_masks_dynbuf_add_2(gui.guipoints, 50, 50);
  dt_masks_dynbuf_add(gui.guipoints_payload, 1);
  _rectangle(25, 25, 75, 75);
  // another region separated from the seed by background
  _mask[0] = _mask[1] = _mask[100] = _mask[101] = 1.0f;
  _decodes = _resets = 0;
  const guint pending = g_idle_add(_pending_tool_change, NULL);
  _run_decoder(&gui);
  assert_false(_event_dispatched);
  g_source_remove(pending);
  assert_non_null(d.mask);
  assert_float_equal(d.mask[0], 0, 0);
  float subject[10000];
  memcpy(subject, d.mask, sizeof(subject));
  d.invert_selection = TRUE;
  _run_decoder(&gui);
  for(int i = 0; i < 10000; i++)
    assert_float_equal(subject[i] + d.mask[i], 1.0f, 1e-6f);
  d.invert_selection = FALSE;
  d.automatic = DT_MASKS_OBJECT_SKY;
  _run_decoder(&gui);
  assert_float_equal(d.mask[0], 1, 0);
  assert_float_equal(d.mask[50 * 100 + 50], 1, 0);
  // shift-click outside the subject adds a disconnected exclusion to the
  // inverted selection; both exclusions must survive the decoder postprocessing
  d.automatic = DT_MASKS_OBJECT_BACKGROUND;
  d.invert_selection = TRUE;
  d.has_selection = TRUE;
  dt_masks_dynbuf_add_2(gui.guipoints, 0, 0);
  dt_masks_dynbuf_add(gui.guipoints_payload, 1);
  gui.guipoints_count = _expected_points = 2;
  _run_decoder(&gui);
  assert_float_equal(d.mask[0], 0, 0);
  assert_float_equal(d.mask[50 * 100 + 50], 0, 0);
  assert_float_equal(d.mask[99 * 100 + 99], 1, 0);
  _expected_points = 1;
  g_free(d.mask);
  dt_masks_dynbuf_free(gui.guipoints);
  dt_masks_dynbuf_free(gui.guipoints_payload);
  g_hash_table_destroy(conf.table);
  g_hash_table_destroy(conf.override_entries);
  g_hash_table_destroy(conf.x_confgen);
  dt_pthread_mutex_destroy(&conf.mutex);
  darktable.conf = NULL;
  darktable.develop = NULL;
}

int main(void)
{
  const struct CMUnitTest tests[] = {
    cmocka_unit_test(_test_subject_ranking),
    cmocka_unit_test(_test_sky_rejection),
    cmocka_unit_test(_test_prompt_lifecycle),
    cmocka_unit_test(_test_decoder_inversion_and_sky),
  };
  return cmocka_run_group_tests(tests, NULL, NULL);
}
// clang-format off
// modelines: These editor modelines have been set for all relevant files by tools/update_modelines.py
// vim: shiftwidth=2 expandtab tabstop=2 cindent
// kate: tab-indents: off; indent-width 2; replace-tabs on; indent-mode cstyle; remove-trailing-spaces modified;
// clang-format on
