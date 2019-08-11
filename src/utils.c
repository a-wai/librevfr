/*
 * (C) Copyright 2019, Arnaud Ferraris <arnaud.ferraris@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0
 */

#include "utils.h"

#define DAY_IN_SECONDS (24*3600)
#define AIRAC_IN_SECONDS (28*DAY_IN_SECONDS)
#define AIRAC_ORIGIN (1420675200)

GString *vfr_get_current_airac(void)
{
    GString *airac = g_string_new(NULL);

    time_t now = time(NULL);
    time_t diff = now - AIRAC_ORIGIN;
    char *months[] = {
        "JAN", "FEB", "MAR", "APR", "MAY", "JUN",
        "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"
    };
    struct tm *airac_date;

    now = AIRAC_ORIGIN + ((now - AIRAC_ORIGIN) / AIRAC_IN_SECONDS) * AIRAC_IN_SECONDS;
    airac_date = gmtime(&now);

    g_string_printf(airac, "%02d_%s_%04d", airac_date->tm_mday,
                                           months[airac_date->tm_mon],
                                           airac_date->tm_year + 1900);

    return airac;
}

GString *vfr_get_current_date(void)
{
    GString *date = g_string_new(NULL);

    time_t now = time(NULL);
    struct tm *current_date = gmtime(&now);

    g_string_printf(date, "%04d-%02d-%02d", current_date->tm_mday,
                                            current_date->tm_mon,
                                            current_date->tm_year + 1900);

    return date;
}

void vfr_ui_empty_list_box(GtkWidget *list)
{
    GtkListBoxRow *row;

    row = gtk_list_box_get_row_at_index(GTK_LIST_BOX(list), 0);
    while (row) {
        gtk_widget_destroy(GTK_WIDGET(row));
        row = gtk_list_box_get_row_at_index(GTK_LIST_BOX(list), 0);
    }
}

GtkWidget *vfr_ui_widget_get_descendent(GtkWidget *parent, GType type)
{
    GtkWidget *child;

    for (GList *l = gtk_container_get_children(GTK_CONTAINER(parent)); l != NULL; l = l->next)
    {
        child = GTK_WIDGET(l->data);
        if (G_OBJECT_TYPE(child) == type)
            return child;
        else {
            child = vfr_ui_widget_get_descendent(child, type);
            if (child)
                return child;
        }
    }

    return NULL;
}
