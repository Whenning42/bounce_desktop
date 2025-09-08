project('bouncedesk', 'cpp',
  version : '0.1.0',
  default_options : ['cpp_std=c++20',
                     'cpp_eh=none',
                     'cpp_rtti=false',
                     'buildtype=debugoptimized'])

gtest_dep = dependency('gtest', main : true)
vncclient_dep = dependency('libvncclient')

bouncedesk_sources = [
  'client.cpp',
  'client_loop.cpp',
  'wayland_backend.cpp'
]

bouncedesk_lib = static_library('bouncedesk',
  bouncedesk_sources,
  dependencies : [vncclient_dep],
  include_directories : ['third_party']
)

client_test = executable('client_test',
  'client_test.cpp',
  link_with : bouncedesk_lib,
  dependencies : [gtest_dep],
  include_directories : ['third_party']
)

test('client_test', client_test)
