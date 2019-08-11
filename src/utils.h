/*
 * (C) Copyright 2019, Arnaud Ferraris <arnaud.ferraris@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0
 */

#ifndef _VFR_UTILS_H
#define _VFR_UTILS_H

#include <gtk/gtk.h>

GString *vfr_get_current_airac(void);
GString *vfr_get_current_date(void);

void vfr_ui_empty_list_box(GtkWidget *list);
GtkWidget *vfr_ui_widget_get_descendent(GtkWidget *parent, GType type);

#endif /* _VFR_UTILS_H */
