#pragma once
#include "Scene/System.h"
#include "Scene/Camera.h"
#include "Utils/ResourceLibrary.h"
#include "Utils/ShaderCBuf.h"

namespace GA
{
	class CSMTestRenderGraph : public System
	{
	public:
		CSMTestRenderGraph(Scene* scene, GDX11::GDX11Context* context, const Camera* camera, uint32_t windowWidth, uint32_t windowHeight);

		void Execute();

		void ResizeViews(uint32_t width, uint32_t height);

	private:
		void ShadowPass();
		void RenderPass();
		void GammaCorrectionPass();

		void SetShaders();
		void SetStates();
		void SetBuffers();
		void SetLightDepthBuffers();

		std::array<DirectX::XMFLOAT4X4, GA::Utils::s_numCascades> CalculateLightSpace(DirectX::FXMVECTOR xmLightDir);

		GDX11::GDX11Context* m_context;
		const Camera* m_camera;
		GA::Utils::ResourceLibrary m_resLib;

		entt::observer m_renderable;
		entt::observer m_dirLight;

		uint32_t m_windowWidth;
		uint32_t m_windowHeight;

		std::array<float, GA::Utils::s_numCascades> m_cascadeFarZDist;
	};
}