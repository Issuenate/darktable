/*
    This file is part of darktable,
    Copyright (C) 2026 darktable developers.

    darktable is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
*/

#include "common/act_on.h"
#include "common/capabilities.h"
#include "common/collection.h"
#include "common/colorlabels.h"
#include "common/darktable.h"
#include "common/datetime.h"
#include "common/image_cache.h"
#include "common/ratings.h"
#include "common/tags.h"
#include "control/control.h"
#include "dtgtk/button.h"
#include "dtgtk/paint.h"
#include "gui/accelerators.h"
#include "gui/gtk.h"
#include "libs/lib.h"
#include "libs/lib_api.h"
#include "views/view.h"

#include <string.h>

DT_MODULE(1)

typedef struct dt_lib_essentials_inspector_t
{
  GtkWidget *empty;
  GtkWidget *content;
  GtkWidget *filename;
  GtkWidget *type;
  GtkWidget *datetime;
  GtkWidget *camera;
  GtkWidget *lens;
  GtkWidget *rating[5];
  GtkWidget *reject;
  GtkWidget *label[DT_COLORLABELS_LAST];
  GtkWidget *album;
  GtkWidget *album_current;
  GtkWidget *open_edit;
  GtkWidget *export;
  GdkRGBA star_fill;
  int current_rating;
} dt_lib_essentials_inspector_t;

static void _update(dt_lib_module_t *self);

/*
 * A click on a single photo is its own confirmation, and asking twice for one
 * reversible star or label is what made the guided interface feel heavier than
 * the advanced one. Only a change that also reaches photos the inspector is
 * not describing needs the dialog.
 */
static gboolean _confirmed(const guint count, const char *title,
                           const char *question)
{
  return count < 2 || dt_gui_show_yes_no_dialog(title, "", "%s", question);
}

static void _set_dimmed(GtkWidget *widget, const gboolean dimmed)
{
  if(dimmed)
    dt_gui_add_class(widget, "dt_dimmed");
  else
    dt_gui_remove_class(widget, "dt_dimmed");
}

/*
 * dtgtk_cairo_paint_star() fills the star only when it is handed a color as
 * paint data, so a rating cannot be shown through CPF_ACTIVE alone. There is
 * no setter for that field; the button reads it directly, see
 * dtgtk/button.c:131.
 */
static void _set_star_filled(dt_lib_essentials_inspector_t *d, const int index,
                             const gboolean filled)
{
  DTGTK_BUTTON(d->rating[index])->icon_data = filled ? &d->star_fill : NULL;
  gtk_widget_queue_draw(d->rating[index]);
}

/* the album names attached to imgid, without the reserved hierarchy prefix */
static GList *_attached_albums(const dt_imgid_t imgid)
{
  GList *tags = NULL;
  GList *albums = NULL;
  dt_tag_get_attached(imgid, &tags, TRUE);
  for(const GList *item = tags; item; item = g_list_next(item))
  {
    const dt_tag_t *tag = item->data;
    if(g_str_has_prefix(tag->tag, DT_ESSENTIALS_ALBUM_PREFIX))
      albums = g_list_prepend(
          albums, g_strdup(tag->tag + strlen(DT_ESSENTIALS_ALBUM_PREFIX)));
  }
  dt_tag_free_result(&tags);
  return g_list_reverse(albums);
}

static void _update(dt_lib_module_t *self)
{
  dt_lib_essentials_inspector_t *d = self->data;
  const int count = dt_act_on_get_images_nb(FALSE, FALSE);
  const dt_imgid_t imgid = dt_act_on_get_main_image();
  /* the cache get can fail even on a valid id, see image_cache.c:253; an
   * image removed while still hovered must fall back to the empty state */
  const dt_image_t *image = count > 0 && dt_is_valid_imgid(imgid)
                                ? dt_image_cache_get(imgid, 'r')
                                : NULL;
  const gboolean valid = image != NULL;
  gtk_widget_set_visible(d->empty, !valid);
  gtk_widget_set_visible(d->content, valid);
  gtk_widget_set_sensitive(d->open_edit, valid);
  gtk_widget_set_sensitive(d->export, valid);
  if(!valid)
    return;

  char datetime[128] = "";
  dt_datetime_img_to_local(datetime, sizeof(datetime), image, FALSE);
  gtk_label_set_text(GTK_LABEL(d->filename), image->filename);
  gtk_label_set_text(GTK_LABEL(d->datetime), datetime);
  gtk_label_set_text(GTK_LABEL(d->camera), image->exif_model);
  gtk_label_set_text(GTK_LABEL(d->lens), image->exif_lens);

  const char *extension = strrchr(image->filename, '.');
  g_autofree char *type =
      extension ? g_ascii_strup(extension + 1, -1) : g_strdup("");
  gtk_label_set_text(GTK_LABEL(d->type), type);
  dt_image_cache_read_release(image);
  d->current_rating = dt_ratings_get(imgid);

  /* read the fill here, not in gui_init(): the widget is not styled yet then */
  gtk_style_context_get_color(gtk_widget_get_style_context(d->rating[0]),
                              gtk_widget_get_state_flags(d->rating[0]),
                              &d->star_fill);
  /* the two rating accessors disagree on reject: dt_ratings_get() reports
   * DT_VIEW_REJECT while dt_image_get_xmp_rating() reports -1, see
   * common/image.c:607. The write side below is ratings.c, so read that. */
  const gboolean rejected = d->current_rating == DT_VIEW_REJECT;
  for(int k = 0; k < 5; k++)
    _set_star_filled(d, k, !rejected && k < d->current_rating);
  _set_dimmed(d->reject, !rejected);
  gtk_widget_set_tooltip_text(d->reject, rejected
                                             ? _("this photo is rejected")
                                             : _("reject this photo"));

  const int labels = dt_colorlabels_get_labels(imgid);
  for(int k = 0; k < DT_COLORLABELS_LAST; k++)
    _set_dimmed(d->label[k], !(labels & (1 << k)));

  GList *albums = _attached_albums(imgid);
  if(albums)
  {
    g_autofree char *names = NULL;
    for(const GList *item = albums; item; item = g_list_next(item))
      dt_util_str_cat(&names, "%s%s", names ? ", " : "", (char *)item->data);
    gtk_label_set_text(GTK_LABEL(d->album_current), names);
  } else
    gtk_label_set_text(GTK_LABEL(d->album_current), _("not in an album"));
  g_list_free_full(albums, g_free);
}

static void _rating_clicked(GtkButton *button, dt_lib_module_t *self)
{
  g_return_if_fail(dt_capability_get(DT_ESSENTIALS_ACTION("library.rate")));
  const int rating =
      GPOINTER_TO_INT(g_object_get_data(G_OBJECT(button), "rating"));
  GList *images = dt_act_on_get_images(FALSE, TRUE, FALSE);
  const guint count = g_list_length(images);
  if(count == 0)
  {
    g_list_free(images);
    return;
  }

  g_autofree char *summary =
      g_strdup_printf(ngettext("set %u selected photo to %d stars?",
                               "set %u selected photos to %d stars?", count),
                      count, rating);
  if(_confirmed(count, _("confirm rating"), summary))
  {
    dt_ratings_apply_on_list(images, rating, TRUE);
    dt_collection_update_query(darktable.collection,
                               DT_COLLECTION_CHANGE_RELOAD,
                               DT_COLLECTION_PROP_RATING_RANGE, images);
    dt_control_queue_redraw_center();
    _update(self);
  }
  g_list_free(images);
}

static void _reject_clicked(GtkButton *button, dt_lib_module_t *self)
{
  g_return_if_fail(dt_capability_get(DT_ESSENTIALS_ACTION("library.reject")));
  (void)button;
  GList *images = dt_act_on_get_images(FALSE, TRUE, FALSE);
  const guint count = g_list_length(images);
  if(count == 0)
  {
    g_list_free(images);
    return;
  }

  g_autofree char *summary = g_strdup_printf(
      ngettext("reject %u selected photo?", "reject %u selected photos?", count),
      count);
  if(_confirmed(count, _("confirm reject"), summary))
  {
    /* dt_ratings_apply_on_list() toggles the rejected flag back off again */
    dt_ratings_apply_on_list(images, DT_VIEW_REJECT, TRUE);
    dt_collection_update_query(darktable.collection,
                               DT_COLLECTION_CHANGE_RELOAD,
                               DT_COLLECTION_PROP_RATING_RANGE, images);
    dt_control_queue_redraw_center();
    _update(self);
  }
  g_list_free(images);
}

static void _label_clicked(GtkButton *button, dt_lib_module_t *self)
{
  g_return_if_fail(dt_capability_get(DT_ESSENTIALS_ACTION("library.label")));
  const int color =
      GPOINTER_TO_INT(g_object_get_data(G_OBJECT(button), "color"));
  GList *images = dt_act_on_get_images(FALSE, TRUE, FALSE);
  const guint count = g_list_length(images);
  if(count == 0)
  {
    g_list_free(images);
    return;
  }

  g_autofree char *summary =
      color == DT_COLORLABELS_LAST
          ? g_strdup_printf(ngettext("clear the color label of %u selected "
                                     "photo?",
                                     "clear the color labels of %u selected "
                                     "photos?",
                                     count),
                            count)
          : g_strdup_printf(
                ngettext("change the %s label of %u selected photo?",
                         "change the %s label of %u selected photos?", count),
                _(dt_colorlabels_name[color]), count);
  if(_confirmed(count, _("confirm color label"), summary))
  {
    dt_colorlabels_toggle_label_on_list(images, color, TRUE);
    dt_collection_update_query(darktable.collection,
                               DT_COLLECTION_CHANGE_RELOAD,
                               DT_COLLECTION_PROP_COLORLABEL, images);
    dt_control_queue_redraw_center();
    _update(self);
  }
  g_list_free(images);
}

/* scoped to the album hierarchy: dt_tag_get_with_usage() would count every
 * tagged image in the catalog to build a list we discard all but a few rows of */
static void _refresh_album_list(dt_lib_essentials_inspector_t *d)
{
  gtk_combo_box_text_remove_all(GTK_COMBO_BOX_TEXT(d->album));
  GList *tags = NULL;
  GList *images = NULL;
  dt_tag_get_tags_images(DT_ESSENTIALS_ALBUM_ROOT, &tags, &images);
  for(const GList *item = tags; item; item = g_list_next(item))
  {
    const dt_tag_t *tag = item->data;
    if(g_str_has_prefix(tag->tag, DT_ESSENTIALS_ALBUM_PREFIX))
      gtk_combo_box_text_append_text(
          GTK_COMBO_BOX_TEXT(d->album),
          tag->tag + strlen(DT_ESSENTIALS_ALBUM_PREFIX));
  }
  dt_tag_free_result(&tags);
  g_list_free(images);
}

static void _album_popup_shown(GObject *combo, GParamSpec *spec,
                               dt_lib_module_t *self)
{
  (void)spec;
  gboolean shown = FALSE;
  g_object_get(combo, "popup-shown", &shown, NULL);
  if(shown)
    _refresh_album_list(self->data);
}

static void _album_add_clicked(GtkWidget *widget, dt_lib_module_t *self)
{
  g_return_if_fail(
      dt_capability_get(DT_ESSENTIALS_ACTION("library.add_to_album")));
  (void)widget;
  dt_lib_essentials_inspector_t *d = self->data;

  g_autofree char *name =
      gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(d->album));
  if(name)
    g_strstrip(name);
  if(!name || !*name)
  {
    dt_toast_log(_("type an album name first"));
    return;
  }

  /* dt_tag_attach_string_list() splits on commas, so a comma here would
   * quietly create a second, unrelated top-level tag */
  if(strchr(name, ','))
  {
    dt_toast_log(_("an album name cannot contain a comma"));
    return;
  }

  GList *images = dt_act_on_get_images(FALSE, TRUE, FALSE);
  const guint count = g_list_length(images);
  if(count == 0)
  {
    g_list_free(images);
    return;
  }

  g_autofree char *summary =
      g_strdup_printf(ngettext("add %u selected photo to album \"%s\"?",
                               "add %u selected photos to album \"%s\"?", count),
                      count, name);
  if(_confirmed(count, _("confirm album"), summary))
  {
    g_autofree char *tag = g_strconcat(DT_ESSENTIALS_ALBUM_PREFIX, name, NULL);
    dt_tag_attach_string_list(tag, images, TRUE);
    dt_image_synch_xmps(images);
    dt_collection_update_query(darktable.collection,
                               DT_COLLECTION_CHANGE_RELOAD,
                               DT_COLLECTION_PROP_TAG, images);
    _refresh_album_list(d);
    _update(self);
    dt_toast_log(ngettext("added %u photo to \"%s\"",
                          "added %u photos to \"%s\"", count),
                 count, name);
  }
  g_list_free(images);
}

static void _album_activated(GtkEntry *entry, dt_lib_module_t *self)
{
  _album_add_clicked(GTK_WIDGET(entry), self);
}

static void _open_edit(GtkButton *button, dt_lib_module_t *self)
{
  g_return_if_fail(dt_capability_get(DT_ESSENTIALS_ACTION("edit.open")));
  (void)button;
  (void)self;
  if(dt_act_on_get_images_nb(FALSE, FALSE) > 0)
    dt_ctl_switch_mode_to("darkroom");
}

static void _open_export(GtkButton *button, dt_lib_module_t *self)
{
  g_return_if_fail(dt_capability_get(DT_ESSENTIALS_ACTION("export.open")));
  (void)button;
  (void)self;
  /* the header owns the export dialog, so the inspector asks for its action
   * rather than opening a second one. dt_action_process() only logs when the
   * path does not resolve, which would leave the button looking dead */
  const float result =
      dt_action_process("lib/essentials_header/export", 0, NULL, "activate", 1.0f);
  if(result == DT_ACTION_NOT_VALID)
    dt_toast_log(_("export is unavailable in this view"));
}

static void _selection_changed(gpointer instance, dt_lib_module_t *self)
{
  (void)instance;
  dt_lib_gui_queue_update(self);
}

static GtkWidget *_info_label(const char *style)
{
  GtkWidget *label = gtk_label_new("");
  gtk_widget_set_halign(label, GTK_ALIGN_START);
  gtk_label_set_ellipsize(GTK_LABEL(label), PANGO_ELLIPSIZE_END);
  gtk_style_context_add_class(gtk_widget_get_style_context(label), style);
  return label;
}

static GtkWidget *_section_title(GtkWidget *parent, const char *text)
{
  GtkWidget *label = gtk_label_new(text);
  gtk_widget_set_halign(label, GTK_ALIGN_START);
  gtk_style_context_add_class(gtk_widget_get_style_context(label),
                              "inspector-section-title");
  gtk_box_pack_start(GTK_BOX(parent), label, FALSE, FALSE, 0);
  return label;
}

const char *name(dt_lib_module_t *self) { return _("photo details"); }

dt_view_type_flags_t views(dt_lib_module_t *self) { return DT_VIEW_LIGHTTABLE; }

uint32_t container(dt_lib_module_t *self)
{
  return DT_UI_CONTAINER_PANEL_RIGHT_CENTER;
}

gboolean expandable(dt_lib_module_t *self) { return FALSE; }

int position(const dt_lib_module_t *self) { return 1; }

void gui_init(dt_lib_module_t *self)
{
  dt_lib_essentials_inspector_t *d = g_malloc0(sizeof(*d));
  self->data = d;
  self->widget = gtk_box_new(GTK_ORIENTATION_VERTICAL, DT_PIXEL_APPLY_DPI(14));
  gtk_widget_set_name(self->widget, "essentials-inspector");

  d->empty = gtk_label_new(_("select a photo"));
  gtk_label_set_line_wrap(GTK_LABEL(d->empty), TRUE);
  gtk_label_set_justify(GTK_LABEL(d->empty), GTK_JUSTIFY_LEFT);
  gtk_widget_set_halign(d->empty, GTK_ALIGN_START);
  gtk_widget_set_valign(d->empty, GTK_ALIGN_START);
  gtk_style_context_add_class(gtk_widget_get_style_context(d->empty),
                              "inspector-empty");
  gtk_box_pack_start(GTK_BOX(self->widget), d->empty, FALSE, FALSE, 0);

  d->content = gtk_box_new(GTK_ORIENTATION_VERTICAL, DT_PIXEL_APPLY_DPI(9));
  GtkWidget *title_line =
      gtk_box_new(GTK_ORIENTATION_HORIZONTAL, DT_PIXEL_APPLY_DPI(8));
  d->filename = _info_label("inspector-title");
  d->type = _info_label("inspector-badge");
  gtk_box_pack_start(GTK_BOX(title_line), d->filename, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(title_line), d->type, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(d->content), title_line, FALSE, FALSE, 0);
  d->datetime = _info_label("inspector-meta");
  d->camera = _info_label("inspector-meta");
  d->lens = _info_label("inspector-meta");
  gtk_box_pack_start(GTK_BOX(d->content), d->datetime, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(d->content), d->camera, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(d->content), d->lens, FALSE, FALSE, 0);

  GtkWidget *separator = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
  gtk_box_pack_start(GTK_BOX(d->content), separator, FALSE, FALSE,
                     DT_PIXEL_APPLY_DPI(5));
  _section_title(d->content, _("rating"));

  GtkWidget *rating_box =
      gtk_box_new(GTK_ORIENTATION_HORIZONTAL, DT_PIXEL_APPLY_DPI(4));
  gtk_widget_set_name(rating_box, "essentials-rating");
  for(int k = 0; k < 5; k++)
  {
    d->rating[k] = dtgtk_button_new(dtgtk_cairo_paint_star, 0, NULL);
    gtk_widget_set_size_request(d->rating[k], DT_PIXEL_APPLY_DPI(30),
                                DT_PIXEL_APPLY_DPI(30));
    g_object_set_data(G_OBJECT(d->rating[k]), "rating", GINT_TO_POINTER(k + 1));
    g_signal_connect(d->rating[k], "clicked", G_CALLBACK(_rating_clicked),
                     self);
    g_autofree char *accessible_name = g_strdup_printf(_("%d stars"), k + 1);
    atk_object_set_name(gtk_widget_get_accessible(d->rating[k]),
                        accessible_name);
    gtk_box_pack_start(GTK_BOX(rating_box), d->rating[k], FALSE, FALSE, 0);
  }

  d->reject = dtgtk_button_new(dtgtk_cairo_paint_reject, 0, NULL);
  gtk_widget_set_size_request(d->reject, DT_PIXEL_APPLY_DPI(30),
                              DT_PIXEL_APPLY_DPI(30));
  gtk_widget_set_tooltip_text(d->reject, _("reject this photo"));
  atk_object_set_name(gtk_widget_get_accessible(d->reject), _("reject"));
  g_signal_connect(d->reject, "clicked", G_CALLBACK(_reject_clicked), self);
  gtk_box_pack_end(GTK_BOX(rating_box), d->reject, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(d->content), rating_box, FALSE, FALSE, 0);

  _section_title(d->content, _("color label"));
  GtkWidget *label_box =
      gtk_box_new(GTK_ORIENTATION_HORIZONTAL, DT_PIXEL_APPLY_DPI(4));
  gtk_widget_set_name(label_box, "essentials-color-labels");
  for(int k = 0; k <= DT_COLORLABELS_LAST; k++)
  {
    const gboolean clear = k == DT_COLORLABELS_LAST;
    GtkWidget *button = dtgtk_button_new(dtgtk_cairo_paint_label, k, NULL);
    gtk_widget_set_size_request(button, DT_PIXEL_APPLY_DPI(30),
                                DT_PIXEL_APPLY_DPI(30));
    gtk_widget_set_tooltip_text(button, clear ? _("clear color labels")
                                              : _(dt_colorlabels_name[k]));
    atk_object_set_name(gtk_widget_get_accessible(button),
                        clear ? _("clear color labels")
                              : _(dt_colorlabels_name[k]));
    g_object_set_data(G_OBJECT(button), "color", GINT_TO_POINTER(k));
    g_signal_connect(button, "clicked", G_CALLBACK(_label_clicked), self);
    if(clear)
      gtk_box_pack_end(GTK_BOX(label_box), button, FALSE, FALSE, 0);
    else
    {
      d->label[k] = button;
      _set_dimmed(button, TRUE);
      gtk_box_pack_start(GTK_BOX(label_box), button, FALSE, FALSE, 0);
    }
  }
  gtk_box_pack_start(GTK_BOX(d->content), label_box, FALSE, FALSE, 0);

  _section_title(d->content, _("album"));
  d->album_current = _info_label("inspector-meta");
  gtk_box_pack_start(GTK_BOX(d->content), d->album_current, FALSE, FALSE, 0);
  GtkWidget *album_box =
      gtk_box_new(GTK_ORIENTATION_HORIZONTAL, DT_PIXEL_APPLY_DPI(6));
  gtk_widget_set_name(album_box, "essentials-album");
  d->album = gtk_combo_box_text_new_with_entry();
  GtkWidget *album_entry = gtk_bin_get_child(GTK_BIN(d->album));
  gtk_entry_set_placeholder_text(GTK_ENTRY(album_entry), _("album name"));
  atk_object_set_name(gtk_widget_get_accessible(album_entry), _("album name"));
  gtk_widget_set_tooltip_text(
      d->album, _("pick an existing album or type a new name"));
  g_signal_connect(d->album, "notify::popup-shown",
                   G_CALLBACK(_album_popup_shown), self);
  g_signal_connect(album_entry, "activate", G_CALLBACK(_album_activated), self);
  gtk_box_pack_start(GTK_BOX(album_box), d->album, TRUE, TRUE, 0);

  GtkWidget *album_add = gtk_button_new_with_label(_("add"));
  gtk_widget_set_tooltip_text(album_add,
                              _("add the selected photos to this album"));
  g_signal_connect(album_add, "clicked", G_CALLBACK(_album_add_clicked), self);
  gtk_box_pack_start(GTK_BOX(album_box), album_add, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(d->content), album_box, FALSE, FALSE, 0);
  _refresh_album_list(d);

  d->open_edit = gtk_button_new_with_label(_("open in edit"));
  gtk_style_context_add_class(gtk_widget_get_style_context(d->open_edit),
                              "suggested-action");
  gtk_widget_set_name(d->open_edit, "essentials-open-edit");
  g_signal_connect(d->open_edit, "clicked", G_CALLBACK(_open_edit), self);
  d->export = gtk_button_new_with_label(_("export..."));
  gtk_widget_set_name(d->export, "essentials-open-export");
  g_signal_connect(d->export, "clicked", G_CALLBACK(_open_export), self);
  gtk_box_pack_end(GTK_BOX(d->content), d->export, FALSE, FALSE, 0);
  gtk_box_pack_end(GTK_BOX(d->content), d->open_edit, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(self->widget), d->content, TRUE, TRUE, 0);

  /* show the whole tree once, then keep later show-all passes away from the
   * two states: views/view.c:415 show-alls the module on every view change,
   * which revealed both at once, and nothing reran _update() until the
   * selection changed */
  gtk_widget_show_all(self->widget);
  gtk_widget_set_no_show_all(d->empty, TRUE);
  gtk_widget_set_no_show_all(d->content, TRUE);

  DT_CONTROL_SIGNAL_HANDLE(DT_SIGNAL_SELECTION_CHANGED, _selection_changed);
  DT_CONTROL_SIGNAL_HANDLE(DT_SIGNAL_MOUSE_OVER_IMAGE_CHANGE,
                           _selection_changed);
  _update(self);
}

void gui_update(dt_lib_module_t *self) { _update(self); }

void gui_cleanup(dt_lib_module_t *self)
{
  g_free(self->data);
  self->data = NULL;
}

// clang-format off
// modelines: These editor modelines have been set for all relevant files by tools/update_modelines.py
// vim: shiftwidth=2 expandtab tabstop=2 cindent
// kate: tab-indents: off; indent-width 2; replace-tabs on; indent-mode cstyle; remove-trailing-spaces modified;
// clang-format on
