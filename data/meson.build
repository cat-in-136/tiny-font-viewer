appdatadir = join_paths(datadir, 'metainfo')
appdata_file = 'io.github.cat-in-136.tiny-font-viewer.appdata.xml'
appdata_conf = configuration_data()
appdata_conf.set('APPLICATION_ID', application_id)
appdata_conf.set('GETTEXT_PACKAGE', meson.project_name())
merged_appdata = i18n.merge_file(
  input: configure_file(input: appdata_file + '.in.in',
    output: appdata_file + '.in',
    configuration: appdata_conf),
  output: appdata_file,
  po_dir: '../po',
  install: true,
  install_dir: appdatadir
)

appstreamcli = find_program('appstreamcli', required: false)
if appstreamcli.found()
  test(
    'validate-appdata',
    appstreamcli,
    args: ['validate', '--no-net', '--explain', merged_appdata.full_path()]
  )
endif

desktop_file = 'io.github.cat-in-136.tiny-font-viewer.desktop'

desktop_conf = configuration_data()
desktop_conf.set('APPLICATION_ID', application_id)
i18n.merge_file(
  input: configure_file(input: desktop_file + '.in.in',
    output: desktop_file + '.in',
    configuration: desktop_conf),
  output: desktop_file,
  po_dir: '../po',
  install: true,
  install_dir: desktopdir,
  type: 'desktop'
)
