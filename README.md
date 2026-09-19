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

### Scripts

Each object script gets `self` and `gameObject`, both handles to its owner object. Use method syntax on them (`self:translate(...)`) or pass them to functions (`scene.translate(self, ...)`).

#### Definitions:

- `Vec3`: `{ x = float, y = float, z = float }`
- `Quat`: `{ w = float, x = float, y = float, z = float } /* angles in radians */`
- `Camera`: `{ fovY = float, nearZ = float, farZ = float }`
- `Color`: `{ r = float, g = float, b = float }`
- `GameObject`: handle or integer id; getters return `nil` if invalid/missing

#### Lifecycle

| Function   | Usage                                 |
| ---------- | ------------------------------------- |
| Start()    | Called once when the script is loaded |
| Update(dt) | Called every frame, `dt` in seconds   |

#### self / gameObject methods

| Function                          | Returns       | Usage                                          |
| --------------------------------- | ------------- | ---------------------------------------------- |
| self:getId() / self.id            | integer       | Id of the object                               |
| self:isValid()                    | boolean       | False if the object was removed                |
| self:getName()                    | string \| nil | Object name                                    |
| self:getPosition()                | Vec3 \| nil   | Position                                       |
| self:setPosition(pos)             |               | Set position                                   |
| self:translate(delta)             |               | Add `Vec3` to position                         |
| self:getRotation()                | Quat \| nil   | Rotation, normalized                           |
| self:setRotation(quat)            |               | Set rotation, normalized; errors if degenerate |
| self:setRotationEuler(euler)      |               | Set rotation from `Vec3` euler (XYZ, radians)  |
| self:rotateAxisAngle(axis, angle) |               | Premultiply rotation; `angle` in radians       |
| self:getScale()                   | Vec3 \| nil   | Scale                                          |
| self:setScale(scale)              |               | Set scale                                      |
| self:hasRenderer()                | boolean       | Has model component                            |
| self:hasCamera()                  | boolean       | Has camera component                           |
| self:hasLight()                   | boolean       | Has light component                            |
| self:getCamera()                  | Camera \| nil | Camera params                                  |
| self:setCamera(camera)            |               | Set camera params                              |
| self:getLightColor()              | Color \| nil  | Light color                                    |
| self:setLightColor(color)         |               | Set light color                                |
| self:setMainCamera()              |               | Make this object the main camera               |

Handles support `==`, `tostring()`, and `.id`; they are read-only.

#### scene

Same as the methods above, but taking an explicit target (`GameObject`, id, or `nil`) as the first argument, plus lookup helpers:

| Function                                   | Returns           | Usage                                                       |
| ------------------------------------------ | ----------------- | ----------------------------------------------------------- |
| scene.findByName(name)                     | GameObject \| nil | Find object by name                                         |
| scene.findAll()                            | GameObject[]      | All objects in the scene                                    |
| scene.isValid(target)                      | boolean           | True if the object exists                                   |
| scene.getName(target)                      | string \| nil     | Object name                                                 |
| scene.mainCamera()                         | GameObject \| nil | Current main camera                                         |
| scene.setMainCamera(target)                |                   | Set main camera; target needs a camera                      |
| scene.getPosition(target)                  | Vec3 \| nil       | Position                                                    |
| scene.setPosition(target, pos)             |                   | Set position                                                |
| scene.translate(target, delta)             |                   | Add `Vec3` to position                                      |
| scene.getRotation(target)                  | Quat \| nil       | Rotation, normalized                                        |
| scene.setRotation(target, quat)            |                   | Set rotation, normalized; errors if degenerate              |
| scene.setRotationEuler(target, euler)      |                   | Set rotation from `Vec3` euler (XYZ, radians)               |
| scene.rotateAxisAngle(target, axis, angle) |                   | Premultiply rotation; `angle` in radians                    |
| scene.getScale(target)                     | Vec3 \| nil       | Scale                                                       |
| scene.setScale(target, scale)              |                   | Set scale                                                   |
| scene.hasRenderer(target)                  | boolean           | Has model component                                         |
| scene.hasCamera(target)                    | boolean           | Has camera component                                        |
| scene.hasLight(target)                     | boolean           | Has light component                                         |
| scene.getCamera(target)                    | Camera \| nil     | Camera params                                               |
| scene.setCamera(target, camera)            |                   | Set camera params (`fovY > 0`, `nearZ > 0`, `farZ > nearZ`) |
| scene.getLightColor(target)                | Color \| nil      | Light color                                                 |
| scene.setLightColor(target, color)         |                   | Set light color                                             |

#### input

Keys/buttons are case-insensitive names (`"W"`, `"space"`, `"escape"`, `"up"`, `"shift"`, `"control"`, `"alt"`, `"F1"`–`"F25"`, ...) or GLFW codes (`87`), buttons as (`"left"`, `"right"`, `"middle"`, `"button4"`–`"button8"`) or `0`–`7`. `mouseDelta`/`scroll` reset every frame.

| Function                  | Returns  | Usage                                    |
| ------------------------- | -------- | ---------------------------------------- |
| input.isKeyDown(key)      | boolean  | True while the key is held               |
| input.isMouseDown(button) | boolean  | True while the mouse button is held      |
| input.mousePos()          | `{x, y}` | Cursor position in pixels                |
| input.mouseDelta()        | `{x, y}` | Cursor movement since last frame         |
| input.scroll()            | `{x, y}` | Scroll offset since last frame           |
| input.setCursorMode(mode) |          | `"normal"` \| `"hidden"` \| `"disabled"` |
| input.cursorMode()        | string   | Current cursor mode                      |

#### time

| Function   | Returns | Usage                         |
| ---------- | ------- | ----------------------------- |
| time.now() | float   | Seconds since the run started |

#### log

| Function       | Usage                |
| -------------- | -------------------- |
| log.info(msg)  | Log at info level    |
| log.warn(msg)  | Log at warning level |
| log.error(msg) | Log at error level   |
