/*
 * (C) Copyright 2019, Arnaud Ferraris <arnaud.ferraris@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0
 */

#include "aircraft.h"

#include <math.h>
#include <json-glib/json-glib.h>

struct _VFRAircraft {
    GString *id;
    GString *manufacturer;
    GString *model;
    GString *label;
    gdouble empty_weight;
    gdouble mtow_weight;
    gint64 cruising_speed;
    GPtrArray *checklists;
};

typedef struct {
    GPtrArray *list;
    VFRAircraft *current;
    guint current_index;
} VFRAircraftList;

static VFRAircraftList *aircraft_list = NULL;

static VFRAircraft *vfr_aircraft_load_from_file(const gchar *filename)
{
    JsonNode *root;
    JsonNode *node;
    JsonObject *object;
    JsonArray *array;
    JsonParser *parser = json_parser_new();
    VFRAircraft *aircraft = NULL;
    VFRChecklist *checklist = NULL;
    guint i, checklist_count;

    if (!json_parser_load_from_file(parser, filename, NULL))
        return NULL;

    aircraft = g_malloc0(sizeof(VFRAircraft));

    root = json_parser_get_root(parser);
    object = json_node_get_object(root);

    aircraft->id = g_string_new(json_object_get_string_member(object, "id"));
    aircraft->manufacturer = g_string_new(json_object_get_string_member(object, "manufacturer"));
    aircraft->model = g_string_new(json_object_get_string_member(object, "model"));
    aircraft->empty_weight = json_object_get_double_member(object, "empty_weight");
    aircraft->mtow_weight = json_object_get_double_member(object, "mtow");
    aircraft->cruising_speed = json_object_get_int_member(object, "cruising_speed");
    if (g_str_equal(json_object_get_string_member(object, "speed_unit"), "kph"))
        aircraft->cruising_speed /= 1.852;

    array = json_object_get_array_member(object, "checklists");
    checklist_count = json_array_get_length(array);
    aircraft->checklists = g_ptr_array_sized_new(checklist_count);
    for (guint i = 0; i < checklist_count; i++) {
        node = json_array_get_element(array, i);
        object = json_node_get_object(node);
        checklist = vfr_checklist_load(json_object_get_string_member(object, "id"));
        g_ptr_array_add(aircraft->checklists, checklist);
    }

    aircraft->label = g_string_new(aircraft->manufacturer->str);
    g_string_append_printf(aircraft->label, " %s", aircraft->model->str);

    g_object_unref(parser);

    return aircraft;
}

static gboolean vfr_aircraft_load()
{
    VFRAircraft *aircraft;
    GString *aircraft_path;
    GString *aircraft_file;
    GDir *aircraft_dir;
    const gchar *current_file;

    if (!aircraft_list)
        return FALSE;

    aircraft_path = g_string_new(g_get_user_config_dir());
    g_string_append(aircraft_path, "/librevfr/aircrafts");
    aircraft_dir = g_dir_open(aircraft_path->str, 0, NULL);

    while(current_file = g_dir_read_name(aircraft_dir), current_file != NULL) {
        aircraft_file = g_string_new(aircraft_path->str);
        g_string_append_printf(aircraft_file, "/%s", current_file);

        if (g_file_test(aircraft_file->str, G_FILE_TEST_IS_REGULAR)) {
            aircraft = vfr_aircraft_load_from_file(aircraft_file->str);
            if (aircraft)
                g_ptr_array_add(aircraft_list->list, aircraft);
        }

        g_string_free(aircraft_file, TRUE);
    }

    g_dir_close(aircraft_dir);
    g_string_free(aircraft_path, TRUE);

    return TRUE;
}

gboolean vfr_aircraft_init()
{
    aircraft_list = g_malloc0(sizeof(VFRAircraftList));
    aircraft_list->list = g_ptr_array_new();

    return vfr_aircraft_load();
}

guint vfr_aircraft_get_count()
{
    if (aircraft_list)
        return aircraft_list->list->len;

    return 0;
}

VFRAircraft *vfr_aircraft_get(guint index)
{
    if (aircraft_list && aircraft_list->list->len > index)
        return aircraft_list->list->pdata[index];

    return NULL;
}


const gchar *vfr_aircraft_get_label(VFRAircraft *aircraft)
{
    if (aircraft)
        return aircraft->label->str;

    return NULL;
}

gdouble vfr_aircraft_get_base_factor(VFRAircraft *aircraft)
{
    if (aircraft)
        return round((60. / (gdouble)aircraft->cruising_speed) * 10) / 10;

    return 0.;
}

guint vfr_aircraft_get_checklist_count(VFRAircraft *aircraft)
{
    if (aircraft)
        return aircraft->checklists->len;

    return 0;
}

VFRChecklist *vfr_aircraft_get_checklist(VFRAircraft *aircraft, guint index)
{
    if (aircraft && index < aircraft->checklists->len)
        return aircraft->checklists->pdata[index];

    return NULL;
}
