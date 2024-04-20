contrib_conf = configuration_data()
contrib_conf.set('VERSION', meson.project_version())
contrib_conf.set('APPLICATION_ID', application_id)
contrib_conf.set('GETTEXT_PACKAGE', meson.project_name())

rpmspec_file = 'tiny-font-viewer.spec'
configure_file(input: rpmspec_file + '.in',
  output: rpmspec_file,
  configuration: contrib_conf)
