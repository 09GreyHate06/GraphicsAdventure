#pragma once

#define GDX11_IMGUI_SUPPORT
#include <GDX11.h>
#include "ImGui/ImGuiManager.h"
#include "Utils/ResourceLibrary.h"
#include "Scene/Camera.h"
#include "Utils/EditorCameraController.h"
#include "Core/Time.h"
#include "Scene/Scene.h"
#include "RenderGraph/LambertianRenderGraph.h"

namespace GA
{
	class App
	{
	public:
		App();


		void Run();

	private:
		void OnEvent(GDX11::Event& event);
		void OnUpdate();
		void OnRender();
		void OnImGuiRender();

		void SetBuffers();
		void SetTextures();
		void SetSwapChain();

		bool OnWindowResizedEvent(GDX11::WindowResizeEvent& event);

		Time m_time;
		std::unique_ptr<GDX11::Window> m_window;
		std::unique_ptr<GDX11::GDX11Context> m_context;

		ImGuiManager m_imguiManager;
		Utils::ResourceLibrary m_resLib;

		Camera m_camera;
		GA::Utils::EditorCameraController m_camController;

		std::unique_ptr<Scene> m_scene;
		std::unique_ptr<LambertianRenderGraph> m_lambertianRenderGraph;

		// temp
		DirectX::XMFLOAT3* pointLightPos;
	};
}