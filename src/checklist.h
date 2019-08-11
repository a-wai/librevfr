/*
 * (C) Copyright 2019, Arnaud Ferraris <arnaud.ferraris@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0
 */

#ifndef _VFR_CHECKLIST_H
#define _VFR_CHECKLIST_H

#include <glib.h>

typedef struct _VFRChecklist VFRChecklist;

VFRChecklist *vfr_checklist_load(const gchar *id);

guint vfr_checklist_get_length(VFRChecklist *checklist);
const gchar *vfr_checklist_get_name(VFRChecklist *checklist);
const gchar *vfr_checklist_get_item(VFRChecklist *checklist, const guint index);
const gchar *vfr_checklist_get_value(VFRChecklist *checklist, const guint index);

#endif /* _VFR_CHECKLIST_H */
