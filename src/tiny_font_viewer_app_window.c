#include "tiny_font_viewer_app_window.h"
#include "tiny_font_viewer_app.h"
#include <gtk/gtk.h>

struct _TinyFontViewerAppWindow
{
  GtkApplicationWindow parent;
};

G_DEFINE_TYPE (TinyFontViewerAppWindow, tiny_font_viewer_app_window, GTK_TYPE_APPLICATION_WINDOW);

static void
tiny_font_viewer_app_window_init (TinyFontViewerAppWindow *app)
{
}

static void
tiny_font_viewer_app_window_class_init (TinyFontViewerAppWindowClass *class)
{
}

TinyFontViewerAppWindow *
tiny_font_viewer_app_window_new (TinyFontViewerApp *app)
{
  return g_object_new (TINY_FONT_VIEWER_APP_WINDOW_TYPE, "application", app,
                       NULL);
}

void
tiny_font_viewer_app_window_open (TinyFontViewerAppWindow *win,
                                  GFile *file)
{
}
