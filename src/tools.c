/*
 * (C) Copyright 2019, Arnaud Ferraris <arnaud.ferraris@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0
 */

#include "tools.h"
#include "aircraft.h"
#include "utils.h"

struct _VFRPrepPage {
    GtkWidget *parent_stack;
    GtkWidget *menu_stack;

    GtkWidget *aircraft_list;
    GtkWidget *cl_list;
    GtkWidget *checklist;

    GtkWidget *done_button;

    GtkWidget *aircraft_label;
    GtkWidget *checklist_label;

    guint current_aircraft;
    guint current_checklist;
};

static void aircraft_selected_cb(GtkListBox *list_box, GtkListBoxRow *row, VFRPrepPage *self)
{
    HdyActionRow *list_item;
    GtkWidget *button;
    VFRAircraft *aircraft;
    guint index;

    index = gtk_list_box_row_get_index(row);
    if (index >= vfr_aircraft_get_count())
        return;

    vfr_ui_empty_list_box(self->cl_list);

    self->current_aircraft = index;
    aircraft = vfr_aircraft_get(self->current_aircraft);
    gtk_label_set_label(GTK_LABEL(self->aircraft_label), vfr_aircraft_get_label(aircraft));

    for (guint i = 0; i < vfr_aircraft_get_checklist_count(aircraft); i++) {
        VFRChecklist *checklist = vfr_aircraft_get_checklist(aircraft, i);

        list_item = hdy_action_row_new();
        hdy_action_row_set_title(list_item, vfr_checklist_get_name(checklist));
        button = gtk_button_new_from_icon_name("document-edit-symbolic", GTK_ICON_SIZE_BUTTON);
        gtk_widget_set_valign(button, GTK_ALIGN_CENTER);
        hdy_action_row_add_action(list_item, button);
        gtk_list_box_insert(GTK_LIST_BOX(self->cl_list), GTK_WIDGET(list_item), -1);
    }

    list_item = hdy_action_row_new();
    hdy_action_row_set_title(list_item, "Add new...");
    gtk_list_box_insert(GTK_LIST_BOX(self->cl_list), GTK_WIDGET(list_item), -1);

    gtk_widget_show_all(self->cl_list);
    gtk_stack_set_visible_child_name(GTK_STACK(self->parent_stack), "aircraft-screen");
}

static void checklist_item_checked_cb(GtkToggleButton *button, VFRPrepPage *self)
{
    GtkWidget *list_box;
    GtkWidget *action_row;
    GtkWidget *check_button;
    HdyActionRow *parent_row;
    GtkListBoxRow *row;
    int idx;

    action_row = gtk_widget_get_ancestor(GTK_WIDGET(button), HDY_TYPE_ACTION_ROW);
    if (!action_row)
        return;

    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button))) {
        gtk_list_box_row_set_activatable(GTK_LIST_BOX_ROW(action_row), FALSE);
        gtk_list_box_row_set_selectable(GTK_LIST_BOX_ROW(action_row), FALSE);
        gtk_widget_set_sensitive(GTK_WIDGET(button), FALSE);

        list_box = gtk_widget_get_ancestor(GTK_WIDGET(action_row), GTK_TYPE_LIST_BOX);
        if (list_box) {
            idx = gtk_list_box_row_get_index(GTK_LIST_BOX_ROW(action_row));
            row = gtk_list_box_get_row_at_index(GTK_LIST_BOX(list_box), idx + 1);

            if (row) {
                check_button = vfr_ui_widget_get_descendent(GTK_WIDGET(row), GTK_TYPE_CHECK_BUTTON);
                if (check_button)
                    gtk_widget_set_sensitive(check_button, TRUE);
                gtk_list_box_row_set_selectable(row, TRUE);
                gtk_list_box_select_row(GTK_LIST_BOX(list_box), row);
                gtk_widget_grab_focus(GTK_WIDGET(row));
            } else {
                row = gtk_list_box_get_row_at_index(GTK_LIST_BOX(self->cl_list), self->current_checklist);
                hdy_action_row_set_icon_name(HDY_ACTION_ROW(row), "object-select-symbolic");
                gtk_widget_set_visible(self->done_button, TRUE);
            }
        }
    }
}

static void checklist_selected_cb(GtkListBox *list_box, GtkListBoxRow *row, VFRPrepPage *self)
{
    HdyActionRow *list_item;
    GtkWidget *button;
    VFRAircraft *aircraft;
    VFRChecklist *checklist;
    GString *label;
    guint index;

    index = gtk_list_box_row_get_index(row);

    aircraft = vfr_aircraft_get(self->current_aircraft);
    if (index >= vfr_aircraft_get_checklist_count(aircraft))
        return;

    vfr_ui_empty_list_box(self->checklist);

    self->current_checklist = index;
    checklist = vfr_aircraft_get_checklist(aircraft, self->current_checklist);
    label = g_string_new(vfr_aircraft_get_label(aircraft));
    g_string_append_printf(label, " - %s", vfr_checklist_get_name(checklist));
    gtk_label_set_label(GTK_LABEL(self->checklist_label), label->str);

    for (guint i = 0; i < vfr_checklist_get_length(checklist); i++) {
        list_item = hdy_action_row_new();
        hdy_action_row_set_title(list_item, vfr_checklist_get_item(checklist, i));

        button = gtk_check_button_new_with_label(vfr_checklist_get_value(checklist, i));
        gtk_widget_set_direction(button, GTK_TEXT_DIR_RTL);
        hdy_action_row_add_action(list_item, button);

        g_signal_connect(button, "toggled", G_CALLBACK(checklist_item_checked_cb), self);
        gtk_list_box_row_set_activatable(GTK_LIST_BOX_ROW(list_item), FALSE);

        gtk_list_box_insert(GTK_LIST_BOX(self->checklist), GTK_WIDGET(list_item), -1);

        if (i > 0) {
            gtk_list_box_row_set_selectable(GTK_LIST_BOX_ROW(list_item), FALSE);
            gtk_widget_set_sensitive(button, FALSE);
        } else {
            gtk_list_box_select_row(GTK_LIST_BOX(self->checklist), GTK_LIST_BOX_ROW(list_item));
            gtk_widget_grab_focus(GTK_WIDGET(list_item));
        }
    }

    gtk_widget_show_all(self->checklist);
    gtk_stack_set_visible_child_name(GTK_STACK(self->parent_stack), "checklist-screen");
}

static void notify_visible_child_cb(GObject *object, GParamSpec *spec, VFRPrepPage *self)
{
    const char *visible = gtk_stack_get_visible_child_name(GTK_STACK(object));

    if (g_str_equal(visible, "main-screen"))
        gtk_stack_set_visible_child_name(GTK_STACK(self->menu_stack), "menu-button");
    else {
        gtk_stack_set_visible_child_name(GTK_STACK(self->menu_stack), "back-button");
        if (g_str_equal(visible, "checklist-screen"))
            gtk_widget_set_visible(self->done_button, FALSE);
    }
}

static void done_button_clicked_cb(GtkButton *button, VFRPrepPage *self)
{
    gtk_stack_set_visible_child_name(GTK_STACK(self->parent_stack), "aircraft-screen");
}

VFRPrepPage *vfr_prep_page_new(GtkWidget *stack, GtkWidget *menu)
{
    PangoAttrList *attr_list = pango_attr_list_new();
    VFRPrepPage *self = g_malloc0(sizeof(VFRPrepPage));
    GtkWidget *label;
    GtkWidget *list_box;
    GtkWidget *box;
    GtkWidget *button;
    GtkWidget *scroll;
    HdyActionRow *list_item;
    VFRAircraft *aircraft;

    self->parent_stack = stack;
    self->menu_stack = menu;

    /*
     * Home screen
     */

    scroll = gtk_scrolled_window_new(NULL, NULL);
    box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_margin_bottom(box, 32);
    gtk_widget_set_margin_top(box, 32);
    gtk_widget_set_margin_start(box, 12);
    gtk_widget_set_margin_end(box, 12);

    // Flight info

    label = gtk_label_new("Flight Information");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_widget_set_margin_bottom(label, 12);
    pango_attr_list_insert(attr_list, pango_attr_weight_new(PANGO_WEIGHT_BOLD));
    gtk_label_set_attributes(GTK_LABEL(label), attr_list);
    gtk_box_pack_start(GTK_BOX(box), label, FALSE, TRUE, 0);

    list_box = gtk_list_box_new();
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(list_box), GTK_SELECTION_NONE);
    gtk_widget_set_margin_bottom(list_box, 32);
    gtk_style_context_add_class(gtk_widget_get_style_context(list_box), "frame");

    list_item = hdy_action_row_new();
    hdy_action_row_set_title(list_item, "Weather");
    hdy_action_row_set_icon_name(list_item, "weather-few-clouds-symbolic");
    gtk_list_box_insert(GTK_LIST_BOX(list_box), GTK_WIDGET(list_item), -1);

    list_item = hdy_action_row_new();
    hdy_action_row_set_title(list_item, "NOTAMs");
    hdy_action_row_set_icon_name(list_item, "dialog-warning-symbolic");
    gtk_list_box_insert(GTK_LIST_BOX(list_box), GTK_WIDGET(list_item), -1);

    gtk_box_pack_start(GTK_BOX(box), list_box, FALSE, TRUE, 0);

    // Checklists

    label = gtk_label_new("Aircrafts");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_widget_set_margin_bottom(label, 12);
    pango_attr_list_insert(attr_list, pango_attr_weight_new(PANGO_WEIGHT_BOLD));
    gtk_label_set_attributes(GTK_LABEL(label), attr_list);
    gtk_box_pack_start(GTK_BOX(box), label, FALSE, TRUE, 0);

    self->aircraft_list = gtk_list_box_new();
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(self->aircraft_list), GTK_SELECTION_NONE);
    gtk_style_context_add_class(gtk_widget_get_style_context(self->aircraft_list), "frame");

    for (guint i = 0; i < vfr_aircraft_get_count(); i++) {
        VFRAircraft *aircraft = vfr_aircraft_get(i);
        GString *subtitle = g_string_new("");
        guint count = vfr_aircraft_get_checklist_count(aircraft);

        g_string_append_printf(subtitle, "%d checklist%s", count, count > 1 ? "s" : "");

        list_item = hdy_action_row_new();
        hdy_action_row_set_title(list_item, vfr_aircraft_get_label(aircraft));
        hdy_action_row_set_subtitle(list_item, subtitle->str);
        gtk_list_box_insert(GTK_LIST_BOX(self->aircraft_list), GTK_WIDGET(list_item), -1);

        g_string_free(subtitle, TRUE);
    }

    list_item = hdy_action_row_new();
    hdy_action_row_set_title(list_item, "Add new aircraft...");
    gtk_list_box_insert(GTK_LIST_BOX(self->aircraft_list), GTK_WIDGET(list_item), -1);

    g_signal_connect(self->aircraft_list, "row-activated", G_CALLBACK(aircraft_selected_cb), self);

    gtk_box_pack_start(GTK_BOX(box), self->aircraft_list, FALSE, TRUE, 0);

    gtk_container_add(GTK_CONTAINER(scroll), box);
    gtk_stack_add_named(GTK_STACK(stack), scroll, "main-screen");

    /*
     * Aircraft screen
     */

    scroll = gtk_scrolled_window_new(NULL, NULL);
    box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_margin_bottom(box, 32);
    gtk_widget_set_margin_top(box, 32);
    gtk_widget_set_margin_start(box, 12);
    gtk_widget_set_margin_end(box, 12);

    self->aircraft_label = gtk_label_new("placeholder");
    gtk_widget_set_halign(self->aircraft_label, GTK_ALIGN_START);
    gtk_widget_set_margin_bottom(self->aircraft_label, 12);
    pango_attr_list_insert(attr_list, pango_attr_weight_new(PANGO_WEIGHT_BOLD));
    gtk_label_set_attributes(GTK_LABEL(self->aircraft_label), attr_list);
    gtk_box_pack_start(GTK_BOX(box), self->aircraft_label, FALSE, TRUE, 0);

    self->cl_list = gtk_list_box_new();
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(self->cl_list), GTK_SELECTION_NONE);
    gtk_style_context_add_class(gtk_widget_get_style_context(self->cl_list), "frame");

    g_signal_connect(self->cl_list, "row-activated", G_CALLBACK(checklist_selected_cb), self);

    gtk_box_pack_start(GTK_BOX(box), self->cl_list, FALSE, TRUE, 0);
    gtk_container_add(GTK_CONTAINER(scroll), box);
    gtk_stack_add_named(GTK_STACK(stack), scroll, "aircraft-screen");

    /*
     * Checklist screen
     */

    scroll = gtk_scrolled_window_new(NULL, NULL);
    box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_margin_bottom(box, 32);
    gtk_widget_set_margin_top(box, 32);
    gtk_widget_set_margin_start(box, 12);
    gtk_widget_set_margin_end(box, 12);

    self->checklist_label = gtk_label_new("placeholder");
    gtk_widget_set_halign(self->checklist_label, GTK_ALIGN_START);
    gtk_widget_set_margin_bottom(self->checklist_label, 12);
    pango_attr_list_insert(attr_list, pango_attr_weight_new(PANGO_WEIGHT_BOLD));
    gtk_label_set_attributes(GTK_LABEL(self->checklist_label), attr_list);
    gtk_box_pack_start(GTK_BOX(box), self->checklist_label, FALSE, TRUE, 0);

    self->checklist = gtk_list_box_new();
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(self->checklist), GTK_SELECTION_SINGLE);
    gtk_widget_set_margin_bottom(self->checklist, 12);
    gtk_style_context_add_class(gtk_widget_get_style_context(self->checklist), "frame");

    gtk_box_pack_start(GTK_BOX(box), self->checklist, FALSE, TRUE, 0);

    self->done_button = gtk_button_new_with_label("Done");
    g_signal_connect(self->done_button, "clicked", G_CALLBACK(done_button_clicked_cb), self);
    gtk_box_pack_start(GTK_BOX(box), self->done_button, FALSE, TRUE, 0);

    gtk_container_add(GTK_CONTAINER(scroll), box);
    gtk_stack_add_named(GTK_STACK(stack), scroll, "checklist-screen");

    g_signal_connect(stack, "notify::visible-child",
                     G_CALLBACK(notify_visible_child_cb), self);

    return self;
}

void vfr_prep_page_back(VFRPrepPage *self)
{
    const char *visible = gtk_stack_get_visible_child_name(GTK_STACK(self->parent_stack));

    if (g_str_equal(visible, "checklist-screen")) {
        done_button_clicked_cb(NULL, self);
    } else if (g_str_equal(visible, "aircraft-screen")) {
        gtk_stack_set_visible_child_name(GTK_STACK(self->parent_stack), "main-screen");
    }
}
