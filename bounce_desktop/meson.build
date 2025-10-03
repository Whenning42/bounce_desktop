configure_file(input: '__init__.py', output: '__init__.py', copy: true)
configure_file(input: 'bounce_desk_test.py', output: 'bounce_desk_test.py', copy: true)

python3 = import('python').find_installation('python3', required: false)
nanobind_dep = dependency('nanobind')

py_sources = [
  '../src/bindings/client_ext.cpp',
  '../third_party/status/exceptions.h',
]

python3.extension_module('_core',
  py_sources,
  include_directories: ['../', '../src'],
  dependencies: [nanobind_dep, gvnc_dep],
  link_with: bouncedesk_lib,
  cpp_args: [],
  override_options: [
    'cpp_rtti=true',
    'cpp_eh=default',
    'warning_level=2',
  ],
  install_rpath: '$ORIGIN',
)

test('bounce_desk_test',
  python3,
  args: [files('bounce_desk_test.py')],
  env: {'PYTHONPATH': meson.build_root()},
  workdir : meson.project_source_root()
)
