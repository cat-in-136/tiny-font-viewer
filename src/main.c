#include "tiny_font_viewer_app.h"
#include <gtk/gtk.h>

int main(int argc, char *argv[]) {
  return g_application_run(G_APPLICATION(tiny_font_viewer_app_new()), argc,
                           argv);
}
