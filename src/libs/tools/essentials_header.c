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
#include "common/darktable.h"
#include "control/conf.h"
#include "control/control.h"
#include "gui/accelerators.h"
#include "gui/essentials_export.h"
#include "gui/gtk.h"
#include "libs/lib.h"
#include "libs/lib_api.h"
#include "views/view.h"

DT_MODULE(1)

typedef struct dt_lib_essentials_header_t
{
  dt_lib_module_t *self;
  GtkWidget *guided;
  GtkWidget *stages[3];
  GtkWidget *search;
  GtkWidget *popover;
  GtkWidget *results;
  gboolean saved_left_panel;
  gboolean advanced_left_panel_visible;
  gboolean saved_center_panels;
  gboolean advanced_center_top_visible;
  gboolean advanced_center_bottom_visible;
  gboolean advanced_bottom_panel_visible;
  guint pending_apply;
  gulong mapped_handler;
  guint pending_export;
} dt_lib_essentials_header_t;

static gboolean _essentials_lighttable_module(const char *name)
{
  static const char *const visible[] = {"essentials_header",
                                        "essentials_library",
                                        "essentials_inspector",
                                        "backgroundjobs",
                                        NULL};
  for(const char *const *item = visible; *item; item++)
    if(!g_strcmp0(name, *item))
      return TRUE;
  return FALSE;
}

static gboolean _essentials_darkroom_module(const char *name)
{
  static const char *const visible[] = {"essentials_header", "modulegroups",
                                        "histogram", "backgroundjobs", NULL};
  for(const char *const *item = visible; *item; item++)
    if(!g_strcmp0(name, *item))
      return TRUE;
  return FALSE;
}

/*
 * Deliberately not dt_lib_set_visible(): that persists the choice under
 * <view>/<module>_visible, so hiding a module for Essentials would rewrite the
 * user's own panel preferences and leave them hidden in Advanced. Essentials
 * only changes what is on screen, never what the user asked for.
 */
static void _set_module_widget_visible(dt_lib_module_t *module,
                                       const gboolean visible)
{
  if(module->expander)
    gtk_widget_set_visible(module->expander, visible);
  if(module->widget && module->widget != module->expander)
    gtk_widget_set_visible(module->widget, visible);
}

static void _set_panel_class(const dt_ui_container_t container,
                             const char *panel_name,
                             const gboolean essentials)
{
  GtkWidget *panel = GTK_WIDGET(dt_ui_get_container(darktable.gui->ui,
                                                     container));
  while(panel && g_strcmp0(gtk_widget_get_name(panel), panel_name))
    panel = gtk_widget_get_parent(panel);
  if(!panel)
    return;
  if(essentials)
    dt_gui_add_class(panel, "essentials-panel");
  else
    dt_gui_remove_class(panel, "essentials-panel");
}

static void _refresh_editor_experience(void);

/* Hide the panels Essentials does not use, remembering what the user had so
 * Advanced gets its own layout back untouched. */
static void _apply_panel_overrides(dt_lib_essentials_header_t *d,
                                   const gboolean essentials,
                                   const gboolean library)
{
  if(essentials)
  {
    if(!d->saved_center_panels)
    {
      d->advanced_center_top_visible =
          dt_ui_panel_visible(darktable.gui->ui, DT_UI_PANEL_CENTER_TOP);
      d->advanced_center_bottom_visible =
          dt_ui_panel_visible(darktable.gui->ui, DT_UI_PANEL_CENTER_BOTTOM);
      d->advanced_bottom_panel_visible =
          dt_ui_panel_visible(darktable.gui->ui, DT_UI_PANEL_BOTTOM);
      d->saved_center_panels = TRUE;
    }
    dt_ui_panel_show(darktable.gui->ui, DT_UI_PANEL_CENTER_TOP, FALSE, FALSE);
    dt_ui_panel_show(darktable.gui->ui, DT_UI_PANEL_CENTER_BOTTOM, FALSE,
                     FALSE);
    dt_ui_panel_show(darktable.gui->ui, DT_UI_PANEL_BOTTOM, FALSE, FALSE);
  }
  else if(d->saved_center_panels)
  {
    dt_ui_panel_show(darktable.gui->ui, DT_UI_PANEL_CENTER_TOP,
                     d->advanced_center_top_visible, FALSE);
    dt_ui_panel_show(darktable.gui->ui, DT_UI_PANEL_CENTER_BOTTOM,
                     d->advanced_center_bottom_visible, FALSE);
    dt_ui_panel_show(darktable.gui->ui, DT_UI_PANEL_BOTTOM,
                     d->advanced_bottom_panel_visible, FALSE);
    d->saved_center_panels = FALSE;
  }
  if(essentials && !library)
  {
    if(!d->saved_left_panel)
    {
      d->advanced_left_panel_visible =
          dt_ui_panel_visible(darktable.gui->ui, DT_UI_PANEL_LEFT);
      d->saved_left_panel = TRUE;
    }
    dt_ui_panel_show(darktable.gui->ui, DT_UI_PANEL_LEFT, FALSE, FALSE);
  }
  else if(d->saved_left_panel)
  {
    dt_ui_panel_show(darktable.gui->ui, DT_UI_PANEL_LEFT,
                     d->advanced_left_panel_visible, FALSE);
    d->saved_left_panel = FALSE;
  }
}

static void _apply_module_visibility(const gboolean essentials,
                                     const gboolean library,
                                     const gboolean edit)
{
  for(GList *item = darktable.lib->plugins; item; item = g_list_next(item))
  {
    dt_lib_module_t *module = item->data;
    gboolean visible = dt_lib_is_visible_in_view(
        module, dt_view_manager_get_current_view(darktable.view_manager));
    /* the guided header is part of the guided interface: leaving it on top of
     * the complete one gave two overlapping sets of controls */
    if(!g_strcmp0(module->plugin_name, "essentials_header"))
      visible = essentials;
    else if(essentials && library)
      visible = _essentials_lighttable_module(module->plugin_name);
    else if(essentials && edit)
      visible = _essentials_darkroom_module(module->plugin_name);
    else if(essentials)
      visible = FALSE;
    _set_module_widget_visible(module, visible);
  }
}

static gboolean _apply_experience(gpointer user_data)
{
  dt_lib_essentials_header_t *d = user_data;
  d->pending_apply = 0;
  if(!darktable.lib || !darktable.view_manager)
    return G_SOURCE_REMOVE;

  const gboolean essentials = dt_essentials_mode_is_active();
  const dt_view_type_flags_t current_view = dt_view_get_current();
  const gboolean library = current_view == DT_VIEW_LIGHTTABLE;
  const gboolean edit = current_view == DT_VIEW_DARKROOM;
  gtk_widget_set_visible(d->guided, essentials);
  GtkWidget *main_window = dt_ui_main_window(darktable.gui->ui);
  if(essentials)
    dt_gui_add_class(main_window, "essentials-ui");
  else
    dt_gui_remove_class(main_window, "essentials-ui");
  _set_panel_class(DT_UI_CONTAINER_PANEL_LEFT_CENTER, "left", essentials);
  _set_panel_class(DT_UI_CONTAINER_PANEL_RIGHT_CENTER, "right", essentials);

  _apply_panel_overrides(d, essentials, library);

  _apply_module_visibility(essentials, library, edit);

  /* The darkroom builds image-operation expanders after library modules enter
   * the view. Reapply the Essentials facade once the shell is mapped so that
   * GTK's final show-all pass cannot reveal the technical module list. */
  if(essentials && edit)
    _refresh_editor_experience();

  for(int k = 0; k < 3; k++)
    gtk_style_context_remove_class(gtk_widget_get_style_context(d->stages[k]),
                                   "active");
  gtk_style_context_add_class(
      gtk_widget_get_style_context(d->stages[library ? 1 : 2]), "active");
  return G_SOURCE_REMOVE;
}

static void _show_library_tool(const char *name)
{
  dt_lib_module_t *module = dt_lib_get_module(name);
  if(module && module->expander)
    dt_lib_gui_set_expanded(module, TRUE);
}

static gboolean _open_export_panel(gpointer user_data)
{
  dt_lib_essentials_header_t *d = user_data;
  d->pending_export = 0;
  if(!darktable.lib || !darktable.view_manager
     || dt_view_get_current() != DT_VIEW_LIGHTTABLE)
    return G_SOURCE_REMOVE;

  /* Ask the four questions that matter and hand the rest to the same
   * dt_control_export() the full module uses. Export used to switch the whole
   * interface to Advanced to borrow that module, which dropped the user out of
   * Essentials to finish the one step the guided workflow is named after. */
  if(dt_essentials_export_dialog() == DT_ESSENTIALS_EXPORT_WANTS_FULL_MODULE)
  {
    /* reveal the real export module in place. Essentials stays on: this is the
     * same borrow-the-module trick the editor's tool rows use, not a trip into
     * the complete interface. The next view change tidies it away again. */
    dt_lib_module_t *export_module = dt_lib_get_module("export");
    if(export_module)
    {
      _set_module_widget_visible(export_module, TRUE);
      _show_library_tool("export");
      dt_toast_log(_("all export settings"));
    }
    else
      dt_toast_log(_("the export module is unavailable"));
  }
  return G_SOURCE_REMOVE;
}

static void _queue_open_export(dt_lib_essentials_header_t *d)
{
  if(!d->pending_export)
    d->pending_export = g_idle_add(_open_export_panel, d);
}

static void _activate_capability(dt_lib_essentials_header_t *d,
                                 const char *id)
{
  if(!id || !dt_capability_get(id))
    return;
  gtk_widget_hide(d->popover);

  if(!g_strcmp0(id, "library.add_photos"))
  {
    const float result = dt_action_process("lib/import/add to library...", 0,
                                           NULL, "activate", 1.0f);
    if(result == DT_ACTION_NOT_VALID)
      dt_toast_log(_("add photos is unavailable in this view"));
  }
  else if(!g_strcmp0(id, "library.all_photos"))
  {
    dt_lib_module_t *collect = dt_lib_get_module("collect");
    if(collect && collect->gui_reset)
      collect->gui_reset(collect);
  }
  else if(!g_strcmp0(id, "edit.open"))
  {
    if(dt_act_on_get_images_nb(FALSE, FALSE) > 0)
      dt_ctl_switch_mode_to("darkroom");
    else
      dt_toast_log(_("select a photo to edit"));
  }
  else if(!g_strcmp0(id, "export.open"))
  {
    if(dt_view_get_current() != DT_VIEW_LIGHTTABLE)
      dt_ctl_switch_mode_to("lighttable");
    _queue_open_export(d);
  }
  else if(!g_strcmp0(id, "ui.advanced"))
  {
    /* the complete interface is a preference now, not a control in the way */
    dt_toast_log(_("turn on the complete interface in "
                   "preferences > miscellaneous > interface"));
  }
  else if(g_str_has_prefix(id, "library."))
  {
    _show_library_tool("collect");
    dt_toast_log(_("use collections to refine %s"),
                 dt_capability_get(id)->name);
  }
  else if(g_str_has_prefix(id, "edit."))
  {
    if(dt_act_on_get_images_nb(FALSE, FALSE) > 0)
    {
      dt_ctl_switch_mode_to("darkroom");
      dt_toast_log(_("%s will be available in the essentials editor"),
                   dt_capability_get(id)->name);
    } else
      dt_toast_log(_("select a photo first"));
  }
}

static void _result_clicked(GtkButton *button, dt_lib_essentials_header_t *d)
{
  _activate_capability(d, g_object_get_data(G_OBJECT(button), "capability-id"));
}

static void _clear_results(dt_lib_essentials_header_t *d)
{
  GList *children = gtk_container_get_children(GTK_CONTAINER(d->results));
  for(GList *item = children; item; item = g_list_next(item))
    gtk_widget_destroy(GTK_WIDGET(item->data));
  g_list_free(children);
}

static void _refresh_results(dt_lib_essentials_header_t *d)
{
  _clear_results(d);
  const char *query = gtk_entry_get_text(GTK_ENTRY(d->search));
  if(!query || !*query)
  {
    gtk_widget_hide(d->popover);
    return;
  }

  dt_capability_match_t matches[6] = {0};
  const size_t count =
      dt_capabilities_search(query, 0, matches, G_N_ELEMENTS(matches));
  for(size_t k = 0; k < count; k++)
  {
    const dt_capability_descriptor_t *capability = matches[k].descriptor;
    GtkWidget *button = gtk_button_new();
    GtkWidget *copy = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    GtkWidget *title = gtk_label_new(_(capability->name));
    GtkWidget *description = gtk_label_new(_(capability->description));
    gtk_widget_set_halign(title, GTK_ALIGN_START);
    gtk_widget_set_halign(description, GTK_ALIGN_START);
    gtk_label_set_ellipsize(GTK_LABEL(description), PANGO_ELLIPSIZE_END);
    gtk_style_context_add_class(gtk_widget_get_style_context(title),
                                "capability-title");
    gtk_style_context_add_class(gtk_widget_get_style_context(description),
                                "capability-description");
    gtk_box_pack_start(GTK_BOX(copy), title, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(copy), description, FALSE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(button), copy);
    g_object_set_data(G_OBJECT(button), "capability-id",
                      (gpointer)capability->id);
    g_signal_connect(button, "clicked", G_CALLBACK(_result_clicked), d);
    gtk_box_pack_start(GTK_BOX(d->results), button, FALSE, FALSE, 0);
  }
  gtk_widget_show_all(d->results);
  gtk_widget_show(d->popover);
}

static void _search_changed(GtkEditable *editable,
                            dt_lib_essentials_header_t *d)
{
  (void)editable;
  _refresh_results(d);
}

static void _search_activate(GtkEntry *entry, dt_lib_essentials_header_t *d)
{
  (void)entry;
  dt_capability_match_t match = {0};
  if(dt_capabilities_search(gtk_entry_get_text(GTK_ENTRY(d->search)), 0,
                             &match, 1) == 1)
    _activate_capability(d, match.descriptor->id);
}

static void _stage_clicked(GtkButton *button, dt_lib_essentials_header_t *d)
{
  const int stage =
      GPOINTER_TO_INT(g_object_get_data(G_OBJECT(button), "stage"));
  if(stage == 0)
    _activate_capability(d, DT_ESSENTIALS_ACTION("library.add_photos"));
  else if(stage == 1)
    dt_ctl_switch_mode_to("lighttable");
  else
    _activate_capability(d, DT_ESSENTIALS_ACTION("edit.open"));
}

static void _export_clicked(GtkButton *button,
                            dt_lib_essentials_header_t *d)
{
  (void)button;
  _activate_capability(d, DT_ESSENTIALS_ACTION("export.open"));
}

static void _refresh_editor_experience(void)
{
  if(!darktable.view_manager || dt_view_get_current() != DT_VIEW_DARKROOM)
    return;

  dt_lib_module_t *modulegroups = dt_lib_get_module("modulegroups");
  dt_view_t *view =
      (dt_view_t *)dt_view_manager_get_current_view(darktable.view_manager);
  if(modulegroups && modulegroups->view_enter)
    modulegroups->view_enter(modulegroups, view, view);
}

static void _queue_apply(dt_lib_essentials_header_t *d)
{
  if(!d->pending_apply)
    d->pending_apply = g_idle_add(_apply_experience, d);
}

static void _view_changed(gpointer instance, dt_view_t *old_view,
                          dt_view_t *new_view, dt_lib_module_t *self)
{
  (void)instance;
  (void)old_view;
  (void)new_view;
  _queue_apply(self->data);
}

static gboolean _main_window_mapped(GtkWidget *widget, GdkEvent *event,
                                    dt_lib_essentials_header_t *d)
{
  (void)widget;
  (void)event;
  _queue_apply(d);
  return GDK_EVENT_PROPAGATE;
}

static GtkWidget *_stage_button(const int number, const char *label)
{
  GtkWidget *button = gtk_button_new();
  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 7);
  GtkWidget *index = gtk_label_new(NULL);
  g_autofree char *markup = g_strdup_printf("<b>%d</b>", number);
  gtk_label_set_markup(GTK_LABEL(index), markup);
  gtk_style_context_add_class(gtk_widget_get_style_context(index),
                              "stage-index");
  GtkWidget *text = gtk_label_new(label);
  gtk_box_pack_start(GTK_BOX(box), index, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(box), text, FALSE, FALSE, 0);
  gtk_container_add(GTK_CONTAINER(button), box);
  gtk_style_context_add_class(gtk_widget_get_style_context(button), "stage");
  gtk_style_context_add_class(gtk_widget_get_style_context(button),
                              "dt_no_hover");
  return button;
}

const char *name(dt_lib_module_t *self) { return _("essentials workflow"); }

dt_view_type_flags_t views(dt_lib_module_t *self) { return DT_VIEW_ALL; }

uint32_t container(dt_lib_module_t *self)
{
  return DT_UI_CONTAINER_PANEL_TOP_CENTER;
}

gboolean expandable(dt_lib_module_t *self) { return FALSE; }

int position(const dt_lib_module_t *self) { return 1000; }

void gui_init(dt_lib_module_t *self)
{
  dt_lib_essentials_header_t *d = g_malloc0(sizeof(*d));
  self->data = d;
  d->self = self;
  self->widget = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
  gtk_widget_set_name(self->widget, "essentials-header");
  gtk_widget_set_hexpand(self->widget, TRUE);

  d->guided = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 18);
  gtk_widget_set_name(d->guided, "essentials-guided");
  for(int k = 0; k < 3; k++)
  {
    static const char *const labels[] = {N_("add photos"), N_("choose"),
                                         N_("edit & export")};
    d->stages[k] = _stage_button(k + 1, _(labels[k]));
    g_object_set_data(G_OBJECT(d->stages[k]), "stage", GINT_TO_POINTER(k));
    g_signal_connect(d->stages[k], "clicked", G_CALLBACK(_stage_clicked), d);
    gtk_box_pack_start(GTK_BOX(d->guided), d->stages[k], FALSE, FALSE, 0);
  }

  d->search = gtk_search_entry_new();
  gtk_widget_set_name(d->search, "essentials-search");
  gtk_widget_set_size_request(d->search, DT_PIXEL_APPLY_DPI(500), -1);
  gtk_entry_set_placeholder_text(GTK_ENTRY(d->search), _("find anything"));
  gtk_widget_set_tooltip_text(d->search, _("find tools and actions (ctrl+k)"));
  atk_object_set_name(gtk_widget_get_accessible(d->search), _("find anything"));
  g_signal_connect(d->search, "changed", G_CALLBACK(_search_changed), d);
  g_signal_connect(d->search, "activate", G_CALLBACK(_search_activate), d);

  d->popover = gtk_popover_new(d->search);
  gtk_widget_set_name(d->popover, "essentials-search-results");
  gtk_popover_set_position(GTK_POPOVER(d->popover), GTK_POS_BOTTOM);
  d->results = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
  gtk_widget_set_size_request(d->results, DT_PIXEL_APPLY_DPI(380), -1);
  gtk_container_add(GTK_CONTAINER(d->popover), d->results);

  GtkWidget *export_button = gtk_button_new_with_label(_("export"));
  gtk_widget_set_name(export_button, "essentials-header-export");
  gtk_widget_set_tooltip_text(export_button, _("export selected photos"));
  atk_object_set_name(gtk_widget_get_accessible(export_button),
                      _("export selected photos"));
  g_signal_connect(export_button, "clicked", G_CALLBACK(_export_clicked), d);
  dt_action_define(DT_ACTION(self), NULL, N_("export"), export_button,
                   &dt_action_def_button);

  gtk_widget_set_halign(d->guided, GTK_ALIGN_CENTER);
  gtk_box_pack_start(GTK_BOX(self->widget), d->guided, FALSE, FALSE, 0);

  GtkWidget *search_line = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 16);
  gtk_box_pack_start(GTK_BOX(search_line), d->search, TRUE, TRUE, 0);
  GtkWidget *header_actions = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
  gtk_box_pack_start(GTK_BOX(header_actions), export_button, FALSE, FALSE, 0);
  gtk_box_pack_end(GTK_BOX(search_line), header_actions, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(self->widget), search_line, FALSE, FALSE, 0);

  dt_action_t *search_action =
      dt_action_define(DT_ACTION(self), NULL, N_("find anything"), d->search,
                       &dt_action_def_entry);
  dt_shortcut_register(search_action, 0, 0, GDK_KEY_k, GDK_CONTROL_MASK);

  DT_CONTROL_SIGNAL_HANDLE(DT_SIGNAL_VIEWMANAGER_VIEW_CHANGED, _view_changed);
  /* The main window outlives this module, so the handler has to be taken back
   * in gui_cleanup(); leaving it connected would hand a freed d to the next
   * map-event. darktable's g_signal_connect() is a statement macro that yields
   * no id, so connect through the call it expands to. The macro's static
   * assertion on the handler's return type is lost here: "map-event" is an
   * event signal, and _main_window_mapped() returns gboolean to match. */
  d->mapped_handler =
    g_signal_connect_data(dt_ui_main_window(darktable.gui->ui), "map-event",
                          G_CALLBACK(_main_window_mapped), d, NULL,
                          (GConnectFlags)0);
}

void gui_cleanup(dt_lib_module_t *self)
{
  dt_lib_essentials_header_t *d = self->data;
  if(d->pending_apply)
    g_source_remove(d->pending_apply);
  if(d->pending_export)
    g_source_remove(d->pending_export);
  GtkWidget *main_window = dt_ui_main_window(darktable.gui->ui);
  if(d->mapped_handler && main_window)
    g_signal_handler_disconnect(main_window, d->mapped_handler);
  g_free(self->data);
  self->data = NULL;
}

// clang-format off
// modelines: These editor modelines have been set for all relevant files by tools/update_modelines.py
// vim: shiftwidth=2 expandtab tabstop=2 cindent
// kate: tab-indents: off; indent-width 2; replace-tabs on; indent-mode cstyle; remove-trailing-spaces modified;
// clang-format on
