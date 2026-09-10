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

#include "iop/retouch.c"
#include <glib/gstdio.h>

#ifdef _WIN32
#include "win/main_wrapper.h"
#endif

static void _check_legacy(void)
{
  dt_iop_retouch_params_t old = { 0 };
  old.rt_forms[0].formid = 7;
  old.rt_forms[0].algorithm = DT_IOP_RETOUCH_HEAL;
  old.rt_forms[0].distort_mode = 2;
  old.max_heal_iter = 4321;
  old.fill_color[1] = 0.37f;
  void *converted = NULL;
  int32_t bytes = 0;
  int version = 0;
  g_assert_cmpint(legacy_params(NULL, &old, 3, &converted, &bytes, &version), ==, 0);
  g_assert_cmpint(version, ==, 4);
  g_assert_cmpint(bytes, ==, sizeof(old));
  dt_iop_retouch_params_t *p = converted;
  g_assert_cmpmem(p, offsetof(dt_iop_retouch_params_t, inpaint_model),
                  &old, offsetof(dt_iop_retouch_params_t, inpaint_model));
  g_assert_cmpstr(p->inpaint_model, ==, "");
  free(p);
}

static void _check_roi_and_history(void)
{
  dt_iop_retouch_params_t p = { 0 };
  p.rt_forms[0].formid = 7;
  p.rt_forms[0].algorithm = DT_IOP_RETOUCH_REMOVE;
  g_strlcpy(p.inpaint_model, "saved-model", sizeof(p.inpaint_model));
  dt_dev_pixelpipe_t pipe = { 0 };
  dt_dev_pixelpipe_iop_t piece = { .pipe = &pipe, .buf_in = { .width = 1024, .height = 768 } };
  init_pipe(NULL, &pipe, &piece);
  commit_params(NULL, (dt_iop_params_t *)&p, &pipe, &piece);
  g_assert_false(piece.process_cl_ready);
  const dt_iop_retouch_data_t *d = piece.data;
  g_assert_cmpstr(d->params.inpaint_model, ==, "saved-model");
  dt_iop_roi_t out = { .x = 200, .y = 100, .width = 128, .height = 128, .scale = 0.5f }, in;
  modify_roi_in(NULL, &piece, &out, &in);
  g_assert_cmpint(in.x, ==, 0);
  g_assert_cmpint(in.y, ==, 0);
  g_assert_cmpint(in.width, ==, 512);
  g_assert_cmpint(in.height, ==, 384);
  rt_paste_forms_from_scale(&p, 0, 3);
  g_assert_cmpint(p.rt_forms[0].scale, ==, 0);
  p.rt_forms[0].algorithm = DT_IOP_RETOUCH_FILL;
  commit_params(NULL, (dt_iop_params_t *)&p, &pipe, &piece);
  g_assert_true(piece.process_cl_ready);
  cleanup_pipe(NULL, &pipe, &piece);
}

#ifdef HAVE_AI
static void _check_model(const char *package, const char *directory)
{
  char *error = dt_ai_models_install_local(package);
  if(error) g_error("model install: %s", error);
  dt_iop_retouch_params_t p = { 0 };
  dt_ai_models_set_active_for_task("inpaint", "unavailable-model");
  g_assert_false(_remove_model_ready(&p));
  g_assert_cmpstr(p.inpaint_model, ==, "");
  dt_ai_models_set_active_for_task("inpaint", "inpaint-lama-carve-512-v1");
  g_assert_true(_remove_model_ready(&p));
  g_assert_cmpstr(p.inpaint_model, ==, "inpaint-lama-carve-512-v1");
  dt_ai_models_set_active_for_task("inpaint", NULL);
  g_assert_true(_remove_model_ready(&p));
  g_assert_cmpstr(p.inpaint_model, ==, "inpaint-lama-carve-512-v1");

  dt_develop_t dev;
  dt_dev_init(&dev, FALSE);
  dev.iop_order_list = dt_ioppr_get_iop_order_list_version(DT_IOP_ORDER_V30);
  dt_ioppr_resync_modules_order(&dev);
  dt_iop_module_t self = { .dev = &dev };
  self.iop_order = dt_ioppr_get_iop_order(dev.iop_order_list, "retouch", 0);
  dt_dev_pixelpipe_t pipe = { 0 };
  pipe.work_profile_info = dt_ioppr_add_profile_info_to_list(&dev, DT_COLORSPACE_LIN_REC2020,
                                                           "", INTENT_PERCEPTUAL);
  dt_atomic_set_int(&pipe.shutdown, DT_DEV_PIXELPIPE_PROCESSING);
  pipe.input_profile_info = pipe.work_profile_info;
  g_assert_nonnull(pipe.work_profile_info);
  g_assert_nonnull(dt_ioppr_get_pipe_current_profile_info(&self, &pipe));
  g_assert_nonnull(dt_ioppr_add_profile_info_to_list(&dev, DT_COLORSPACE_SRGB, "", INTENT_PERCEPTUAL));
  dt_dev_pixelpipe_iop_t piece = { .pipe = &pipe, .colors = 4 };
  init_pipe(&self, &pipe, &piece);
  commit_params(&self, (dt_iop_params_t *)&p, &pipe, &piece);

  const int size = 256;
  const size_t count = 4 * (size_t)size * size;
  float *original = dt_alloc_align_float(count);
  float *full = dt_alloc_align_float(count);
  float *half = dt_alloc_align_float(count);
  float mask[64 * 64];
  for(int y = 0; y < size; y++)
    for(int x = 0; x < size; x++)
    {
      const size_t i = 4 * ((size_t)y * size + x);
      original[i] = 0.12f + 0.06f * ((x / 16 + y / 16) % 2);
      original[i + 1] = 0.24f;
      original[i + 2] = 0.08f;
      original[i + 3] = (float)x / size;
      if(x >= 108 && x < 148 && y >= 108 && y < 148)
      {
        original[i] = 0.8f;
        original[i + 1] = 0.01f;
        original[i + 2] = 0.6f;
      }
    }
  for(int y = 0; y < 64; y++)
    for(int x = 0; x < 64; x++)
      mask[y * 64 + x] = CLAMP(MIN(MIN(x, 63 - x), MIN(y, 63 - y)) / 8.0f, 0.0f, 1.0f);
  memcpy(full, original, count * sizeof(float));
  memcpy(half, original, count * sizeof(float));
  dt_iop_roi_t roi = { .width = size, .height = size, .scale = 1.0f };
  dt_iop_roi_t mr = { .x = 96, .y = 96, .width = 64, .height = 64, .scale = 1.0f };
  g_assert_true(_retouch_remove(&self, &piece, full, &roi, mask, &mr, 0.0f));
  g_assert_cmpmem(full, count * sizeof(float), original, count * sizeof(float));
  g_assert_null(((dt_iop_retouch_data_t *)piece.data)->inpaint);
  g_assert_true(_retouch_remove(&self, &piece, full, &roi, mask, &mr, 1.0f));
  g_assert_true(_retouch_remove(&self, &piece, half, &roi, mask, &mr, 0.5f));
  double difference = 0.0;
  for(int y = 0; y < size; y++)
    for(int x = 0; x < size; x++)
    {
      const size_t i = 4 * ((size_t)y * size + x);
      const gboolean outside = x <= 96 || x >= 159 || y <= 96 || y >= 159;
      if(outside) g_assert_cmpmem(full + i, 4 * sizeof(float), original + i, 4 * sizeof(float));
      g_assert_cmpfloat(full[i + 3], ==, original[i + 3]);
      for(int c = 0; c < 3; c++)
      {
        g_assert_true(isfinite(full[i + c]));
        g_assert_cmpfloat_with_epsilon(half[i + c], 0.5f * (original[i + c] + full[i + c]), 0.00001f);
        difference += fabsf(full[i + c] - original[i + c]);
      }
    }
  g_assert_cmpfloat(difference, >, 10.0);

  char *output_path = g_build_filename(directory, "remove-result.ppm", NULL);
  FILE *output = g_fopen(output_path, "wb");
  g_assert_nonnull(output);
  fprintf(output, "P6\n%d %d\n255\n", size, size);
  for(size_t i = 0; i < count; i += 4)
  {
    const unsigned char rgb[] = { CLAMP(full[i], 0, 1) * 255,
      CLAMP(full[i + 1], 0, 1) * 255, CLAMP(full[i + 2], 0, 1) * 255 };
    fwrite(rgb, 1, 3, output);
  }
  fclose(output);
  g_free(output_path);

  mr.x = mr.y = 0;
  g_assert_true(_retouch_remove(&self, &piece, full, &roi, mask, &mr, 1.0f));
  memcpy(full, original, count * sizeof(float));
  dt_ai_registry_set_enabled(FALSE);
  g_assert_false(_retouch_remove(&self, &piece, full, &roi, mask, &mr, 1.0f));
  g_assert_cmpmem(full, count * sizeof(float), original, count * sizeof(float));
  dt_ai_registry_set_enabled(TRUE);
  g_strlcpy(p.inpaint_model, "missing-model", sizeof(p.inpaint_model));
  commit_params(&self, (dt_iop_params_t *)&p, &pipe, &piece);
  g_assert_false(_retouch_remove(&self, &piece, full, &roi, mask, &mr, 1.0f));
  g_assert_cmpmem(full, count * sizeof(float), original, count * sizeof(float));
  cleanup_pipe(&self, &pipe, &piece);
  dt_dev_cleanup(&dev);
  dt_free_align(original);
  dt_free_align(full);
  dt_free_align(half);
  g_assert_true(dt_ai_models_delete("inpaint-lama-carve-512-v1"));
  g_print("real-model removal, edge crop, opacity, alpha, model pinning and failure checks passed\n");
}
#endif

int main(int argc, char *argv[])
{
  if(argc != 3 && argc != 4)
  {
    g_printerr("usage: %s <datadir> <moduledir> [LaMa.dtmodel]\n", argv[0]);
    return 1;
  }
  _check_legacy();
  _check_roi_and_history();
  char *directory = g_dir_make_tmp("darktable-retouch-remove-XXXXXX", NULL);
  g_assert_nonnull(directory);
  char *models_conf = g_strdup_printf("plugins/ai/models_path=%s/models", directory);
  char *args[] = { argv[0], "--library", ":memory:", "--configdir", directory,
    "--cachedir", directory, "--datadir", argv[1], "--moduledir", argv[2],
    "--disable-opencl", "-d", "ai", "--conf", "write_sidecar_files=never",
    "--conf", "plugins/ai/enabled=TRUE", "--conf", "plugins/ai/auto_check_updates=FALSE",
    "--conf", models_conf, NULL };
  g_assert_cmpint(dt_init(G_N_ELEMENTS(args) - 1, args, FALSE, TRUE, NULL), ==, 0);
#ifdef HAVE_AI
  if(argc == 4) _check_model(argv[3], directory);
#endif
  dt_cleanup();
  g_print("retouch removal tests passed; isolated test directory: %s\n", directory);
  g_free(models_conf);
  g_free(directory);
  return 0;
}
// clang-format off
// modelines: These editor modelines have been set for all relevant files by tools/update_modelines.py
// vim: shiftwidth=2 expandtab tabstop=2 cindent
// kate: tab-indents: off; indent-width 2; replace-tabs on; indent-mode cstyle; remove-trailing-spaces modified;
// clang-format on
