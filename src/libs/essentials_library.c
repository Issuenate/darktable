/*
    This file is part of darktable,
    Copyright (C) 2026 darktable developers.

    darktable is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
*/

#include "common/capabilities.h"
#include "common/collection.h"
#include "common/darktable.h"
#include "control/conf.h"
#include "control/control.h"
#include "gui/accelerators.h"
#include "gui/gtk.h"
#include "libs/lib.h"
#include "libs/lib_api.h"

DT_MODULE(1)

typedef struct dt_essentials_destination_t
{
  const char *capability_id;
  dt_collection_properties_t property;
  /* an empty rule leaves the browser open on every value of the property */
  const char *filter;
} dt_essentials_destination_t;

static const dt_essentials_destination_t _destinations[] = {
    {DT_ESSENTIALS_ACTION("library.all_photos"), DT_COLLECTION_PROP_UNDEF, ""},
    {DT_ESSENTIALS_ACTION("library.recently_added"),
     DT_COLLECTION_PROP_IMPORT_TIMESTAMP, ""},
    {DT_ESSENTIALS_ACTION("library.by_date"), DT_COLLECTION_PROP_DAY, ""},
    {DT_ESSENTIALS_ACTION("library.albums"), DT_COLLECTION_PROP_TAG,
     DT_ESSENTIALS_ALBUM_FILTER},
    {DT_ESSENTIALS_ACTION("library.people"), DT_COLLECTION_PROP_TAG, ""},
    {DT_ESSENTIALS_ACTION("library.places"), DT_COLLECTION_PROP_GEOTAGGING, ""},
    {DT_ESSENTIALS_ACTION("library.tags"), DT_COLLECTION_PROP_TAG, ""}};

static void _add_photos_clicked(GtkButton *button, dt_lib_module_t *self)
{
  (void)button;
  (void)self;
  g_return_if_fail(
      dt_capability_get(DT_ESSENTIALS_ACTION("library.add_photos")));
  const float result = dt_action_process("lib/import/add to library...", 0,
                                         NULL, "activate", 1.0f);
  if(result == DT_ACTION_NOT_VALID)
    dt_toast_log(_("add photos is unavailable in this view"));
}

static void
_show_collection_browser(dt_lib_module_t *module,
                         const dt_collection_properties_t property,
                         const char *filter)
{
  dt_conf_set_int("plugins/lighttable/collect/num_rules", 1);
  dt_conf_set_int("plugins/lighttable/collect/item0", property);
  dt_conf_set_int("plugins/lighttable/collect/mode0", 0);
  dt_conf_set_string("plugins/lighttable/collect/string0", filter);
  dt_collection_set_query_flags(darktable.collection, COLLECTION_QUERY_FULL);
  dt_collection_update_query(darktable.collection,
                             DT_COLLECTION_CHANGE_NEW_QUERY,
                             DT_COLLECTION_PROP_UNDEF, NULL);
  dt_lib_gui_queue_update(module);
  GtkWidget *widget = module->expander ? module->expander : module->widget;
  if(widget)
    gtk_widget_show(widget);
  dt_lib_gui_set_expanded(module, TRUE);
}

static void _destination_clicked(GtkButton *button, dt_lib_module_t *self)
{
  (void)self;
  const dt_essentials_destination_t *destination =
      g_object_get_data(G_OBJECT(button), "destination");
  const char *id = destination->capability_id;
  g_return_if_fail(dt_capability_get(id));

  dt_lib_module_t *collect = dt_lib_get_module("collect");
  if(!collect)
    return;
  if(destination->property == DT_COLLECTION_PROP_UNDEF)
  {
    if(collect->gui_reset)
      collect->gui_reset(collect);
  }
  else
  {
    _show_collection_browser(collect, destination->property,
                             destination->filter);
    dt_toast_log(_("choose a value to filter %s"), dt_capability_get(id)->name);
  }
}

const char *name(dt_lib_module_t *self) { return _("library"); }

dt_view_type_flags_t views(dt_lib_module_t *self) { return DT_VIEW_LIGHTTABLE; }

uint32_t container(dt_lib_module_t *self)
{
  return DT_UI_CONTAINER_PANEL_LEFT_CENTER;
}

gboolean expandable(dt_lib_module_t *self) { return FALSE; }

int position(const dt_lib_module_t *self) { return 1; }

void gui_init(dt_lib_module_t *self)
{
  self->widget = gtk_box_new(GTK_ORIENTATION_VERTICAL, DT_PIXEL_APPLY_DPI(3));
  gtk_widget_set_name(self->widget, "essentials-library");
  GtkWidget *title = gtk_label_new(_("library"));
  gtk_widget_set_halign(title, GTK_ALIGN_START);
  gtk_style_context_add_class(gtk_widget_get_style_context(title),
                              "library-title");
  gtk_box_pack_start(GTK_BOX(self->widget), title, FALSE, FALSE,
                     DT_PIXEL_APPLY_DPI(5));

  GtkWidget *add_photos = gtk_button_new_with_label(_("browse usb or folder"));
  gtk_widget_set_name(add_photos, "essentials-library-add");
  gtk_style_context_add_class(gtk_widget_get_style_context(add_photos),
                              "suggested-action");
  gtk_widget_set_tooltip_text(
      add_photos, _("select photos from a usb drive or folder without copying"));
  atk_object_set_name(gtk_widget_get_accessible(add_photos),
                      _("browse a usb drive or folder"));
  g_signal_connect(add_photos, "clicked", G_CALLBACK(_add_photos_clicked),
                   self);
  gtk_box_pack_start(GTK_BOX(self->widget), add_photos, FALSE, FALSE,
                     DT_PIXEL_APPLY_DPI(6));

  GtkWidget *storage_note =
      gtk_label_new(_("originals stay on the drive while you edit"));
  gtk_widget_set_halign(storage_note, GTK_ALIGN_START);
  gtk_label_set_line_wrap(GTK_LABEL(storage_note), TRUE);
  gtk_style_context_add_class(gtk_widget_get_style_context(storage_note),
                              "library-storage-note");
  gtk_box_pack_start(GTK_BOX(self->widget), storage_note, FALSE, FALSE,
                     DT_PIXEL_APPLY_DPI(8));

  for(guint k = 0; k < G_N_ELEMENTS(_destinations); k++)
  {
    const dt_capability_descriptor_t *capability =
        dt_capability_get(_destinations[k].capability_id);
    GtkWidget *button = gtk_button_new_with_label(_(capability->name));
    gtk_widget_set_halign(gtk_bin_get_child(GTK_BIN(button)), GTK_ALIGN_START);
    gtk_widget_set_tooltip_text(button, _(capability->description));
    gtk_style_context_add_class(gtk_widget_get_style_context(button),
                                "library-destination");
    g_object_set_data(G_OBJECT(button), "destination",
                      (gpointer)&_destinations[k]);
    g_signal_connect(button, "clicked", G_CALLBACK(_destination_clicked), self);
    gtk_box_pack_start(GTK_BOX(self->widget), button, FALSE, FALSE, 0);
  }
}

void gui_cleanup(dt_lib_module_t *self) { (void)self; }

// clang-format off
// modelines: These editor modelines have been set for all relevant files by tools/update_modelines.py
// vim: shiftwidth=2 expandtab tabstop=2 cindent
// kate: tab-indents: off; indent-width 2; replace-tabs on; indent-mode cstyle; remove-trailing-spaces modified;
// clang-format on
