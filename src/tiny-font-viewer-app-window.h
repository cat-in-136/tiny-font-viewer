#pragma once

#include "tiny-font-viewer-app.h"
#include <gtk/gtk.h>

#define TINY_FONT_VIEWER_APP_WINDOW_TYPE \
  (tiny_font_viewer_app_window_get_type ())
G_DECLARE_FINAL_TYPE (TinyFontViewerAppWindow, tiny_font_viewer_app_window, TINY_FONT_VIEWER, APP_WINDOW, GtkApplicationWindow)

TinyFontViewerAppWindow *
tiny_font_viewer_app_window_new (TinyFontViewerApp *app);
void tiny_font_viewer_app_window_show_preview (TinyFontViewerAppWindow *win,
                                               GFile *file,
                                               int face_index);
