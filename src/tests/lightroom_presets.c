/*
    This file is part of darktable,
    Copyright (C) 2026 darktable developers.

    darktable is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    darktable is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with darktable. If not, see <http://www.gnu.org/licenses/>.
*/

#include "common/darktable.h"
#include "common/database.h"
#include "common/styles.h"
#include "gui/styles.h"

#include <glib/gstdio.h>

#ifdef _WIN32
#include "win/main_wrapper.h"
#endif

static char *_fixture(const char *directory, const char *name,
                      const char *attributes, const char *children)
{
  char *path = g_build_filename(directory, name, NULL);
  char *xml = g_strdup_printf
    ("<x:xmpmeta xmlns:x='adobe:ns:meta/'>"
     "<r:RDF xmlns:r='http://www.w3.org/1999/02/22-rdf-syntax-ns#'>"
     "<r:Description xmlns:c='http://ns.adobe.com/camera-raw-settings/1.0/' %s>"
     "%s</r:Description></r:RDF></x:xmpmeta>", attributes, children);
  g_assert_true(g_file_set_contents(path, xml, -1, NULL));
  g_free(xml);
  return path;
}

static void _check_float(const char *style, const char *operation,
                          const char *field, const float expected)
{
  sqlite3_stmt *stmt = NULL;
  g_assert_cmpint(sqlite3_prepare_v2(dt_database_get(darktable.db),
    "SELECT op_params, module, blendop_version, enabled FROM data.style_items"
    " WHERE styleid = (SELECT id FROM data.styles WHERE name = ?1) AND operation = ?2",
    -1, &stmt, NULL), ==, SQLITE_OK);
  sqlite3_bind_text(stmt, 1, style, -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 2, operation, -1, SQLITE_TRANSIENT);
  g_assert_cmpint(sqlite3_step(stmt), ==, SQLITE_ROW);
  dt_iop_module_so_t *so = dt_iop_get_module_so(operation);
  g_assert_nonnull(so);
  g_assert_cmpint(sqlite3_column_int(stmt, 1), ==, so->version());
  g_assert_cmpint(sqlite3_column_bytes(stmt, 0), ==, so->get_introspection()->size);
  g_assert_cmpint(sqlite3_column_int(stmt, 2), ==, DEVELOP_BLEND_VERSION);
  g_assert_cmpint(sqlite3_column_int(stmt, 3), ==, 1);
  const float *value = so->get_p((void *)sqlite3_column_blob(stmt, 0), field);
  g_assert_nonnull(value);
  g_assert_cmpfloat_with_epsilon(*value, expected, 0.00001f);
  g_assert_cmpint(sqlite3_step(stmt), ==, SQLITE_DONE);
  sqlite3_finalize(stmt);
}

static void _check_curve(const char *style, const char *operation, const int priority,
                         const int channel, const int node, const int count,
                         const float x, const float y)
{
  sqlite3_stmt *stmt = NULL;
  g_assert_cmpint(sqlite3_prepare_v2(dt_database_get(darktable.db),
    "SELECT op_params FROM data.style_items WHERE styleid = (SELECT id FROM data.styles WHERE name = ?1)"
    " AND operation = ?2 AND multi_priority = ?3", -1, &stmt, NULL), ==, SQLITE_OK);
  sqlite3_bind_text(stmt, 1, style, -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 2, operation, -1, SQLITE_TRANSIENT);
  sqlite3_bind_int(stmt, 3, priority);
  g_assert_cmpint(sqlite3_step(stmt), ==, SQLITE_ROW);
  dt_iop_module_so_t *so = dt_iop_get_module_so(operation);
  void *params = (void *)sqlite3_column_blob(stmt, 0);
  const char *name = !strcmp(operation, "rgbcurve") ? "curve_nodes" : "curve";
  dt_introspection_field_t *field = so->get_f(name);
  void *curve = dt_introspection_access_array(field, so->get_p(params, name), channel, &field);
  g_assert_nonnull(curve);
  void *point = dt_introspection_access_array(field, curve, node, &field);
  float *actual_x = dt_introspection_get_child(field, point, "x", NULL);
  float *actual_y = dt_introspection_get_child(field, point, "y", NULL);
  g_assert_nonnull(actual_x);
  g_assert_nonnull(actual_y);
  if(x >= 0) g_assert_cmpfloat_with_epsilon(*actual_x, x, 0.00001f);
  g_assert_cmpfloat_with_epsilon(*actual_y, y, 0.00001f);
  field = so->get_f("curve_num_nodes");
  int *actual_count = dt_introspection_access_array(field, so->get_p(params, "curve_num_nodes"), channel, NULL);
  g_assert_nonnull(actual_count);
  g_assert_cmpint(*actual_count, ==, count);
  g_assert_cmpint(sqlite3_step(stmt), ==, SQLITE_DONE);
  sqlite3_finalize(stmt);
}

int main(int argc, char *argv[])
{
  if(argc != 3)
  {
    g_printerr("usage: %s <datadir> <moduledir>\n", argv[0]);
    return 1;
  }
  char *directory = g_dir_make_tmp("darktable-lightroom-presets-XXXXXX", NULL);
  g_assert_nonnull(directory);
  char *args[] = { argv[0], "--library", ":memory:", "--configdir", directory,
    "--cachedir", directory, "--datadir", argv[1], "--moduledir", argv[2],
    "--disable-opencl", "--conf", "write_sidecar_files=never", NULL };
  g_assert_cmpint(dt_init(G_N_ELEMENTS(args) - 1, args, FALSE, TRUE, NULL), ==, 0);

  char *path = _fixture(directory, "combined.xmp",
    "c:PresetType='Normal' c:Exposure2012='1.25' c:GrainAmount='50' c:GrainFrequency='75' "
    "c:SplitToningShadowHue='240' c:SplitToningShadowSaturation='30' "
    "c:SplitToningHighlightHue='60' c:SplitToningHighlightSaturation='10' c:SplitToningBalance='-20' "
    "c:Clarity2012='20' c:PostCropVignetteAmount='-40' c:PostCropVignetteMidpoint='25' "
    "c:PostCropVignetteFeather='60' c:Temperature='6500' c:Contrast2012='20'",
    "<c:Name><r:Alt><r:li xml:lang='de'>Deutsch</r:li>"
    "<r:li xml:lang='x-default'>test &amp; preset</r:li></r:Alt></c:Name>"
    "<c:Look><r:Description c:Name='nested' c:Exposure2012='5'/></c:Look>");
  char *name = dt_get_style_name(path);
  g_assert_cmpstr(name, ==, "test & preset");
  g_free(name);
  dt_styles_import_from_file(path);
  _check_float("test & preset", "exposure", "exposure", 1.25);
  _check_float("test & preset", "grain", "strength", 40);
  _check_float("test & preset", "grain", "scale", 400.0 / 53.3);
  _check_float("test & preset", "splittoning", "shadow_hue", 240.0 / 360);
  _check_float("test & preset", "splittoning", "highlight_saturation", 0.1);
  _check_float("test & preset", "splittoning", "balance", 0.4);
  _check_float("test & preset", "bilat", "detail", 0.13);
  _check_float("test & preset", "vignette", "brightness", -0.4);
  _check_float("test & preset", "vignette", "scale", 85);
  char *description = dt_styles_get_description("test & preset");
  g_assert_nonnull(strstr(description, "Contrast2012"));
  g_assert_nonnull(strstr(description, "Temperature"));
  g_assert_nonnull(strstr(description, "Look"));
  g_assert_null(strstr(description, "Exposure2012"));
  g_free(description);
  dt_styles_import_from_file(path);
  _check_float("test & preset", "exposure", "exposure", 1.25);
  g_remove(path);
  g_free(path);

  path = _fixture(directory, "zero.XMP", "",
    "<c:PresetType>Normal</c:PresetType><c:Exposure2012>0</c:Exposure2012>");
  name = dt_get_style_name(path);
  g_assert_cmpstr(name, ==, "zero");
  g_free(name);
  dt_styles_import_from_file(path);
  _check_float("zero", "exposure", "exposure", 0);
  g_remove(path);
  g_free(path);

  const char *invalid[] = { "nan", "inf", "1e999", "6", "-6", "1.2junk", "1,5", "", "  " };
  for(size_t i = 0; i < G_N_ELEMENTS(invalid); i++)
  {
    char *attributes = g_strdup_printf
      ("c:PresetType='Normal' c:Name='test &amp; preset' c:Exposure2012='%s'", invalid[i]);
    path = _fixture(directory, "invalid.xmp", attributes, "");
    g_assert_null(dt_get_style_name(path));
    dt_styles_import_from_file(path);
    _check_float("test & preset", "exposure", "exposure", 1.25);
    g_free(attributes);
    g_remove(path);
    g_free(path);
  }

  const char *structured_scalars[] = { "Exposure2012", "HueAdjustmentRed", "ParametricShadows" };
  for(size_t i = 0; i < G_N_ELEMENTS(structured_scalars); i++)
  {
    char *children = g_strdup_printf("<c:%s><r:Seq><r:li>1</r:li></r:Seq></c:%s>",
                                     structured_scalars[i], structured_scalars[i]);
    path = _fixture(directory, "structured.xmp", "c:PresetType='Normal' c:Name='test &amp; preset'",
                    children);
    g_assert_null(dt_get_style_name(path));
    dt_styles_import_from_file(path);
    _check_float("test & preset", "exposure", "exposure", 1.25);
    g_free(children);
    g_remove(path);
    g_free(path);
  }
  path = _fixture(directory, "split-defaults.xmp",
                  "c:PresetType='Normal' c:SplitToningHighlightSaturation='20'", "");
  dt_styles_import_from_file(path);
  _check_float("split-defaults", "splittoning", "highlight_hue", 0);
  _check_float("split-defaults", "splittoning", "highlight_saturation", 0.2);
  _check_float("split-defaults", "splittoning", "shadow_saturation", 0);
  g_remove(path);
  g_free(path);

  const char *rejected[] = {
    "c:PresetType='Normal' c:Name='unsupported' c:Dehaze='20'",
    "c:PresetType='Look' c:Name='profile' c:Exposure2012='1'",
    "c:Name='sidecar' c:Exposure2012='1'"
  };
  for(size_t i = 0; i < G_N_ELEMENTS(rejected); i++)
  {
    path = _fixture(directory, "rejected.xmp", rejected[i], "");
    g_assert_null(dt_get_style_name(path));
    dt_styles_import_from_file(path);
    g_remove(path);
    g_free(path);
  }
  g_assert_false(dt_styles_exists("unsupported"));
  g_assert_false(dt_styles_exists("profile"));
  g_assert_false(dt_styles_exists("sidecar"));

  path = _fixture(directory, "duplicate.xmp", "c:PresetType='Normal' c:Exposure2012='1'",
                  "<c:Exposure2012>2</c:Exposure2012>");
  g_assert_null(dt_get_style_name(path));
  g_remove(path);
  g_assert_true(g_file_set_contents(path, "<!DOCTYPE x [<!ENTITY value '1'>]><x/>", -1, NULL));
  g_assert_null(dt_get_style_name(path));
  g_assert_true(g_file_set_contents(path, "<broken", -1, NULL));
  g_assert_null(dt_get_style_name(path));
  g_remove(path);
  g_free(path);

  dt_styles_save_to_file("zero", directory, TRUE);
  dt_styles_delete_by_name("zero");
  path = g_build_filename(directory, "zero.dtstyle", NULL);
  name = dt_get_style_name(path);
  g_assert_cmpstr(name, ==, "zero");
  g_free(name);
  dt_styles_import_from_file(path);
  _check_float("zero", "exposure", "exposure", 0);
  g_remove(path);
  g_free(path);

  path = _fixture(directory, "curves.xmp", "c:PresetType='Normal' c:ToneCurveName2012='Custom'",
    "<c:ToneCurvePV2012><r:Seq><r:li>0, 0</r:li><r:li>128, 160</r:li><r:li>255, 255</r:li>"
    "</r:Seq></c:ToneCurvePV2012>"
    "<c:ToneCurvePV2012Red><r:Seq><r:li>0, 10</r:li><r:li>255, 240</r:li></r:Seq></c:ToneCurvePV2012Red>"
    "<c:ToneCurvePV2012Blue><r:Seq><r:li>0, 0</r:li><r:li>128, 100</r:li><r:li>255, 255</r:li>"
    "</r:Seq></c:ToneCurvePV2012Blue>");
  dt_styles_import_from_file(path);
  for(int ch = 0; ch < 3; ch++)
    _check_curve("curves", "rgbcurve", 0, ch, 1, 3, 128.0 / 255, 160.0 / 255);
  _check_curve("curves", "rgbcurve", 1, 0, 0, 2, 0, 10.0 / 255);
  _check_curve("curves", "rgbcurve", 1, 1, 1, 2, 1, 1);
  _check_curve("curves", "rgbcurve", 1, 2, 1, 3, 128.0 / 255, 100.0 / 255);
  description = dt_styles_get_description("curves");
  g_assert_null(strstr(description, "ToneCurve"));
  g_free(description);
  g_remove(path);
  g_free(path);

  path = _fixture(directory, "legacy.xmp", "c:PresetType='Normal'",
    "<c:ToneCurveGreen><r:Seq><r:li>0, 0</r:li><r:li>255, 220</r:li></r:Seq></c:ToneCurveGreen>");
  dt_styles_import_from_file(path);
  _check_curve("legacy", "rgbcurve", 0, 0, 1, 2, 1, 1);
  _check_curve("legacy", "rgbcurve", 0, 1, 1, 2, 1, 220.0 / 255);
  g_remove(path);
  g_free(path);

  path = _fixture(directory, "parametric.xmp", "c:PresetType='Normal' c:ParametricShadows='40'",
    "<c:ToneCurvePV2012><r:Seq><r:li>0,0</r:li><r:li>255,255</r:li></r:Seq></c:ToneCurvePV2012>"
    "<c:ToneCurvePV2012Red><r:Seq><r:li>0,5</r:li><r:li>255,255</r:li></r:Seq></c:ToneCurvePV2012Red>");
  dt_styles_import_from_file(path);
  _check_curve("parametric", "rgbcurve", 0, 0, 1, 6, 0.125, 0.175);
  _check_curve("parametric", "rgbcurve", 0, 1, 2, 6, 0.375, 0.375);
  _check_curve("parametric", "rgbcurve", 1, 0, 1, 2, 1, 1);
  _check_curve("parametric", "rgbcurve", 2, 0, 0, 2, 0, 5.0 / 255);
  description = dt_styles_get_description("parametric");
  g_assert_null(strstr(description, "Parametric"));
  g_free(description);
  g_remove(path);
  g_free(path);
  path = _fixture(directory, "bad-splits.xmp", "c:PresetType='Normal' c:ParametricShadows='40' "
    "c:ParametricShadowSplit='50' c:ParametricMidtoneSplit='50'", "");
  g_assert_null(dt_get_style_name(path));
  g_remove(path);
  g_free(path);

  dt_styles_save_to_file("curves", directory, TRUE);
  dt_styles_delete_by_name("curves");
  path = g_build_filename(directory, "curves.dtstyle", NULL);
  dt_styles_import_from_file(path);
  _check_curve("curves", "rgbcurve", 0, 2, 1, 3, 128.0 / 255, 160.0 / 255);
  _check_curve("curves", "rgbcurve", 1, 2, 1, 3, 128.0 / 255, 100.0 / 255);
  g_remove(path);
  g_free(path);

  const char *bad_curves[] = {
    "<r:Seq/>", "<r:Seq><r:li>0,0</r:li></r:Seq>",
    "<r:Seq><r:li>0,0</r:li><r:li>0,20</r:li><r:li>255,255</r:li></r:Seq>",
    "<r:Seq><r:li>0,0</r:li><r:li>200,100</r:li><r:li>100,200</r:li><r:li>255,255</r:li></r:Seq>",
    "<r:Seq><r:li>0,nan</r:li><r:li>255,255</r:li></r:Seq>",
    "<r:Seq><r:li>0,0,0</r:li><r:li>255,255</r:li></r:Seq>",
    "<r:Seq><r:li>0,0</r:li><r:li>255,256</r:li></r:Seq>",
    "<r:Seq><r:li>1,0</r:li><r:li>255,255</r:li></r:Seq>",
    "<r:Bag><r:li>0,0</r:li><r:li>255,255</r:li></r:Bag>"
  };
  for(size_t i = 0; i < G_N_ELEMENTS(bad_curves); i++)
  {
    char *children = g_strdup_printf("<c:ToneCurvePV2012>%s</c:ToneCurvePV2012>", bad_curves[i]);
    path = _fixture(directory, "badcurve.xmp", "c:PresetType='Normal' c:Name='curves'", children);
    g_assert_null(dt_get_style_name(path));
    dt_styles_import_from_file(path);
    _check_curve("curves", "rgbcurve", 0, 0, 1, 3, 128.0 / 255, 160.0 / 255);
    g_free(children);
    g_remove(path);
    g_free(path);
  }
  GString *nodes = g_string_new("<c:ToneCurvePV2012><r:Seq>");
  for(int i = 0; i < 21; i++) g_string_append_printf(nodes, "<r:li>%d,%d</r:li>", i * 12, i * 12);
  g_string_append(nodes, "</r:Seq></c:ToneCurvePV2012>");
  path = _fixture(directory, "oversized.xmp", "c:PresetType='Normal'", nodes->str);
  g_assert_null(dt_get_style_name(path));
  g_string_free(nodes, TRUE);
  g_remove(path);
  g_free(path);

  path = _fixture(directory, "hsl.xmp", "c:PresetType='Normal' c:HueAdjustmentRed='60' "
    "c:SaturationAdjustmentBlue='-100' c:LuminanceAdjustmentGreen='90'", "");
  dt_styles_import_from_file(path);
  _check_curve("hsl", "colorzones", 0, 2, 0, 8, -1, 0.6);
  _check_curve("hsl", "colorzones", 0, 1, 5, 8, -1, 0);
  _check_curve("hsl", "colorzones", 0, 0, 3, 8, -1, 0.7);
  _check_curve("hsl", "colorzones", 0, 1, 0, 8, -1, 0.5);
  _check_curve("hsl", "colorzones", 0, 2, 7, 8, -1, 0.5);
  g_remove(path);
  g_free(path);

  const char *colors[] = { "Red", "Orange", "Yellow", "Green", "Aqua", "Blue", "Purple", "Magenta" };
  const char *adjustments[] = { "LuminanceAdjustment", "SaturationAdjustment", "HueAdjustment" };
  GString *attributes = g_string_new("c:PresetType='Normal'");
  for(int ch = 0; ch < 3; ch++)
    for(int i = 0; i < 8; i++)
      g_string_append_printf(attributes, " c:%s%s='0'", adjustments[ch], colors[i]);
  path = _fixture(directory, "hsl-neutral.xmp", attributes->str, "");
  dt_styles_import_from_file(path);
  for(int ch = 0; ch < 3; ch++)
    for(int i = 0; i < 8; i++)
      _check_curve("hsl-neutral", "colorzones", 0, ch, i, 8, -1, 0.5);
  description = dt_styles_get_description("hsl-neutral");
  g_assert_null(strstr(description, "Adjustment"));
  g_free(description);
  g_string_free(attributes, TRUE);
  g_remove(path);
  g_free(path);
  path = _fixture(directory, "hsl-invalid.xmp", "c:PresetType='Normal' c:HueAdjustmentRed='101'", "");
  g_assert_null(dt_get_style_name(path));
  g_remove(path);
  g_free(path);

  sqlite3_stmt *stmt = NULL;
  g_assert_cmpint(sqlite3_prepare_v2(dt_database_get(darktable.db),
    "SELECT (SELECT COUNT(*) FROM main.images) + (SELECT COUNT(*) FROM main.history)",
    -1, &stmt, NULL), ==, SQLITE_OK);
  g_assert_cmpint(sqlite3_step(stmt), ==, SQLITE_ROW);
  g_assert_cmpint(sqlite3_column_int(stmt, 0), ==, 0);
  sqlite3_finalize(stmt);

  GString *gradient = g_string_new("P6\n128 128\n255\n");
  for(int y = 0; y < 128; y++)
    for(int x = 0; x < 128; x++)
    {
      const unsigned char pixel[] = { x * 2, y * 2, x + y };
      g_string_append_len(gradient, (const char *)pixel, sizeof(pixel));
    }
  path = g_build_filename(directory, "gradient.ppm", NULL);
  g_assert_true(g_file_set_contents(path, gradient->str, gradient->len, NULL));
  g_string_free(gradient, TRUE);
  g_free(path);

  dt_cleanup();
  g_print("Lightroom preset import tests passed; isolated test directory: %s\n", directory);
  g_free(directory);
  return 0;
}
// clang-format off
// modelines: These editor modelines have been set for all relevant files by tools/update_modelines.py
// vim: shiftwidth=2 expandtab tabstop=2 cindent
// kate: tab-indents: off; indent-width 2; replace-tabs on; indent-mode cstyle; remove-trailing-spaces modified;
// clang-format on
