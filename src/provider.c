/*
 * (C) Copyright 2019, Arnaud Ferraris <arnaud.ferraris@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0
 */

#include "provider.h"
#include "provider-sia.h"
#include "provider-basulm.h"

#include <stdio.h>

struct _VFRProvider {
    GString *name;
    GString *id;

    GPtrArray *terrains;

    vfr_provider_cb needs_update;
    vfr_provider_cb update_terrains;
    vfr_provider_cb load_terrains;
};

GPtrArray *vfr_provider_init(void)
{
    GPtrArray *array = g_ptr_array_new();

    g_ptr_array_add(array, vfr_provider_sia_init());
    g_ptr_array_add(array, vfr_provider_basulm_init());

    for (guint i = 0; i < array->len; i++) {
        VFRProvider *provider = array->pdata[i];

        if (provider->needs_update(provider)) {

            printf("%s (ID %s) needs update\n", vfr_provider_get_name(provider),
                                                vfr_provider_get_id(provider));
            provider->update_terrains(provider);
        }

        provider->load_terrains(provider);
    }

    return array;
}

VFRProvider *vfr_provider_new(const gchar *name, const gchar *id)
{
    VFRProvider *provider = g_malloc0(sizeof(VFRProvider));

    provider->name = g_string_new(name);
    provider->id = g_string_new(id);
    provider->terrains = g_ptr_array_new();

    return provider;
}

const gchar *vfr_provider_get_name(VFRProvider *provider)
{
    if (provider)
        return provider->name->str;

    return NULL;
}

const gchar *vfr_provider_get_id(VFRProvider *provider)
{
    if (provider)
        return provider->id->str;

    return NULL;
}

guint vfr_provider_get_terrain_count(VFRProvider *provider)
{
    if (provider)
        return provider->terrains->len;

    return 0;
}

VFRTerrain *vfr_provider_get_terrain_by_name(VFRProvider *provider, GString *name)
{
    return NULL;
}

VFRTerrain *vfr_provider_get_terrain_by_icao(VFRProvider *provider, GString *icao)
{
    return NULL;
}

VFRTerrain *vfr_provider_get_terrain_by_index(VFRProvider *provider, int index)
{
    if (provider)
        return provider->terrains->pdata[index];

    return NULL;
}

void vfr_provider_add_terrain(VFRProvider *provider, VFRTerrain *terrain)
{
    if (provider && terrain)
        g_ptr_array_add(provider->terrains, terrain);
}

void vfr_provider_set_callbacks(VFRProvider *provider, vfr_provider_cb needs_update,
                                vfr_provider_cb update_terrains, vfr_provider_cb load_terrains)
{
    if (provider) {
        provider->needs_update = needs_update;
        provider->update_terrains = update_terrains;
        provider->load_terrains = load_terrains;
    }
}
