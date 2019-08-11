/*
 * (C) Copyright 2019, Arnaud Ferraris <arnaud.ferraris@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0
 */

#include "provider-basulm.h"

#include "provider-basulm-apikey.h"

#include "utils.h"

#include <stdlib.h>
#include <unistd.h>

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <curl/curl.h>

#include <glib/gstdio.h>
#include <json-glib/json-glib.h>

static gboolean basulm_check_dirs(VFRProvider *self)
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

static CURL *basulm_create_request(struct curl_slist **headers)
{
    struct curl_slist *slist;
    GString *str = g_string_new(NULL);
    CURL *curl = curl_easy_init();

    slist = curl_slist_append(NULL, "Authorization: api_key " BASULM_APIKEY);
    slist = curl_slist_append(slist, "Cache-Control: no-cache");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, slist);

    *headers = slist;

    return curl;
}

static GString *basulm_read_latest_version(VFRProvider *self)
{
    GString *version_file = g_string_new(g_get_user_data_dir());
    GString *version = NULL;
    gchar *file_contents = NULL;

    g_string_append_printf(version_file, "/librevfr/%s/version", vfr_provider_get_id(self));

    if (g_file_get_contents(version_file->str, &file_contents, NULL, NULL))
        version = g_string_new(file_contents);

    g_string_free(version_file, TRUE);
    if (file_contents)
        g_free(file_contents);

    return version;
}

static gboolean basulm_needs_update(VFRProvider *self)
{
    GString *data_version = g_string_new(g_get_user_data_dir());
    GString *data_list = g_string_new(g_get_user_data_dir());
    GString *current_version = vfr_get_current_date();
    GString *latest_version = basulm_read_latest_version(self);
    gboolean result = FALSE;

    g_string_append_printf(data_version, "/librevfr/%s/version", vfr_provider_get_id(self));
    g_string_append_printf(data_list, "/librevfr/%s/list", vfr_provider_get_id(self));

    if (!basulm_check_dirs(self) ||
        g_access(data_version->str, F_OK) < 0 ||
        g_access(data_list->str, F_OK) < 0) {
        result = TRUE;
    }

    if (!latest_version || !g_string_equal(latest_version, current_version))
        result = TRUE;

    g_string_free(data_version, TRUE);
    g_string_free(data_list, TRUE);
    g_string_free(current_version, TRUE);
    if (latest_version)
        g_string_free(latest_version, TRUE);

    return result;
}

static gboolean basulm_load_terrains(VFRProvider *self)
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

static gboolean basulm_write_list(VFRProvider *self)
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

static gboolean basulm_update_list(VFRProvider *self)
{
    struct curl_slist *headers;
    CURL *curl = basulm_create_request(&headers);

    GString *tmpfile = g_string_new(g_get_tmp_dir());
    GString *url = g_string_new("https://basulm.ffplum.fr/getbasulm/get/basulm/liste");
    FILE *file;

    JsonNode *root;
    JsonNode *node;
    JsonObject *object;
    JsonArray *array;
    JsonParser *parser = json_parser_new();

    guint count;

    g_string_append_printf(tmpfile, "/librevfr-%s-list", vfr_provider_get_id(self));
    file = g_fopen(tmpfile->str, "w+");

    curl_easy_setopt(curl, CURLOPT_URL, url->str);
    g_string_free(url, TRUE);

    curl_easy_setopt(curl, CURLOPT_WRITEDATA, file);
    curl_easy_perform(curl);
    fclose(file);

    // Parse data
    if (!json_parser_load_from_file(parser, tmpfile->str, NULL))
        return FALSE;

    g_remove(tmpfile->str);
    g_string_free(tmpfile, TRUE);

    root = json_parser_get_root(parser);
    object = json_node_get_object(root);

    if (!g_str_equal(json_object_get_string_member(object, "status"), "ok"))
        return FALSE;

    array = json_object_get_array_member(object, "liste");
    count = json_array_get_length(array);
    for (guint i = 0; i < count; i++) {
        VFRTerrain *terrain;
        const gchar *name;
        const gchar *code;

        node = json_array_get_element(array, i);
        object = json_node_get_object(node);
        name = json_object_get_string_member(object, "toponyme");
        code = json_object_get_string_member(object, "code_terrain");

        terrain = vfr_terrain_new(name, code, FALSE);
        vfr_provider_add_terrain(self, terrain);
    }

    g_object_unref(parser);

    return basulm_write_list(self);
}

static gboolean basulm_update_terrains(VFRProvider *self)
{
    CURL *curl = curl_easy_init();
    GString *current_date = vfr_get_current_date();
    GString *data_version = g_string_new(g_get_user_data_dir());
    GString *data_dir = g_string_new(g_get_user_data_dir());
    GString *base_url = g_string_new("https://basulm.ffplum.fr/PDF/");
    GString *vacfile = g_string_new(NULL);
    GString *url = g_string_new(NULL);
    FILE *file;

    g_string_append_printf(data_version, "/librevfr/%s/version", vfr_provider_get_id(self));
    g_string_append_printf(data_dir, "/librevfr/%s/files", vfr_provider_get_id(self));

    basulm_update_list(self);

    for (guint i = 0; i < vfr_provider_get_terrain_count(self); i++) {
        VFRTerrain *terrain = vfr_provider_get_terrain_by_index(self, i);

        g_string_printf(vacfile, "%s/%s.pdf", data_dir->str, vfr_terrain_get_icao(terrain));
        file = g_fopen(vacfile->str, "w");

        g_string_printf(url, "%s/%s.pdf", base_url->str, vfr_terrain_get_icao(terrain));

        curl_easy_setopt(curl, CURLOPT_URL, url->str);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, file);
        curl_easy_perform(curl);

        fclose(file);
    }

    curl_easy_cleanup(curl);

    file = g_fopen(data_version->str, "w");
    fprintf(file, "%s", current_date->str);
    fclose(file);

    return TRUE;
}

VFRProvider *vfr_provider_basulm_init(void)
{
    VFRProvider *self = vfr_provider_new("BASULM France", "basulm");

    vfr_provider_set_callbacks(self, basulm_needs_update, basulm_update_terrains, basulm_load_terrains);

    return self;
}
