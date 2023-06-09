#pragma once
#include "Scene/System.h"
#include "Scene/Camera.h"
#include "Utils/ResourceLibrary.h"

namespace GA
{
	class LambertianRenderGraph : public System
	{
	public:
		LambertianRenderGraph(Scene* scene, GDX11::GDX11Context* context, const Camera* camera, uint32_t windowWidth, uint32_t windowHeight);

		void Execute();

		void ResizeViews(uint32_t width, uint32_t height);

	private:
		void ShadowPass();
		void SolidPhongPass(const DirectX::XMFLOAT3& viewPos, const DirectX::XMFLOAT4X4& viewProj /*column major*/);
		void SkyboxPass(const DirectX::XMFLOAT4X4& viewProj /*column major*/);
		void TransparentPhongPass(const DirectX::XMFLOAT3& viewPos, const DirectX::XMFLOAT4X4& viewProj /*column major*/);
		void CompositePass();
		void GammaCorrectionPass();

		void SetLights();

		void SetShaders();
		void SetStates();
		void SetBuffers();
		void SetLightDepthBuffers();

		GDX11::GDX11Context* m_context;
		const Camera* m_camera;
		GA::Utils::ResourceLibrary m_resLib;

		entt::observer m_renderable;
		entt::observer m_dirLights;
		entt::observer m_pointLights;
		entt::observer m_spotLights;
		entt::observer m_skybox;

		uint32_t m_windowWidth;
		uint32_t m_windowHeight;
	};
}