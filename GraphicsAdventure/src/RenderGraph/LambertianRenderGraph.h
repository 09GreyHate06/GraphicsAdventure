#pragma once
#include "Scene/System.h"
#include "Utils/ResourceLibrary.h"

namespace GA
{
	class LambertianRenderGraph : public System
	{
	public:
		LambertianRenderGraph(Scene* scene, GDX11::GDX11Context* context, uint32_t windowWidth, uint32_t windowHeight);

		void Execute(const DirectX::XMFLOAT3& viewPos, const DirectX::XMFLOAT4X4& viewProj /*column major*/);

		void ResizeViews(uint32_t width, uint32_t height);

	private:
		void SolidPhongPass(const DirectX::XMFLOAT3& viewPos, const DirectX::XMFLOAT4X4& viewProj /*column major*/);
		void SkyboxPass(const DirectX::XMFLOAT4X4& viewProj /*column major*/);
		void TransparentPhongPass(const DirectX::XMFLOAT3& viewPos, const DirectX::XMFLOAT4X4& viewProj /*column major*/);
		void CompositePass();
		void GammaCorrectionPass();

		void SetLights();

		void SetShaders();
		void SetStates();
		void SetBuffers();

		GDX11::GDX11Context* m_context;
		GA::Utils::ResourceLibrary m_resLib;

		entt::observer m_renderable;
		entt::observer m_dirLights;
		entt::observer m_pointLights;
		entt::observer m_spotLights;
		entt::observer m_skybox;
	};
}