#include "extract.hpp"

#include "src/asset/store.hpp"
#include "src/scene/scene.hpp"

FrameScene gatherFrameScene(const Scene &scene, const AssetStore &assets,
                            std::vector<RenderObject> &objectsOut,
                            std::vector<FrameLight> &lightsOut) {
  objectsOut.clear();
  lightsOut.clear();

  for (const GameObject &object : scene.objects()) {
    if (object.renderer.has_value()) {
      const Model &model = assets.model(object.renderer->model);
      const Mat4 world = object.transform.matrix();
      for (const ModelPart &part : model.parts) {
        const GpuMesh &mesh = assets.mesh(part.mesh);
        objectsOut.push_back(RenderObject{
            .worldMat = world * part.local,
            .vbuf = mesh.vbuf,
            .ibuf = mesh.ibuf,
            .indexCount = mesh.indexCount,
            .indexType = mesh.indexType,
            .material = part.material,
        });
      }
    }

    if (object.light.has_value()) {
      const LightParams &light = *object.light;
      lightsOut.push_back(FrameLight{
          .pos = object.transform.position,
          .color = light.color,
      });
    }
  }

  FrameScene frame;
  if (const GameObject *cameraObject = scene.mainCamera()) {
    frame.camera = FrameCamera{
        .position = cameraObject->transform.position,
        .rotation = cameraObject->transform.rotation,
        .fovY = cameraObject->camera->fovY,
        .nearZ = cameraObject->camera->nearZ,
        .farZ = cameraObject->camera->farZ,
    };
  }
  frame.lights = lightsOut;
  frame.objects = objectsOut;
  return frame;
}
