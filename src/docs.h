/*
 * (C) Copyright 2019, Arnaud Ferraris <arnaud.ferraris@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0
 */

#ifndef _VFR_DOCS_PAGE_H
#define _VFR_DOCS_PAGE_H

#include <gtk/gtk.h>
#include <evince-view.h>
#include <evince-document.h>

#define HANDY_USE_UNSTABLE_API
#include <handy.h>

typedef struct _VFRDocsPage VFRDocsPage;

VFRDocsPage *vfr_docs_page_new(GtkWidget *stack, GtkWidget *menu_stack);
void vfr_docs_page_back(VFRDocsPage *self);

#endif /* _VFR_DOCS_PAGE_H */
