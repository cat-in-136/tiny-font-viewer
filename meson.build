project('tiny-font-viewer', 'c',
        version: '0.0.0',
        license: 'GPLv2',
        meson_version: '>= 1.3'
)

cc = meson.get_compiler('c')

gnome = import('gnome')
i18n = import('i18n')

prefix = get_option('prefix')

bindir = join_paths(prefix, get_option('bindir'))
datadir = join_paths(prefix, get_option('datadir'))
libexecdir = join_paths(prefix, get_option('libexecdir'))
localedir = join_paths(prefix, get_option('localedir'))

pkgdatadir = join_paths(datadir, meson.project_name())

#desktopdir = join_paths(datadir, 'applications')
#servicesdir = join_paths(datadir, 'dbus-1', 'services')

glib_req_version = '>= 2.78.0'
gtk_req_version = '>= 4.12.0'
harfbuzz_req_version = '>= 8.2.0'

glib_dep = dependency('glib-2.0', version: glib_req_version)
gtk_dep = dependency('gtk4', version: gtk_req_version)
harfbuzz_dep = dependency('harfbuzz', version: harfbuzz_req_version)
freetype2_dep = dependency('freetype2')
fribidi_dep = dependency('fribidi')

application_id = 'io.github.cat-in-136.font-viewer'

config_h = configuration_data()
config_h.set_quoted('VERSION', meson.project_version())
config_h.set_quoted('GETTEXT_PACKAGE', meson.project_name())
config_h.set_quoted('APPLICATION_ID', application_id)
configure_file(output: 'config.h', configuration: config_h)

root_inc = include_directories('.')

subdir('src')
