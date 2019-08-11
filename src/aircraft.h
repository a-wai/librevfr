/*
 * (C) Copyright 2019, Arnaud Ferraris <arnaud.ferraris@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0
 */

#ifndef _VFR_AIRCRAFT_H
#define _VFR_AIRCRAFT_H

#include <glib.h>

#include "checklist.h"

typedef struct _VFRAircraft VFRAircraft;

gboolean vfr_aircraft_init();

guint vfr_aircraft_get_count();
VFRAircraft *vfr_aircraft_get(guint index);

const gchar *vfr_aircraft_get_label(VFRAircraft *aircraft);
gdouble vfr_aircraft_get_base_factor(VFRAircraft *aircraft);

guint vfr_aircraft_get_checklist_count(VFRAircraft *aircraft);
VFRChecklist *vfr_aircraft_get_checklist(VFRAircraft *aircraft, guint index);

#endif /* _VFR_AIRCRAFT_H */
