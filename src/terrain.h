/*
 * (C) Copyright 2019, Arnaud Ferraris <arnaud.ferraris@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0
 */

#ifndef _VFR_TERRAIN_H
#define _VFR_TERRAIN_H

#include <glib.h>

typedef struct _VFRTerrain VFRTerrain;

VFRTerrain *vfr_terrain_new(const gchar *name, const gchar *icao, gboolean favorite);

const gchar *vfr_terrain_get_name(VFRTerrain *terrain);
const gchar *vfr_terrain_get_icao(VFRTerrain *terrain);
gboolean vfr_terrain_is_favorite(VFRTerrain *terrain);

void vfr_terrain_set_favorite(VFRTerrain *terrain, gboolean favorite);

#endif /* _VFR_TERRAIN_H */
