/*
 * (C) Copyright 2019, Arnaud Ferraris <arnaud.ferraris@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0
 */

#include "provider.h"

#include "provider-sia.h"
#include "provider-basulm.h"

#include <unistd.h>

#include <glib/gstdio.h>

struct _VFRProvider {
    GString *name;
    GString *id;

    GPtrArray *terrains;

    vfr_provider_cb needs_update;
    vfr_provider_cb update_terrains;
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

        vfr_provider_load_terrains(provider);
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
                                vfr_provider_cb update_terrains)
{
    if (provider) {
        provider->needs_update = needs_update;
        provider->update_terrains = update_terrains;
    }
}

gboolean vfr_provider_check_dirs(VFRProvider *self)
{
    GString *data_dir = g_string_new(g_get_user_data_dir());

    g_string_append_printf(data_dir, "/librevfr/%s/files", vfr_provider_get_id(self));

    if (g_access(data_dir->str, F_OK) < 0) {
        g_mkdir_with_parents(data_dir->str, 0755);
        g_string_free(data_dir, TRUE);
        return FALSE;
    }

    return TRUE;
}

gboolean vfr_provider_write_list(VFRProvider *self)
{
    GString *data_list = g_string_new(g_get_user_data_dir());
    char *line = NULL;
    size_t size = 0;
    gboolean favorite;
    FILE *file;
    int len;

    g_string_append_printf(data_list, "/librevfr/%s/list", vfr_provider_get_id(self));
    file = g_fopen(data_list->str, "w");
    if (!file)
        return FALSE;

    for (guint i = 0; i < vfr_provider_get_terrain_count(self); i++) {
        VFRTerrain *terrain = vfr_provider_get_terrain_by_index(self, i);

        fprintf(file, "%s;%s;%d\n", vfr_terrain_get_name(terrain),
                                    vfr_terrain_get_icao(terrain),
                                    vfr_terrain_is_favorite(terrain));
    }

    return TRUE;
}

gboolean vfr_provider_load_terrains(VFRProvider *self)
{
    GString *data_list = g_string_new(g_get_user_data_dir());
    char *line = NULL;
    size_t size = 0;
    gboolean favorite;
    FILE *file;
    int len;

    g_string_append_printf(data_list, "/librevfr/%s/list", vfr_provider_get_id(self));
    file = g_fopen(data_list->str, "r");
    if (!file)
        return FALSE;

    while ((len = getline(&line, &size, file)) && !feof(file)) {
        char **split;
        VFRTerrain *terrain;

        line[len-1] = 0;
        split = g_strsplit(line, ";", 0);
        if (split && split[0] && split[1]) {
            if (split[2] && split[2][0] == '1')
                favorite = TRUE;
            else
                favorite = FALSE;
            terrain = vfr_terrain_new(split[0], split[1], favorite);
            vfr_provider_add_terrain(self, terrain);
            g_strfreev(split);
        }
    }
    free(line);

    return TRUE;
}
