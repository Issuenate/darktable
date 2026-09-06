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

#include "gui/essentials_export.h"

#include "common/act_on.h"
#include "common/colorspaces.h"
#include "common/darktable.h"
#include "control/conf.h"
#include "control/control.h"
#include "control/jobs/control_jobs.h"
#include "gui/gtk.h"
#include "imageio/imageio_module.h"

#include <string.h>

#define EXPORT_PREFIX "plugins/lighttable/export/"

typedef struct _essentials_format_t
{
  const char *label;
  const char *module;   /* imageio format plugin name */
  gboolean lossy;       /* only a lossy format needs the quality question */
} _essentials_format_t;

static const _essentials_format_t _formats[] = {
  { N_("JPEG  -  best for sharing"), "jpeg", TRUE },
  { N_("PNG  -  lossless, larger"),  "png",  FALSE },
  { N_("TIFF  -  for printing"),     "tiff", FALSE },
};

typedef struct _essentials_size_t
{
  const char *label;
  int max_edge;         /* 0 keeps the full size */
} _essentials_size_t;

static const _essentials_size_t _sizes[] = {
  { N_("original size"),          0 },
  { N_("large  -  2048 px"),   2048 },
  { N_("medium  -  1024 px"),  1024 },
  { N_("small  -  640 px"),     640 },
};

typedef struct _dialog_t
{
  GtkWidget *dialog;
  GtkWidget *folder;
  GtkWidget *format;
  GtkWidget *size;
  GtkWidget *quality;
  GtkWidget *quality_row;
  gchar *directory;
} _dialog_t;

/*
 * plugins/imageio/storage/disk/file_directory is a filename *pattern*, not a
 * folder: the default is "$(FILE_FOLDER)/darktable_exported/$(FILE_NAME)". Ask
 * the user for a folder and store it verbatim and darktable takes the whole
 * thing as the output file's name, writing Exports.jpg and then Exports_01.jpg
 * rather than putting anything inside the folder. So the folder shown here is
 * the pattern with its filename component removed, and what is written back
 * always ends in $(FILE_NAME).
 */
gchar *dt_essentials_export_directory_from_pattern(const char *pattern)
{
  if(pattern && *pattern)
  {
    /* a plain path is already a folder: an earlier build of this dialog stored
     * one here, and a user may have typed one into the full module */
    if(!strstr(pattern, "$("))
      return g_strdup(pattern);

    gchar *directory = g_path_get_dirname(pattern);
    /* a pattern's last component is the filename; anything above it that still
     * carries a variable is not a real folder we can show in a chooser */
    if(directory && *directory && g_strcmp0(directory, ".")
       && !strstr(directory, "$("))
      return directory;
    g_free(directory);
  }
  const char *pictures = g_get_user_special_dir(G_USER_DIRECTORY_PICTURES);
  return g_strdup(pictures ? pictures : g_get_home_dir());
}

static void _update_quality_visibility(_dialog_t *d)
{
  const int index = gtk_combo_box_get_active(GTK_COMBO_BOX(d->format));
  const gboolean lossy = index >= 0 && index < (int)G_N_ELEMENTS(_formats)
                         && _formats[index].lossy;
  gtk_widget_set_visible(d->quality_row, lossy);
}

static void _format_changed(GtkComboBox *combo, _dialog_t *d)
{
  _update_quality_visibility(d);
}

static void _choose_folder(GtkButton *button, _dialog_t *d)
{
  GtkFileChooserNative *chooser = gtk_file_chooser_native_new(
      _("export to folder"), GTK_WINDOW(d->dialog),
      GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER, _("_choose"), _("_cancel"));
  if(d->directory)
    gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(chooser), d->directory);

  if(gtk_native_dialog_run(GTK_NATIVE_DIALOG(chooser)) == GTK_RESPONSE_ACCEPT)
  {
    g_free(d->directory);
    d->directory = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(chooser));
    gtk_button_set_label(GTK_BUTTON(d->folder), d->directory);
  }
  g_object_unref(chooser);
}

static GtkWidget *_row(GtkWidget *parent, const char *label, GtkWidget *control)
{
  GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, DT_PIXEL_APPLY_DPI(12));
  GtkWidget *caption = gtk_label_new(label);
  gtk_widget_set_halign(caption, GTK_ALIGN_START);
  gtk_widget_set_size_request(caption, DT_PIXEL_APPLY_DPI(96), -1);
  dt_gui_add_class(caption, "essentials-export-label");
  gtk_box_pack_start(GTK_BOX(row), caption, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(row), control, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(parent), row, FALSE, FALSE, 0);
  return row;
}

/* a private response, so it cannot collide with GTK's own */
#define DT_EXPORT_RESPONSE_MORE 1000

dt_essentials_export_result_t dt_essentials_export_dialog(void)
{
  GList *images = dt_act_on_get_images(TRUE, TRUE, TRUE);
  const guint count = g_list_length(images);
  if(count == 0)
  {
    dt_toast_log(_("select photos to export first"));
    return DT_ESSENTIALS_EXPORT_CANCELLED;
  }

  _dialog_t d = { 0 };
  g_autofree char *title =
    g_strdup_printf(ngettext("export %u photo", "export %u photos", count), count);

  d.dialog = gtk_dialog_new_with_buttons
    (title, GTK_WINDOW(dt_ui_main_window(darktable.gui->ui)),
     GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
     _("more options..."), DT_EXPORT_RESPONSE_MORE,
     _("_cancel"), GTK_RESPONSE_CANCEL,
     title, GTK_RESPONSE_ACCEPT,
     NULL);
  /* every other format darktable can write -- avif, heif, jxl, j2k, webp, pdf,
   * ppm, pfm, xcf -- plus profiles, styles, metadata and filename patterns,
   * live in the full module. This is the way there. */
  GtkWidget *more = gtk_dialog_get_widget_for_response(GTK_DIALOG(d.dialog),
                                                       DT_EXPORT_RESPONSE_MORE);
  if(more)
  {
    gtk_widget_set_tooltip_text(more, _("all formats and export settings"));
    dt_gui_add_class(more, "essentials-tertiary");
    GtkWidget *box = gtk_widget_get_parent(more);
    if(GTK_IS_BUTTON_BOX(box))
      gtk_button_box_set_child_secondary(GTK_BUTTON_BOX(box), more, TRUE);
  }
  gtk_dialog_set_default_response(GTK_DIALOG(d.dialog), GTK_RESPONSE_ACCEPT);
  dt_gui_dialog_apply_experience(d.dialog);
  gtk_widget_set_name(d.dialog, "essentials-export");

  GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(d.dialog));
  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, DT_PIXEL_APPLY_DPI(10));
  gtk_container_set_border_width(GTK_CONTAINER(box), DT_PIXEL_APPLY_DPI(16));
  gtk_box_pack_start(GTK_BOX(content), box, TRUE, TRUE, 0);

  /* where: start from whatever the full export module last used, so the two
   * agree and a user moving between them is not surprised */
  d.directory = dt_essentials_export_directory_from_pattern(
    dt_conf_get_string_const("plugins/imageio/storage/disk/file_directory"));
  d.folder = gtk_button_new_with_label(d.directory);
  gtk_widget_set_tooltip_text(d.folder, _("choose where the copies are saved"));
  gtk_label_set_ellipsize(GTK_LABEL(gtk_bin_get_child(GTK_BIN(d.folder))),
                          PANGO_ELLIPSIZE_START);
  g_signal_connect(d.folder, "clicked", G_CALLBACK(_choose_folder), &d);
  _row(box, _("save to"), d.folder);

  d.format = gtk_combo_box_text_new();
  for(guint k = 0; k < G_N_ELEMENTS(_formats); k++)
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(d.format), _(_formats[k].label));
  gtk_combo_box_set_active(GTK_COMBO_BOX(d.format), 0);
  g_signal_connect(d.format, "changed", G_CALLBACK(_format_changed), &d);
  _row(box, _("format"), d.format);

  d.size = gtk_combo_box_text_new();
  for(guint k = 0; k < G_N_ELEMENTS(_sizes); k++)
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(d.size), _(_sizes[k].label));
  gtk_combo_box_set_active(GTK_COMBO_BOX(d.size), 0);
  _row(box, _("size"), d.size);

  d.quality = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 50, 100, 1);
  gtk_range_set_value(GTK_RANGE(d.quality),
                      CLAMP(dt_conf_get_int("plugins/imageio/format/jpeg/quality"),
                            50, 100));
  gtk_scale_set_value_pos(GTK_SCALE(d.quality), GTK_POS_RIGHT);
  gtk_widget_set_tooltip_text(d.quality,
                              _("higher keeps more detail and makes a larger file"));
  d.quality_row = _row(box, _("quality"), d.quality);

  gtk_widget_show_all(d.dialog);
  _update_quality_visibility(&d);

  dt_essentials_export_result_t result = DT_ESSENTIALS_EXPORT_CANCELLED;
  const gint response = gtk_dialog_run(GTK_DIALOG(d.dialog));
  if(response == DT_EXPORT_RESPONSE_MORE)
    result = DT_ESSENTIALS_EXPORT_WANTS_FULL_MODULE;
  else if(response == GTK_RESPONSE_ACCEPT)
  {
    const int format_index = gtk_combo_box_get_active(GTK_COMBO_BOX(d.format));
    const int size_index = gtk_combo_box_get_active(GTK_COMBO_BOX(d.size));
    const _essentials_format_t *format =
      &_formats[CLAMP(format_index, 0, (int)G_N_ELEMENTS(_formats) - 1)];
    const int max_edge = _sizes[CLAMP(size_index, 0, (int)G_N_ELEMENTS(_sizes) - 1)].max_edge;

    /* write through the same keys the full module reads, so the choice made
     * here is the choice it shows */
    g_autofree char *pattern = g_build_filename(d.directory, "$(FILE_NAME)", NULL);
    dt_conf_set_string("plugins/imageio/storage/disk/file_directory", pattern);
    dt_conf_set_string(EXPORT_PREFIX "storage_name", "disk");
    dt_conf_set_string(EXPORT_PREFIX "format_name", format->module);
    if(format->lossy)
      dt_conf_set_int("plugins/imageio/format/jpeg/quality",
                      (int)gtk_range_get_value(GTK_RANGE(d.quality)));

    dt_imageio_module_format_t *fmt = dt_imageio_get_format_by_name(format->module);
    dt_imageio_module_storage_t *storage = dt_imageio_get_storage_by_name("disk");
    if(fmt && storage)
    {
      g_autofree char *style = dt_conf_get_string(EXPORT_PREFIX "style");
      dt_control_export(images,
                        max_edge, max_edge,
                        dt_imageio_get_index_of_format(fmt),
                        dt_imageio_get_index_of_storage(storage),
                        TRUE,           /* high quality resampling */
                        FALSE,          /* never upscale: it cannot add detail */
                        TRUE,           /* max_edge is a bounding box */
                        FALSE, 1.0,     /* not scaling by a factor */
                        FALSE,          /* masks are an advanced concern */
                        style ? style : "",
                        dt_conf_get_bool(EXPORT_PREFIX "style_append"),
                        DT_COLORSPACE_SRGB, "", DT_INTENT_PERCEPTUAL,
                        NULL);
      images = NULL;  /* dt_control_export() takes the list */
      result = DT_ESSENTIALS_EXPORT_QUEUED;
      dt_toast_log(ngettext("exporting %u photo to %s",
                            "exporting %u photos to %s", count),
                   count, d.directory);
    }
    else
      dt_toast_log(_("that export format is unavailable"));
  }

  gtk_widget_destroy(d.dialog);
  g_free(d.directory);
  g_list_free(images);
  return result;
}

// clang-format off
// modelines: These editor modelines have been set for all relevant files by tools/update_modelines.py
// vim: shiftwidth=2 expandtab tabstop=2 cindent
// kate: tab-indents: off; indent-width 2; replace-tabs on; indent-mode cstyle; remove-trailing-spaces modified;
// clang-format on
