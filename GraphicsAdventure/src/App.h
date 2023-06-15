#pragma once

#define GDX11_IMGUI_SUPPORT
#include <GDX11.h>
#include "ImGui/ImGuiManager.h"
#include "Utils/ResourceLibrary.h"
#include "Rendering/Camera.h"
#include "Utils/EditorCameraController.h"
#include "Core/Time.h"

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

		void SetLight();
		void DrawPlane(const std::shared_ptr<GDX11::ShaderResourceView>& diffuseMap, const std::shared_ptr<GDX11::ShaderResourceView>& normalMap, const std::shared_ptr<GDX11::ShaderResourceView>& heightMap, float heightMapScale,
			const std::shared_ptr<GDX11::SamplerState>& sam, const DirectX::XMFLOAT3& pos, const DirectX::XMFLOAT3& rot, const DirectX::XMFLOAT3& scale,
			const DirectX::XMFLOAT4& col, const DirectX::XMFLOAT2& tiling, float shininess);

		void DrawCube(const std::shared_ptr<GDX11::ShaderResourceView>& diffuseMap, const std::shared_ptr<GDX11::ShaderResourceView>& normalMap, const std::shared_ptr<GDX11::ShaderResourceView>& heightMap, float heightMapScale,
			const std::shared_ptr<GDX11::SamplerState>& sam, const DirectX::XMFLOAT3& pos, const DirectX::XMFLOAT3& rot, const DirectX::XMFLOAT3& scale,
			const DirectX::XMFLOAT4& col, const DirectX::XMFLOAT2& tiling, float shininess);

		void SetShaders();
		void SetBuffers();
		void SetTextures();

		void SetSwapChain();
		void SetViews();
		void SetStates();

		bool OnWindowResizedEvent(GDX11::WindowResizeEvent& event);

		Time m_time;
		std::unique_ptr<GDX11::Window> m_window;
		std::unique_ptr<GDX11::GDX11Context> m_context;

		ImGuiManager m_imguiManager;
		Utils::ResourceLibrary m_resLib;

		Camera m_camera;
		GA::Utils::EditorCameraController m_camController;

		float m_gamma = 2.2f;

		// temp
		DirectX::XMFLOAT3 m_lightDir = {50.0f, 30.0f, 0.0f};
		float m_heightMapScale = 0.1f;
	};
}