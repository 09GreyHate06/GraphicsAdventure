#include "Scene/Entity.h"
#include "LambertianRenderGraph.h"
#include "Utils/BasicMesh.h"
#include "Utils/ShaderCBuf.h"
#include "Scene/Components.h"
#include "Utils/Macros.h"

using namespace DirectX;
using namespace GDX11;
using namespace Microsoft::WRL;

#define GAMMA 2.2f

// ------------ keys -------------

#define RTV_MAIN                            "main"
#define RTV_SCENE                           "scene"
#define SRV_SCENE                           "scene"
#define DSV_SCENE                           "scene"
#define RTVA_TRANSPARENT_PHONG_PASS         "transparent_phong_pass"
#define SRV_TRANSPARENT_PHONG_PASS_ACC      "transparent_phong_pass_accumulation"
#define SRV_TRANSPARENT_PHONG_PASS_REV      "transparent_phong_pass_reveal"


#define VS_PHONG                            "phong" 
#define VS_SKYBOX                           "skybox"
#define VS_FS_OUT_TC_POS                    "fullscreen_out_tc_pos"
#define VS_FS_OUT_POS                       "fullscreen_out_pos"
#define VS_BASIC                            "basic"
#define VS_CUBE_SHADOW_MAP                  "cube_shadow_map"
								            
#define PS_PHONG                            "phong"
#define PS_PHONG_OIT                        "phong_oit"
#define PS_SKYBOX                           "skybox"
#define PS_PHONG_OIT_COMPOSITE              "phong_oit_composite"
#define PS_GAMMA_CORRECTION                 "gamma_correction"
#define PS_NULLPTR                          "null"
								            
#define GS_CUBE_SHADOW_MAP                  "cube_shadow_map"
#define GS_NULLPTR                          "null"

#define IL_PHONG                            "phong"
#define IL_SKYBOX                           "skybox"
#define IL_FS_OUT_TC_POS                    "fullscreen_out_tc_pos"
#define IL_FS_OUT_POS                       "fullscreen_out_pos"
#define IL_BASIC                            "basic"
#define IL_CUBE_SHADOW_MAP                  "cube_shadow_map"

#define S_DEFAULT                           "default"

#define RS_CULL_NONE                        "cull_none"
#define RS_DEPTH_SLOPE_SCALED_BIAS          "depth_slope_scaled_bias"
#define BS_OVER_OP                          "over_op"
#define BS_WEIGHTED_BLENDED_OIT_OP          "weighted_blended_oit_op"
#define DSS_DEPTH_WRITE_ZERO                "depth_write_zero"
#define DSS_DEPTH_WRITE_ZERO_OP_LESS_EQUAL  "depth_write_zero_op_less_equal"
#define SS_POINT_CLAMP                      "point_clamp"
#define SS_LINEAR_CLAMP                     "linear_clamp"

#define VB_FS_QUAD                          "fs_quad.vb"
#define IB_FS_QUAD                          "fs_quad.ib"
#define VB_CUBE                             "cube.vb"
#define IB_CUBE                             "cube.ib"

#define CB_VS_PHONG_SYSTEM                  "phong.vs.SystemCBuf"
#define CB_VS_PHONG_ENTITY                  "phong.vs.EntityCBuf"
#define CB_PS_PHONG_SYSTEM                  "phong.ps.SystemCBuf"
#define CB_PS_PHONG_ENTITY                  "phong.ps.EntityCBuf"
#define CB_PS_GAMMA_CORRECTION_SYSTEM       "gamma_correction.ps.SystemCBuf"
#define CB_VS_SKYBOX_SYSTEM                 "skybox.vs.SystemCBuf"
#define CB_VS_BASIC_SYSTEM                  "basic.vs.SystemCBuf"
#define CB_VS_BASIC_ENTITY                  "basic.vs.Entity"
#define CB_VS_CUBE_SHADOW_MAP_ENTITY        "cube_shadow_map.vs.EntityCBuf"
#define CB_GS_CUBE_SHADOW_MAP_SYSTEM        "cube_shadow_map.gs.SystemCBuf"


#define DSV_DIRLIGHT_SHADOW_MAP(x)          "dirLight_shadow_map" + std::to_string((x))
#define SRV_DIRLIGHT_SHADOW_MAP             "dirLight_shadow_map"

#define DSV_POINTLIGHT_SHADOW_MAP(x)        "pointlight_shadow_map" + std::to_string((x))
#define SRV_POINTLIGHT_SHADOW_MAP           "pointlight_shadow_map"

#define DSV_SPOTLIGHT_SHADOW_MAP(x)        "spotlight_shadow_map" + std::to_string((x))
#define SRV_SPOTLIGHT_SHADOW_MAP           "spotlight_shadow_map"

#define SHADOWMAP_SIZE 2040

namespace GA
{
	LambertianRenderGraph::LambertianRenderGraph(Scene* scene, GDX11::GDX11Context* context, uint32_t windowWidth, uint32_t windowHeight)
		: System(scene), m_context(context)
	{
		m_dirLights.connect(GetRegistry(), entt::collector.group<TransformComponent, DirectionalLightComponent>(entt::exclude<>));
		m_pointLights.connect(GetRegistry(), entt::collector.group<TransformComponent, PointLightComponent>(entt::exclude<>));
		m_spotLights.connect(GetRegistry(), entt::collector.group<TransformComponent, SpotLightComponent>(entt::exclude<>));
		m_renderable.connect(GetRegistry(), entt::collector.group<TransformComponent, MeshComponent, MaterialComponent>(entt::exclude<>));
		m_skybox.connect(GetRegistry(), entt::collector.group<SkyboxComponent>(entt::exclude<>));

		ResizeViews(windowWidth, windowHeight);
		SetShaders();
		SetStates();
		SetBuffers();
		SetLightDepthBuffers();
	}

	void LambertianRenderGraph::Execute(const DirectX::XMFLOAT3& viewPos, const DirectX::XMFLOAT4X4& viewProj /*column major*/)
	{
		// set lights and shadow pass
		SetLights();
		SolidPhongPass(viewPos, viewProj);
		SkyboxPass(viewProj);
		TransparentPhongPass(viewPos, viewProj);
		CompositePass();
		GammaCorrectionPass();
	}

	void LambertianRenderGraph::ShadowPass()
	{

	}

	void LambertianRenderGraph::SolidPhongPass(const DirectX::XMFLOAT3& viewPos, const DirectX::XMFLOAT4X4& viewProj /*column major*/)
	{
		D3D11_VIEWPORT vp = {};
		vp.TopLeftX = 0.0f;
		vp.TopLeftY = 0.0f;
		vp.Width = (float)m_windowWidth;
		vp.Height = (float)m_windowHeight;
		vp.MinDepth = 0.0f;
		vp.MaxDepth = 1.0f;
		m_context->GetDeviceContext()->RSSetViewports(1, &vp);

		auto rtv = m_resLib.Get<RenderTargetView>(RTV_SCENE);
		auto dsv = m_resLib.Get<DepthStencilView>(DSV_SCENE);
		rtv->Clear(0.0f, 0.0f, 0.0f, 0.0f);
		dsv->Clear(D3D11_CLEAR_DEPTH, 1.0f, 0xff);

		if (m_renderable.empty()) return;

		m_resLib.Get<RasterizerState>(S_DEFAULT)->Bind();
		m_resLib.Get<BlendState>(S_DEFAULT)->Bind(nullptr, 0xff);
		m_resLib.Get<DepthStencilState>(S_DEFAULT)->Bind(0xff);

		rtv->Bind(dsv.get());

		auto vs = m_resLib.Get<VertexShader>(VS_PHONG);
		auto ps = m_resLib.Get<PixelShader>(PS_PHONG);
		vs->Bind();
		ps->Bind();
		m_resLib.Get<InputLayout>(IL_PHONG)->Bind();

		// system cbufs
		{
			m_resLib.Get<Buffer>(CB_PS_PHONG_SYSTEM)->PSBindAsCBuf(ps->GetResBinding("SystemCBuf"));

			GA::Utils::PhongVSSystemCBuf cbufData = {};
			cbufData.viewPos = viewPos;
			cbufData.viewProjection = viewProj;

			auto cbuf = m_resLib.Get<Buffer>(CB_VS_PHONG_SYSTEM);
			cbuf->SetData(&cbufData);
			cbuf->VSBindAsCBuf(vs->GetResBinding("SystemCBuf"));
		}

		m_resLib.Get<ShaderResourceView>(SRV_DIRLIGHT_SHADOW_MAP)->PSBind(ps->GetResBinding("dirLightShadowMaps"));
		m_resLib.Get<SamplerState>(SS_LINEAR_CLAMP)->PSBind(ps->GetResBinding("dirLightShadowMapsSampler"));
		m_resLib.Get<ShaderResourceView>(SRV_POINTLIGHT_SHADOW_MAP)->PSBind(ps->GetResBinding("pointLightShadowMaps"));
		m_resLib.Get<SamplerState>(SS_LINEAR_CLAMP)->PSBind(ps->GetResBinding("pointLightShadowMapsSampler"));
		m_resLib.Get<ShaderResourceView>(SRV_SPOTLIGHT_SHADOW_MAP)->PSBind(ps->GetResBinding("spotLightShadowMaps"));
		m_resLib.Get<SamplerState>(SS_LINEAR_CLAMP)->PSBind(ps->GetResBinding("spotLightShadowMapsSampler"));

		for (const auto& e : m_renderable)
		{
			const auto& [transform, mesh, mat] = GetRegistry().get<TransformComponent, MeshComponent, MaterialComponent>(e);

			if (mat.color.w < (1.0f - GA_UTILS_EPSILONF))
				continue;

			mesh.vb->BindAsVB();
			mesh.ib->BindAsIB(DXGI_FORMAT_R32_UINT);

			mat.diffuseMap->PSBind(ps->GetResBinding("diffuseMap"));
			mat.samplerState->PSBind(ps->GetResBinding("samplerState"));

			if (mat.normalMap)
			{
				mat.normalMap->PSBind(ps->GetResBinding("normalMap"));
				if (mat.depthMap)
					mat.depthMap->PSBind(ps->GetResBinding("depthMap"));
			}

			XMMATRIX xmTransform = transform.GetTransform();
			XMFLOAT4X4 fTransform;
			XMFLOAT4X4 normalMatrix;
			XMStoreFloat4x4(&fTransform, XMMatrixTranspose(xmTransform));
			XMStoreFloat4x4(&normalMatrix, XMMatrixInverse(nullptr, xmTransform));

			// set cbufs
			{
				GA::Utils::PhongVSEntityCBuf cbufData = {};
				cbufData.transform = fTransform;
				cbufData.normalMatrix = normalMatrix;

				auto cbuf = m_resLib.Get<Buffer>(CB_VS_PHONG_ENTITY);
				cbuf->SetData(&cbufData);
				cbuf->VSBindAsCBuf(vs->GetResBinding("EntityCBuf"));
			}

			{
				GA::Utils::PhongPSEntityCBuf cbufData = {};
				cbufData.mat.color = mat.color;
				cbufData.mat.tiling = mat.tiling;
				cbufData.mat.shininess = mat.shininess;
				cbufData.mat.enableNormalMapping = mat.normalMap ? TRUE : FALSE;
				cbufData.mat.enableParallaxMapping = mat.normalMap && mat.depthMap ? TRUE : FALSE;
				cbufData.mat.depthMapScale = mat.depthMapScale;
				cbufData.receiveShadows = mesh.receiveShadows;

				auto cbuf = m_resLib.Get<Buffer>(CB_PS_PHONG_ENTITY);
				cbuf->PSBindAsCBuf(ps->GetResBinding("EntityCBuf"));
				cbuf->SetData(&cbufData);
			}

			m_context->GetDeviceContext()->IASetPrimitiveTopology(mesh.topology);
			GDX11_CONTEXT_THROW_INFO_ONLY(m_context->GetDeviceContext()->DrawIndexed(mesh.ib->GetDesc().ByteWidth / sizeof(uint32_t), 0, 0));
		}
	}

	void LambertianRenderGraph::SkyboxPass(const DirectX::XMFLOAT4X4& viewProj /*column major*/)
	{
		if (m_skybox.empty()) return;

		m_resLib.Get<DepthStencilState>(DSS_DEPTH_WRITE_ZERO_OP_LESS_EQUAL)->Bind(0xff);
		m_resLib.Get<RasterizerState>(RS_CULL_NONE)->Bind();

		auto vs = m_resLib.Get<VertexShader>(VS_SKYBOX);
		auto ps = m_resLib.Get<PixelShader>(PS_SKYBOX);
		vs->Bind();
		ps->Bind();
		m_resLib.Get<InputLayout>(IL_SKYBOX)->Bind();

		auto cbuf = m_resLib.Get<Buffer>(CB_VS_SKYBOX_SYSTEM);
		cbuf->VSBindAsCBuf(vs->GetResBinding("EntityCBuf"));
		cbuf->SetData(&viewProj);

		m_resLib.Get<SamplerState>(SS_POINT_CLAMP)->PSBind(ps->GetResBinding("sam"));
		GetRegistry().get<SkyboxComponent>(*m_skybox.data()).skybox->PSBind(ps->GetResBinding("tex"));

		m_resLib.Get<Buffer>(VB_CUBE)->BindAsVB();
		auto cbIb = m_resLib.Get<Buffer>(IB_CUBE);
		cbIb->BindAsIB(DXGI_FORMAT_R32_UINT);

		m_context->GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		GDX11_CONTEXT_THROW_INFO_ONLY(m_context->GetDeviceContext()->DrawIndexed(cbIb->GetDesc().ByteWidth / sizeof(uint32_t), 0, 0));
	}

	void LambertianRenderGraph::TransparentPhongPass(const DirectX::XMFLOAT3& viewPos, const DirectX::XMFLOAT4X4& viewProj /*column major*/)
	{
		auto rtva = m_resLib.Get<RenderTargetViewArray>(RTVA_TRANSPARENT_PHONG_PASS);
		auto dsv = m_resLib.Get<DepthStencilView>(DSV_SCENE);
		rtva->at(0)->Clear(0.0f, 0.0f, 0.0f, 0.0f); // accumulation
		rtva->at(1)->Clear(1.0f, 1.0f, 1.0f, 1.0f); // reveal

		if (m_renderable.empty()) return;

		m_resLib.Get<RasterizerState>(RS_CULL_NONE)->Bind();
		m_resLib.Get<BlendState>(BS_WEIGHTED_BLENDED_OIT_OP)->Bind(nullptr, 0xff);
		m_resLib.Get<DepthStencilState>(DSS_DEPTH_WRITE_ZERO)->Bind(0xff);

		RenderTargetView::Bind(*rtva, dsv.get());

		auto vs = m_resLib.Get<VertexShader>(VS_PHONG);
		auto ps = m_resLib.Get<PixelShader>(PS_PHONG_OIT);
		vs->Bind();
		ps->Bind();
		m_resLib.Get<InputLayout>(IL_PHONG)->Bind();

		// system cbufs
		{
			m_resLib.Get<Buffer>(CB_PS_PHONG_SYSTEM)->PSBindAsCBuf(ps->GetResBinding("SystemCBuf"));

			GA::Utils::PhongVSSystemCBuf cbufData = {};
			cbufData.viewPos = viewPos;
			cbufData.viewProjection = viewProj;

			auto cbuf = m_resLib.Get<Buffer>(CB_VS_PHONG_SYSTEM);
			cbuf->SetData(&cbufData);
			cbuf->VSBindAsCBuf(vs->GetResBinding("SystemCBuf"));
		}

		m_resLib.Get<ShaderResourceView>(SRV_DIRLIGHT_SHADOW_MAP)->PSBind(ps->GetResBinding("dirLightShadowMaps"));
		m_resLib.Get<SamplerState>(SS_LINEAR_CLAMP)->PSBind(ps->GetResBinding("dirLightShadowMapsSampler"));
		m_resLib.Get<ShaderResourceView>(SRV_POINTLIGHT_SHADOW_MAP)->PSBind(ps->GetResBinding("pointLightShadowMaps"));
		m_resLib.Get<SamplerState>(SS_LINEAR_CLAMP)->PSBind(ps->GetResBinding("pointLightShadowMapsSampler"));
		m_resLib.Get<ShaderResourceView>(SRV_SPOTLIGHT_SHADOW_MAP)->PSBind(ps->GetResBinding("spotLightShadowMaps"));
		m_resLib.Get<SamplerState>(SS_LINEAR_CLAMP)->PSBind(ps->GetResBinding("spotLightShadowMapsSampler"));

		for (const auto& e : m_renderable)
		{
			const auto& [transform, mesh, mat] = GetRegistry().get<TransformComponent, MeshComponent, MaterialComponent>(e);

			if (mat.color.w >= (1.0f - GA_UTILS_EPSILONF))
				continue;

			mesh.vb->BindAsVB();
			mesh.ib->BindAsIB(DXGI_FORMAT_R32_UINT);

			mat.diffuseMap->PSBind(ps->GetResBinding("diffuseMap"));
			mat.samplerState->PSBind(ps->GetResBinding("samplerState"));

			if (mat.normalMap)
			{
				mat.normalMap->PSBind(ps->GetResBinding("normalMap"));
				if (mat.depthMap)
					mat.depthMap->PSBind(ps->GetResBinding("depthMap"));
			}

			XMMATRIX xmTransform = transform.GetTransform();
			XMFLOAT4X4 fTransform;
			XMFLOAT4X4 normalMatrix;
			XMStoreFloat4x4(&fTransform, XMMatrixTranspose(xmTransform));
			XMStoreFloat4x4(&normalMatrix, XMMatrixInverse(nullptr, xmTransform));

			{
				GA::Utils::PhongVSEntityCBuf cbufData = {};
				cbufData.transform = fTransform;
				cbufData.normalMatrix = normalMatrix;

				auto cbuf = m_resLib.Get<Buffer>(CB_VS_PHONG_ENTITY);
				cbuf->SetData(&cbufData);
				cbuf->VSBindAsCBuf(vs->GetResBinding("EntityCBuf"));
			}

			{
				GA::Utils::PhongPSEntityCBuf cbufData = {};
				cbufData.mat.color = mat.color;
				cbufData.mat.tiling = mat.tiling;
				cbufData.mat.shininess = mat.shininess;
				cbufData.mat.enableNormalMapping = mat.normalMap ? TRUE : FALSE;
				cbufData.mat.enableParallaxMapping = mat.normalMap && mat.depthMap ? TRUE : FALSE;
				cbufData.mat.depthMapScale = mat.depthMapScale;
				cbufData.receiveShadows = mesh.receiveShadows;

				auto cbuf = m_resLib.Get<Buffer>(CB_PS_PHONG_ENTITY);
				cbuf->PSBindAsCBuf(ps->GetResBinding("EntityCBuf"));
				cbuf->SetData(&cbufData);
			}

			m_context->GetDeviceContext()->IASetPrimitiveTopology(mesh.topology);
			GDX11_CONTEXT_THROW_INFO_ONLY(m_context->GetDeviceContext()->DrawIndexed(mesh.ib->GetDesc().ByteWidth / sizeof(uint32_t), 0, 0));
		}
	}

	void LambertianRenderGraph::CompositePass()
	{
		m_resLib.Get<RasterizerState>(S_DEFAULT)->Bind();
		m_resLib.Get<BlendState>(BS_OVER_OP)->Bind(nullptr, 0xff);

		m_resLib.Get<RenderTargetView>(RTV_SCENE)->Bind(nullptr);

		m_resLib.Get<VertexShader>(VS_FS_OUT_POS)->Bind();
		auto ps = m_resLib.Get<PixelShader>(PS_PHONG_OIT_COMPOSITE);
		ps->Bind();
		m_resLib.Get<InputLayout>(IL_FS_OUT_POS)->Bind();

		m_resLib.Get<ShaderResourceView>(SRV_TRANSPARENT_PHONG_PASS_ACC)->PSBind(ps->GetResBinding("accumulationMap"));
		m_resLib.Get<ShaderResourceView>(SRV_TRANSPARENT_PHONG_PASS_REV)->PSBind(ps->GetResBinding("revealMap"));

		m_resLib.Get<Buffer>(VB_FS_QUAD)->BindAsVB();
		auto ib = m_resLib.Get<Buffer>(IB_FS_QUAD);
		ib->BindAsIB(DXGI_FORMAT_R32_UINT);
		m_context->GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		GDX11_CONTEXT_THROW_INFO_ONLY(m_context->GetDeviceContext()->DrawIndexed(ib->GetDesc().ByteWidth / sizeof(uint32_t), 0, 0));
	}

	void LambertianRenderGraph::GammaCorrectionPass()
	{
		m_resLib.Get<RasterizerState>(S_DEFAULT)->Bind();

		auto rtv = m_resLib.Get<RenderTargetView>(RTV_MAIN);
		rtv->Clear(0.0f, 0.0f, 0.0f, 0.0f);
		rtv->Bind(nullptr);

		m_resLib.Get<VertexShader>(VS_FS_OUT_TC_POS)->Bind();
		auto ps = m_resLib.Get<PixelShader>(PS_GAMMA_CORRECTION);
		ps->Bind();
		m_resLib.Get<InputLayout>(IL_FS_OUT_TC_POS)->Bind();

		m_resLib.Get<ShaderResourceView>(SRV_SCENE)->PSBind(ps->GetResBinding("tex"));
		m_resLib.Get<SamplerState>(SS_POINT_CLAMP)->PSBind(ps->GetResBinding("samplerState"));

		DirectX::XMFLOAT4 gamma = { GAMMA, 0.0f, 0.0f, 0.0f };
		auto cbuf = m_resLib.Get<Buffer>(CB_PS_GAMMA_CORRECTION_SYSTEM);
		cbuf->SetData(&gamma);
		cbuf->PSBindAsCBuf(ps->GetResBinding("SystemCBuf"));

		m_resLib.Get<Buffer>(VB_FS_QUAD)->BindAsVB();
		auto ib = m_resLib.Get<Buffer>(IB_FS_QUAD);
		ib->BindAsIB(DXGI_FORMAT_R32_UINT);
		m_context->GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		GDX11_CONTEXT_THROW_INFO_ONLY(m_context->GetDeviceContext()->DrawIndexed(ib->GetDesc().ByteWidth / sizeof(uint32_t), 0, 0));
	}

	void LambertianRenderGraph::SetLights()
	{
		GA::Utils::PhongPSSystemCBuf psSysCbuf = {};
		psSysCbuf.activeDirLights = (uint32_t)m_dirLights.size();
		psSysCbuf.activePointLights = (uint32_t)m_pointLights.size();
		psSysCbuf.activeSpotLights = (uint32_t)m_spotLights.size();

		D3D11_VIEWPORT vp = {};
		vp.TopLeftX = 0.0f;
		vp.TopLeftY = 0.0f;
		vp.Width = (float)SHADOWMAP_SIZE;
		vp.Height = (float)SHADOWMAP_SIZE;
		vp.MinDepth = 0.0f;
		vp.MaxDepth = 1.0f;
		m_context->GetDeviceContext()->RSSetViewports(1, &vp);

		m_resLib.Get<RasterizerState>(RS_DEPTH_SLOPE_SCALED_BIAS)->Bind();
		m_resLib.Get<BlendState>(S_DEFAULT)->Bind(nullptr, 0xff);
		m_resLib.Get<DepthStencilState>(S_DEFAULT)->Bind(0xff);

		uint32_t index = 0;
		for (const auto& e : m_dirLights)
		{
			const auto& [transform, dirLight] = GetRegistry().get<TransformComponent, DirectionalLightComponent>(e);

			XMVECTOR xmDirection = transform.GetForward();
			XMFLOAT3 direction;
			XMStoreFloat3(&direction, xmDirection);
			XMMATRIX xmLightSpace = XMMatrixInverse(nullptr, transform.GetTransform()) * XMMatrixOrthographicLH(20.0f, 20.0f, 0.1f, 500.0f);
			XMFLOAT4X4 lightSpace;
			XMStoreFloat4x4(&lightSpace, XMMatrixTranspose(xmLightSpace));

			psSysCbuf.dirLights[index].direction = direction;
			psSysCbuf.dirLights[index].color = dirLight.color;
			psSysCbuf.dirLights[index].ambientIntensity = dirLight.ambientIntensity;
			psSysCbuf.dirLights[index].intensity = dirLight.intensity;
			psSysCbuf.dirLights[index].lightSpace = lightSpace;


			// shadow map pass
			auto dsv = m_resLib.Get<DepthStencilView>(DSV_DIRLIGHT_SHADOW_MAP(index));
			dsv->Clear(D3D11_CLEAR_DEPTH, 1.0f, 0xff);

			if (m_renderable.empty()) return;

			dsv->Bind();
			// todo: cant run this in graphics debug. Have to bind a rtv because of stupid warning
			// m_resLib.Get<RenderTargetView>(RTV_MAIN)->Bind(dsv.get());

			auto vs = m_resLib.Get<VertexShader>(VS_BASIC);
			vs->Bind();
			m_resLib.Get<PixelShader>(PS_NULLPTR)->Bind();
			m_resLib.Get<InputLayout>(IL_BASIC)->Bind();

			{
				auto cbuf = m_resLib.Get<Buffer>(CB_VS_BASIC_SYSTEM);
				cbuf->SetData(&lightSpace);
				cbuf->VSBindAsCBuf(vs->GetResBinding("SystemCBuf"));
			}

			// draw to depth map
			for (const auto& e : m_renderable)
			{
				const auto& [transform, mesh] = GetRegistry().get<TransformComponent, MeshComponent>(e);

				if (!mesh.castShadows) continue;

				{
					XMFLOAT4X4 fTransform; 
					XMStoreFloat4x4(&fTransform, XMMatrixTranspose(transform.GetTransform()));
					auto cbuf = m_resLib.Get<Buffer>(CB_VS_BASIC_ENTITY);
					cbuf->SetData(&fTransform);
					cbuf->VSBindAsCBuf(vs->GetResBinding("EntityCBuf"));
				}

				mesh.vb->BindAsVB();
				mesh.ib->BindAsIB(DXGI_FORMAT_R32_UINT);
				m_context->GetDeviceContext()->IASetPrimitiveTopology(mesh.topology);

				GDX11_CONTEXT_THROW_INFO_ONLY(m_context->GetDeviceContext()->DrawIndexed(mesh.ib->GetDesc().ByteWidth / sizeof(uint32_t), 0, 0));
			}

			++index;
		}

		index = 0;
		for (const auto& e : m_pointLights)
		{
			const auto& [transform, pointLight] = GetRegistry().get<TransformComponent, PointLightComponent>(e);

			XMFLOAT4X4 lightSpace;
			XMStoreFloat4x4(&lightSpace, XMMatrixTranspose(XMMatrixTranslation(-transform.position.x, -transform.position.y, -transform.position.z)));

			psSysCbuf.pointLights[index].position = transform.position;
			psSysCbuf.pointLights[index].color = pointLight.color;
			psSysCbuf.pointLights[index].ambientIntensity = pointLight.ambientIntensity;
			psSysCbuf.pointLights[index].intensity = pointLight.intensity;
			psSysCbuf.pointLights[index].nearZ = pointLight.shadowNearZ;
			psSysCbuf.pointLights[index].farZ = pointLight.shadowFarZ;
			psSysCbuf.pointLights[index].lightSpace = lightSpace;

			// shadow map pass
			auto dsv = m_resLib.Get<DepthStencilView>(DSV_POINTLIGHT_SHADOW_MAP(index));
			dsv->Clear(D3D11_CLEAR_DEPTH, 1.0f, 0xff);

			if (m_renderable.empty()) return;

			dsv->Bind();

			auto vs = m_resLib.Get<VertexShader>(VS_CUBE_SHADOW_MAP);
			auto gs = m_resLib.Get<GeometryShader>(GS_CUBE_SHADOW_MAP);
			vs->Bind();
			gs->Bind();
			m_resLib.Get<PixelShader>(PS_NULLPTR)->Bind();
			m_resLib.Get<InputLayout>(VS_CUBE_SHADOW_MAP)->Bind();

			XMMATRIX xmPointLightProjection = XMMatrixPerspectiveFovLH(XMConvertToRadians(90.0f), 1.0f, pointLight.shadowNearZ, pointLight.shadowFarZ);
			XMMATRIX xmPointLightTranslation = XMMatrixTranslation(transform.position.x, transform.position.y, transform.position.z);
			XMMATRIX xmPointLightSpace[6] =
			{
				// + x
				XMMatrixInverse(nullptr, XMMatrixRotationQuaternion(XMQuaternionRotationRollPitchYaw(0.0f, XMConvertToRadians(90.0f), 0.0f)) * xmPointLightTranslation) * xmPointLightProjection,

				// -x
				XMMatrixInverse(nullptr, XMMatrixRotationQuaternion(XMQuaternionRotationRollPitchYaw(0.0f, XMConvertToRadians(-90.0f), 0.0f)) * xmPointLightTranslation) * xmPointLightProjection,

				// +y
				XMMatrixInverse(nullptr, XMMatrixRotationQuaternion(XMQuaternionRotationRollPitchYaw(XMConvertToRadians(-90.0f), 0.0f, 0.0f)) * xmPointLightTranslation) * xmPointLightProjection,

				// -y
				XMMatrixInverse(nullptr, XMMatrixRotationQuaternion(XMQuaternionRotationRollPitchYaw(XMConvertToRadians(90.0f), 0.0f, 0.0f)) * xmPointLightTranslation) * xmPointLightProjection,

				// +z
				XMMatrixInverse(nullptr, XMMatrixRotationQuaternion(XMQuaternionRotationRollPitchYaw(0.0, 0.0f, 0.0f)) * xmPointLightTranslation) * xmPointLightProjection,

				// -z
				XMMatrixInverse(nullptr, XMMatrixRotationQuaternion(XMQuaternionRotationRollPitchYaw(0.0, XMConvertToRadians(180.0f), 0.0f)) * xmPointLightTranslation) * xmPointLightProjection,
			};

			XMFLOAT4X4 pointLightSpace[6];
			for (int i = 0; i < 6; i++)
				XMStoreFloat4x4(&pointLightSpace[i], XMMatrixTranspose(xmPointLightSpace[i]));

			{
				auto cbuf = m_resLib.Get<Buffer>(CB_GS_CUBE_SHADOW_MAP_SYSTEM);
				cbuf->SetData(pointLightSpace);
				cbuf->GSBindAsCBuf(gs->GetResBinding("SystemCBuf"));
			}

			// draw to depth map
			for (const auto& e : m_renderable)
			{
				const auto& [transform, mesh] = GetRegistry().get<TransformComponent, MeshComponent>(e);

				if (!mesh.castShadows) continue;

				{
					XMFLOAT4X4 fTransform;
					XMStoreFloat4x4(&fTransform, XMMatrixTranspose(transform.GetTransform()));
					auto cbuf = m_resLib.Get<Buffer>(CB_VS_CUBE_SHADOW_MAP_ENTITY);
					cbuf->SetData(&fTransform);
					cbuf->VSBindAsCBuf(vs->GetResBinding("EntityCBuf"));
				}

				mesh.vb->BindAsVB();
				mesh.ib->BindAsIB(DXGI_FORMAT_R32_UINT);
				m_context->GetDeviceContext()->IASetPrimitiveTopology(mesh.topology);

				GDX11_CONTEXT_THROW_INFO_ONLY(m_context->GetDeviceContext()->DrawIndexed(mesh.ib->GetDesc().ByteWidth / sizeof(uint32_t), 0, 0));
			}

			m_resLib.Get<GeometryShader>(GS_NULLPTR)->Bind();
			
			++index;
		}

		index = 0;
		for (const auto& e : m_spotLights)
		{
			const auto& [transform, spotLight] = GetRegistry().get<TransformComponent, SpotLightComponent>(e);

			XMVECTOR xmDirection = transform.GetForward();
			XMFLOAT3 direction;
			XMStoreFloat3(&direction, xmDirection);

			XMMATRIX xmLightSpace = XMMatrixInverse(nullptr, transform.GetTransform()) * XMMatrixPerspectiveFovLH(XMConvertToRadians(90.0f), 1.0f, spotLight.shadowNearZ, spotLight.shadowFarZ);
			XMFLOAT4X4 lightSpace;
			XMStoreFloat4x4(&lightSpace, XMMatrixTranspose(xmLightSpace));

			psSysCbuf.spotLights[index].direction = direction;
			psSysCbuf.spotLights[index].position = transform.position;
			psSysCbuf.spotLights[index].color = spotLight.color;
			psSysCbuf.spotLights[index].ambientIntensity = spotLight.ambientIntensity;
			psSysCbuf.spotLights[index].intensity = spotLight.intensity;
			psSysCbuf.spotLights[index].innerCutOffCosAngle = cosf(XMConvertToRadians(spotLight.innerCutOffAngle));
			psSysCbuf.spotLights[index].outerCutOffCosAngle = cosf(XMConvertToRadians(spotLight.outerCutOffAngle));
			psSysCbuf.spotLights[index].lightSpace = lightSpace;

			// shadow map pass
			auto dsv = m_resLib.Get<DepthStencilView>(DSV_SPOTLIGHT_SHADOW_MAP(index));
			dsv->Clear(D3D11_CLEAR_DEPTH, 1.0f, 0xff);

			if (m_renderable.empty()) return;

			dsv->Bind();
			// todo: cant run this in graphics debug. Have to bind a rtv because of stupid warning
			// m_resLib.Get<RenderTargetView>(RTV_MAIN)->Bind(dsv.get());

			auto vs = m_resLib.Get<VertexShader>(VS_BASIC);
			vs->Bind();
			m_resLib.Get<PixelShader>(PS_NULLPTR)->Bind();
			m_resLib.Get<InputLayout>(IL_BASIC)->Bind();

			{
				auto cbuf = m_resLib.Get<Buffer>(CB_VS_BASIC_SYSTEM);
				cbuf->SetData(&lightSpace);
				cbuf->VSBindAsCBuf(vs->GetResBinding("SystemCBuf"));
			}

			// draw to depth map
			for (const auto& e : m_renderable)
			{
				const auto& [transform, mesh] = GetRegistry().get<TransformComponent, MeshComponent>(e);

				if (!mesh.castShadows) continue;

				{
					XMFLOAT4X4 fTransform;
					XMStoreFloat4x4(&fTransform, XMMatrixTranspose(transform.GetTransform()));
					auto cbuf = m_resLib.Get<Buffer>(CB_VS_BASIC_ENTITY);
					cbuf->SetData(&fTransform);
					cbuf->VSBindAsCBuf(vs->GetResBinding("EntityCBuf"));
				}

				mesh.vb->BindAsVB();
				mesh.ib->BindAsIB(DXGI_FORMAT_R32_UINT);
				m_context->GetDeviceContext()->IASetPrimitiveTopology(mesh.topology);

				GDX11_CONTEXT_THROW_INFO_ONLY(m_context->GetDeviceContext()->DrawIndexed(mesh.ib->GetDesc().ByteWidth / sizeof(uint32_t), 0, 0));
			}

			++index;
		}

		m_resLib.Get<Buffer>(CB_PS_PHONG_SYSTEM)->SetData(&psSysCbuf);
	}






















	void LambertianRenderGraph::ResizeViews(uint32_t width, uint32_t height)
	{
		m_windowWidth = width;
		m_windowHeight = height;

		// main rtv
		{
			if (m_resLib.Exist<RenderTargetView>(RTV_MAIN))
			{
				m_resLib.Remove<RenderTargetView>(RTV_MAIN);
				HRESULT hr;
				GDX11_CONTEXT_THROW_INFO(m_context->GetSwapChain()->ResizeBuffers(1, width, height, DXGI_FORMAT_R8G8B8A8_UNORM, 0));
			}

			D3D11_RENDER_TARGET_VIEW_DESC desc = {};
			desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
			desc.Texture2D.MipSlice = 0;
			ComPtr<ID3D11Texture2D> backbuffer;
			HRESULT hr;
			GDX11_CONTEXT_THROW_INFO(m_context->GetSwapChain()->GetBuffer(0, __uuidof(ID3D11Texture2D), &backbuffer));
			m_resLib.Add(RTV_MAIN, RenderTargetView::Create(m_context, desc, Texture2D::Create(m_context, backbuffer.Get())));
		}

		// scene rtv
		{
			if (m_resLib.Exist<RenderTargetView>(RTV_SCENE))
			{
				m_resLib.Remove<RenderTargetView>(RTV_SCENE);
				m_resLib.Remove<ShaderResourceView>(RTV_SCENE);
			}

			D3D11_RENDER_TARGET_VIEW_DESC desc = {};
			desc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
			desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
			desc.Texture2D.MipSlice = 0;

			D3D11_TEXTURE2D_DESC texDesc = {};
			texDesc.Width = width;
			texDesc.Height = height;
			texDesc.ArraySize = 1;
			texDesc.MipLevels = 1;
			texDesc.Format = desc.Format;
			texDesc.SampleDesc.Count = 1;
			texDesc.SampleDesc.Quality = 0;
			texDesc.Usage = D3D11_USAGE_DEFAULT;
			texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
			texDesc.CPUAccessFlags = 0;
			texDesc.MiscFlags = 0;

			auto tex = Texture2D::Create(m_context, texDesc, (void*)nullptr);

			D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			srvDesc.Format = texDesc.Format;
			srvDesc.Texture2D.MipLevels = 1;
			srvDesc.Texture2D.MostDetailedMip = 0;
			srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;

			m_resLib.Add(RTV_SCENE, ShaderResourceView::Create(m_context, srvDesc, tex));
			m_resLib.Add(RTV_SCENE, RenderTargetView::Create(m_context, desc, tex));
		}


		// scene dsv
		{
			if (m_resLib.Exist<DepthStencilView>(DSV_SCENE))
				m_resLib.Remove<DepthStencilView>(DSV_SCENE);

			D3D11_DEPTH_STENCIL_VIEW_DESC desc = {};
			desc.Format = DXGI_FORMAT_D32_FLOAT;
			desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
			desc.Texture2D.MipSlice = 0;

			D3D11_TEXTURE2D_DESC texDesc = {};
			texDesc.Width = width;
			texDesc.Height = height;
			texDesc.ArraySize = 1;
			texDesc.MipLevels = 1;
			texDesc.Format = desc.Format;
			texDesc.SampleDesc.Count = 1;
			texDesc.SampleDesc.Quality = 0;
			texDesc.Usage = D3D11_USAGE_DEFAULT;
			texDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
			texDesc.CPUAccessFlags = 0;
			texDesc.MiscFlags = 0;

			m_resLib.Add(DSV_SCENE, DepthStencilView::Create(m_context, desc, Texture2D::Create(m_context, texDesc, (void*)nullptr)));
		}

		// transparent mesh rtv
		{
			if (m_resLib.Exist<RenderTargetViewArray>(RTVA_TRANSPARENT_PHONG_PASS))
			{
				m_resLib.Remove<RenderTargetViewArray>(RTVA_TRANSPARENT_PHONG_PASS);
				m_resLib.Remove<ShaderResourceView>(SRV_TRANSPARENT_PHONG_PASS_ACC);
				m_resLib.Remove<ShaderResourceView>(SRV_TRANSPARENT_PHONG_PASS_REV);
			}

			auto rtvArr = std::make_shared<RenderTargetViewArray>();

			// accumulation rtv
			{
				D3D11_RENDER_TARGET_VIEW_DESC desc = {};
				desc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
				desc.Texture2D.MipSlice = 0;
				desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;

				D3D11_TEXTURE2D_DESC texDesc = {};
				texDesc.Width = width;
				texDesc.Height = height;
				texDesc.ArraySize = 1;
				texDesc.MipLevels = 1;
				texDesc.Format = desc.Format;
				texDesc.SampleDesc.Count = 1;
				texDesc.SampleDesc.Quality = 0;
				texDesc.Usage = D3D11_USAGE_DEFAULT;
				texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
				texDesc.CPUAccessFlags = 0;
				texDesc.MiscFlags = 0;

				D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
				srvDesc.Format = texDesc.Format;
				srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
				srvDesc.Texture2D.MipLevels = 1;
				srvDesc.Texture2D.MostDetailedMip = 0;

				auto tex = Texture2D::Create(m_context, texDesc, (void*)nullptr);
				auto rtv = RenderTargetView::Create(m_context, desc, tex);
				m_resLib.Add(SRV_TRANSPARENT_PHONG_PASS_ACC, ShaderResourceView::Create(m_context, srvDesc, tex));

				rtvArr->push_back(rtv);
			}

			// reveal rtv
			{
				D3D11_RENDER_TARGET_VIEW_DESC desc = {};
				desc.Format = DXGI_FORMAT_R8_UNORM;
				desc.Texture2D.MipSlice = 0;
				desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;

				D3D11_TEXTURE2D_DESC texDesc = {};
				texDesc.Width = width;
				texDesc.Height = height;
				texDesc.ArraySize = 1;
				texDesc.MipLevels = 1;
				texDesc.Format = desc.Format;
				texDesc.SampleDesc.Count = 1;
				texDesc.SampleDesc.Quality = 0;
				texDesc.Usage = D3D11_USAGE_DEFAULT;
				texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
				texDesc.CPUAccessFlags = 0;
				texDesc.MiscFlags = 0;

				D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
				srvDesc.Format = texDesc.Format;
				srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
				srvDesc.Texture2D.MipLevels = 1;
				srvDesc.Texture2D.MostDetailedMip = 0;

				auto tex = Texture2D::Create(m_context, texDesc, (void*)nullptr);
				auto rtv = RenderTargetView::Create(m_context, desc, tex);
				m_resLib.Add(SRV_TRANSPARENT_PHONG_PASS_REV, ShaderResourceView::Create(m_context, srvDesc, tex));

				rtvArr->push_back(rtv);
			}

			m_resLib.Add(RTVA_TRANSPARENT_PHONG_PASS, rtvArr);
		}
	}

	void LambertianRenderGraph::SetShaders()
	{
		m_resLib.Add(VS_PHONG, VertexShader::Create(m_context, "res/cso/phong.vs.cso"));
		m_resLib.Add(PS_PHONG, PixelShader::Create(m_context, "res/cso/phong.ps.cso"));
		m_resLib.Add(PS_PHONG_OIT, PixelShader::Create(m_context, "res/cso/phong_oit.ps.cso"));
		m_resLib.Add(IL_PHONG, InputLayout::Create(m_context, m_resLib.Get<VertexShader>(VS_PHONG)));

		m_resLib.Add(VS_FS_OUT_TC_POS, VertexShader::Create(m_context, "res/cso/fullscreen_out_tc_pos.vs.cso"));
		m_resLib.Add(IL_FS_OUT_TC_POS, InputLayout::Create(m_context, m_resLib.Get<VertexShader>(VS_FS_OUT_TC_POS)));

		m_resLib.Add(VS_FS_OUT_POS, VertexShader::Create(m_context, "res/cso/fullscreen_out_pos.vs.cso"));
		m_resLib.Add(IL_FS_OUT_POS, InputLayout::Create(m_context, m_resLib.Get<VertexShader>(VS_FS_OUT_POS)));

		m_resLib.Add(PS_PHONG_OIT_COMPOSITE, PixelShader::Create(m_context, "res/cso/phong_oit_composite.ps.cso"));
		m_resLib.Add(PS_GAMMA_CORRECTION, PixelShader::Create(m_context, "res/cso/gamma_correction.ps.cso"));

		m_resLib.Add(VS_SKYBOX, VertexShader::Create(m_context, "res/cso/skybox.vs.cso"));
		m_resLib.Add(PS_SKYBOX, PixelShader::Create(m_context, "res/cso/skybox.ps.cso"));
		m_resLib.Add(IL_SKYBOX, InputLayout::Create(m_context, m_resLib.Get<VertexShader>(VS_SKYBOX)));

		m_resLib.Add(VS_BASIC, VertexShader::Create(m_context, "res/cso/basic.vs.cso"));
		m_resLib.Add(PS_NULLPTR, PixelShader::Create(m_context));
		m_resLib.Add(IL_BASIC, InputLayout::Create(m_context, m_resLib.Get<VertexShader>(VS_BASIC)));

		m_resLib.Add(VS_CUBE_SHADOW_MAP, VertexShader::Create(m_context, "res/cso/cube_shadow_map.vs.cso"));
		m_resLib.Add(GS_CUBE_SHADOW_MAP, GeometryShader::Create(m_context, "res/cso/cube_shadow_map.gs.cso"));
		m_resLib.Add(IL_CUBE_SHADOW_MAP, InputLayout::Create(m_context, m_resLib.Get<VertexShader>(VS_CUBE_SHADOW_MAP)));

		m_resLib.Add(GS_NULLPTR, GeometryShader::Create(m_context));
	}

	void LambertianRenderGraph::SetStates()
	{
		// rs
		{
			m_resLib.Add(S_DEFAULT, RasterizerState::Create(m_context, CD3D11_RASTERIZER_DESC(CD3D11_DEFAULT())));

			D3D11_RASTERIZER_DESC desc = CD3D11_RASTERIZER_DESC(CD3D11_DEFAULT());
			desc.CullMode = D3D11_CULL_NONE;
			m_resLib.Add(RS_CULL_NONE, RasterizerState::Create(m_context, desc));

			desc = CD3D11_RASTERIZER_DESC(CD3D11_DEFAULT());
			desc.DepthBias = 40;
			desc.SlopeScaledDepthBias = 6.0f;
			desc.DepthBiasClamp = 1.0f;
			m_resLib.Add(RS_DEPTH_SLOPE_SCALED_BIAS, RasterizerState::Create(m_context, desc));
		}

		// bs
		{
			m_resLib.Add(S_DEFAULT, BlendState::Create(m_context, CD3D11_BLEND_DESC(CD3D11_DEFAULT())));

			// over op
			D3D11_BLEND_DESC desc = CD3D11_BLEND_DESC(CD3D11_DEFAULT());
			desc.AlphaToCoverageEnable = FALSE;
			desc.IndependentBlendEnable = FALSE;
			desc.RenderTarget[0].BlendEnable = TRUE;
			desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
			desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
			desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
			desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
			desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
			m_resLib.Add(BS_OVER_OP, BlendState::Create(m_context, desc));



			//  weighted blended oit 
			desc = CD3D11_BLEND_DESC(CD3D11_DEFAULT());
			desc.AlphaToCoverageEnable = FALSE;
			desc.IndependentBlendEnable = TRUE;

			// accummulation
			auto& brt0 = desc.RenderTarget[0];
			brt0.BlendEnable = TRUE;
			brt0.SrcBlend = D3D11_BLEND_ONE;
			brt0.DestBlend = D3D11_BLEND_ONE;
			brt0.BlendOp = D3D11_BLEND_OP_ADD;
			brt0.SrcBlendAlpha = D3D11_BLEND_ONE;
			brt0.DestBlendAlpha = D3D11_BLEND_ONE;
			brt0.BlendOpAlpha = D3D11_BLEND_OP_ADD;
			brt0.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

			// reveal
			auto& brt1 = desc.RenderTarget[1];
			brt1.BlendEnable = TRUE;
			brt1.SrcBlend = D3D11_BLEND_ZERO;
			brt1.DestBlend = D3D11_BLEND_INV_SRC_COLOR;
			brt1.BlendOp = D3D11_BLEND_OP_ADD;
			brt1.SrcBlendAlpha = D3D11_BLEND_ZERO;
			brt1.DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
			brt1.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
			m_resLib.Add(BS_WEIGHTED_BLENDED_OIT_OP, BlendState::Create(m_context, desc));
		}

		// dss
		{
			m_resLib.Add(S_DEFAULT, DepthStencilState::Create(m_context, CD3D11_DEPTH_STENCIL_DESC(CD3D11_DEFAULT())));

			D3D11_DEPTH_STENCIL_DESC desc = CD3D11_DEPTH_STENCIL_DESC(CD3D11_DEFAULT());
			desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
			m_resLib.Add(DSS_DEPTH_WRITE_ZERO, DepthStencilState::Create(m_context, desc));

			desc = CD3D11_DEPTH_STENCIL_DESC(CD3D11_DEFAULT());
			desc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
			desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
			m_resLib.Add(DSS_DEPTH_WRITE_ZERO_OP_LESS_EQUAL, DepthStencilState::Create(m_context, desc));
		}

		// ss
		{
			D3D11_SAMPLER_DESC desc = CD3D11_SAMPLER_DESC(CD3D11_DEFAULT());
			desc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
			desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
			desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
			desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
			for (int i = 0; i < 4; i++)
				desc.BorderColor[i] = 0.0f;
			desc.MinLOD = 0.0f;
			desc.MaxLOD = D3D11_FLOAT32_MAX;
			m_resLib.Add(SS_POINT_CLAMP, SamplerState::Create(m_context, desc));

			desc = CD3D11_SAMPLER_DESC(CD3D11_DEFAULT());
			desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
			desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
			desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
			desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
			for (int i = 0; i < 4; i++)
				desc.BorderColor[i] = 0.0f;
			desc.MinLOD = 0.0f;
			desc.MaxLOD = D3D11_FLOAT32_MAX;
			m_resLib.Add(SS_LINEAR_CLAMP, SamplerState::Create(m_context, desc));
		}
	}

	void LambertianRenderGraph::SetBuffers()
	{
		// fs quad
		{
			float vert[] =
			{
				-1.0f,  1.0f,
				 1.0f,  1.0f,
				 1.0f, -1.0f,
				-1.0f, -1.0f
			};

			uint32_t ind[] = 
			{
				0, 1, 2,
				2, 3, 0
			};

			D3D11_BUFFER_DESC desc = {};
			desc.ByteWidth = sizeof(vert);
			desc.Usage = D3D11_USAGE_DEFAULT;
			desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
			desc.CPUAccessFlags = 0;
			desc.MiscFlags = 0;
			desc.StructureByteStride = 2 * sizeof(float);
			m_resLib.Add(VB_FS_QUAD, Buffer::Create(m_context, desc, vert));

			desc = {};
			desc.ByteWidth = sizeof(ind);
			desc.Usage = D3D11_USAGE_DEFAULT;
			desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
			desc.CPUAccessFlags = 0;
			desc.MiscFlags = 0;
			desc.StructureByteStride = sizeof(uint32_t);
			m_resLib.Add(IB_FS_QUAD, Buffer::Create(m_context, desc, ind));
		}

		// cube
		{
			auto vert = GA::Utils::CreateCubeVerticesCompact(-1.0f, 1.0f);
			auto ind = GA::Utils::CreateCubeIndicesCompact();

			D3D11_BUFFER_DESC desc = {};
			desc.ByteWidth = (uint32_t)vert.size() * sizeof(float);
			desc.Usage = D3D11_USAGE_DEFAULT;
			desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
			desc.CPUAccessFlags = 0;
			desc.MiscFlags = 0;
			desc.StructureByteStride = 3 * sizeof(float);
			m_resLib.Add(VB_CUBE, Buffer::Create(m_context, desc, vert.data()));

			desc = {};
			desc.ByteWidth = (uint32_t)ind.size() * sizeof(uint32_t);
			desc.Usage = D3D11_USAGE_DEFAULT;
			desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
			desc.CPUAccessFlags = 0;
			desc.MiscFlags = 0;
			desc.StructureByteStride = sizeof(uint32_t);
			m_resLib.Add(IB_CUBE, Buffer::Create(m_context, desc, ind.data()));
		}

		// phong cbufs
		{
			D3D11_BUFFER_DESC desc = {};
			desc.ByteWidth = sizeof(GA::Utils::PhongVSSystemCBuf);
			desc.Usage = D3D11_USAGE_DYNAMIC;
			desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
			desc.MiscFlags = 0;
			desc.StructureByteStride = 0;
			m_resLib.Add(CB_VS_PHONG_SYSTEM, Buffer::Create(m_context, desc, nullptr));

			desc = {};
			desc.ByteWidth = sizeof(GA::Utils::PhongVSEntityCBuf);
			desc.Usage = D3D11_USAGE_DYNAMIC;
			desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
			desc.MiscFlags = 0;
			desc.StructureByteStride = 0;
			m_resLib.Add(CB_VS_PHONG_ENTITY, Buffer::Create(m_context, desc, nullptr));

			desc = {};
			desc.ByteWidth = sizeof(GA::Utils::PhongPSSystemCBuf);
			desc.Usage = D3D11_USAGE_DYNAMIC;
			desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
			desc.MiscFlags = 0;
			desc.StructureByteStride = 0;
			m_resLib.Add(CB_PS_PHONG_SYSTEM, Buffer::Create(m_context, desc, nullptr));

			desc = {};
			desc.ByteWidth = sizeof(GA::Utils::PhongPSEntityCBuf);
			desc.Usage = D3D11_USAGE_DYNAMIC;
			desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
			desc.MiscFlags = 0;
			desc.StructureByteStride = 0;
			m_resLib.Add(CB_PS_PHONG_ENTITY, Buffer::Create(m_context, desc, nullptr));
		}

		{
			D3D11_BUFFER_DESC desc = {};
			desc.ByteWidth = sizeof(XMFLOAT4);
			desc.Usage = D3D11_USAGE_DYNAMIC;
			desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
			desc.MiscFlags = 0;
			desc.StructureByteStride = 0;
			m_resLib.Add(CB_PS_GAMMA_CORRECTION_SYSTEM, Buffer::Create(m_context, desc, nullptr));
		}

		{
			D3D11_BUFFER_DESC desc = {};
			desc.ByteWidth = sizeof(XMFLOAT4X4);
			desc.Usage = D3D11_USAGE_DYNAMIC;
			desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
			desc.MiscFlags = 0;
			desc.StructureByteStride = 0;
			m_resLib.Add(CB_VS_SKYBOX_SYSTEM, Buffer::Create(m_context, desc, nullptr));
		}

		{
			D3D11_BUFFER_DESC desc = {};
			desc.ByteWidth = sizeof(XMFLOAT4X4);
			desc.Usage = D3D11_USAGE_DYNAMIC;
			desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
			desc.MiscFlags = 0;
			desc.StructureByteStride = 0;
			m_resLib.Add(CB_VS_BASIC_SYSTEM, Buffer::Create(m_context, desc, nullptr));
		}

		{
			D3D11_BUFFER_DESC desc = {};
			desc.ByteWidth = sizeof(XMFLOAT4X4);
			desc.Usage = D3D11_USAGE_DYNAMIC;
			desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
			desc.MiscFlags = 0;
			desc.StructureByteStride = 0;
			m_resLib.Add(CB_VS_BASIC_ENTITY, Buffer::Create(m_context, desc, nullptr));
		}


		{
			D3D11_BUFFER_DESC desc = {};
			desc.ByteWidth = sizeof(XMFLOAT4X4);
			desc.Usage = D3D11_USAGE_DYNAMIC;
			desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
			desc.MiscFlags = 0;
			desc.StructureByteStride = 0;
			m_resLib.Add(CB_VS_CUBE_SHADOW_MAP_ENTITY, Buffer::Create(m_context, desc, nullptr));
		}

		{
			D3D11_BUFFER_DESC desc = {};
			desc.ByteWidth = sizeof(XMFLOAT4X4) * 6;
			desc.Usage = D3D11_USAGE_DYNAMIC;
			desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
			desc.MiscFlags = 0;
			desc.StructureByteStride = 0;
			m_resLib.Add(CB_GS_CUBE_SHADOW_MAP_SYSTEM, Buffer::Create(m_context, desc, nullptr));
		}
	}

	void LambertianRenderGraph::SetLightDepthBuffers()
	{
		// dir light depth buffers
		{
			D3D11_TEXTURE2D_DESC texDesc = {};
			texDesc.Width = SHADOWMAP_SIZE;
			texDesc.Height = SHADOWMAP_SIZE;
			texDesc.ArraySize = GA::Utils::s_maxLights;
			texDesc.MipLevels = 1;
			texDesc.Format = DXGI_FORMAT_R32_TYPELESS;
			texDesc.SampleDesc.Count = 1;
			texDesc.SampleDesc.Quality = 0;
			texDesc.Usage = D3D11_USAGE_DEFAULT;
			texDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
			texDesc.CPUAccessFlags = 0;
			texDesc.MiscFlags = 0;
			auto tex = Texture2D::Create(m_context, texDesc, (void*)nullptr);

			for (int i = 0; i < GA::Utils::s_maxLights; i++)
			{
				D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
				dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
				dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
				dsvDesc.Texture2DArray.FirstArraySlice = i;
				dsvDesc.Texture2DArray.ArraySize = 1;
				dsvDesc.Texture2DArray.MipSlice = 0;
				m_resLib.Add(DSV_DIRLIGHT_SHADOW_MAP(i), DepthStencilView::Create(m_context, dsvDesc, tex));
			}

			D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
			srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
			srvDesc.Texture2DArray.ArraySize = GA::Utils::s_maxLights;
			srvDesc.Texture2DArray.FirstArraySlice = 0;
			srvDesc.Texture2DArray.MipLevels = 1;
			srvDesc.Texture2DArray.MostDetailedMip = 0;
			m_resLib.Add(SRV_DIRLIGHT_SHADOW_MAP, ShaderResourceView::Create(m_context, srvDesc, tex));
		}

		// point light depth buffers
		{
			D3D11_TEXTURE2D_DESC texDesc = {};
			texDesc.Width = SHADOWMAP_SIZE;
			texDesc.Height = SHADOWMAP_SIZE;
			texDesc.ArraySize = GA::Utils::s_maxLights * 6;
			texDesc.MipLevels = 1;
			texDesc.Format = DXGI_FORMAT_R32_TYPELESS;
			texDesc.SampleDesc.Count = 1;
			texDesc.SampleDesc.Quality = 0;
			texDesc.Usage = D3D11_USAGE_DEFAULT;
			texDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
			texDesc.CPUAccessFlags = 0;
			texDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;
			auto tex = Texture2D::Create(m_context, texDesc, (void*)nullptr);

			for (int i = 0; i < GA::Utils::s_maxLights; i++)
			{
				D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
				dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
				dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
				dsvDesc.Texture2DArray.FirstArraySlice = i * 6;
				dsvDesc.Texture2DArray.ArraySize = 6;
				dsvDesc.Texture2DArray.MipSlice = 0;
				m_resLib.Add(DSV_POINTLIGHT_SHADOW_MAP(i), DepthStencilView::Create(m_context, dsvDesc, tex));
			}

			D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
			srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBEARRAY;
			srvDesc.Texture2DArray.ArraySize = GA::Utils::s_maxLights;
			srvDesc.Texture2DArray.FirstArraySlice = 0;
			srvDesc.Texture2DArray.MipLevels = 1;
			srvDesc.Texture2DArray.MostDetailedMip = 0;
			m_resLib.Add(SRV_POINTLIGHT_SHADOW_MAP, ShaderResourceView::Create(m_context, srvDesc, tex));
		}

		// wpot light depth buffers
		{
			D3D11_TEXTURE2D_DESC texDesc = {};
			texDesc.Width = SHADOWMAP_SIZE;
			texDesc.Height = SHADOWMAP_SIZE;
			texDesc.ArraySize = GA::Utils::s_maxLights;
			texDesc.MipLevels = 1;
			texDesc.Format = DXGI_FORMAT_R32_TYPELESS;
			texDesc.SampleDesc.Count = 1;
			texDesc.SampleDesc.Quality = 0;
			texDesc.Usage = D3D11_USAGE_DEFAULT;
			texDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
			texDesc.CPUAccessFlags = 0;
			texDesc.MiscFlags = 0;
			auto tex = Texture2D::Create(m_context, texDesc, (void*)nullptr);

			for (int i = 0; i < GA::Utils::s_maxLights; i++)
			{
				D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
				dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
				dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
				dsvDesc.Texture2DArray.FirstArraySlice = i;
				dsvDesc.Texture2DArray.ArraySize = 1;
				dsvDesc.Texture2DArray.MipSlice = 0;
				m_resLib.Add(DSV_SPOTLIGHT_SHADOW_MAP(i), DepthStencilView::Create(m_context, dsvDesc, tex));
			}

			D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
			srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
			srvDesc.Texture2DArray.ArraySize = GA::Utils::s_maxLights;
			srvDesc.Texture2DArray.FirstArraySlice = 0;
			srvDesc.Texture2DArray.MipLevels = 1;
			srvDesc.Texture2DArray.MostDetailedMip = 0;
			m_resLib.Add(SRV_SPOTLIGHT_SHADOW_MAP, ShaderResourceView::Create(m_context, srvDesc, tex));
		}
	}
}
