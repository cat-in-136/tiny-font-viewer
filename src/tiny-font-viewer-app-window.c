/* tiny-font-viewer-app-window.c
 *
 * Copyright (C) 2024 @cat_in_136
 *
 * Based on gnome-font-viewer code (font-view-window.c) by:
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

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_TYPE1_TABLES_H
#include FT_SFNT_NAMES_H
#include FT_TRUETYPE_IDS_H
#include FT_MULTIPLE_MASTERS_H

#include "open-type-layout.h"
#include "sushi-font-widget.h"
#include "tiny-font-viewer-app-window.h"
#include "tiny-font-viewer-app.h"
#include <fontconfig/fontconfig.h>
#include <freetype2/ft2build.h>
#include <gdk/gdk.h>
#include <gio/gio.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <hb-ft.h>
#include <hb-ot.h>
#include <hb.h>

#define WHITESPACE_CHARS "\f \t"
#define MATCH_VERSION_STR "Version"
#define FixedToFloat(f) (((float) (f)) / 65536.0)

struct _TinyFontViewerAppWindow
{
  GtkApplicationWindow parent;

  GtkSpinButton *face_index_spin;
  GtkAdjustment *face_index_adjustment;
  GtkToggleButton *info_button;
  GtkScrolledWindow *swin_preview;
  GtkViewport *viewport_preview;
  GtkScrolledWindow *swin_info;
  GtkGrid *grid_info;
  SushiFontWidget *font_widget;

  GtkShortcutController *shortcut_controller;

  GFile *font_file;
};

G_DEFINE_TYPE (TinyFontViewerAppWindow, tiny_font_viewer_app_window, GTK_TYPE_APPLICATION_WINDOW);

static void
face_index_spin_value_changed_cb (GtkSpinButton *button, gpointer user_data)
{
  TinyFontViewerAppWindow *const win = TINY_FONT_VIEWER_APP_WINDOW (user_data);
  const int face_index = gtk_spin_button_get_value (win->face_index_spin);

  g_object_set (win->font_widget,
                "face-index", face_index,
                NULL);
  sushi_font_widget_load (SUSHI_FONT_WIDGET (win->font_widget));
}

static char *
preview_visible_child_closure (TinyFontViewerAppWindow *win,
                               gboolean info_active)
{
  return g_strdup (info_active ? "info" : "preview");
}

static void
strip_whitespace (gchar **original)
{
  g_strstrip (*original);

  g_auto (GStrv) split = NULL;
  g_autoptr (GString) reassembled = NULL;
  const gchar *str;
  gint idx, n_stripped;
  size_t len;

  split = g_strsplit (*original, "\n", -1);
  reassembled = g_string_new (NULL);
  n_stripped = 0;

  for (idx = 0; split[idx] != NULL; idx++)
    {
      str = split[idx];

      len = strspn (str, WHITESPACE_CHARS);
      if (len)
        str += len;

      if (strlen (str) == 0 &&
          ((split[idx + 1] == NULL) || strlen (split[idx + 1]) == 0))
        continue;

      if (n_stripped++ > 0)
        g_string_append (reassembled, "\n");
      g_string_append (reassembled, str);
    }

  g_free (*original);
  *original = g_strdup (reassembled->str);
}

static void
strip_version (gchar **original)
{
  gchar *ptr, *stripped;

  ptr = g_strstr_len (*original, -1, MATCH_VERSION_STR);
  if (!ptr)
    return;

  ptr += strlen (MATCH_VERSION_STR);
  stripped = g_strdup (ptr);

  strip_whitespace (&stripped);

  g_free (*original);
  *original = stripped;
}

static void
add_to_grid_info (
    GtkGrid *grid,
    const gchar *name,
    const gchar *value,
    gboolean multiline)
{
  GtkWidget *name_w, *label;
  GtkWidget *const last_child = gtk_widget_get_last_child (GTK_WIDGET (grid));
  int row = 0;

  if (last_child != NULL)
    {
      gtk_grid_query_child (grid, last_child, NULL, &row, NULL, NULL);
      row++; // use next row
    }

  name_w = gtk_label_new (name);
  gtk_widget_add_css_class (GTK_WIDGET (name_w), "dim-label");
  gtk_widget_set_halign (name_w, GTK_ALIGN_END);
  gtk_widget_set_valign (name_w, GTK_ALIGN_START);
  gtk_grid_attach (grid,
                   name_w,
                   0, row, 1, 1);

  label = gtk_label_new (value);
  gtk_widget_set_halign (label, GTK_ALIGN_START);
  gtk_widget_set_valign (label, GTK_ALIGN_START);
  gtk_label_set_selectable (GTK_LABEL (label), TRUE);
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_label_set_ellipsize (GTK_LABEL (label), PANGO_ELLIPSIZE_END);
  gtk_label_set_max_width_chars (GTK_LABEL (label), 64);
  gtk_grid_attach_next_to (grid,
                           label,
                           name_w,
                           GTK_POS_RIGHT,
                           1, 1);

  gtk_label_set_ellipsize (GTK_LABEL (label), PANGO_ELLIPSIZE_END);
  gtk_label_set_max_width_chars (GTK_LABEL (label), 64);

  if (multiline && g_utf8_strlen (value, -1) > 64)
    {
      gtk_label_set_width_chars (GTK_LABEL (label), 64);
      gtk_label_set_lines (GTK_LABEL (label), 10);

      {
        const gchar *p = value;
        int i = 0;
        while (p)
          {
            p = strchr (p + 1, '\n');
            i++;
          }
        if (i > 3)
          { /* multi-paragraph text */
            gtk_label_set_ellipsize (GTK_LABEL (label), PANGO_ELLIPSIZE_NONE);
            gtk_label_set_lines (GTK_LABEL (label), -1);
          }
      }
    }
}

static char *
describe_axis (FT_Var_Axis *ax)
{
  /* Translators, this string is used to display information about
   * a 'font variation axis'. The %s gets replaced with the name
   * of the axis, for example 'Width'. The three %g get replaced
   * with the minimum, maximum and default values for the axis.
   */
  return g_strdup_printf (_ ("%s %g — %g, default %g"), ax->name,
                          FixedToFloat (ax->minimum),
                          FixedToFloat (ax->maximum), FixedToFloat (ax->def));
}

static char *
get_sfnt_name (FT_Face face,
               guint id)
{
  guint count, i;

  count = FT_Get_Sfnt_Name_Count (face);
  for (i = 0; i < count; i++)
    {
      FT_SfntName sname;

      if (FT_Get_Sfnt_Name (face, i, &sname) != 0)
        continue;

      if (sname.name_id != id)
        continue;

      /* only handle the unicode names for US langid */
      if (!(sname.platform_id == TT_PLATFORM_MICROSOFT &&
            sname.encoding_id == TT_MS_ID_UNICODE_CS &&
            sname.language_id == TT_MS_LANGID_ENGLISH_UNITED_STATES))
        continue;

      return g_convert ((gchar *) sname.string, sname.string_len, "UTF-8",
                        "UTF-16BE", NULL, NULL, NULL);
    }
  return NULL;
}

/* According to the OpenType spec, valid values for the subfamilyId field
 * of InstanceRecords are 2, 17 or values in the range (255,32768). See
 * https://www.microsoft.com/typography/otspec/fvar.htm#instanceRecord
 */
static gboolean
is_valid_subfamily_id (guint id)
{
  return id == 2 || id == 17 || (255 < id && id < 32768);
}

static void
describe_instance (FT_Face face,
                   FT_Var_Named_Style *ns,
                   int pos,
                   GString *s)
{
  g_autofree char *str = NULL;

  if (is_valid_subfamily_id (ns->strid))
    str = get_sfnt_name (face, ns->strid);

  if (str == NULL)
    str = g_strdup_printf (_ ("Instance %d"), pos);

  if (s->len > 0)
    g_string_append (s, ", ");
  g_string_append (s, str);
}

static char *
get_features (FT_Face face)
{
  g_autoptr (GString) s = NULL;
  hb_font_t *hb_font;
  int i, j, k;

  s = g_string_new ("");

  hb_font = hb_ft_font_create (face, NULL);
  if (hb_font)
    {
      hb_tag_t tables[2] = { HB_OT_TAG_GSUB, HB_OT_TAG_GPOS };
      hb_face_t *hb_face;

      hb_face = hb_font_get_face (hb_font);

      for (i = 0; i < 2; i++)
        {
          hb_tag_t features[80];
          unsigned int count = G_N_ELEMENTS (features);
          unsigned int script_index = 0;
          unsigned int lang_index = HB_OT_LAYOUT_DEFAULT_LANGUAGE_INDEX;

          hb_ot_layout_language_get_feature_tags (hb_face, tables[i],
                                                  script_index, lang_index, 0,
                                                  &count, features);
          for (j = 0; j < count; j++)
            {
              for (k = 0; k < G_N_ELEMENTS (open_type_layout_features); k++)
                {
                  if (open_type_layout_features[k].tag == features[j])
                    {
                      if (s->len > 0)
                        /* Translators, this seperates the list of Layout
                         * Features. */
                        g_string_append (s, C_ ("OpenType layout", ", "));
                      g_string_append (
                          s,
                          g_dpgettext2 (NULL, "OpenType layout",
                                        open_type_layout_features[k].name));
                      break;
                    }
                }
            }
        }
    }

  if (s->len > 0)
    return g_strdup (s->str);

  return NULL;
}

static void
populate_grid (TinyFontViewerAppWindow *win,
               FT_Face face)
{
  g_autoptr (GFileInfo) info = NULL;
  g_autofree gchar *path = NULL;
  PS_FontInfoRec ps_info;

  add_to_grid_info (win->grid_info, _ ("Name"), face->family_name, FALSE);

  path = g_file_get_path (win->font_file);
  add_to_grid_info (win->grid_info, _ ("Location"), path, FALSE);

  if (face->style_name)
    add_to_grid_info (win->grid_info, _ ("Style"), face->style_name, FALSE);

  info = g_file_query_info (win->font_file,
                            G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE
                            "," G_FILE_ATTRIBUTE_STANDARD_SIZE,
                            G_FILE_QUERY_INFO_NONE, NULL, NULL);

  if (info != NULL)
    {
      g_autofree gchar *s = g_content_type_get_description (
          g_file_info_get_content_type (info));
      add_to_grid_info (win->grid_info, _ ("Type"), s, FALSE);
    }

  if (FT_IS_SFNT (face))
    {
      gint i, len;
      g_autofree gchar *version = NULL, *copyright = NULL,
                       *description = NULL;
      g_autofree gchar *designer = NULL, *manufacturer = NULL,
                       *license = NULL;

      len = FT_Get_Sfnt_Name_Count (face);
      for (i = 0; i < len; i++)
        {
          FT_SfntName sname;

          if (FT_Get_Sfnt_Name (face, i, &sname) != 0)
            continue;

          /* only handle the unicode names for US langid */
          if (!(sname.platform_id == TT_PLATFORM_MICROSOFT &&
                sname.encoding_id == TT_MS_ID_UNICODE_CS &&
                sname.language_id == TT_MS_LANGID_ENGLISH_UNITED_STATES))
            continue;

          switch (sname.name_id)
            {
            case TT_NAME_ID_COPYRIGHT:
              if (!copyright)
                copyright =
                    g_convert ((gchar *) sname.string, sname.string_len,
                               "UTF-8", "UTF-16BE", NULL, NULL, NULL);
              break;
            case TT_NAME_ID_VERSION_STRING:
              if (!version)
                version =
                    g_convert ((gchar *) sname.string, sname.string_len,
                               "UTF-8", "UTF-16BE", NULL, NULL, NULL);
              break;
            case TT_NAME_ID_DESCRIPTION:
              if (!description)
                description =
                    g_convert ((gchar *) sname.string, sname.string_len,
                               "UTF-8", "UTF-16BE", NULL, NULL, NULL);
              break;
            case TT_NAME_ID_MANUFACTURER:
              if (!manufacturer)
                manufacturer =
                    g_convert ((gchar *) sname.string, sname.string_len,
                               "UTF-8", "UTF-16BE", NULL, NULL, NULL);
              break;
            case TT_NAME_ID_DESIGNER:
              if (!designer)
                designer =
                    g_convert ((gchar *) sname.string, sname.string_len,
                               "UTF-8", "UTF-16BE", NULL, NULL, NULL);
              break;
            case TT_NAME_ID_LICENSE:
              if (!license)
                license =
                    g_convert ((gchar *) sname.string, sname.string_len,
                               "UTF-8", "UTF-16BE", NULL, NULL, NULL);
              break;
            default:
              break;
            }
        }
      if (version)
        {
          strip_version (&version);
          add_to_grid_info (win->grid_info, _ ("Version"), version, FALSE);
        }
      if (copyright)
        {
          strip_whitespace (&copyright);
          add_to_grid_info (win->grid_info, _ ("Copyright"), copyright, TRUE);
        }
      if (description)
        {
          strip_whitespace (&description);
          add_to_grid_info (win->grid_info, _ ("Description"), description, TRUE);
        }
      if (manufacturer)
        {
          strip_whitespace (&manufacturer);
          add_to_grid_info (win->grid_info, _ ("Manufacturer"), manufacturer, TRUE);
        }
      if (designer)
        {
          strip_whitespace (&designer);
          add_to_grid_info (win->grid_info, _ ("Designer"), designer, TRUE);
        }
      if (license)
        {
          strip_whitespace (&license);
          add_to_grid_info (win->grid_info, _ ("License"), license, TRUE);
        }
    }
  else if (FT_Get_PS_Font_Info (face, &ps_info) == 0)
    {
      if (ps_info.version && g_utf8_validate (ps_info.version, -1, NULL))
        {
          g_autofree gchar *compressed = g_strcompress (ps_info.version);
          strip_version (&compressed);
          add_to_grid_info (win->grid_info, _ ("Version"), compressed, FALSE);
        }
      if (ps_info.notice && g_utf8_validate (ps_info.notice, -1, NULL))
        {
          g_autofree gchar *compressed = g_strcompress (ps_info.notice);
          strip_whitespace (&compressed);
          add_to_grid_info (win->grid_info, _ ("Copyright"), compressed, TRUE);
        }
    }
}

static void
populate_details (TinyFontViewerAppWindow *win,
                  FT_Face face)
{
  g_autofree gchar *glyph_count = NULL, *features = NULL;
  FT_MM_Var *ft_mm_var;

  glyph_count = g_strdup_printf ("%ld", face->num_glyphs);
  add_to_grid_info (win->grid_info, _ ("Glyph Count"), glyph_count, FALSE);

  add_to_grid_info (win->grid_info, _ ("Color Glyphs"),
                    FT_HAS_COLOR (face) ? _ ("yes") : _ ("no"), FALSE);

  features = get_features (face);
  if (features)
    add_to_grid_info (win->grid_info, _ ("Layout Features"), features, TRUE);

  if (FT_Get_MM_Var (face, &ft_mm_var) == 0)
    {
      int i;
      for (i = 0; i < ft_mm_var->num_axis; i++)
        {
          g_autofree gchar *s = describe_axis (&ft_mm_var->axis[i]);
          add_to_grid_info (win->grid_info, i == 0 ? _ ("Variation Axes") : "", s, FALSE);
        }
      {
        g_autoptr (GString) str = g_string_new ("");
        for (i = 0; i < ft_mm_var->num_namedstyles; i++)
          describe_instance (face, &ft_mm_var->namedstyle[i], i, str);

        add_to_grid_info (win->grid_info, _ ("Named Styles"), str->str, TRUE);
      }
      free (ft_mm_var);
    }
}

static void
action_go_next_face_index (GSimpleAction *action, GVariant *parameter, gpointer win)
{
  gtk_spin_button_spin (TINY_FONT_VIEWER_APP_WINDOW (win)->face_index_spin,
                        GTK_SPIN_STEP_FORWARD, 1);
}

static void
action_go_prev_face_index (GSimpleAction *action, GVariant *parameter, gpointer win)
{
  gtk_spin_button_spin (TINY_FONT_VIEWER_APP_WINDOW (win)->face_index_spin,
                        GTK_SPIN_STEP_BACKWARD, 1);
}

static const GActionEntry win_entries[] = {
  { "go-next-face-index", action_go_next_face_index, NULL, NULL, NULL },
  { "go-prev-face-index", action_go_prev_face_index, NULL, NULL, NULL },
};

static void
tiny_font_viewer_app_window_init (TinyFontViewerAppWindow *win)
{
  gtk_widget_init_template (GTK_WIDGET (win));

  g_action_map_add_action_entries (G_ACTION_MAP (win), win_entries,
                                   G_N_ELEMENTS (win_entries), win);
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

  gtk_widget_class_bind_template_child (widget_class, TinyFontViewerAppWindow, face_index_spin);
  gtk_widget_class_bind_template_child (widget_class, TinyFontViewerAppWindow, face_index_adjustment);
  gtk_widget_class_bind_template_child (widget_class, TinyFontViewerAppWindow, info_button);
  gtk_widget_class_bind_template_child (widget_class, TinyFontViewerAppWindow, swin_preview);
  gtk_widget_class_bind_template_child (widget_class, TinyFontViewerAppWindow, viewport_preview);
  gtk_widget_class_bind_template_child (widget_class, TinyFontViewerAppWindow, swin_info);
  gtk_widget_class_bind_template_child (widget_class, TinyFontViewerAppWindow, grid_info);

  gtk_widget_class_bind_template_callback (widget_class, face_index_spin_value_changed_cb);
  gtk_widget_class_bind_template_callback (widget_class, preview_visible_child_closure);
}

TinyFontViewerAppWindow *
tiny_font_viewer_app_window_new (TinyFontViewerApp *app)
{
  return g_object_new (TINY_FONT_VIEWER_APP_WINDOW_TYPE,        //
                       "application", app,                      //
                       "show-menubar", TRUE,                    //
                       "icon-name", TINY_FONT_VIEWER_ICON_NAME, //
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
  g_autofree gchar *title = NULL;

  if (face == NULL)
    {
      return;
    }

  uri = sushi_font_widget_get_uri (font_widget);
  win->font_file = g_file_new_for_uri (uri);

  if (face->family_name)
    {
      title = g_strdup_printf ("%s — %s", face->family_name, _ ("Font Viewer"));
    }
  else
    {
      g_autofree gchar *basename = g_file_get_basename (win->font_file);
      title = g_strdup_printf ("%s — %s", basename, _ ("Font Viewer"));
    }
  gtk_window_set_title (GTK_WINDOW (win), title);

  // Show face_index spin button in case of font collection
  if (face->num_faces > 1)
    {
      gtk_adjustment_configure (win->face_index_adjustment,
                                face->face_index & 0xFFFF,
                                0,
                                face->num_faces,
                                1,
                                1,
                                1);

      gtk_widget_set_visible (GTK_WIDGET (win->face_index_spin), TRUE);
    }
  else
    {
      gtk_widget_set_visible (GTK_WIDGET (win->face_index_spin), FALSE);
    }

  // clear info grid
  while (gtk_widget_get_last_child (GTK_WIDGET (win->grid_info)))
    {
      gtk_grid_remove (win->grid_info, gtk_widget_get_last_child (GTK_WIDGET (win->grid_info)));
    }

  populate_grid (win, face);
  populate_details (win, face);
}

static void
font_widget_error_cb (TinyFontViewerAppWindow *win,
                      GError *error,
                      SushiFontWidget *font_widget)
{
  show_error (win, _ ("Could Not Display Font"), error->message);
}

gboolean
tiny_font_viewer_app_window_is_file_opened (TinyFontViewerAppWindow *win)
{
  return (win->font_widget != NULL);
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
