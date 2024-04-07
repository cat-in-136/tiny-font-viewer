sources = [
  'tiny_font_viewer_app.c',
  'tiny_font_viewer_app_window.c',
  'main.c',
]

executable('fontviewer', sources,
  include_directories: [ root_inc, include_directories('.') ],
  dependencies : [
    gtk_dep,
  ],
  install: true)
