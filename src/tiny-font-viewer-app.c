/* tiny-font-viewer-app.c
 *
 * Copyright (C) 2024 @cat_in_136
 *
 * Based on gnome-font-viewer code (font-view-application.c) by:
 *
 * Copyright (C) 2002-2003  James Henstridge <james@daa.com.au>
 * Copyright (C) 2010 Cosimo Cecchi <cosimoc@gnome.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "tiny-font-viewer-app.h"
#include "config.h"
#include "pango/pango-font.h"
#include "tiny-font-viewer-app-window.h"
#include <fontconfig/fontconfig.h>
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
action_toggle (GSimpleAction *action, GVariant *parameter, gpointer app)
{
  g_autoptr (GVariant) state = g_action_get_state (G_ACTION (action));
  g_action_change_state (G_ACTION (action), g_variant_new_boolean (!g_variant_get_boolean (state)));
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
          tiny_font_viewer_app_open_file (app, file, 0);
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
open_system_font_response_cb (GObject *source,
                              GAsyncResult *result,
                              gpointer user_data)
{
  GtkFontDialog *const dialog = GTK_FONT_DIALOG (source);
  TinyFontViewerApp *const app = TINY_FONT_VIEWER_APP (user_data);
  g_autoptr (GFile) file = NULL;
  int face_index = 0;

  PangoFontFace *const face = gtk_font_dialog_choose_face_finish (dialog, result, NULL);
  if (face)
    {
      FcObjectSet *os = FcObjectSetBuild (FC_FILE, FC_INDEX, NULL);
      FcFontSet *fontset = NULL;

      // find a font matching its family and style.
      {
        FcPattern *pat = FcPatternCreate ();

        PangoFontFamily *family = pango_font_face_get_family (face);
        const char *const familyname = pango_font_family_get_name (family);

        FcPatternAddString (pat, FC_FAMILY, (const FcChar8 *) familyname);
        fontset = FcFontList (NULL, pat, os);

        if (fontset->nfont > 1)
          {
            const char *const facename = pango_font_face_get_face_name (face);
            FcFontSetDestroy (fontset);
            FcPatternAddString (pat, FC_STYLE, (const FcChar8 *) facename);
            fontset = FcFontList (NULL, pat, os);
          }
        else
          {
            // If only one font matches with the family, it can assume legacy font having no style.
          }
        FcPatternDestroy (pat);
      }

      // find a font matching fullname if still not found.
      if (fontset->nfont == 0)
        {
          g_autoptr (PangoFontDescription) desc = pango_font_face_describe (face);
          g_autofree char *fullname = pango_font_description_to_string (desc);

          FcFontSetDestroy (fontset);

          FcPattern *pat = FcPatternCreate ();
          fontset = FcFontList (NULL, pat, os);
          FcPatternAddString (pat, FC_FULLNAME, (const FcChar8 *) fullname);
          FcPatternDestroy (pat);
        }

      if (fontset->nfont > 0)
        {
          const FcPattern *const matched = fontset->fonts[0];
          FcChar8 *path = NULL;

          FcPatternGetString (matched, FC_FILE, 0, &path);
          FcPatternGetInteger (matched, FC_INDEX, 0, &face_index);

          file = g_file_new_for_path ((const char *) path);
        }
      else
        {
          g_autoptr (PangoFontDescription) desc = pango_font_face_describe (face);
          g_autofree char *fullname = pango_font_description_to_string (desc);
          g_warning ("Font not found for %s", fullname);
        }
      FcFontSetDestroy (fontset);

      FcObjectSetDestroy (os);
    }

  if (file)
    {
      tiny_font_viewer_app_open_file (app, file, face_index);
    }
}

static void
action_open_system_font (GSimpleAction *action, GVariant *parameter, gpointer app)
{
  g_autoptr (GtkFontDialog) dialog = NULL;

  dialog = gtk_font_dialog_new ();
  gtk_font_dialog_set_title (dialog, _ ("Open system font"));

  gtk_font_dialog_choose_face (dialog, NULL, NULL, NULL, open_system_font_response_cb, app);
}

static void
action_quit (GSimpleAction *action, GVariant *parameter, gpointer app)
{
  g_application_quit (G_APPLICATION (app));
}

static void
dark_changed (GSimpleAction *action, GVariant *parameter, gpointer app)
{
  GtkSettings *settings = gtk_settings_get_default ();

  g_object_set (G_OBJECT (settings),
                "gtk-application-prefer-dark-theme",
                g_variant_get_boolean (parameter),
                NULL);

  g_simple_action_set_state (action, parameter);
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
  { "open-system-font", action_open_system_font, NULL, NULL, NULL },
  { "quit", action_quit, NULL, NULL, NULL },
  { "dark", action_toggle, NULL, "false", dark_changed },
  { "about", action_about, NULL, NULL, NULL }
};

static const struct
{
  const char *action;
  const char *accels[2];
} app_accelsp[] = {
  { "app.open", { "<Ctrl>O", NULL } },
  { "app.quit", { "<Ctrl>Q", NULL } },
  { "app.dark", { "<Ctrl>D", NULL } },
  { "win.go-next-face-index", { "<Alt>Right", NULL } },
  { "win.go-prev-face-index", { "<Alt>Left", NULL } },
  { "win.show-help-overlay", { "<Ctrl>question", NULL } }
};

static void
tiny_font_viewer_app_startup (GApplication *app)
{
  g_autoptr (GtkBuilder) builder = gtk_builder_new_from_resource (
      "/io/github/cat-in-136/tiny-font-viewer/tiny-font-viewer-app-menu.ui");

  G_APPLICATION_CLASS (tiny_font_viewer_app_parent_class)->startup (app);

  if (!FcInit ())
    {
      g_critical ("Can't initialize fontconfig library");
    }

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
      tiny_font_viewer_app_open_file (TINY_FONT_VIEWER_APP (app), files[i], 0);
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
tiny_font_viewer_app_open_file (TinyFontViewerApp *app, GFile *file, int face_index)
{
  TinyFontViewerAppWindow *const win = create_blank_window (G_APPLICATION (app));
  tiny_font_viewer_app_window_show_preview (win, file, face_index);
}
