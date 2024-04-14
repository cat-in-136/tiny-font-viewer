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
open_response_cb (GObject *source,
                  GAsyncResult *result,
                  gpointer user_data)
{
  GtkFileDialog *const dialog = GTK_FILE_DIALOG (source);
  TinyFontViewerApp *const app = TINY_FONT_VIEWER_APP (user_data);
  GListModel *const files = gtk_file_dialog_open_multiple_finish (dialog, result, NULL);
  if (files)
    {
      const size_t n_files = g_list_model_get_n_items (files);
      for (int i = 0; i < n_files; i++)
        {
          GFile *file = G_FILE (g_list_model_get_item (files, i));
          tiny_font_viewer_app_open_file (app, file);
        }
    }
}

static void
action_open (GSimpleAction *action, GVariant *parameter, gpointer app)
{
  g_autoptr (GtkFileDialog) dialog = NULL;
  g_autoptr (GListStore) filters = NULL;
  g_autoptr (GtkFileFilter) font_file_filter = NULL;

  dialog = gtk_file_dialog_new ();
  gtk_file_dialog_set_title (dialog, _ ("Open font files"));

  filters = g_list_store_new (GTK_TYPE_FILE_FILTER);

  font_file_filter = gtk_file_filter_new ();
  gtk_file_filter_add_mime_type (font_file_filter, "font/*");
  gtk_file_filter_set_name (font_file_filter, _ ("Fonts"));
  g_list_store_append (filters, font_file_filter);

  gtk_file_dialog_set_default_filter (dialog, font_file_filter);
  gtk_file_dialog_set_filters (dialog, G_LIST_MODEL (filters));

  gtk_file_dialog_open_multiple (dialog, NULL, NULL, open_response_cb, app);
}

static void
action_quit (GSimpleAction *action, GVariant *parameter, gpointer app)
{
  g_application_quit (G_APPLICATION (app));
}

static void
action_about (GSimpleAction *action, GVariant *parameter, gpointer app)
{
  GtkWindow *const win = gtk_application_get_active_window (GTK_APPLICATION (app));

  static const char *authors[] = {
    "@cat_in_136",
    NULL
  };

  gtk_show_about_dialog (win,
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

static const GActionEntry app_entries[] = {
  { "open", action_open, NULL, NULL, NULL },
  { "quit", action_quit, NULL, NULL, NULL },
  { "about", action_about, NULL, NULL, NULL }
};

static const struct
{
  const char *action;
  const char *accels[2];
} app_accelsp[] = {
  { "app.open", { "<Ctrl>O", NULL } },
  { "app.quit", { "<Ctrl>Q", NULL } },
  { "win.show-help-overlay", { "<Ctrl>question", NULL } }
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
  for (int i = 0; i < G_N_ELEMENTS (app_accelsp); i++)
    {
      gtk_application_set_accels_for_action (GTK_APPLICATION (app),
                                             app_accelsp[i].action,
                                             app_accelsp[i].accels);
    }
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
      tiny_font_viewer_app_open_file (TINY_FONT_VIEWER_APP (app), files[i]);
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

void
tiny_font_viewer_app_open_file (TinyFontViewerApp *app, GFile *file)
{
  TinyFontViewerAppWindow *const win = create_blank_window (G_APPLICATION (app));
  tiny_font_viewer_app_window_show_preview (win, file, 0);
}
