/*
 * (C) Copyright 2019, Arnaud Ferraris <arnaud.ferraris@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0
 */

#include "provider-sia.h"

#include "utils.h"

#include <stdlib.h>
#include <unistd.h>

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <curl/curl.h>

#include <glib/gstdio.h>

static gboolean sia_check_dirs(VFRProvider *self)
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

static gboolean sia_needs_update(VFRProvider *self)
{
    GString *data_airac = g_string_new(g_get_user_data_dir());
    GString *data_list = g_string_new(g_get_user_data_dir());
    GString *current_airac = NULL;
    char cached_airac[16];
    FILE *file;

    g_string_append_printf(data_airac, "/librevfr/%s/version", vfr_provider_get_id(self));
    g_string_append_printf(data_list, "/librevfr/%s/list", vfr_provider_get_id(self));

    if (!sia_check_dirs(self) ||
        g_access(data_airac->str, F_OK) < 0 ||
        g_access(data_list->str, F_OK) < 0) {
        goto update_needed;
    }

    file = g_fopen(data_airac->str, "r");
    if (!file)
        goto update_needed;

    memset(cached_airac, 0, 16);
    fread(cached_airac, 16, 1, file);
    fclose(file);

    current_airac = vfr_get_current_airac();
    if (!g_str_equal(cached_airac, current_airac->str))
        goto update_needed;

    return FALSE;

update_needed:
    g_string_free(data_airac, TRUE);
    g_string_free(data_list, TRUE);
    if (current_airac)
        g_string_free(current_airac, TRUE);
    return TRUE;
}

static gboolean sia_load_terrains(VFRProvider *self)
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

static gboolean sia_write_list(VFRProvider *self)
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

static gboolean sia_update_list(VFRProvider *self)
{
    CURL *curl = curl_easy_init();
    GString *tmpfile = g_string_new(g_get_tmp_dir());
    GString *url = g_string_new("https://www.sia.aviation-civile.gouv.fr/dvd/eAIP_");
    GString *airac = vfr_get_current_airac();
    char *line = NULL;
    size_t len = 0;
    char **icao;
    char **name;
    char *start;
    char *end;
    FILE *file;

    g_string_append_printf(tmpfile, "/librevfr-%s-list", vfr_provider_get_id(self));
    file = g_fopen(tmpfile->str, "w+");

    g_string_append_printf(url, "%s/Atlas-VAC/Javascript/AeroArraysVac.js", airac->str);
    curl_easy_setopt(curl, CURLOPT_URL, url->str);
    g_string_free(url, TRUE);

    curl_easy_setopt(curl, CURLOPT_WRITEDATA, file);
    curl_easy_perform(curl);

    fseek(file, 0, SEEK_SET);
    getline(&line, &len, file);
    start = strchr(line, '\"') + 1;
    end = strrchr(line, '\"');
    *end = 0;
    icao = g_strsplit(start, "\",\"", 0);
    getline(&line, &len, file);
    start = strchr(line, '\"') + 1;
    end = strrchr(line, '\"');
    *end = 0;
    name = g_strsplit(start, "\",\"", 0);
    free(line);
    fclose(file);
    g_remove(tmpfile->str);
    g_string_free(tmpfile, TRUE);

    len = 0;
    while (icao[len]) {
        VFRTerrain *terrain;

        for (size_t i = 1; i < strlen(name[len]); i++) {
            if (isupper(name[len][i]) && name[len][i-1] != ' ')
                name[len][i] += 0x20;
        }

        terrain = vfr_terrain_new(name[len], icao[len], FALSE);
        vfr_provider_add_terrain(self, terrain);
        len++;
    }

    g_strfreev(icao);
    g_strfreev(name);

    return sia_write_list(self);
}

static gboolean sia_update_terrains(VFRProvider *self)
{
    CURL *curl = curl_easy_init();
    GString *current_airac = vfr_get_current_airac();
    GString *data_airac = g_string_new(g_get_user_data_dir());
    GString *data_dir = g_string_new(g_get_user_data_dir());
    GString *base_url = g_string_new("https://www.sia.aviation-civile.gouv.fr/dvd/eAIP_");
    GString *vacfile = g_string_new(NULL);
    GString *url = g_string_new(NULL);
    FILE *file;

    g_string_append_printf(data_airac, "/librevfr/%s/version", vfr_provider_get_id(self));
    g_string_append_printf(data_dir, "/librevfr/%s/files", vfr_provider_get_id(self));
    g_string_append_printf(base_url, "%s/Atlas-VAC/PDF_AIPparSSection/VAC/AD", current_airac->str);

    sia_update_list(self);

    for (guint i = 0; i < vfr_provider_get_terrain_count(self); i++) {
        VFRTerrain *terrain = vfr_provider_get_terrain_by_index(self, i);

        g_string_printf(vacfile, "%s/%s.pdf", data_dir->str, vfr_terrain_get_icao(terrain));
        file = g_fopen(vacfile->str, "w");

        g_string_printf(url, "%s/AD-2.%s.pdf", base_url->str, vfr_terrain_get_icao(terrain));

        curl_easy_setopt(curl, CURLOPT_URL, url->str);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, file);
        curl_easy_perform(curl);

        fclose(file);
    }

    curl_easy_cleanup(curl);

    file = g_fopen(data_airac->str, "w");
    fprintf(file, "%s", current_airac->str);
    fclose(file);

    return TRUE;
}

VFRProvider *vfr_provider_sia_init(void)
{
    VFRProvider *self = vfr_provider_new("VAC France", "sia");

    vfr_provider_set_callbacks(self, sia_needs_update, sia_update_terrains, sia_load_terrains);

    return self;
}
