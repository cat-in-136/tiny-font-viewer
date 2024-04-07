#pragma once

#include <gtk/gtk.h>

#define TINY_FONT_VIEWER_APP_TYPE (tiny_font_viewer_app_get_type ())
G_DECLARE_FINAL_TYPE (TinyFontViewerApp, tiny_font_viewer_app, TINY_FONT_VIEWER, APP, GtkApplication)

TinyFontViewerApp *tiny_font_viewer_app_new (void);
