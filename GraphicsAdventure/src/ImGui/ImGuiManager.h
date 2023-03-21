#pragma once
#include <GDX11.h>
#include <imgui.h>

namespace GA
{
	class ImGuiManager
	{
	public:
		ImGuiManager(const GDX11::Window* window, const GDX11::GDX11Context* context);
		ImGuiManager() = default;

		void Set(const GDX11::Window* window, const GDX11::GDX11Context* context);

		void Begin();
		void End();

	private:
		const GDX11::Window* m_window;
	};
}