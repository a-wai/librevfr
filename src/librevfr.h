/*
 * (C) Copyright 2019, Arnaud Ferraris <arnaud.ferraris@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0
 */

#ifndef _VFR_LIBREVFR_H
#define _VFR_LIBREVFR_H

#include <gtk/gtk.h>
#define HANDY_USE_UNSTABLE_API
#include <handy.h>

G_BEGIN_DECLS

#define VFR_TYPE_MAIN_WINDOW (vfr_main_window_get_type())

G_DECLARE_FINAL_TYPE(VFRMainWindow, vfr_main_window, VFR, MAIN_WINDOW, GtkApplicationWindow)

VFRMainWindow *vfr_main_window_new(GtkApplication *application);

G_END_DECLS

#endif /* _VFR_LIBREVFR_H */
