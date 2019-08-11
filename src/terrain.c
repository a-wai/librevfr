/*
 * (C) Copyright 2019, Arnaud Ferraris <arnaud.ferraris@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0
 */

#include "terrain.h"

struct _VFRTerrain {
    GString *name;
    GString *icao;
    gboolean favorite;
};

VFRTerrain *vfr_terrain_new(const gchar *name, const gchar *icao, gboolean favorite)
{
    VFRTerrain *terrain = g_malloc0(sizeof(VFRTerrain));

    terrain->name = g_string_new(name);
    terrain->icao = g_string_new(icao);
    terrain->favorite = favorite;

    return terrain;
}

const gchar *vfr_terrain_get_name(VFRTerrain *terrain)
{
    if (terrain)
        return terrain->name->str;

    return NULL;
}

const gchar *vfr_terrain_get_icao(VFRTerrain *terrain)
{
    if (terrain)
        return terrain->icao->str;

    return NULL;
}

gboolean vfr_terrain_is_favorite(VFRTerrain *terrain)
{
    if (terrain)
        return terrain->favorite;

    return FALSE;
}

void vfr_terrain_set_favorite(VFRTerrain *terrain, gboolean favorite)
{
    if (terrain)
        terrain->favorite = favorite;
}
