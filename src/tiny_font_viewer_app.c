#include "tiny_font_viewer_app.h"
#include "config.h"
#include "tiny_font_viewer_app_window.h"
#include <gtk/gtk.h>

struct _TinyFontViewerApp {
  GtkApplication parent;
};

G_DEFINE_TYPE(TinyFontViewerApp, tiny_font_viewer_app, GTK_TYPE_APPLICATION);

static void tiny_font_viewer_app_init(TinyFontViewerApp *app) {}

static void quit_activated(GSimpleAction *action, GVariant *parameter,
                           gpointer app) {
  g_application_quit(G_APPLICATION(app));
}

static const GActionEntry app_entries[] = {
    {"quit", quit_activated, NULL, NULL, NULL}};

static void tiny_font_viewer_app_startup(GApplication *app) {
  const char *quit_accels[2] = {"Escape", NULL};

  G_APPLICATION_CLASS(tiny_font_viewer_app_parent_class)->startup(app);

  g_action_map_add_action_entries(G_ACTION_MAP(app), app_entries,
                                  G_N_ELEMENTS(app_entries), app);
  gtk_application_set_accels_for_action(GTK_APPLICATION(app), "app.quit",
                                        quit_accels);
}

static void tiny_font_viewer_app_activate(GApplication *app) {
  TinyFontViewerAppWindow *win;

  win = tiny_font_viewer_app_window_new(TINY_FONT_VIEWER_APP(app));
  gtk_window_present(GTK_WINDOW(win));
}

static void tiny_font_viewer_app_open(GApplication *app, GFile **files,
                                      int n_files, const char *hint) {
  GList *windows;
  TinyFontViewerAppWindow *win;
  int i;

  windows = gtk_application_get_windows(GTK_APPLICATION(app));
  if (windows) {
    win = TINY_FONT_VIEWER_APP_WINDOW(windows->data);
  } else {
    win = tiny_font_viewer_app_window_new(TINY_FONT_VIEWER_APP(app));
  }
  for (i = 0; i < n_files; i++) {
    GFile *file = files[i];
    // TODO
  }
  gtk_window_present(GTK_WINDOW(win));
}

static void tiny_font_viewer_app_class_init(TinyFontViewerAppClass *class) {
  G_APPLICATION_CLASS(class)->startup = tiny_font_viewer_app_startup;
  G_APPLICATION_CLASS(class)->activate = tiny_font_viewer_app_activate;
  G_APPLICATION_CLASS(class)->open = tiny_font_viewer_app_open;
}

TinyFontViewerApp *tiny_font_viewer_app_new(void) {
  return g_object_new(TINY_FONT_VIEWER_APP_TYPE,           //
                      "application-id", APPLICATION_ID,    //
                      "flags", G_APPLICATION_HANDLES_OPEN, //
                      NULL);
}
