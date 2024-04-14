#include "tiny-font-viewer-app.h"
#include "config.h"
#include "tiny-font-viewer-app-window.h"
#include <glib/gi18n.h>
#include <gtk/gtk.h>

struct _TinyFontViewerApp
{
  GtkApplication parent;
};

G_DEFINE_TYPE (TinyFontViewerApp, tiny_font_viewer_app, GTK_TYPE_APPLICATION);

static void
tiny_font_viewer_app_init (TinyFontViewerApp *app)
{
}

static void
action_about (GSimpleAction *action, GVariant *parameter, gpointer app)
{
  const GList *windows = NULL;
  TinyFontViewerAppWindow *win = NULL;

  // find window
  windows = gtk_application_get_windows (GTK_APPLICATION (app));
  g_return_if_fail (windows != NULL);
  win = TINY_FONT_VIEWER_APP_WINDOW (windows->data);

  static const char *authors[] = {
    "@cat_in_136",
    NULL
  };

  gtk_show_about_dialog (GTK_WINDOW (win),
                         "version", VERSION,
                         "authors", authors,
                         "program-name", _ ("Font Viewer"),
                         "comments", _ ("View font files"),
                         "logo-icon-name", TINY_FONT_VIEWER_ICON_NAME,
                         //"translator-credits", _ ("translator-credits"),
                         "license-type", GTK_LICENSE_GPL_2_0,
                         "wrap-license", TRUE,
                         "website", "https://github.com/cat-in-136/tiny-font-viewer",
                         NULL);
}

static void
action_quit (GSimpleAction *action, GVariant *parameter, gpointer app)
{
  g_application_quit (G_APPLICATION (app));
}

static const GActionEntry app_entries[] = {
  { "about", action_about, NULL, NULL, NULL },
  { "quit", action_quit, NULL, NULL, NULL }
};

static void
tiny_font_viewer_app_startup (GApplication *app)
{
  g_autoptr (GtkBuilder) builder = gtk_builder_new_from_resource (
      "/io/github/cat-in-136/tiny-font-viewer/tiny-font-viewer-app-menu.ui");

  G_APPLICATION_CLASS (tiny_font_viewer_app_parent_class)->startup (app);

  g_action_map_add_action_entries (G_ACTION_MAP (app), app_entries,
                                   G_N_ELEMENTS (app_entries), app);
  gtk_application_set_menubar (GTK_APPLICATION (app),
                               G_MENU_MODEL (gtk_builder_get_object (builder, "menubar")));
}

static TinyFontViewerAppWindow *
create_blank_window (GApplication *app)
{
  const GList *windows = NULL;
  TinyFontViewerAppWindow *win = NULL;

  // reuse the existing window if blank window exists
  windows = gtk_application_get_windows (GTK_APPLICATION (app));
  for (const GList *l = windows; l != NULL; l = l->next)
    {
      if (!tiny_font_viewer_app_window_is_file_opened (TINY_FONT_VIEWER_APP_WINDOW (l->data)))
        {
          win = TINY_FONT_VIEWER_APP_WINDOW (l->data);
          break;
        }
    }

  if (!win)
    {
      win = tiny_font_viewer_app_window_new (TINY_FONT_VIEWER_APP (app));
    }

  gtk_window_present (GTK_WINDOW (win));

  return win;
}

static void
tiny_font_viewer_app_activate (GApplication *app)
{
  create_blank_window (app);
}

static void
tiny_font_viewer_app_open (GApplication *app, GFile **files, int n_files, const char *hint)
{
  for (int i = 0; i < n_files; i++)
    {
      TinyFontViewerAppWindow *const win = create_blank_window (app);
      tiny_font_viewer_app_window_show_preview (win, files[i], 0);
    }
}

static void
tiny_font_viewer_app_class_init (TinyFontViewerAppClass *klass)
{
  G_APPLICATION_CLASS (klass)->startup = tiny_font_viewer_app_startup;
  G_APPLICATION_CLASS (klass)->activate = tiny_font_viewer_app_activate;
  G_APPLICATION_CLASS (klass)->open = tiny_font_viewer_app_open;
}

TinyFontViewerApp *
tiny_font_viewer_app_new (void)
{
  return g_object_new (TINY_FONT_VIEWER_APP_TYPE,           //
                       "application-id", APPLICATION_ID,    //
                       "flags", G_APPLICATION_HANDLES_OPEN, //
                       NULL);
}
