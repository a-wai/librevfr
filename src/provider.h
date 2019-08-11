/*
 * (C) Copyright 2019, Arnaud Ferraris <arnaud.ferraris@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0
 */

#ifndef _VFR_PROVIDER_H
#define _VFR_PROVIDER_H

#include <glib.h>

#include "terrain.h"

typedef struct _VFRProvider VFRProvider;

typedef gboolean (*vfr_provider_cb)(VFRProvider *provider);

GPtrArray *vfr_provider_init(void);

VFRProvider *vfr_provider_new(const gchar *name, const gchar *id);

VFRProvider *vfr_provider_register(VFRProvider *provider);

const gchar *vfr_provider_get_name(VFRProvider *provider);
const gchar *vfr_provider_get_id(VFRProvider *provider);

guint vfr_provider_get_terrain_count(VFRProvider *provider);
VFRTerrain *vfr_provider_get_terrain_by_name(VFRProvider *provider, GString *name);
VFRTerrain *vfr_provider_get_terrain_by_icao(VFRProvider *provider, GString *icao);
VFRTerrain *vfr_provider_get_terrain_by_index(VFRProvider *provider, int index);

void vfr_provider_add_terrain(VFRProvider *provider, VFRTerrain *terrain);

void vfr_provider_set_callbacks(VFRProvider *provider, vfr_provider_cb needs_update,
                                vfr_provider_cb update_terrains);

gboolean vfr_provider_check_dirs(VFRProvider *self);
gboolean vfr_provider_write_list(VFRProvider *self);
gboolean vfr_provider_load_terrains(VFRProvider *self);

#endif /* _VFR_PROVIDER_H */
