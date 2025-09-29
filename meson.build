project('bouncedesk', 'cpp',
  version : '0.1.0',
  default_options : ['cpp_std=c++20',
                     'cpp_eh=none',
                     'cpp_rtti=false',
                     'warning_level=3',
                     'werror=true'])

gtest_dep = dependency('gtest', main : true)
gmock_dep = dependency('gmock')
sdl2_dep = dependency('sdl2')

# Note: We dynamically link GPL'd libvncclient/server.
# if this is a problem, we can migrate to gtk-vnc as our
# client and we can GPL our mock_vnc_server which is our
# only libvncserver consumer.
#
# TODO: Consider migrating onto gtk-vnc.
#
# gtk-vnc docs link: https://lazka.github.io/pgi-docs/GVnc-1.0/index.html
gvnc_dep = dependency('gvnc-1.0')
vncserver_dep = dependency('libvncserver', static: false)

test_deps = [gtest_dep, gmock_dep, gvnc_dep]

bouncedesk_sources = [
  'client.cpp',
  'wayland_backend.cpp',
  'reaper/reaper.cpp',
  'process.cpp',
  'display_vars.cpp',
  'launch_weston.cpp',
  'mock_vnc_server.cpp',
  'sdl_viewer.cpp',
  'process/env_vars.cpp',
  'process/fd.cpp',
  'process/process_helpers.cpp',
  'process/stream.cpp'
]

bouncedesk_lib = static_library('bouncedesk',
  bouncedesk_sources,
  dependencies : [gvnc_dep, vncserver_dep, sdl2_dep],
)

reaper_sources = [
  'reaper/reaper.cpp',
  'reaper/impl.cpp',
  'reaper/cleanup.cpp',
  'process.cpp',
  'process/env_vars.cpp',
  'process/fd.cpp',
  'process/process_helpers.cpp',
  'process/stream.cpp'
]

reaper = executable('reaper',
  ['reaper/impl_main.cpp'] + reaper_sources,
)

client_test = executable('client_test',
  'client_test.cpp',
  link_with : bouncedesk_lib,
  dependencies : test_deps,
)

reaper_test = executable('reaper_test',
  ['reaper/reaper_test.cpp'] + reaper_sources,
  link_with : bouncedesk_lib,
  dependencies : test_deps,
)

ipc_test = executable('ipc_test',
  'reaper/ipc_test.cpp',
  dependencies : test_deps,
)

reaper_tests_reaper_parent = executable('reaper_tests_reaper_parent',
  'reaper/tests/reaper_parent.cpp',
  link_with : bouncedesk_lib,
)

display_vars_test = executable('display_vars_test',
  ['display_vars_test.cpp', 'display_vars.cpp'],
  dependencies : test_deps,
)

process_test = executable('process_test',
  ['process_test.cpp',
  'process.cpp',
  'process/env_vars.cpp',
  'process/fd.cpp',
  'process/process_helpers.cpp',
  'process/stream.cpp'],
  dependencies : test_deps,
)

launch_weston_test = executable('launch_weston_test',
  ['launch_weston_test.cpp', 'launch_weston.cpp'],
  link_with : bouncedesk_lib,
  dependencies : test_deps,
)

export_display = executable('export_display',
  ['export_display_main.cpp', 'display_vars.cpp']
)

integration_test = executable('integration_test', ['integration_test_main.cpp'], link_with : bouncedesk_lib, dependencies: [gvnc_dep])

test('client_test', client_test, workdir : meson.project_source_root())
test('reaper_test', reaper_test, workdir : meson.project_source_root())
test('ipc_test', ipc_test, workdir : meson.project_source_root())
test('display_vars_test', display_vars_test, workdir : meson.project_source_root())
test('process_test', process_test, workdir : meson.project_source_root())
test('launch_weston_test', launch_weston_test, workdir : meson.project_source_root())

# Python extension module (optional)
python3 = import('python').find_installation('python3', required: false)
nanobind_dep = dependency('nanobind')

if python3.found() and nanobind_dep.found()
  py_cpp_args = []

  # Python extension sources
  py_sources = [
    'client_ext.cpp',
    'third_party/status/exceptions.h',
  ]
  
  # Build Python extension
  python3.extension_module('bounce_desk',
    py_sources,
    dependencies: [nanobind_dep, gvnc_dep],
    link_with: bouncedesk_lib,
    cpp_args: py_cpp_args,
    override_options: [
      'cpp_rtti=true',
      'cpp_eh=default',
      'warning_level=2',
    ]
  )
  
  # Python tests
  test('bounce_desk_test',
    python3,
    args: [files('bounce_desk_test.py')],
    env: {'PYTHONPATH': meson.current_build_dir()},
    workdir : meson.project_source_root()
  )
endif

