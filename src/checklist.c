/*
 * (C) Copyright 2019, Arnaud Ferraris <arnaud.ferraris@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0
 */

#include "checklist.h"

#include <json-glib/json-glib.h>

struct _VFRChecklist {
    GString *id;
    GString *name;
    GPtrArray *items;
    GPtrArray *values;
};

VFRChecklist *vfr_checklist_load(const gchar *id)
{
    GString *file;
    JsonNode *root;
    JsonNode *node;
    JsonObject *object;
    JsonArray *array;
    JsonParser *parser = json_parser_new();
    VFRChecklist *checklist = NULL;
    guint i, item_count;

    file = g_string_new(g_get_user_config_dir());
    g_string_append_printf(file, "/librevfr/aircrafts/checklists/%s.json", id);
    if (!json_parser_load_from_file(parser, file->str, NULL))
        return NULL;

    checklist = g_malloc0(sizeof(VFRChecklist));

    root = json_parser_get_root(parser);
    object = json_node_get_object(root);

    checklist->id = g_string_new(id);
    checklist->name = g_string_new(json_object_get_string_member(object, "checklist"));

    array = json_object_get_array_member(object, "items");
    item_count = json_array_get_length(array);
    checklist->items = g_ptr_array_sized_new(item_count);
    checklist->values = g_ptr_array_sized_new(item_count);
    for (guint i = 0; i < item_count; i++) {
        GString *item;
        GString *value;

        node = json_array_get_element(array, i);
        object = json_node_get_object(node);

        item = g_string_new(json_object_get_string_member(object, "item"));
        g_ptr_array_add(checklist->items, item);
        value = g_string_new(json_object_get_string_member(object, "value"));
        g_ptr_array_add(checklist->values, value);
    }

    g_object_unref(parser);

    return checklist;
}

guint vfr_checklist_get_length(VFRChecklist *checklist)
{
    if (checklist)
        return checklist->items->len;

    return 0;
}

const gchar *vfr_checklist_get_name(VFRChecklist *checklist)
{
    const gchar *str = NULL;

    if (checklist)
        str = checklist->name->str;

    return str;
}

const gchar *vfr_checklist_get_item(VFRChecklist *checklist, const guint index)
{
    const gchar *str = NULL;

    if (checklist && checklist->items->len > index) {
        GString *g_str = checklist->items->pdata[index];
        str = g_str->str;
    }

    return str;
}

const gchar *vfr_checklist_get_value(VFRChecklist *checklist, const guint index)
{
    const gchar *str = NULL;

    if (checklist && checklist->values->len > index) {
        GString *g_str = checklist->values->pdata[index];
        str = g_str->str;
    }

    return str;
}
