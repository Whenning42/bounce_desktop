project('bouncedesk', 'cpp',
  version : '0.1.0',
  default_options : ['cpp_std=c++20',
                     'cpp_eh=none',
                     'cpp_rtti=false',
                     'warning_level=3',
                     'werror=true'])

add_project_arguments('-Wshadow', language : 'cpp')

gtest_dep = dependency('gtest', main : true)
gmock_dep = dependency('gmock')
vncclient_dep = dependency('libvncclient')

bouncedesk_sources = [
  'client.cpp',
  'wayland_backend.cpp',
  'reaper/reaper.cpp'
]

bouncedesk_lib = static_library('bouncedesk',
  bouncedesk_sources,
  dependencies : [vncclient_dep],
  include_directories : ['third_party'],
)

client_test = executable('client_test',
  'client_test.cpp',
  link_with : bouncedesk_lib,
  dependencies : [gtest_dep],
  include_directories : ['third_party']
)

reaper_sources = [
  'reaper/reaper.cpp',
  'reaper/impl.cpp',
  'reaper/cleanup.cpp'
]

reaper = executable('reaper',
  ['reaper/impl_main.cpp'] + reaper_sources,
  include_directories : ['third_party'],
  export_dynamic: true
)

reaper_test = executable('reaper_test',
  'reaper/reaper_test.cpp',
  link_with : bouncedesk_lib,
  dependencies : [gtest_dep, gmock_dep],
  include_directories : ['third_party'],
  export_dynamic: true
)

ipc_test = executable('ipc_test',
  'reaper/ipc_test.cpp',
  dependencies : [gtest_dep],
  include_directories : ['third_party']
)

reaper_tests_reaper_parent = executable('reaper_tests_reaper_parent',
  'reaper/tests/reaper_parent.cpp',
  link_with : bouncedesk_lib,
  include_directories : ['third_party']
)

test('client_test', client_test, is_parallel : false,  workdir : meson.source_root())
test('reaper_test', reaper_test, is_parallel : false, workdir : meson.source_root())
test('ipc_test', ipc_test, is_parallel : false, workdir : meson.source_root())

