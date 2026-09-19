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

## Usage

To run the example in `example/`, run:

```sh
just run example/scene.json
```

## Documentation

### Scene

Scenes are stored in json files.

#### Definitions:

- `Vec3`: `[float, float, float] /* x, y, z*/`
- `Quat`: `[float, float, float, float] /* w, x, y, z */`

#### Options

| Key Path                     | Type                    | Usage                                       |
| ---------------------------- | ----------------------- | ------------------------------------------- |
| mainCamera                   | string                  | Name of the object to use as the camera     |
| objects                      | GameObject[]            | All objects in the scene                    |
| objects[]/name               | string                  | Unique identifier to reference from scripts |
| objects[]/transform          | Transform \| undefined  | Position, rotation and scale                |
| objects[]/transform/position | Vec3 \| undefined       | Position                                    |
| objects[]/transform/rotation | Quat \| undefined       | Rotation, as a quaternion                   |
| objects[]/transform/scale    | Vec3 \| undefined       | Scale                                       |
| objects[]/renderer           | Renderer \| undefined   | glTF model to render                        |
| objects[]/renderer/model     | string                  | Path to the .glb for the model              |
| objects[]/light              | LightData \| undefined  | If set, makes the object a point light      |
| objects[]/light/color        | Vec3 \| undefined       | Color of the light in RGB                   |
| objects[]/camera             | CameraData \| undefined | If set, makes the object a camera           |
| objects[]/camera/fovY        | float                   | FOV of the camera                           |
| objects[]/camera/nearZ       | float                   | Min distance objects are rendered from      |
| objects[]/camera/farZ        | float                   | Max distance objects are rendered from      |
| objects[]/scripts            | string[]                | Paths of scripts to attach                  |
