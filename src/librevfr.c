/*
 * (C) Copyright 2019, Arnaud Ferraris <arnaud.ferraris@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0
 */

#include "librevfr.h"

#include "aircraft.h"
#include "flight.h"

#include "nav.h"
#include "docs.h"
#include "tools.h"

struct _VFRMainWindow
{
    GtkApplicationWindow parent_instance;

    GtkWidget *header_stack;
    GtkWidget *menu_button;
    GtkWidget *back_button;

    GtkWidget *main_box;
    GtkWidget *main_stack;
    HdyViewSwitcherBar *main_switcher_bar;

    GtkWidget *prep_page;
    GtkWidget *nav_page;
    GtkWidget *docs_page;

    VFRPrepPage *prep;
    VFRNavPage *nav;
    VFRDocsPage *docs;
};

G_DEFINE_TYPE (VFRMainWindow, vfr_main_window, GTK_TYPE_APPLICATION_WINDOW)

VFRMainWindow *vfr_main_window_new(GtkApplication *application)
{
    return g_object_new(VFR_TYPE_MAIN_WINDOW, "application", application, NULL);
}

static void vfr_main_window_back_clicked_cb (GtkWidget *sender, VFRMainWindow *self)
{
    const char *visible = gtk_stack_get_visible_child_name(GTK_STACK(self->main_stack));

    if (g_str_equal(visible, "docs-page"))
        vfr_docs_page_back(self->docs);
    else if (g_str_equal(visible, "prep-page"))
        vfr_prep_page_back(self->prep);
    else
        vfr_nav_page_back(self->nav);
}

static void show_window (GtkApplication *app)
{
    VFRMainWindow *window;

    window = vfr_main_window_new(app);

    gtk_widget_show_all(GTK_WIDGET(window));
}

static void vfr_main_window_constructed (GObject *object)
{
    VFRMainWindow *self = VFR_MAIN_WINDOW (object);

    G_OBJECT_CLASS (vfr_main_window_parent_class)->constructed (object);

    self->prep = vfr_prep_page_new(self->prep_page, self->header_stack);
    self->nav = vfr_nav_page_new(self->nav_page, self->header_stack);
    self->docs = vfr_docs_page_new(self->docs_page, self->header_stack);
}

static void vfr_main_window_class_init(VFRMainWindowClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

    object_class->constructed = vfr_main_window_constructed;

    gtk_widget_class_set_template_from_resource (widget_class, "/com/a-wai/librevfr/ui/librevfr.ui");
    gtk_widget_class_bind_template_child (widget_class, VFRMainWindow, header_stack);
    gtk_widget_class_bind_template_child (widget_class, VFRMainWindow, menu_button);
    gtk_widget_class_bind_template_child (widget_class, VFRMainWindow, back_button);
    gtk_widget_class_bind_template_child (widget_class, VFRMainWindow, main_box);
    gtk_widget_class_bind_template_child (widget_class, VFRMainWindow, main_stack);
    gtk_widget_class_bind_template_child (widget_class, VFRMainWindow, main_switcher_bar);
    gtk_widget_class_bind_template_child (widget_class, VFRMainWindow, prep_page);
    gtk_widget_class_bind_template_child (widget_class, VFRMainWindow, nav_page);
    gtk_widget_class_bind_template_child (widget_class, VFRMainWindow, docs_page);
    gtk_widget_class_bind_template_callback_full (widget_class, "back_clicked_cb", G_CALLBACK(vfr_main_window_back_clicked_cb));
}

static void vfr_main_window_init(VFRMainWindow *self)
{
    gtk_widget_init_template (GTK_WIDGET (self));
}

int main (int argc, char *argv[])
{
    GtkApplication *app;
    int status;

    hdy_init(&argc, &argv);
    vfr_aircraft_init();
    vfr_flight_init();

    app = gtk_application_new("com.a-wai.LibreVFR", G_APPLICATION_FLAGS_NONE);

    g_signal_connect(app, "activate", G_CALLBACK(show_window), NULL);

    status = g_application_run(G_APPLICATION(app), argc, argv);

    g_object_unref(app);

    return status;

}
