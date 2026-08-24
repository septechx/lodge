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
  'src/render/allocator.cpp',
  'src/render/init.cpp',
  'src/render/pipeline.cpp',
  'src/render/texture.cpp',
  'src/render/renderer.cpp',
  'src/render/render.cpp',
  sources: shader_deps,
  dependencies: deps,
  cpp_args: ['-Wno-missing-field-initializers'],
)
