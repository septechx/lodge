project(
  'lodge',
  'cpp',
  version: '0.1.0',
  default_options: [
    'cpp_std=c++23',
    'warning_level=3',
  ],
)

deps = [
  dependency('vulkan'),
  dependency('glfw3'),
]

imgui_include = include_directories('thirdparty/imgui', 'thirdparty/imgui/backends')
imgui_sources = files(
  'thirdparty/imgui/imgui.cpp',
  'thirdparty/imgui/imgui_draw.cpp',
  'thirdparty/imgui/imgui_tables.cpp',
  'thirdparty/imgui/imgui_widgets.cpp',
  'thirdparty/imgui/backends/imgui_impl_glfw.cpp',
  'thirdparty/imgui/backends/imgui_impl_vulkan.cpp',
)

shaders = [
  ['shader.vert', 'vertex'],
  ['shader.frag', 'fragment'],
]

shader_deps = []
foreach shader : shaders
  shader_deps += custom_target(
    'spirv_@0@'.format(shader[1]),
    input: shader[0],
    output: '@0@.spv'.format(shader[0]),
    command: ['glslc', '@INPUT@', '-o', '@OUTPUT@'],
    build_by_default: true,
  )
endforeach

executable(
  'lodge',
  'src/main.cpp',
  'src/utils.cpp',
  'src/core/event.cpp',
  'src/core/layer_stack.cpp',
  'src/core/debug_layer.cpp',
  'src/render/allocator.cpp',
  'src/render/init.cpp',
  'src/render/pipeline.cpp',
  'src/render/texture.cpp',
  'src/render/renderer.cpp',
  'src/render/render.cpp',
  imgui_sources,
  sources: shader_deps,
  dependencies: deps,
  include_directories: imgui_include,
  cpp_args: ['-Wno-missing-field-initializers'],
)

catch2_dep = dependency('catch2-with-main', required: false)
if not catch2_dep.found()
  catch2_dep = dependency('catch2', required: false)
endif

if catch2_dep.found()
  test_math = executable(
    'test_math',
    'tests/test_math.cpp',
    dependencies: catch2_dep,
    include_directories: include_directories('src', 'thirdparty/imgui', 'thirdparty/imgui/backends'),
  )
  test('math', test_math)
endif
