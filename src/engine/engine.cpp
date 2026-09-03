#include "engine.hpp"

#include "src/asset/model/load.hpp"
#include "src/asset/store.hpp"
#include "src/consts.hpp"
#include "src/core/control_layer.hpp"
#include "src/core/debug_layer.hpp"
#include "src/scene/scene.hpp"

#include <spdlog/spdlog.h>

Engine::Engine(std::vector<std::string> args) {
  if (std::ranges::find(args, "x11") != args.end()) {
    glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_X11);
  }

  if (!glfwInit()) {
    spdlog::error("glfw init failed");
    std::exit(1);
  }

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

  m_window.reset(glfwCreateWindow(WIDTH, HEIGHT, "Lodge", nullptr, nullptr));

  if (!m_window) {
    spdlog::error("glfw window failed");
    std::exit(1);
  }

  m_renderer = std::make_unique<Renderer>(*m_window);
  m_assets = std::make_unique<AssetStore>(m_renderer->getDevice());
  m_scene = std::make_unique<Scene>();

  buildScene();
  FrameScene frame =
      gatherFrameScene(*m_scene, *m_assets, m_frameObjects, m_frameLights);
  m_renderer->initScene(*m_assets, frame);

  m_events = std::make_unique<EventQueue>();
  m_events->attach(*m_window);

  m_layers = std::make_unique<LayerStack>();

  m_layers->pushLayer("control", std::make_unique<ControlLayer>(*m_window));

  if (std::ranges::find(args, "debug") != args.end()) {
    m_layers->pushOverlay("debug",
                          std::make_unique<DebugLayer>(*m_window, *m_renderer,
                                                       *m_assets, *m_scene));
  }
}

void Engine::buildScene() {
  ModelHandle box = loadModel(*m_assets, "models/Box6.glb");

  GameObject &prop = m_scene->create("Box6");
  prop.renderer = ModelRenderer{box};

  GameObject &camera = m_scene->create("Main Camera");
  camera.camera = CameraParams{};
  m_scene->setMainCamera(camera.id);
  camera.transform.position = Vec3{0.0f, 1.0f, 1.0f};
  camera.transform.rotation = Quat::fromEuler(Vec3{-LDG_PI / 4.0f, 0.0f, 0.0f});

  GameObject &light = m_scene->create("Light");
  light.light = LightParams{};
  light.transform.position = Vec3{4.0f, 4.0f, 4.0f};
  light.transform.scale = Vec3{0.2f, 0.2f, 0.2f};
}

void Engine::run() {
  auto last = std::chrono::steady_clock::now();
  while (!glfwWindowShouldClose(m_window.get()) &&
         !static_cast<ControlLayer *>(m_layers->getLayer("control"))
              ->shouldQuit()) {
    auto now = std::chrono::steady_clock::now();
    float dt = std::chrono::duration<float>(now - last).count();
    last = now;

    glfwPollEvents();

    Event event;
    while (m_events->poll(event)) {
      const auto *resize = std::get_if<events::WindowResized>(&event);
      if (resize != nullptr) {
        m_renderer->onResize(resize->width, resize->height);
      }
      m_layers->onEvent(event);
    }

    m_layers->onUpdate(dt);

    FrameScene frame =
        gatherFrameScene(*m_scene, *m_assets, m_frameObjects, m_frameLights);
    m_renderer->drawFrame(frame);
  }
}
