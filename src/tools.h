/*
 * (C) Copyright 2019, Arnaud Ferraris <arnaud.ferraris@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0
 */

#ifndef _VFR_TOOLS_PAGE_H
#define _VFR_TOOLS_PAGE_H

#include <gtk/gtk.h>

#define HANDY_USE_UNSTABLE_API
#include <handy.h>

typedef struct _VFRPrepPage VFRPrepPage;

VFRPrepPage *vfr_prep_page_new(GtkWidget *stack, GtkWidget *menu);
void vfr_prep_page_back(VFRPrepPage *self);

#endif /* _VFR_TOOLS_PAGE_H */
