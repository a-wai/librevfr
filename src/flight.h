/*
 * (C) Copyright 2019, Arnaud Ferraris <arnaud.ferraris@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0
 */

#ifndef _VFR_FLIGHT_H
#define _VFR_FLIGHT_H

#include <glib.h>

typedef struct _VFRFlight VFRFlight;

typedef struct {
    GString *name;
    gint64 heading;
    gint64 distance;
    gint64 altitude;
} VFRFlightLeg;

gboolean vfr_flight_init();

guint vfr_flight_get_count();
VFRFlight *vfr_flight_get(guint index);

const gchar *vfr_flight_get_label(VFRFlight *flight);
const gchar *vfr_flight_get_name(VFRFlight *flight);

guint vfr_flight_get_leg_count(VFRFlight *flight);
VFRFlightLeg *vfr_flight_get_leg(VFRFlight *flight, guint index);
GPtrArray *vfr_flight_get_legs(VFRFlight *flight);

#endif /* _VFR_FLIGHT_H */
