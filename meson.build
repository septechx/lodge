project(
  'lodge',
  'cpp',
  'c',
  version: '0.1.0',
  default_options: [
    'cpp_std=c++23',
    'warning_level=3',
  ],
)

deps = [
  dependency('vulkan'),
  dependency('glfw3'),
  dependency('spdlog'),
]

include = include_directories('thirdparty', 'thirdparty/imgui', 'thirdparty/imgui/backends', 'thirdparty/tinygltf')
deps_sources = files(
  'thirdparty/imgui/imgui.cpp',
  'thirdparty/imgui/imgui_draw.cpp',
  'thirdparty/imgui/imgui_tables.cpp',
  'thirdparty/imgui/imgui_widgets.cpp',
  'thirdparty/imgui/backends/imgui_impl_glfw.cpp',
  'thirdparty/imgui/backends/imgui_impl_vulkan.cpp',
  'thirdparty/tinygltf/tiny_gltf_v3.c',
)

shaders = [
  ['shaders/opaque.vert', 'opaque.vert.spv'],
  ['shaders/opaque.frag', 'opaque.frag.spv'],
  ['shaders/transparent.vert', 'transparent.vert.spv'],
  ['shaders/transparent.frag', 'transparent.frag.spv'],
]

shader_deps = []
foreach shader : shaders
  shader_deps += custom_target(
    'spirv_@0@'.format(shader[1]),
    input: shader[0],
    output: shader[1],
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
  'src/asset/texture/load.cpp',
  'src/asset/model/load.cpp',
  'src/render/allocator.cpp',
  'src/render/init.cpp',
  'src/render/renderer.cpp',
  'src/render/render.cpp',
  'src/render/device.cpp',
  'src/render/swapchain.cpp',
  'src/render/buffers.cpp',
  'src/render/pipelines/pipeline.cpp',
  'src/render/pipelines/opaque.cpp',
  'src/render/pipelines/transparent.cpp',
  deps_sources,
  sources: shader_deps,
  dependencies: deps,
  include_directories: include,
  cpp_args: ['-Wno-missing-field-initializers'],
  c_args: ['-DTINYGLTF3_ENABLE_FS']
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
    include_directories: include_directories('src'),
  )
  test('math', test_math)
endif
