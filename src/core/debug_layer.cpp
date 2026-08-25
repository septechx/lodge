#include "debug_layer.hpp"

#include "../render/camera.hpp"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

#include <algorithm>
#include <cmath>
#include <print>

static float radians(float deg) { return deg * (LDG_PI / 180.0f); }
static float degrees(float rad) { return rad * (180.0f / LDG_PI); }

DebugLayer::DebugLayer(GLFWwindow &window, Renderer &renderer)
    : m_window(window), m_renderer(renderer) {
  syncYawPitchFromCamera();
}

DebugLayer::~DebugLayer() {
  if (m_initialized) {
    onDetach();
  }
}

void DebugLayer::onAttach() {
  if (m_initialized)
    return;

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  io.IniFilename = nullptr;

  ImGui::StyleColorsDark();

  if (!ImGui_ImplGlfw_InitForVulkan(&m_window, true)) {
    std::println(stderr, "ImGui_ImplGlfw_InitForVulkan failed");
    exit(1);
  }

  VkFormat colorFormat = m_renderer.getSwapchainFormat();
  VkPipelineRenderingCreateInfoKHR pci = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR,
      .colorAttachmentCount = 1,
      .pColorAttachmentFormats = &colorFormat,
      .depthAttachmentFormat = VK_FORMAT_UNDEFINED,
  };
  ImGui_ImplVulkan_InitInfo initInfo = {
      .ApiVersion = VK_API_VERSION_1_3,
      .Instance = m_renderer.getInstance(),
      .PhysicalDevice = m_renderer.getDevice().physical,
      .Device = m_renderer.getDevice().device,
      .QueueFamily = m_renderer.getDevice().queueFamily,
      .Queue = m_renderer.getDevice().queue,
      .MinImageCount = m_renderer.getSwapchain().imageCount,
      .ImageCount = m_renderer.getSwapchain().imageCount,
      .MSAASamples = VK_SAMPLE_COUNT_1_BIT,
      .DescriptorPoolSize = 1000,
      .UseDynamicRendering = true,
      .PipelineRenderingCreateInfo = pci,
  };
  if (!ImGui_ImplVulkan_Init(&initInfo)) {
    std::println(stderr, "ImGui_ImplVulkan_Init failed");
    exit(1);
  }

  m_initialized = true;
  std::println(stderr, "ImGui initialized");
}

void DebugLayer::onDetach() {
  if (!m_initialized)
    return;

  vkDeviceWaitIdle(m_renderer.getDevice().device);
  ImGui_ImplVulkan_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
  m_initialized = false;
}

void DebugLayer::syncYawPitchFromCamera() {
  Camera &cam = m_renderer.getCamera();
  Vec3 forward = (cam.center - cam.eye).normalize();
  if (forward.length() < 0.001f)
    return;
  m_yaw = degrees(std::atan2(forward.z, forward.x));
  m_pitch = degrees(std::asin(std::clamp(forward.y, -1.0f, 1.0f)));
}

void DebugLayer::updateFreeFly(float dt) {
  if (!m_freeFlyEnabled)
    return;

  ImGuiIO &io = ImGui::GetIO();

  Camera &cam = m_renderer.getCamera();

  bool wantKeyboard = io.WantCaptureKeyboard;
  bool wantMouse = io.WantCaptureMouse;

  Vec3 forward = (cam.center - cam.eye).normalize();
  if (forward.length() < 0.001f)
    forward = Vec3{0, 0, -1};
  Vec3 worldUp{0, 1, 0};
  Vec3 right = forward.cross(worldUp).normalize();
  if (right.length() < 0.001f) {
    right = Vec3{1, 0, 0};
  }

  float speed = m_moveSpeed * dt;
  if (glfwGetKey(&m_window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ||
      glfwGetKey(&m_window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS) {
    speed *= 2.5f;
  }

  Vec3 delta{0, 0, 0};
  if (!wantKeyboard) {
    if (glfwGetKey(&m_window, GLFW_KEY_W) == GLFW_PRESS) {
      delta = delta + forward * speed;
    }
    if (glfwGetKey(&m_window, GLFW_KEY_S) == GLFW_PRESS) {
      delta = delta - forward * speed;
    }
    if (glfwGetKey(&m_window, GLFW_KEY_A) == GLFW_PRESS) {
      delta = delta - right * speed;
    }
    if (glfwGetKey(&m_window, GLFW_KEY_D) == GLFW_PRESS) {
      delta = delta + right * speed;
    }
    if (glfwGetKey(&m_window, GLFW_KEY_SPACE) == GLFW_PRESS) {
      delta = delta + worldUp * speed;
    }
    if (glfwGetKey(&m_window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {
      delta = delta - worldUp * speed;
    }
  }

  if (delta.x != 0.0f || delta.y != 0.0f || delta.z != 0.0f) {
    cam.eye = cam.eye + delta;
    cam.center = cam.center + delta;
  }

  bool rightHeld =
      glfwGetMouseButton(&m_window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
  if (rightHeld && m_freeFlyEnabled) {
    if (wantMouse) {
      m_firstMouse = true;
      return;
    }
    double mx, my;
    glfwGetCursorPos(&m_window, &mx, &my);
    if (m_firstMouse) {
      m_lastX = mx;
      m_lastY = my;
      m_firstMouse = false;
      syncYawPitchFromCamera();
    } else {
      double dx = mx - m_lastX;
      double dy = my - m_lastY;
      m_lastX = mx;
      m_lastY = my;

      m_yaw += static_cast<float>(dx) * m_lookSpeed;
      m_pitch -= static_cast<float>(dy) * m_lookSpeed;
      m_pitch = std::clamp(m_pitch, -89.0f, 89.0f);

      float dist = (cam.center - cam.eye).length();
      if (dist < 0.001f)
        dist = 2.5f;
      float yawRad = radians(m_yaw);
      float pitchRad = radians(m_pitch);
      Vec3 newForward;
      newForward.x = std::cos(yawRad) * std::cos(pitchRad);
      newForward.y = std::sin(pitchRad);
      newForward.z = std::sin(yawRad) * std::cos(pitchRad);
      newForward = newForward.normalize();
      cam.center = cam.eye + newForward * dist;
    }
  } else {
    m_firstMouse = true;
  }
}

void DebugLayer::buildUI(float dt) {
  Camera &cam = m_renderer.getCamera();

  ImGui::Begin("Debug Menu", &m_visible);

  ImGui::Text("FPS: %.1f (%.2f ms)", 1.0f / (dt > 0.0001f ? dt : 0.0001f),
              dt * 1000.0f);
  ImGui::Text("Swapchain: %u x %u", m_renderer.getSwapchain().extent.width,
              m_renderer.getSwapchain().extent.height);
  ImGui::Separator();

  if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen)) {
    float eye[3] = {cam.eye.x, cam.eye.y, cam.eye.z};
    if (ImGui::DragFloat3("Eye", eye, 0.05f)) {
      cam.eye = Vec3{eye[0], eye[1], eye[2]};
      syncYawPitchFromCamera();
    }
    float center[3] = {cam.center.x, cam.center.y, cam.center.z};
    if (ImGui::DragFloat3("Center", center, 0.05f)) {
      cam.center = Vec3{center[0], center[1], center[2]};
      syncYawPitchFromCamera();
    }
    float up[3] = {cam.up.x, cam.up.y, cam.up.z};
    if (ImGui::DragFloat3("Up", up, 0.02f, -1.0f, 1.0f)) {
      Vec3 newUp{up[0], up[1], up[2]};
      if (newUp.length() > 0.001f) {
        cam.up = newUp.normalize();
      }
    }

    ImGui::Separator();
    ImGui::SliderFloat("FOV Y", &cam.fovY, 30.0f, 120.0f);
    ImGui::SliderFloat("Near", &cam.nearZ, 0.01f, 2.0f);
    ImGui::SliderFloat("Far", &cam.farZ, 5.0f, 100.0f);

    float dist = (cam.center - cam.eye).length();
    ImGui::Text("Distance: %.2f  Yaw: %.1f  Pitch: %.1f", dist, m_yaw, m_pitch);

    if (ImGui::Button("Reset Camera")) {
      cam.reset();
      syncYawPitchFromCamera();
    }
    ImGui::SameLine();
    if (ImGui::Button("Look At Origin")) {
      cam.center = Vec3{0, 0, 0};
      syncYawPitchFromCamera();
    }
  }

  if (ImGui::CollapsingHeader("Movement", ImGuiTreeNodeFlags_DefaultOpen)) {
    ImGui::Checkbox("Free Fly", &m_freeFlyEnabled);
    ImGui::SliderFloat("Move Speed", &m_moveSpeed, 0.5f, 10.0f);
    ImGui::SliderFloat("Look Speed", &m_lookSpeed, 0.02f, 0.5f);
  }

  ImGui::End();
}

void DebugLayer::onUpdate(float dt) {
  if (!m_initialized)
    return;

  updateFreeFly(dt);

  ImGui_ImplVulkan_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();

  if (m_visible) {
    buildUI(dt);
  }

  ImGui::Render();
}

bool DebugLayer::onEvent(const Event &event) {
  if (!m_initialized)
    return false;

  if (auto *key = std::get_if<events::KeyPressed>(&event); key != nullptr) {
    if (key->key == GLFW_KEY_F1) {
      m_visible = !m_visible;
      return true;
    }
    return false;
  }

  ImGuiIO &io = ImGui::GetIO();
  if (io.WantCaptureMouse) {
    if (std::holds_alternative<events::MouseButtonPressed>(event) ||
        std::holds_alternative<events::MouseButtonReleased>(event) ||
        std::holds_alternative<events::MouseMoved>(event) ||
        std::holds_alternative<events::MouseScrolled>(event)) {
      return true;
    }
  }
  if (io.WantCaptureKeyboard) {
    if (std::holds_alternative<events::KeyPressed>(event) ||
        std::holds_alternative<events::KeyReleased>(event)) {
      return true;
    }
  }

  return false;
}
