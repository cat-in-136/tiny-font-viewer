sources = [
  'open-type-layout.h',
  'sushi-font-loader.h',
  'sushi-font-loader.c',
  'sushi-font-widget.h',
  'sushi-font-widget.c',
  'tiny-font-viewer-app.h',
  'tiny-font-viewer-app.c',
  'tiny-font-viewer-app-window.h',
  'tiny-font-viewer-app-window.c',
  'main.c',
]

resources = gnome.compile_resources('tiny-font-viewer-resources',
  'tiny-font-viewer.gresource.xml',
  source_dir: '.',
  c_name: 'tiny_font_viewer')

executable('fontviewer', sources + resources,
  include_directories: [ root_inc, include_directories('.') ],
  dependencies : [
    glib_dep,
    gtk_dep,
    harfbuzz_dep,
    freetype2_dep,
    fribidi_dep,
  ],
  install: true)
