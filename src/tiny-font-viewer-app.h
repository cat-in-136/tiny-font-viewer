/* tiny-font-viewer-app.h
 *
 * Copyright (C) 2024 @cat_in_136
 *
 * Based on gnome-font-viewer code (font-view-application.h) by:
 *
 * Copyright (C) 2002-2003  James Henstridge <james@daa.com.au>
 * Copyright (C) 2010 Cosimo Cecchi <cosimoc@gnome.org>
 * Copyright 2022 Christopher Davis <christopherdavis@gnome.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <gtk/gtk.h>

#define TINY_FONT_VIEWER_ICON_NAME "fonts"

#define TINY_FONT_VIEWER_APP_TYPE (tiny_font_viewer_app_get_type ())
G_DECLARE_FINAL_TYPE (TinyFontViewerApp, tiny_font_viewer_app, TINY_FONT_VIEWER, APP, GtkApplication)

TinyFontViewerApp *tiny_font_viewer_app_new (void);

void tiny_font_viewer_app_open_file (TinyFontViewerApp *app, GFile *file, int face_index);
