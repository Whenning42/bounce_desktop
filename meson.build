project('bounce_desktop', 'cpp',
  version: '0.1.6', # if changed, update pyproject.toml too.
  default_options: ['cpp_std=c++20',
                     'cpp_eh=none',
                     'cpp_rtti=false',
                     'warning_level=3',
                     'werror=true'])

gtest_dep = dependency('gtest', main: true)
gmock_dep = dependency('gmock')
sdl2_dep = dependency('sdl2')
gvnc_dep = dependency('gvnc-1.0')

# Note: Avoid non-test uses of libvncclient/libvncserver to avoid GPL issues.
vncserver_dep = dependency('libvncserver', static: false)

test_deps = [gtest_dep, gmock_dep, gvnc_dep]

bouncedesk_sources = [
  'src/desktop/client.cpp',
  'src/desktop/weston_backend.cpp',
  'src/reaper/reaper.cpp',
  'src/process/process.cpp',
  'src/weston/display_vars.cpp',
  'src/weston/launch_weston.cpp',
  'src/vnc_test/mock_vnc_server.cpp',
  'src/desktop/sdl_viewer.cpp',
  'src/process/env_vars.cpp',
  'src/process/fd.cpp',
  'src/process/process_helpers.cpp',
  'src/process/stream.cpp'
]

bouncedesk_lib = static_library('bouncedesk',
  bouncedesk_sources,
  include_directories: include_directories('src'),
  dependencies: [gvnc_dep, vncserver_dep, sdl2_dep],
)

reaper_sources = [
  'src/reaper/reaper.cpp',
  'src/reaper/impl.cpp',
  'src/reaper/cleanup.cpp',
  'src/process/process.cpp',
  'src/process/env_vars.cpp',
  'src/process/fd.cpp',
  'src/process/process_helpers.cpp',
  'src/process/stream.cpp'
]

reaper = executable('reaper',
  ['src/reaper/impl_main.cpp'] + reaper_sources,
  include_directories: include_directories('src'),
)

client_test = executable('client_test',
  'src/desktop/client_test.cpp',
  include_directories: include_directories('src'),
  link_with: bouncedesk_lib,
  dependencies: test_deps,
)

reaper_test = executable('reaper_test',
  ['src/reaper/reaper_test.cpp'] + reaper_sources,
  include_directories: include_directories('src'),
  link_with: bouncedesk_lib,
  dependencies: test_deps,
)

ipc_test = executable('ipc_test',
  'src/reaper/ipc_test.cpp',
  include_directories: include_directories('src'),
  dependencies: test_deps,
)

reaper_tests_reaper_parent = executable('reaper_tests_reaper_parent',
  'src/reaper/tests/reaper_parent.cpp',
  include_directories: include_directories('src'),
  link_with: bouncedesk_lib,
)

display_vars_test = executable('display_vars_test',
  ['src/weston/display_vars_test.cpp', 'src/weston/display_vars.cpp'],
  include_directories: include_directories('src'),
  dependencies: test_deps,
)

process_test = executable('process_test',
  ['src/process/process_test.cpp',
  'src/process/process.cpp',
  'src/process/env_vars.cpp',
  'src/process/fd.cpp',
  'src/process/process_helpers.cpp',
  'src/process/stream.cpp'],
  include_directories: include_directories('src'),
  dependencies: test_deps,
)

launch_weston_test = executable('launch_weston_test',
  ['src/weston/launch_weston_test.cpp', 'src/weston/launch_weston.cpp'],
  include_directories: include_directories('src'),
  link_with: bouncedesk_lib,
  dependencies: test_deps,
)

export_display = executable('export_display',
  ['src/weston/export_display_main.cpp', 'src/weston/display_vars.cpp'],
  include_directories: include_directories('src'),
  install_dir: 'bounce_desktop/bin',
  install: true
)

integration_test = executable('integration_test', ['src/test/integration_test_main.cpp'], include_directories: include_directories('src'), link_with: bouncedesk_lib, dependencies: [gvnc_dep])
multi_instance_integration_test = executable('multi_instance_integration_test', ['src/test/multi_instance_integration_test_main.cpp'], include_directories: include_directories('src'), link_with: bouncedesk_lib, dependencies: [gvnc_dep])

test('client_test', client_test, workdir: meson.project_source_root())
test('reaper_test', reaper_test, workdir: meson.project_source_root())
test('ipc_test', ipc_test, workdir: meson.project_source_root())
test('display_vars_test', display_vars_test, workdir: meson.project_source_root())
test('process_test', process_test, workdir: meson.project_source_root())
test('launch_weston_test', launch_weston_test, workdir: meson.project_source_root())

# Python extension module
subdir('bounce_desktop')
