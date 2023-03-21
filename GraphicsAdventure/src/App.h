#pragma once

#define GDX11_IMGUI_SUPPORT
#include <GDX11.h>
#include "ImGui/ImGuiManager.h"
#include "Utils/ResourceLibrary.h"

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


		void SetShaders();
		void SetBuffers();
		void SetTextures();

		bool OnWindowResizedEvent(GDX11::WindowResizeEvent& event);

		std::unique_ptr<GDX11::Window> m_window;
		std::unique_ptr<GDX11::GDX11Context> m_context;

		ImGuiManager m_imguiManager;
		Utils::ResourceLibrary m_resLib;
	};
}