#include "tiny-font-viewer-app-window.h"
#include "sushi-font-widget.h"
#include "tiny-font-viewer-app.h"
#include <glib/gi18n.h>
#include <gtk/gtk.h>

struct _TinyFontViewerAppWindow
{
  GtkApplicationWindow parent;

  GtkToggleButton *info_button;
  GtkScrolledWindow *swin_preview;
  GtkViewport *viewport_preview;
  SushiFontWidget *font_widget;

  GFile *font_file;
};

G_DEFINE_TYPE (TinyFontViewerAppWindow, tiny_font_viewer_app_window, GTK_TYPE_APPLICATION_WINDOW);

static void
tiny_font_viewer_app_window_init (TinyFontViewerAppWindow *win)
{
  gtk_widget_init_template (GTK_WIDGET (win));
}

static void
font_view_window_dispose (GObject *object)
{
  TinyFontViewerAppWindow *self = TINY_FONT_VIEWER_APP_WINDOW (object);

  gtk_widget_dispose_template (GTK_WIDGET (self), TINY_FONT_VIEWER_APP_WINDOW_TYPE);

  G_OBJECT_CLASS (tiny_font_viewer_app_window_parent_class)->dispose (object);
}

static void
tiny_font_viewer_app_window_class_init (TinyFontViewerAppWindowClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose = font_view_window_dispose;

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/io/github/cat-in-136/tiny-font-viewer/tiny-font-viewer-app-window.ui");

  gtk_widget_class_bind_template_child (widget_class, TinyFontViewerAppWindow, info_button);
  gtk_widget_class_bind_template_child (widget_class, TinyFontViewerAppWindow, swin_preview);
  gtk_widget_class_bind_template_child (widget_class, TinyFontViewerAppWindow, viewport_preview);
}

TinyFontViewerAppWindow *
tiny_font_viewer_app_window_new (TinyFontViewerApp *app)
{
  return g_object_new (TINY_FONT_VIEWER_APP_WINDOW_TYPE, "application", app,
                       NULL);
}

static void
show_error (TinyFontViewerAppWindow *win,
            const char *body,
            const char *detail)
{
  g_autofree GtkAlertDialog *alert = gtk_alert_dialog_new ("%s", body);
  gtk_alert_dialog_set_detail (alert, detail);
  gtk_alert_dialog_show (alert, GTK_WINDOW (win));
}

static void
font_widget_loaded_cb (TinyFontViewerAppWindow *win,
                       SushiFontWidget *font_widget)
{
  FT_Face face = sushi_font_widget_get_ft_face (font_widget);
  const gchar *uri;

  if (face == NULL)
    {
      return;
    }

  uri = sushi_font_widget_get_uri (font_widget);
  win->font_file = g_file_new_for_uri (uri);

  if (face->family_name)
    {
      gtk_window_set_title (GTK_WINDOW (win), face->family_name);
    }
  else
    {
      g_autofree gchar *basename = g_file_get_basename (win->font_file);
      gtk_window_set_title (GTK_WINDOW (win), basename);
    }

  //  load_font_info (win);
}

static void
font_widget_error_cb (TinyFontViewerAppWindow *win,
                      GError *error,
                      SushiFontWidget *font_widget)
{
  show_error (win, _ ("Could Not Display Font"), error->message);
}

void
tiny_font_viewer_app_window_show_preview (TinyFontViewerAppWindow *win,
                                          GFile *file,
                                          int face_index)
{
  g_autofree char *uri = g_file_get_uri (file);

  /* SushiFontWidget currently does not like for any of its properties to be
   * null on construction. Thus, we need to create it lazily.
   *
   * TODO: Refactor SushiFontWidget so that it can be included in the template.
   * */
  if (win->font_widget == NULL)
    {
      win->font_widget = sushi_font_widget_new (uri, face_index);

      gtk_widget_set_vexpand (GTK_WIDGET (win->font_widget), TRUE);

      gtk_viewport_set_child (win->viewport_preview, GTK_WIDGET (win->font_widget));

      g_signal_connect_swapped (win->font_widget, "loaded",
                                G_CALLBACK (font_widget_loaded_cb), win);
      g_signal_connect_swapped (win->font_widget, "error",
                                G_CALLBACK (font_widget_error_cb), win);
    }
  else
    {
      g_object_set (win->font_widget,
                    "uri", uri,
                    "face-index", face_index,
                    NULL);
      sushi_font_widget_load (SUSHI_FONT_WIDGET (win->font_widget));
    }

  gtk_toggle_button_set_active (win->info_button, FALSE);
}
