/*
 * (C) Copyright 2019, Arnaud Ferraris <arnaud.ferraris@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0
 */

#include "nav.h"

#include "aircraft.h"
#include "flight.h"

#include <math.h>

struct _VFRNavPage {
    GtkWidget *parent_stack;
    GtkWidget *menu_stack;

    GtkWidget *flights_list;
    GtkWidget *nav_log;

    GtkWidget *flight_label;

    GtkWidget *done_button;
    GtkWidget *start_button;

    guint current_aircraft;
    guint current_flight;

    GPtrArray *log;
    GTimer *timer;
    guint timeout_source;
};

typedef struct {
    GtkWidget *entry_timer;
    GtkWidget *entry_button;
    GtkWidget *entry_details;

    gint64 duration;
} LogEntry;

static void nav_log_clicked_cb(GtkToggleButton *button, VFRNavPage *self)
{
    GtkWidget *list_box;
    GtkWidget *row;
    GtkWidget *box;
    GtkListBoxRow *box_row;
    GString *str;
    LogEntry *entry;
    guint i;
    gint elapsed, hours, minutes, seconds;

    box = gtk_widget_get_ancestor(GTK_WIDGET(button), GTK_TYPE_BOX);
    row = gtk_widget_get_ancestor(GTK_WIDGET(button), GTK_TYPE_LIST_BOX_ROW);
    list_box = gtk_widget_get_ancestor(GTK_WIDGET(button), GTK_TYPE_LIST_BOX);

    i = gtk_list_box_row_get_index(GTK_LIST_BOX_ROW(row));

    entry = self->log->pdata[i];
    gtk_list_box_row_set_selectable(GTK_LIST_BOX_ROW(row), FALSE);
    gtk_widget_set_visible(GTK_WIDGET(button), FALSE);

    gtk_widget_set_margin_top(box, 8);
    gtk_widget_set_margin_bottom(box, 8);

    elapsed = (gint)g_timer_elapsed(self->timer, NULL);
    hours = elapsed / 3600;
    elapsed %= 3600;
    minutes = elapsed / 60;
    seconds = elapsed % 60;

    str = g_string_new("");
    g_string_printf(str, "%02d:%02d:%02d", hours, minutes, seconds);
    gtk_label_set_label(GTK_LABEL(entry->entry_timer), str->str);

    i++;
    if (i < self->log->len) {
        entry = self->log->pdata[i];
        box_row = gtk_list_box_get_row_at_index(GTK_LIST_BOX(list_box), i);
        gtk_widget_set_visible(entry->entry_details, TRUE);
        gtk_widget_set_sensitive(entry->entry_button, TRUE);
        gtk_list_box_row_set_selectable(box_row, TRUE);
        gtk_list_box_select_row(GTK_LIST_BOX(list_box), box_row);
        gtk_widget_grab_focus(entry->entry_timer);
        g_timer_reset(self->timer);
    } else {
        g_timer_stop(self->timer);
        g_source_remove(self->timeout_source);
        gtk_widget_set_visible(self->done_button, TRUE);
    }
}

static GtkWidget *nav_log_widget_add(VFRFlightLeg *leg, VFRNavPage *self)
{
    GtkWidget *row = gtk_list_box_row_new();
    GtkWidget *hbox;
    GtkWidget *vbox;
    GtkWidget *widget;
    LogEntry *entry = g_malloc0(sizeof(LogEntry));
    char tmp[1024];
    gint64 duration = 0;
    gdouble base_factor;

    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

    PangoAttrList *attr_list = pango_attr_list_new();
    pango_attr_list_insert(attr_list, pango_attr_weight_new(PANGO_WEIGHT_BOLD));

    /*
     * First line: Name + heading
     */

    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_margin_start(hbox, 12);
    gtk_widget_set_margin_end(hbox, 12);
    gtk_widget_set_margin_top(hbox, 8);
    gtk_widget_set_margin_bottom(hbox, 8);

    widget = gtk_label_new(leg->name->str);
    gtk_widget_set_halign(widget, GTK_ALIGN_START);
    gtk_label_set_attributes(GTK_LABEL(widget), attr_list);
    gtk_box_pack_start(GTK_BOX(hbox), widget, TRUE, TRUE, 0);

    sprintf(tmp, "%03ld°", leg->heading);
    widget = gtk_label_new(tmp);
    gtk_box_pack_end(GTK_BOX(hbox), widget, FALSE, TRUE, 8);

    gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

    // Details box

    entry->entry_details = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

    /*
     * Second line: Distance, time & elevation
     */

    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, TRUE);
    gtk_widget_set_margin_top(hbox, 8);
    gtk_widget_set_margin_bottom(hbox, 8);

    sprintf(tmp, "%ld Nm", leg->distance);
    widget = gtk_label_new(tmp);
    gtk_box_pack_start(GTK_BOX(hbox), widget, TRUE, TRUE, 0);
    widget = gtk_separator_new(GTK_ORIENTATION_VERTICAL);
    gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, TRUE, 0);

    base_factor = vfr_aircraft_get_base_factor(vfr_aircraft_get(self->current_aircraft));
    duration = (gint64)round(base_factor * leg->distance);
    entry->duration = duration * 60;

    sprintf(tmp, "%ld'", duration);
    widget = gtk_label_new(tmp);
    gtk_box_pack_start(GTK_BOX(hbox), widget, TRUE, TRUE, 0);
    widget = gtk_separator_new(GTK_ORIENTATION_VERTICAL);
    gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, TRUE, 0);

    sprintf(tmp, "%ld ft", leg->altitude);
    widget = gtk_label_new(tmp);
    gtk_box_pack_start(GTK_BOX(hbox), widget, TRUE, TRUE, 0);

    gtk_box_pack_start(GTK_BOX(entry->entry_details), hbox, TRUE, TRUE, 0);

    /*
     * Third line: timer + button
     */

    attr_list = pango_attr_list_new();
    pango_attr_list_insert(attr_list, pango_attr_weight_new(PANGO_WEIGHT_BOLD));

    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);

    entry->entry_timer = gtk_label_new("00:00:00");
    gtk_label_set_attributes(GTK_LABEL(entry->entry_timer), attr_list);
    gtk_box_pack_start(GTK_BOX(hbox), entry->entry_timer, TRUE, TRUE, 0);

    entry->entry_button = gtk_button_new_with_label("Top");
    gtk_widget_set_halign(entry->entry_button, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(entry->entry_button, GTK_ALIGN_CENTER);
    g_signal_connect(entry->entry_button, "clicked", G_CALLBACK(nav_log_clicked_cb), self);
    gtk_box_pack_start(GTK_BOX(hbox), entry->entry_button, FALSE, TRUE, 8);

    widget = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(entry->entry_details), widget, FALSE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(entry->entry_details), hbox, TRUE, TRUE, 0);

    widget = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(vbox), widget, FALSE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), entry->entry_details, TRUE, TRUE, 0);

    gtk_container_add(GTK_CONTAINER(row), vbox);

    gtk_list_box_row_set_activatable(GTK_LIST_BOX_ROW(row), FALSE);
    gtk_list_box_row_set_selectable(GTK_LIST_BOX_ROW(row), FALSE);
    gtk_widget_set_sensitive(entry->entry_button, FALSE);

    gtk_list_box_insert(GTK_LIST_BOX(self->nav_log), GTK_WIDGET(row), -1);
    g_ptr_array_add(self->log, entry);

    return row;
}

static void flight_selected_cb(GtkListBox *list_box, GtkListBoxRow *row, VFRNavPage *self)
{
    HdyActionRow *list_item;
    GtkWidget *button;
    VFRFlight *flight;
    guint index;

    index = gtk_list_box_row_get_index(row);
    if (index >= vfr_flight_get_count())
        return;

    self->current_flight = index;
    flight = vfr_flight_get(self->current_flight);
    gtk_label_set_label(GTK_LABEL(self->flight_label), vfr_flight_get_label(flight));

    for (guint i = 0; i < vfr_flight_get_leg_count(flight); i++) {
        GtkWidget *row = nav_log_widget_add(vfr_flight_get_leg(flight, i), self);
    }

    gtk_widget_show_all(self->nav_log);
    gtk_stack_set_visible_child_name(GTK_STACK(self->parent_stack), "nav-log");
}

static void notify_visible_child_cb(GObject *object, GParamSpec *spec, VFRNavPage *self)
{
    const char *visible = gtk_stack_get_visible_child_name(GTK_STACK(object));
    guint i = 1;

    if (g_str_equal(visible, "flights-list"))
        gtk_stack_set_visible_child_name(GTK_STACK(self->menu_stack), "menu-button");
    else {
        gtk_stack_set_visible_child_name(GTK_STACK(self->menu_stack), "back-button");
        if (g_str_equal(visible, "nav-log")) {
            while (i < self->log->len) {
                LogEntry *entry = self->log->pdata[i];
                gtk_widget_set_visible(entry->entry_details, FALSE);
                i++;
            }

            gtk_widget_set_visible(self->start_button, TRUE);
            gtk_widget_set_visible(self->done_button, FALSE);
        }
    }
}

static void update_timer_cb(VFRNavPage *self)
{
    GtkListBoxRow *row = gtk_list_box_get_selected_row(GTK_LIST_BOX(self->nav_log));
    LogEntry *entry = self->log->pdata[gtk_list_box_row_get_index(row)];
    GString *str = g_string_new(NULL);
    gint remaining = entry->duration - (gint)g_timer_elapsed(self->timer, NULL);
    gint hours, minutes, seconds;
    gboolean overdue = FALSE;

    if (remaining < 0) {
        overdue = TRUE;
        remaining *= -1;
    }

    hours = remaining / 3600;
    remaining %= 3600;
    minutes = remaining / 60;
    seconds = remaining % 60;

    g_string_printf(str, "%s%02d:%02d:%02d", overdue ? "-" : "", hours, minutes, seconds);
    gtk_label_set_label(GTK_LABEL(entry->entry_timer), str->str);
    if (overdue) {
        PangoAttrList *attr_list = gtk_label_get_attributes(GTK_LABEL(entry->entry_timer));
        pango_attr_list_change(attr_list, pango_attr_foreground_new(0xFFFF, 0, 0));
    }
    g_string_free(str, FALSE);
}

static void start_button_clicked_cb(GtkButton *button, VFRNavPage *self)
{
    GtkWidget *list_box;
    GtkWidget *row;
    GPtrArray *p = self->log;
    LogEntry *first_entry = p->pdata[0];

    // Launch timer

    row = gtk_widget_get_ancestor(first_entry->entry_details, GTK_TYPE_LIST_BOX_ROW);
    gtk_list_box_row_set_selectable(GTK_LIST_BOX_ROW(row), TRUE);
    list_box = gtk_widget_get_ancestor(first_entry->entry_details, GTK_TYPE_LIST_BOX);
    gtk_list_box_select_row(GTK_LIST_BOX(list_box), GTK_LIST_BOX_ROW(row));

    gtk_widget_set_visible(first_entry->entry_details, TRUE);
    gtk_widget_set_sensitive(first_entry->entry_button, TRUE);
    gtk_widget_set_visible(self->start_button, FALSE);

    self->timer = g_timer_new();
    self->timeout_source = g_timeout_add(100, G_SOURCE_FUNC(update_timer_cb), self);
}

static void done_button_clicked_cb(GtkButton *button, VFRNavPage *self)
{
    GtkListBoxRow *row;

    row = gtk_list_box_get_row_at_index(GTK_LIST_BOX(self->nav_log), 0);
    while (row) {
        gtk_widget_destroy(GTK_WIDGET(row));
        row = gtk_list_box_get_row_at_index(GTK_LIST_BOX(self->nav_log), 0);
        g_ptr_array_remove_index(self->log, 0);
    }

    gtk_stack_set_visible_child_name(GTK_STACK(self->parent_stack), "flights-list");
}

VFRNavPage *vfr_nav_page_new(GtkWidget *stack, GtkWidget *menu)
{
    PangoAttrList *attr_list = pango_attr_list_new();
    VFRNavPage *self = g_malloc0(sizeof(VFRNavPage));
    GtkWidget *label;
    GtkWidget *list_box;
    GtkWidget *box;
    GtkWidget *row;
    GtkWidget *button;
    GtkWidget *scroll;
    HdyActionRow *list_item;
    LogEntry *log_entry;

    self->current_aircraft = 0;

    self->parent_stack = stack;
    self->menu_stack = menu;
    self->log = g_ptr_array_new_with_free_func(free);

    /*
     * Flights list
     */

    scroll = gtk_scrolled_window_new(NULL, NULL);
    box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_margin_bottom(box, 32);
    gtk_widget_set_margin_top(box, 32);
    gtk_widget_set_margin_start(box, 12);
    gtk_widget_set_margin_end(box, 12);

    label = gtk_label_new("Flights");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_widget_set_margin_bottom(label, 12);
    pango_attr_list_insert(attr_list, pango_attr_weight_new(PANGO_WEIGHT_BOLD));
    gtk_label_set_attributes(GTK_LABEL(label), attr_list);
    gtk_box_pack_start(GTK_BOX(box), label, FALSE, TRUE, 0);

    self->flights_list = gtk_list_box_new();
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(self->flights_list), GTK_SELECTION_NONE);
    gtk_style_context_add_class(gtk_widget_get_style_context(self->flights_list), "frame");

    for (guint i = 0; i < vfr_flight_get_count(); i++) {
        VFRFlight *flight = vfr_flight_get(i);

        list_item = hdy_action_row_new();
        hdy_action_row_set_title(list_item, vfr_flight_get_name(flight));
        hdy_action_row_set_subtitle(list_item, vfr_flight_get_label(flight));

        button = gtk_button_new_from_icon_name("document-edit-symbolic", GTK_ICON_SIZE_BUTTON);
        gtk_widget_set_valign(button, GTK_ALIGN_CENTER);
        hdy_action_row_add_action(list_item, button);

        gtk_list_box_insert(GTK_LIST_BOX(self->flights_list), GTK_WIDGET(list_item), -1);
    }

    list_item = hdy_action_row_new();
    hdy_action_row_set_title(list_item, "Add new...");
    gtk_list_box_row_set_selectable(GTK_LIST_BOX_ROW(list_item), FALSE);
    gtk_list_box_insert(GTK_LIST_BOX(self->flights_list), GTK_WIDGET(list_item), -1);

    g_signal_connect(self->flights_list, "row-activated", G_CALLBACK(flight_selected_cb), self);

    gtk_box_pack_start(GTK_BOX(box), self->flights_list, FALSE, TRUE, 0);

    gtk_container_add(GTK_CONTAINER(scroll), box);
    gtk_stack_add_named(GTK_STACK(stack), scroll, "flights-list");

    /*
     * Nav log
     */

    scroll = gtk_scrolled_window_new(NULL, NULL);
    box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_margin_bottom(box, 32);
    gtk_widget_set_margin_top(box, 32);
    gtk_widget_set_margin_start(box, 12);
    gtk_widget_set_margin_end(box, 12);

    self->flight_label = gtk_label_new("placeholder");
    gtk_widget_set_halign(self->flight_label, GTK_ALIGN_START);
    gtk_widget_set_margin_bottom(self->flight_label, 12);
    pango_attr_list_insert(attr_list, pango_attr_weight_new(PANGO_WEIGHT_BOLD));
    gtk_label_set_attributes(GTK_LABEL(self->flight_label), attr_list);
    gtk_box_pack_start(GTK_BOX(box), self->flight_label, FALSE, TRUE, 0);

    self->start_button = gtk_button_new_with_label("Start");
    g_signal_connect(self->start_button, "clicked", G_CALLBACK(start_button_clicked_cb), self);
    gtk_widget_set_margin_bottom(self->start_button, 12);
    gtk_box_pack_start(GTK_BOX(box), self->start_button, FALSE, TRUE, 0);

    self->nav_log = gtk_list_box_new();
    gtk_widget_set_margin_bottom(self->nav_log, 12);
    gtk_style_context_add_class(gtk_widget_get_style_context(self->nav_log), "frame");

    gtk_box_pack_start(GTK_BOX(box), self->nav_log, FALSE, TRUE, 0);

    self->done_button = gtk_button_new_with_label("Done");
    g_signal_connect(self->done_button, "clicked", G_CALLBACK(done_button_clicked_cb), self);
    gtk_box_pack_start(GTK_BOX(box), self->done_button, FALSE, TRUE, 0);

    gtk_container_add(GTK_CONTAINER(scroll), box);
    gtk_stack_add_named(GTK_STACK(stack), scroll, "nav-log");

    g_signal_connect(stack, "notify::visible-child",
                     G_CALLBACK(notify_visible_child_cb), self);

    return self;
}

void vfr_nav_page_back(VFRNavPage *self)
{
    const char *visible = gtk_stack_get_visible_child_name(GTK_STACK(self->parent_stack));

    if (g_str_equal(visible, "nav-log"))
        done_button_clicked_cb(NULL, self);
}
