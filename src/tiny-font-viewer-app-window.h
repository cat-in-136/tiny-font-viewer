/* tiny-font-viewer-app-window.h
 *
 * Copyright (C) 2024 @cat_in_136
 *
 * Based on gnome-font-viewer code (font-view-window.h) by:
 *
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

#include "tiny-font-viewer-app.h"
#include <gtk/gtk.h>

#define TINY_FONT_VIEWER_APP_WINDOW_TYPE \
  (tiny_font_viewer_app_window_get_type ())
G_DECLARE_FINAL_TYPE (TinyFontViewerAppWindow, tiny_font_viewer_app_window, TINY_FONT_VIEWER, APP_WINDOW, GtkApplicationWindow)

TinyFontViewerAppWindow *
tiny_font_viewer_app_window_new (TinyFontViewerApp *app);

gboolean tiny_font_viewer_app_window_is_file_opened (TinyFontViewerAppWindow *win);
void tiny_font_viewer_app_window_show_preview (TinyFontViewerAppWindow *win,
                                               GFile *file,
                                               int face_index);
