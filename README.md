# Lodge

Lodge is a small 3D game engine with lua scripting, written in C++ using vulkan.

## Building

First, setup the build directory:

```sh
meson setup build
```

Then build with:

```sh
ninja -C build
```

The resulting executable will be in `./build/lodge`.
