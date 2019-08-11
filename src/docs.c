/*
 * (C) Copyright 2019, Arnaud Ferraris <arnaud.ferraris@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0
 */

#include "docs.h"

#include "provider.h"
#include "utils.h"

struct _VFRDocsPage {
    GtkWidget *parent_stack;
    GtkWidget *menu_stack;

    GtkWidget *label;
    GtkWidget *list_box;
    GtkWidget *data_box;
    GtkWidget *data_label;
    GtkWidget *pdf_view;
    EvDocumentModel *pdf_model;
    EvDocument *pdf;

    VFRProvider *current_provider;
    GPtrArray *providers;
};

static void data_selected_cb(GtkListBox *list_box, GtkListBoxRow *row, VFRDocsPage *self)
{
    const char *selected = hdy_action_row_get_subtitle(HDY_ACTION_ROW(row));
    GError *err = NULL;
    char file[1024];
    const char *uri;

    sprintf(file, "%s/librevfr/%s/files/%s.pdf", g_get_user_data_dir(),
                                           vfr_provider_get_id(self->current_provider),
                                           selected);
    uri = g_filename_to_uri(file, NULL, NULL);

    self->pdf = ev_document_factory_get_document(uri, &err);
    if (err) {
        printf("Unable to open %s: %s\n", file, err->message);
        return;
    }

    self->pdf_model = ev_document_model_new_with_document(self->pdf);
    ev_view_set_model(EV_VIEW(self->pdf_view), self->pdf_model);

    gtk_stack_set_visible_child_name(GTK_STACK(self->parent_stack), "pdf");
}

static void docs_widget_add(VFRTerrain *terrain, VFRDocsPage *self)
{
    HdyActionRow *item = hdy_action_row_new();
    hdy_action_row_set_subtitle(item, vfr_terrain_get_icao(terrain));
    hdy_action_row_set_title(item, vfr_terrain_get_name(terrain));
    gtk_list_box_insert(GTK_LIST_BOX(self->data_box), GTK_WIDGET(item), -1);
}

static void provider_selected_cb(GtkListBox *list_box, GtkListBoxRow *row, VFRDocsPage *self)
{
    const char *selected = hdy_action_row_get_title(HDY_ACTION_ROW(row));
    GtkListBoxRow *current;
    HdyActionRow *item;
    guint index;

    index = gtk_list_box_row_get_index(row);
    if (index >= self->providers->len)
        return;

    vfr_ui_empty_list_box(self->data_box);

    self->current_provider = self->providers->pdata[index];
    gtk_label_set_label(GTK_LABEL(self->data_label), vfr_provider_get_name(self->current_provider));

    for (guint i = 0; i < vfr_provider_get_terrain_count(self->current_provider); i++) {
        docs_widget_add(vfr_provider_get_terrain_by_index(self->current_provider, i), self);
    }

    gtk_widget_show_all(self->data_box);
    gtk_stack_set_visible_child_name(GTK_STACK(self->parent_stack), "data-box");
}

static void notify_visible_child_cb(GObject *object, GParamSpec *spec, gpointer data)
{
    VFRDocsPage *self = data;
    const char *visible = gtk_stack_get_visible_child_name(GTK_STACK(object));

    if (g_str_equal(visible, "list-box"))
        gtk_stack_set_visible_child_name(GTK_STACK(self->menu_stack), "menu-button");
    else
        gtk_stack_set_visible_child_name(GTK_STACK(self->menu_stack), "back-button");
}

VFRDocsPage *vfr_docs_page_new (GtkWidget *stack, GtkWidget *menu_stack)
{
    PangoAttrList *attr_list = pango_attr_list_new();
    VFRDocsPage *self = g_malloc0(sizeof(VFRDocsPage));
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    GtkWidget *sublist;
    GtkWidget *scroll;

    ev_init();
    self->providers = vfr_provider_init();

    self->parent_stack = stack;
    self->menu_stack = menu_stack;

    gtk_widget_set_margin_bottom(box, 32);
    gtk_widget_set_margin_top(box, 32);
    gtk_widget_set_margin_start(box, 12);
    gtk_widget_set_margin_end(box, 12);

    self->label = gtk_label_new("VAC Charts");
    gtk_widget_set_halign(self->label, GTK_ALIGN_START);
    gtk_widget_set_margin_bottom(self->label, 12);
    pango_attr_list_insert(attr_list, pango_attr_weight_new(PANGO_WEIGHT_BOLD));
    gtk_label_set_attributes(GTK_LABEL(self->label), attr_list);
    gtk_box_pack_start(GTK_BOX(box), self->label, FALSE, TRUE, 0);

    self->list_box = gtk_list_box_new();
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(self->list_box), GTK_SELECTION_NONE);
    gtk_style_context_add_class(gtk_widget_get_style_context(self->list_box), "frame");

    for (guint i = 0; i < self->providers->len; i++) {
        HdyActionRow *list_item = hdy_action_row_new();
        GtkWidget *button = gtk_switch_new();

        hdy_action_row_set_title(list_item, vfr_provider_get_name(self->providers->pdata[i]));
        gtk_switch_set_state(GTK_SWITCH(button), TRUE);
        gtk_widget_set_valign(button, GTK_ALIGN_CENTER);
        hdy_action_row_add_action(list_item, button);
        g_object_bind_property(button, "state", list_item, "activatable", G_BINDING_SYNC_CREATE);
        gtk_list_box_insert(GTK_LIST_BOX(self->list_box), GTK_WIDGET(list_item), -1);
    }

    g_signal_connect(self->list_box, "row-activated", G_CALLBACK(provider_selected_cb), self);

    gtk_box_pack_start(GTK_BOX(box), self->list_box, FALSE, TRUE, 0);
    gtk_stack_add_named(GTK_STACK(stack), box, "list-box");

    box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_margin_bottom(box, 32);
    gtk_widget_set_margin_top(box, 32);
    gtk_widget_set_margin_start(box, 12);
    gtk_widget_set_margin_end(box, 12);

    self->data_label = gtk_label_new("placeholder");
    gtk_widget_set_halign(self->data_label, GTK_ALIGN_START);
    gtk_widget_set_margin_bottom(self->data_label, 12);
    pango_attr_list_insert(attr_list, pango_attr_weight_new(PANGO_WEIGHT_BOLD));
    gtk_label_set_attributes(GTK_LABEL(self->data_label), attr_list);
    gtk_box_pack_start(GTK_BOX(box), self->data_label, FALSE, TRUE, 0);

    scroll = gtk_scrolled_window_new(NULL, NULL);
    self->data_box = gtk_list_box_new();
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(self->data_box), GTK_SELECTION_NONE);
    gtk_style_context_add_class(gtk_widget_get_style_context(self->data_box), "frame");
    g_signal_connect(self->data_box, "row-activated", G_CALLBACK(data_selected_cb), self);

    gtk_container_add(GTK_CONTAINER(scroll), self->data_box);
    gtk_box_pack_start(GTK_BOX(box), scroll, TRUE, TRUE, 0);

    gtk_stack_add_named(GTK_STACK(stack), box, "data-box");

    scroll = gtk_scrolled_window_new(NULL, NULL);
    self->pdf_view = ev_view_new();
    gtk_container_add(GTK_CONTAINER(scroll), self->pdf_view);
    gtk_stack_add_named(GTK_STACK(stack), scroll, "pdf");

    g_signal_connect(stack, "notify::visible-child",
                     G_CALLBACK(notify_visible_child_cb), self);

    return self;
}

void vfr_docs_page_back(VFRDocsPage *self)
{
    const char *visible = gtk_stack_get_visible_child_name(GTK_STACK(self->parent_stack));

    if (g_str_equal(visible, "data-box")) {
        gtk_stack_set_visible_child_name(GTK_STACK(self->parent_stack), "list-box");
    } else if (g_str_equal(visible, "pdf")) {
        gtk_stack_set_visible_child_name(GTK_STACK(self->parent_stack), "data-box");
    }
}
