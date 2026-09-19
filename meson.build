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
  dependency('luajit'),
  dependency('dl'),
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

python = find_program('python3')
embed_script = files('tools/embed_spv.py')

shaders = [
  ['shaders/opaque.vert', 'opaque.vert.spv', 'k_opaqueVert', 'opaque.vert.embedded.cpp'],
  ['shaders/opaque.frag', 'opaque.frag.spv', 'k_opaqueFrag', 'opaque.frag.embedded.cpp'],
  ['shaders/opaque_grab.frag', 'opaque_grab.frag.spv', 'k_opaqueGrabFrag', 'opaque_grab.frag.embedded.cpp'],
  ['shaders/opaque_comp.frag', 'opaque_comp.frag.spv', 'k_opaqueCompFrag', 'opaque_comp.frag.embedded.cpp'],
  ['shaders/transparent.vert', 'transparent.vert.spv', 'k_transparentVert', 'transparent.vert.embedded.cpp'],
  ['shaders/transparent.frag', 'transparent.frag.spv', 'k_transparentFrag', 'transparent.frag.embedded.cpp'],
  ['shaders/sky.vert', 'sky.vert.spv', 'k_skyVert', 'sky.vert.embedded.cpp'],
  ['shaders/sky.frag', 'sky.frag.spv', 'k_skyFrag', 'sky.frag.embedded.cpp'],
  ['shaders/ssr.comp', 'ssr.comp.spv', 'k_ssrComp', 'ssr.comp.embedded.cpp'],
]

embedded_sources = []
foreach shader : shaders
  spv = custom_target(
    'spirv_@0@'.format(shader[1]),
    input: shader[0],
    output: shader[1],
    command: ['glslc', '@INPUT@', '-o', '@OUTPUT@'],
    build_by_default: true,
  )
  embedded_sources += custom_target(
    'embed_@0@'.format(shader[3]),
    input: spv,
    output: shader[3],
    command: [python, embed_script, '@INPUT@', '@OUTPUT@', shader[2]],
    build_by_default: true,
  )
endforeach

lodge_datadir = get_option('prefix') / get_option('datadir') / 'lodge'

executable(
  'lodge',
  'src/main.cpp',
  'src/utils.cpp',
  'src/core/event.cpp',
  'src/core/layer_stack.cpp',
  'src/core/control_layer.cpp',
  'src/core/debug_layer.cpp',
  'src/engine/engine.cpp',
  'src/asset/texture/load.cpp',
  'src/asset/model/load.cpp',
  'src/asset/store.cpp',
  'src/scene/scene.cpp',
  'src/scene/extract.cpp',
  'src/scene/io.cpp',
  'src/render/allocator.cpp',
  'src/render/init.cpp',
  'src/render/renderer.cpp',
  'src/render/render.cpp',
  'src/render/device.cpp',
  'src/render/swapchain.cpp',
  'src/render/buffers.cpp',
  'src/render/bake.cpp',
  'src/render/material_store.cpp',
  'src/render/draw_groups.cpp',
  'src/render/pipelines/pipeline.cpp',
  'src/render/pipelines/selection.cpp',
  'src/render/pipelines/builder.cpp',
  'src/render/pipelines/opaque.cpp',
  'src/render/pipelines/transparent.cpp',
  'src/render/pipelines/sky.cpp',
  'src/render/pipelines/ssr.cpp',
  'src/debug/renderdoc.cpp',
  'src/script/manager.cpp',
  'src/script/layer.cpp',
  'src/script/api/input.cpp',
  'src/script/api/input_state.cpp',
  'src/script/api/scene.cpp',
  'src/script/api/time.cpp',
  'src/script/api/log.cpp',
  'src/serialize/json.cpp',
  'src/render/pipelines/embedded_shaders.hpp',
  deps_sources,
  sources: embedded_sources,
  dependencies: deps,
  include_directories: include,
  cpp_args: ['-Wno-missing-field-initializers', '-DLODGE_INSTALL_DATADIR="@0@"'.format(lodge_datadir)],
  c_args: ['-DTINYGLTF3_ENABLE_FS'],
  install: true,
)

install_subdir(
  'example',
  install_dir: get_option('datadir') / 'lodge',
  strip_directory: true,
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

  test_scene = executable(
    'test_scene',
    'tests/test_scene.cpp',
    'src/scene/scene.cpp',
    dependencies: catch2_dep,
    include_directories: include_directories('src'),
  )
  test('scene', test_scene)

  test_material = executable(
    'test_material',
    'tests/test_material.cpp',
    'src/render/pipelines/selection.cpp',
    'src/render/material_store.cpp',
    'src/render/draw_groups.cpp',
    dependencies: catch2_dep,
    include_directories: include_directories('src', '.'),
  )
  test('material', test_material)

  test_pipeline = executable(
    'test_pipeline',
    'tests/test_pipeline.cpp',
    dependencies: [catch2_dep, dependency('vulkan')],
    include_directories: include_directories('src', '.'),
  )
  test('pipeline', test_pipeline)

  test_script = executable(
    'test_script',
    'tests/test_script.cpp',
    'src/script/api/input.cpp',
    'src/script/api/input_state.cpp',
    'src/script/api/scene.cpp',
    'src/script/api/time.cpp',
    'src/script/api/log.cpp',
    'src/script/manager.cpp',
    'src/scene/scene.cpp',
    dependencies: [catch2_dep, dependency('luajit'), dependency('spdlog'), dependency('glfw3')],
    include_directories: include_directories('src'),
  )
  test('script', test_script)
endif
