/*
 * (C) Copyright 2019, Arnaud Ferraris <arnaud.ferraris@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0
 */

#include "flight.h"

#include <json-glib/json-glib.h>

struct _VFRFlight {
    GString *id;
    GString *name;
    GString *origin;
    GString *orig_icao;
    GString *destination;
    GString *dest_icao;
    GPtrArray *legs;

    GString *label;
};

typedef struct {
    GPtrArray *list;
    VFRFlight *current;
    guint current_index;
} VFRFlightList;

static VFRFlightList *flight_list = NULL;

static VFRFlight *vfr_flight_load_from_file(const gchar *filename)
{
    JsonNode *root;
    JsonNode *node;
    JsonObject *object;
    JsonArray *array;
    JsonParser *parser = json_parser_new();
    VFRFlight *flight = NULL;
    VFRFlightLeg *leg = NULL;
    guint i, leg_count;

    if (!json_parser_load_from_file(parser, filename, NULL))
        return NULL;

    flight = g_malloc0(sizeof(VFRFlight));

    root = json_parser_get_root(parser);
    object = json_node_get_object(root);

    flight->id = g_string_new(json_object_get_string_member(object, "id"));
    flight->name = g_string_new(json_object_get_string_member(object, "name"));
    flight->origin = g_string_new(json_object_get_string_member(object, "origin"));
    flight->orig_icao = g_string_new(json_object_get_string_member(object, "orig_icao"));
    flight->destination = g_string_new(json_object_get_string_member(object, "destination"));
    flight->dest_icao = g_string_new(json_object_get_string_member(object, "orig_icao"));

    array = json_object_get_array_member(object, "legs");
    leg_count = json_array_get_length(array);
    flight->legs = g_ptr_array_sized_new(leg_count);
    for (guint i = 0; i < leg_count; i++) {
        node = json_array_get_element(array, i);
        object = json_node_get_object(node);

        leg = g_malloc0(sizeof(VFRFlightLeg));
        leg->name = g_string_new(json_object_get_string_member(object, "name"));
        leg->heading = json_object_get_int_member(object, "heading");
        leg->distance = json_object_get_int_member(object, "distance");
        leg->altitude = json_object_get_int_member(object, "altitude");

        g_ptr_array_add(flight->legs, leg);
    }

    flight->label = g_string_new(flight->origin->str);
    g_string_append_printf(flight->label, " → %s", flight->destination->str);

    g_object_unref(parser);

    return flight;
}

static gboolean vfr_flight_load()
{
    VFRFlight *flight;
    GString *flight_path;
    GString *flight_file;
    GDir *flight_dir;
    const gchar *current_file;

    if (!flight_list)
        return FALSE;

    flight_path = g_string_new(g_get_user_config_dir());
    g_string_append(flight_path, "/librevfr/flights");
    flight_dir = g_dir_open(flight_path->str, 0, NULL);

    while(current_file = g_dir_read_name(flight_dir), current_file != NULL) {
        flight_file = g_string_new(flight_path->str);
        g_string_append_printf(flight_file, "/%s", current_file);

        if (g_file_test(flight_file->str, G_FILE_TEST_IS_REGULAR)) {
            flight = vfr_flight_load_from_file(flight_file->str);
            if (flight)
                g_ptr_array_add(flight_list->list, flight);
        }

        g_string_free(flight_file, TRUE);
    }

    g_dir_close(flight_dir);
    g_string_free(flight_path, TRUE);

    return TRUE;
}

gboolean vfr_flight_init()
{
    flight_list = g_malloc0(sizeof(VFRFlightList));
    flight_list->list = g_ptr_array_new();

    return vfr_flight_load();
}

guint vfr_flight_get_count()
{
    if (flight_list)
        return flight_list->list->len;

    return 0;
}

VFRFlight *vfr_flight_get(guint index)
{
    if (flight_list && flight_list->list->len > index)
        return flight_list->list->pdata[index];

    return NULL;
}


const gchar *vfr_flight_get_label(VFRFlight *flight)
{
    if (flight)
        return flight->label->str;

    return NULL;
}

const gchar *vfr_flight_get_name(VFRFlight *flight)
{
    if (flight)
        return flight->name->str;

    return NULL;
}

guint vfr_flight_get_leg_count(VFRFlight *flight)
{
    if (flight)
        return flight->legs->len;

    return 0;
}

VFRFlightLeg *vfr_flight_get_leg(VFRFlight *flight, guint index)
{
    if (flight && index < flight->legs->len)
        return flight->legs->pdata[index];

    return NULL;
}

GPtrArray *vfr_flight_get_legs(VFRFlight *flight)
{
    if (flight)
        return flight->legs;

    return NULL;
}
