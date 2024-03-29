project(
  'vmp',
  'c',
  version: '0.92b',
  license: 'MIT',
  default_options: ['warning_level=3',
  		    'c_std=gnu99']
)

cc = meson.get_compiler('c')

egl = dependency('egl')
png = dependency('libpng')
wegl = dependency('wayland-egl')
math = cc.find_library('m')
sdl2 = dependency('sdl2')
glesv2 = dependency('glesv2')
freetype = dependency('freetype2')
xkbcommon = dependency('xkbcommon')

avutil = cc.find_library('avutil')
avcodec = cc.find_library('avcodec')
avdevice = cc.find_library('avdevice')
avformat = cc.find_library('avformat')
avfilter = cc.find_library('avfilter')
swscale = cc.find_library('swscale')
swresample = cc.find_library('swresample')

wayland_client      = dependency('wayland-client')
wayland_cursor      = dependency('wayland-cursor')
wayland_protos      = dependency('wayland-protocols')
wayland_scanner_dep = dependency('wayland-scanner')
wayland_scanner     = find_program(
  wayland_scanner_dep.get_pkgconfig_variable('wayland_scanner')
)

wl_protocol_dir = wayland_protos.get_pkgconfig_variable('pkgdatadir')

protocols = [
  [wl_protocol_dir, 'stable/xdg-shell/xdg-shell.xml'],
  [wl_protocol_dir, 'unstable/xdg-output/xdg-output-unstable-v1.xml'],
  [wl_protocol_dir, 'unstable/pointer-gestures/pointer-gestures-unstable-v1.xml'],
  'wlr-layer-shell-unstable-v1.xml'
]

protos_src = []
protos_headers = []

foreach p : protocols
  xml = join_paths(p)
  protos_src += custom_target(
    xml.underscorify() + '_client_c',
    input: xml,
    output: '@BASENAME@-protocol.c',
    command: [wayland_scanner, 'public-code', '@INPUT@', '@OUTPUT@'],
  )
  protos_headers += custom_target(
    xml.underscorify() + '_client_h',
    input: xml,
    output: '@BASENAME@-client-protocol.h',
    command: [wayland_scanner, 'client-header', '@INPUT@', '@OUTPUT@'],
  )
endforeach


vmp_dependencies = [
		  wayland_client,
		  wayland_cursor,
		  png,
		  freetype,
		  math,
		  sdl2,
		  glesv2,
		  avutil,
		  avcodec,
		  avdevice,
		  avformat,
		  avfilter,
		  swresample,
		  swscale,
		  xkbcommon,
		  egl,
		  wegl]

vmp_version = '"@0@"'.format(meson.project_version())
pkg_datadir = join_paths(get_option('prefix'), get_option('datadir')) / 'vmp'
add_project_arguments('-DPKG_DATADIR="' + pkg_datadir + '"',
		      '-DVMP_VERSION=@0@'.format(vmp_version),
		      '-Wno-unused-parameter',
		      language: 'c')

if get_option('buildtype') == 'debug'
    add_project_arguments('-DDEBUG',language: 'c')
endif

vmp_inc = include_directories(
	'src/mt_math',
	'src/mt_core',
	'src/media_player',
	'src/media_coder',
	'src/kinetic_ui',
	'src/kinetic_ui/egl',
	'src/kinetic_ui/handler',
	'src/kinetic_ui/texture',
	'src/vmp',
	'src/mt_core_ext')

if build_machine.system() == 'freebsd'
   epoll = dependency('epoll-shim')
   pthread = dependency('threads')
   vmp_dependencies += pthread
   vmp_dependencies += epoll
endif

com_sources = ['src/vmp/analyzer.c',
	       'src/vmp/config.c',
	       'src/vmp/filemanager.c',
	       'src/vmp/kvlist.c',
	       'src/vmp/library.c',
	       'src/vmp/remote.c',
	       'src/vmp/songlist.c',
	       'src/vmp/ui.c',
	       'src/vmp/vmp.c',
	       
	       'src/mt_core_ext/mt_string_ext.c',
	       'src/mt_core_ext/mt_map_ext.c',

	       'src/mt_core/mt_string.c',
	       'src/mt_core/mt_path.c',
	       'src/mt_core/mt_log.c',
	       'src/mt_core/mt_map.c',
	       'src/mt_core/mt_memory.c',
	       'src/mt_core/mt_number.c',
	       'src/mt_core/mt_time.c',
	       'src/mt_core/mt_vector.c',
	       'src/mt_core/mt_wrapper.c',

	       'src/mt_math/mt_matrix_4d.c',

	       'src/media_coder/coder.c',
	       'src/media_player/decoder.c',
	       'src/media_player/packetqueue.c',
	       'src/media_player/framequeue.c',
	       'src/media_player/clock.c',
	       'src/media_player/mediaplayer.c',

	       'src/kinetic_ui/ku_event.c',
	       'src/kinetic_ui/ku_html.c',
	       'src/kinetic_ui/ku_css.c',
	       'src/kinetic_ui/egl/ku_gl.c',
	       'src/kinetic_ui/ku_rect.c',
	       'src/kinetic_ui/ku_bitmap.c',
	       'src/kinetic_ui/texture/tg_css.c',
	       'src/kinetic_ui/texture/tg_knob.c',
	       'src/kinetic_ui/texture/tg_text.c',
	       'src/kinetic_ui/texture/tg_scaledimg.c',
	       'src/kinetic_ui/handler/vh_anim.c',
	       'src/kinetic_ui/handler/vh_button.c',
	       'src/kinetic_ui/handler/vh_drag.c',
	       'src/kinetic_ui/handler/vh_key.c',
	       'src/kinetic_ui/handler/vh_knob.c',
#	       'src/kinetic_ui/handler/vh_roll.c',
	       'src/kinetic_ui/handler/vh_slider.c',
	       'src/kinetic_ui/handler/vh_textinput.c',
	       'src/kinetic_ui/handler/vh_touch.c',
	       'src/kinetic_ui/handler/vh_cv_body.c',
	       'src/kinetic_ui/handler/vh_cv_scrl.c',
	       'src/kinetic_ui/handler/vh_cv_evnt.c',
	       'src/kinetic_ui/handler/vh_tbl_body.c',
	       'src/kinetic_ui/handler/vh_tbl_evnt.c',
	       'src/kinetic_ui/handler/vh_tbl_scrl.c',
	       'src/kinetic_ui/handler/vh_tbl_head.c',
	       'src/kinetic_ui/handler/vh_table.c',
	       'src/kinetic_ui/ku_view.c',
	       'src/kinetic_ui/egl/ku_gl_atlas.c',
	       'src/kinetic_ui/egl/ku_gl_floatbuffer.c',
	       'src/kinetic_ui/egl/ku_gl_shader.c',
	       'src/kinetic_ui/ku_gen_html.c',
	       'src/kinetic_ui/ku_gen_css.c',
	       'src/kinetic_ui/ku_gen_type.c',
	       'src/kinetic_ui/ku_renderer_egl.c',
	       'src/kinetic_ui/ku_renderer_soft.c',
	       'src/kinetic_ui/ku_fontconfig.c',
	       'src/kinetic_ui/ku_recorder.c',
	       'src/kinetic_ui/ku_connector_wayland.c',
	       'src/kinetic_ui/ku_window.c',
	       'src/kinetic_ui/ku_png.c',
	       'src/kinetic_ui/ku_draw.c',
	       'src/kinetic_ui/ku_text.c',
	       'src/kinetic_ui/ku_gen_textstyle.c'] + protos_src + protos_headers

pkg_datadir = join_paths(get_option('prefix'), get_option('datadir')) / 'vmp'
install_data('res/html/main.html', install_dir : pkg_datadir / 'html' )
install_data('res/html/main.css', install_dir : pkg_datadir / 'html' )
install_data('res/img/accept.png', install_dir : pkg_datadir / 'img' )
install_data('res/img/close.png', install_dir : pkg_datadir / 'img' )
install_data('res/img/closeblack.png', install_dir : pkg_datadir / 'img' )
install_data('res/img/filter.png', install_dir : pkg_datadir / 'img' )
install_data('res/img/max.png', install_dir : pkg_datadir / 'img' )
install_data('res/img/meta.png', install_dir : pkg_datadir / 'img' )
install_data('res/img/mute.png', install_dir : pkg_datadir / 'img' )
install_data('res/img/next.png', install_dir : pkg_datadir / 'img' )
install_data('res/img/pause.png', install_dir : pkg_datadir / 'img' )
install_data('res/img/play.png', install_dir : pkg_datadir / 'img' )
install_data('res/img/prev.png', install_dir : pkg_datadir / 'img' )
install_data('res/img/settings.png', install_dir : pkg_datadir / 'img' )
install_data('res/img/shuffle.png', install_dir : pkg_datadir / 'img' )
install_data('res/img/sorting.png', install_dir : pkg_datadir / 'img' )
install_data('res/img/unmute.png', install_dir : pkg_datadir / 'img' )
install_data('res/img/upload.png', install_dir : pkg_datadir / 'img' )
install_data('res/img/visu.png', install_dir : pkg_datadir / 'img' )

executable(
  'vmp',
  com_sources,
  include_directories: [vmp_inc],
  dependencies: vmp_dependencies,
  install: true
)

mt_core_test = executable(
	   'mt_core_test',
	   ['src/mt_core/mt_log.c',
	    'src/mt_core/mt_memory.c',
	    'src/mt_core/mt_vector.c',
	    'src/mt_core/mt_map.c',
	    'src/mt_core/mt_string.c',
	    'src/mt_core/mt_channel.c',
	    'src/mt_core/mt_path.c',
	    'src/mt_core_test/mt_core_test.c',
	    'src/mt_core_test/mt_memory_test.c',
	    'src/mt_core_test/mt_vector_test.c',
	    'src/mt_core_test/mt_map_test.c',
	    'src/mt_core_test/mt_string_test.c',
	    'src/mt_core_test/mt_channel_test.c',
	    'src/mt_core_test/mt_path_test.c'
	    ],
	    include_directories: ['src/mt_core'],
	    dependencies: vmp_dependencies)

test('MT_CORE_TEST', mt_core_test)

scripttests = find_program('tst/run.sh')

test('SCRIPTED TESTS',
      scripttests,
      timeout: 0,
      workdir : meson.current_source_dir(),
      args : ['BUILD_PATH=' + meson.current_build_dir(),
       	       'PROJECT_ROOT=' + meson.current_source_dir()])
