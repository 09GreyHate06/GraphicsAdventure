#include "CSMTestRenderGraph.h"
#include "Scene/Components.h"
#include "entt/entt.hpp"
#include "Utils/Macros.h"

using namespace GDX11;
using namespace DirectX;
using namespace Microsoft::WRL;

// ------------ keys -------------

#define RTV_MAIN                            "main"
#define RTV_SCENE                           "scene"
#define SRV_SCENE                           "scene"
#define DSV_SCENE                           "scene"

#define VS_FS_OUT_TC_POS                    "fullscreen_out_tc_pos"
#define IL_FS_OUT_TC_POS                    "fullscreen_out_tc_pos"
#define VS_DIRLIGHT_CSM                     "dirlight_csm"
#define IL_DIRLIGHT_CSM                     "dirlight_csm"
#define GS_DIRLIGHT_CSM                     "dirlight_csm"
#define VS_CSM_TEST                         "csm_test"
#define IL_CSM_TEST                         "csm_test"
#define PS_CSM_TEST                         "csm_test"

#define PS_GAMMA_CORRECTION                 "gamma_correction"
#define PS_NULLPTR                          "null"
#define GS_NULLPTR                          "null"

#define S_DEFAULT                           "default"

#define RS_CULL_NONE                        "cull_none"
#define RS_DEPTH_SLOPE_SCALED_BIAS          "depth_slope_scaled_bias"
#define SS_POINT_CLAMP                      "point_clamp"
#define SS_LINEAR_CLAMP                     "linear_clamp"

#define VB_FS_QUAD                          "fs_quad.vb"
#define IB_FS_QUAD                          "fs_quad.ib"

#define CB_PS_GAMMA_CORRECTION_SYSTEM       "gamma_correction.ps.SystemCBuf"

#define DSV_DIRLIGHT_SHADOW_MAP             "dirLight_shadow_map"
#define SRV_DIRLIGHT_SHADOW_MAP             "dirLight_shadow_map"
#define RTV_DIRLIGHT_SHADOW_MAP             "dirLight_shadow_map"
#define RTV_SRV_DIRLIGHT_SHADOW_MAP         "rtv_dirLight_shadow_map"

#define CB_VS_DIRLIGHT_CSM_ENTITY           "dirlight_csm.vs.EntityCBuf"
#define CB_GS_DIRLIGHT_CSM_SYSTEM           "dirlight_csm.gs.SystemCBuf"
#define CB_VS_CSM_TEST_SYSTEM               "csm_test.vs.SystemCBuf"
#define CB_VS_CSM_TEST_ENTITY               "csm_test.vs.EntityCBuf"
#define CB_PS_CSM_TEST_SYSTEM               "csm_test.ps.SystemCBuf"
#define CB_PS_CSM_TEST_ENTITY               "csm_test.ps.EntityCBuf"

#define SHADOWMAP_SIZE 1024

#define GAMMA 2.2

namespace GA
{
	struct FrustumCorners
	{
		union
		{
			struct
			{
				DirectX::XMFLOAT4 nTopLeft;
				DirectX::XMFLOAT4 nTopRight;
				DirectX::XMFLOAT4 nBottomRight;
				DirectX::XMFLOAT4 nBottomLeft;

				DirectX::XMFLOAT4 fTopLeft;
				DirectX::XMFLOAT4 fTopRight;
				DirectX::XMFLOAT4 fBottomRight;
				DirectX::XMFLOAT4 fBottomLeft;
			};

			DirectX::XMFLOAT4 corners[8];
		};

		FrustumCorners() = default;

		FrustumCorners(float fov, float aspect, float nearZ, float farZ)
		{
			float t = tanf(DirectX::XMConvertToRadians(fov) * 0.5f);
			float nHalfX = nearZ * aspect * t;
			float nHalfY = nearZ * t;

			float fHalfX = farZ * aspect * t;
			float fHalfY = farZ * t;

			nTopLeft     = { -nHalfX,  nHalfY, nearZ, 1.0f };
			nTopRight    = {  nHalfX,  nHalfY, nearZ, 1.0f };
			nBottomRight = {  nHalfX, -nHalfY, nearZ, 1.0f };
			nBottomLeft  = { -nHalfX, -nHalfY, nearZ, 1.0f };

			fTopLeft     = { -fHalfX,  fHalfY, farZ, 1.0f };
			fTopRight    = {  fHalfX,  fHalfY, farZ, 1.0f };
			fBottomRight = {  fHalfX, -fHalfY, farZ, 1.0f };
			fBottomLeft  = { -fHalfX, -fHalfY, farZ, 1.0f };
		}

		void Transform(DirectX::FXMMATRIX m)
		{
			for (int i = 0; i < 8; i++)
				DirectX::XMStoreFloat4(&corners[i], DirectX::XMVector4Transform(DirectX::XMLoadFloat4(&corners[i]), m));
		}

		std::pair<DirectX::XMFLOAT3 /*min*/, DirectX::XMFLOAT3 /*max*/> GetMinMax()
		{
			float minX = std::numeric_limits<float>::max();
			float maxX = std::numeric_limits<float>::lowest();
			float minY = std::numeric_limits<float>::max();
			float maxY = std::numeric_limits<float>::lowest();
			float minZ = std::numeric_limits<float>::max();
			float maxZ = std::numeric_limits<float>::lowest();

			for (int i = 0; i < 8; i++)
			{
				minX = std::min(minX, corners[i].x);
				maxX = std::max(maxX, corners[i].x);
				minY = std::min(minY, corners[i].y);
				maxY = std::max(maxY, corners[i].y);
				minZ = std::min(minZ, corners[i].z);
				maxZ = std::max(maxZ, corners[i].z);
			}

			constexpr float zMult = 10.0f;
			if (minZ < 0)
			{
				minZ *= zMult;
			}
			else
			{
				minZ /= zMult;
			}
			if (maxZ < 0)
			{
				maxZ /= zMult;
			}
			else
			{
				maxZ *= zMult;
			}

			return { DirectX::XMFLOAT3(minX, minY, minZ), DirectX::XMFLOAT3(maxX, maxY, maxZ) };
		}

		DirectX::XMVECTOR GetCenter()
		{
			XMVECTOR center = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
			for (int i = 0; i < 8; i++)
				center += XMLoadFloat4(&corners[i]);

			return (center / 8);
		}
	};


	CSMTestRenderGraph::CSMTestRenderGraph(Scene* scene, GDX11::GDX11Context* context, const Camera* camera, uint32_t windowWidth, uint32_t windowHeight)
		: System(scene), m_context(context), m_camera(camera)
	{
		m_renderable.connect(GetRegistry(), entt::collector.group<TransformComponent, MeshComponent, MaterialComponent>(entt::exclude<>));
		m_dirLight.connect(GetRegistry(), entt::collector.group<TransformComponent, DirectionalLightComponent>(entt::exclude<>));

		ResizeViews(windowWidth, windowHeight);
		SetShaders();
		SetStates();
		SetBuffers();
		SetLightDepthBuffers();
	}

	void CSMTestRenderGraph::Execute()
	{
		ShadowPass();
		RenderPass();
		GammaCorrectionPass();
	}

	void CSMTestRenderGraph::ShadowPass()
	{
		auto dsv = m_resLib.Get<DepthStencilView>(DSV_DIRLIGHT_SHADOW_MAP);
		dsv->Clear(D3D11_CLEAR_DEPTH, 1.0f, 0xff);
		auto rtv = m_resLib.Get<RenderTargetView>(RTV_DIRLIGHT_SHADOW_MAP);
		rtv->Clear(0.0f, 0.0f, 0.0f, 0.0f);

		if (m_dirLight.empty()) return;

		rtv->Bind(dsv.get());

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

		GA::Utils::CSMTestPSSystemCBuf psSysCbuf = {};
		for each (const auto& e in m_dirLight)
		{
			const auto& [transform, dirLight] = GetRegistry().get<TransformComponent, DirectionalLightComponent>(e);

			XMVECTOR xmDirection = transform.GetForward();
			XMFLOAT3 direction;
			XMStoreFloat3(&direction, xmDirection);

			psSysCbuf.dirLight.direction = direction;
			psSysCbuf.dirLight.color = dirLight.color;
			psSysCbuf.dirLight.ambientIntensity = dirLight.ambientIntensity;
			psSysCbuf.dirLight.intensity = dirLight.intensity;

			auto ls = CalculateLightSpace(xmDirection);
			for (int i = 0; i < GA::Utils::s_numCascades; i++)
			{
				psSysCbuf.dirLight.lightSpaces[i] = ls[i];
				psSysCbuf.dirLight.cascadeFarZDist[i].x = m_cascadeFarZDist[i];
			}

			if (m_renderable.empty()) continue;

			auto vs = m_resLib.Get<VertexShader>(VS_DIRLIGHT_CSM);
			auto gs = m_resLib.Get<GeometryShader>(GS_DIRLIGHT_CSM);
			vs->Bind();
			gs->Bind();
			m_resLib.Get<PixelShader>(PS_NULLPTR)->Bind();
			m_resLib.Get<InputLayout>(IL_DIRLIGHT_CSM)->Bind();

			{
				auto cbuf = m_resLib.Get<Buffer>(CB_GS_DIRLIGHT_CSM_SYSTEM);
				cbuf->SetData(psSysCbuf.dirLight.lightSpaces);
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
					auto cbuf = m_resLib.Get<Buffer>(CB_VS_DIRLIGHT_CSM_ENTITY);
					cbuf->SetData(&fTransform);
					cbuf->VSBindAsCBuf(vs->GetResBinding("EntityCBuf"));
				}

				mesh.vb->BindAsVB();
				mesh.ib->BindAsIB(DXGI_FORMAT_R32_UINT);
				m_context->GetDeviceContext()->IASetPrimitiveTopology(mesh.topology);

				GDX11_CONTEXT_THROW_INFO_ONLY(m_context->GetDeviceContext()->DrawIndexed(mesh.ib->GetDesc().ByteWidth / sizeof(uint32_t), 0, 0));
			}
		}

		m_resLib.Get<GeometryShader>(GS_NULLPTR)->Bind();
		m_resLib.Get<Buffer>(CB_PS_CSM_TEST_SYSTEM)->SetData(&psSysCbuf);
	}

	void CSMTestRenderGraph::RenderPass()
	{
		auto rtv = m_resLib.Get<RenderTargetView>(RTV_SCENE);
		auto dsv = m_resLib.Get<DepthStencilView>(DSV_SCENE);
		rtv->Clear(0.0f, 0.0f, 0.0f, 0.0f);
		dsv->Clear(D3D11_CLEAR_DEPTH, 1.0f, 0xff);

		if (m_renderable.empty()) return;

		D3D11_VIEWPORT vp = {};
		vp.TopLeftX = 0.0f;
		vp.TopLeftY = 0.0f;
		vp.Width = (float)m_windowWidth;
		vp.Height = (float)m_windowHeight;
		vp.MinDepth = 0.0f;
		vp.MaxDepth = 1.0f;
		m_context->GetDeviceContext()->RSSetViewports(1, &vp);

		m_resLib.Get<RasterizerState>(S_DEFAULT)->Bind();
		m_resLib.Get<BlendState>(S_DEFAULT)->Bind(nullptr, 0xff);
		m_resLib.Get<DepthStencilState>(S_DEFAULT)->Bind(0xff);

		rtv->Bind(dsv.get());

		auto vs = m_resLib.Get<VertexShader>(VS_CSM_TEST);
		auto ps = m_resLib.Get<PixelShader>(PS_CSM_TEST);
		vs->Bind();
		ps->Bind();
		m_resLib.Get<InputLayout>(IL_CSM_TEST)->Bind();

		// system cbufs
		{
			m_resLib.Get<Buffer>(CB_PS_CSM_TEST_SYSTEM)->PSBindAsCBuf(ps->GetResBinding("SystemCBuf"));

			GA::Utils::CSMTestVSSystemCBuf cbufData = {};
			cbufData.viewPos = m_camera->GetDesc().position;
			XMFLOAT4X4 viewProj;
			XMFLOAT4X4 view;
			XMStoreFloat4x4(&viewProj, XMMatrixTranspose(m_camera->GetViewMatrix() * m_camera->GetProjectionMatrix()));
			XMStoreFloat4x4(&view, XMMatrixTranspose(m_camera->GetViewMatrix()));
			cbufData.viewProjection = viewProj;
			cbufData.view = view;

			auto cbuf = m_resLib.Get<Buffer>(CB_VS_CSM_TEST_SYSTEM);
			cbuf->SetData(&cbufData);
			cbuf->VSBindAsCBuf(vs->GetResBinding("SystemCBuf"));
		}

		m_resLib.Get<ShaderResourceView>(SRV_DIRLIGHT_SHADOW_MAP)->PSBind(ps->GetResBinding("dirLightShadowMaps"));
		m_resLib.Get<SamplerState>(SS_LINEAR_CLAMP)->PSBind(ps->GetResBinding("dirLightShadowMapsSampler"));

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
				GA::Utils::CSMTestVSEntityCBuf cbufData = {};
				cbufData.transform = fTransform;
				cbufData.normalMatrix = normalMatrix;

				auto cbuf = m_resLib.Get<Buffer>(CB_VS_CSM_TEST_ENTITY);
				cbuf->SetData(&cbufData);
				cbuf->VSBindAsCBuf(vs->GetResBinding("EntityCBuf"));
			}

			{
				GA::Utils::CSMTestPSEntityCBuf cbufData = {};
				cbufData.mat.color = mat.color;
				cbufData.mat.tiling = mat.tiling;
				cbufData.mat.shininess = mat.shininess;
				cbufData.mat.enableNormalMapping = mat.normalMap ? TRUE : FALSE;
				cbufData.mat.enableParallaxMapping = mat.normalMap && mat.depthMap ? TRUE : FALSE;
				cbufData.mat.depthMapScale = mat.depthMapScale;
				cbufData.receiveShadows = mesh.receiveShadows;

				auto cbuf = m_resLib.Get<Buffer>(CB_PS_CSM_TEST_ENTITY);
				cbuf->PSBindAsCBuf(ps->GetResBinding("EntityCBuf"));
				cbuf->SetData(&cbufData);
			}

			m_context->GetDeviceContext()->IASetPrimitiveTopology(mesh.topology);
			GDX11_CONTEXT_THROW_INFO_ONLY(m_context->GetDeviceContext()->DrawIndexed(mesh.ib->GetDesc().ByteWidth / sizeof(uint32_t), 0, 0));
		}
	}

	void CSMTestRenderGraph::GammaCorrectionPass()
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

	void CSMTestRenderGraph::ResizeViews(uint32_t width, uint32_t height)
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
				m_resLib.Remove<ShaderResourceView>(SRV_SCENE);
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
			m_resLib.Add(SRV_SCENE, RenderTargetView::Create(m_context, desc, tex));
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
	}

	void CSMTestRenderGraph::SetShaders()
	{
		m_resLib.Add(VS_DIRLIGHT_CSM, VertexShader::Create(m_context, "res/cso/dirlight_csm.vs.cso"));
		m_resLib.Add(GS_DIRLIGHT_CSM, GeometryShader::Create(m_context, "res/cso/dirlight_csm.gs.cso"));
		m_resLib.Add(IL_DIRLIGHT_CSM, InputLayout::Create(m_context, m_resLib.Get<VertexShader>(VS_DIRLIGHT_CSM)));
		m_resLib.Add(PS_NULLPTR, PixelShader::Create(m_context, "res/cso/dirlight_csm.ps.cso"));
		m_resLib.Add(GS_NULLPTR, GeometryShader::Create(m_context));

		m_resLib.Add(VS_CSM_TEST, VertexShader::Create(m_context, "res/cso/csm_test.vs.cso"));
		m_resLib.Add(PS_CSM_TEST, PixelShader::Create(m_context, "res/cso/csm_test.ps.cso"));
		m_resLib.Add(IL_CSM_TEST, InputLayout::Create(m_context, m_resLib.Get<VertexShader>(VS_CSM_TEST)));

		m_resLib.Add(VS_FS_OUT_TC_POS, VertexShader::Create(m_context, "res/cso/fullscreen_out_tc_pos.vs.cso"));
		m_resLib.Add(IL_FS_OUT_TC_POS, InputLayout::Create(m_context, m_resLib.Get<VertexShader>(VS_FS_OUT_TC_POS)));
		m_resLib.Add(PS_GAMMA_CORRECTION, PixelShader::Create(m_context, "res/cso/gamma_correction.ps.cso"));
	}

	void CSMTestRenderGraph::SetStates()
	{
		// rs
		{
			m_resLib.Add(S_DEFAULT, RasterizerState::Create(m_context, CD3D11_RASTERIZER_DESC(CD3D11_DEFAULT())));

			D3D11_RASTERIZER_DESC desc = CD3D11_RASTERIZER_DESC(CD3D11_DEFAULT());
			desc.DepthBias = 40;
			desc.SlopeScaledDepthBias = 6.0f;
			desc.DepthBiasClamp = 1.0f;
			m_resLib.Add(RS_DEPTH_SLOPE_SCALED_BIAS, RasterizerState::Create(m_context, desc));
		}

		// bs
		{
			m_resLib.Add(S_DEFAULT, BlendState::Create(m_context, CD3D11_BLEND_DESC(CD3D11_DEFAULT())));
		}

		// dss
		{
			m_resLib.Add(S_DEFAULT, DepthStencilState::Create(m_context, CD3D11_DEPTH_STENCIL_DESC(CD3D11_DEFAULT())));
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

	void CSMTestRenderGraph::SetBuffers()
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
			m_resLib.Add(CB_VS_DIRLIGHT_CSM_ENTITY, Buffer::Create(m_context, desc, nullptr));
		}

		{
			D3D11_BUFFER_DESC desc = {};
			desc.ByteWidth = sizeof(XMFLOAT4X4) * GA::Utils::s_numCascades;
			desc.Usage = D3D11_USAGE_DYNAMIC;
			desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
			desc.MiscFlags = 0;
			desc.StructureByteStride = 0;
			m_resLib.Add(CB_GS_DIRLIGHT_CSM_SYSTEM, Buffer::Create(m_context, desc, nullptr));
		}

		{
			D3D11_BUFFER_DESC desc = {};
			desc.ByteWidth = sizeof(GA::Utils::CSMTestVSSystemCBuf);
			desc.Usage = D3D11_USAGE_DYNAMIC;
			desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
			desc.MiscFlags = 0;
			desc.StructureByteStride = 0;
			m_resLib.Add(CB_VS_CSM_TEST_SYSTEM, Buffer::Create(m_context, desc, nullptr));
		}

		{
			D3D11_BUFFER_DESC desc = {};
			desc.ByteWidth = sizeof(GA::Utils::CSMTestVSEntityCBuf);
			desc.Usage = D3D11_USAGE_DYNAMIC;
			desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
			desc.MiscFlags = 0;
			desc.StructureByteStride = 0;
			m_resLib.Add(CB_VS_CSM_TEST_ENTITY, Buffer::Create(m_context, desc, nullptr));
		}

		{
			D3D11_BUFFER_DESC desc = {};
			desc.ByteWidth = sizeof(GA::Utils::CSMTestPSSystemCBuf);
			desc.Usage = D3D11_USAGE_DYNAMIC;
			desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
			desc.MiscFlags = 0;
			desc.StructureByteStride = 0;
			m_resLib.Add(CB_PS_CSM_TEST_SYSTEM, Buffer::Create(m_context, desc, nullptr));
		}

		{
			D3D11_BUFFER_DESC desc = {};
			desc.ByteWidth = sizeof(GA::Utils::CSMTestPSEntityCBuf);
			desc.Usage = D3D11_USAGE_DYNAMIC;
			desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
			desc.MiscFlags = 0;
			desc.StructureByteStride = 0;
			m_resLib.Add(CB_PS_CSM_TEST_ENTITY, Buffer::Create(m_context, desc, nullptr));
		}
	}

	void CSMTestRenderGraph::SetLightDepthBuffers()
	{
		{
			D3D11_TEXTURE2D_DESC texDesc = {};
			texDesc.Width = SHADOWMAP_SIZE;
			texDesc.Height = SHADOWMAP_SIZE;
			texDesc.ArraySize = (uint32_t)GA::Utils::s_numCascades;
			texDesc.MipLevels = 1;
			texDesc.Format = DXGI_FORMAT_R32_TYPELESS;
			texDesc.SampleDesc.Count = 1;
			texDesc.SampleDesc.Quality = 0;
			texDesc.Usage = D3D11_USAGE_DEFAULT;
			texDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
			texDesc.CPUAccessFlags = 0;
			texDesc.MiscFlags = 0;
			auto tex = Texture2D::Create(m_context, texDesc, (void*)nullptr);

			D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
			dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
			dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
			dsvDesc.Texture2DArray.ArraySize = (uint32_t)GA::Utils::s_numCascades;
			dsvDesc.Texture2DArray.FirstArraySlice = 0;
			dsvDesc.Texture2DArray.MipSlice = 0;
			m_resLib.Add(DSV_DIRLIGHT_SHADOW_MAP, DepthStencilView::Create(m_context, dsvDesc, tex));

			D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
			srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
			srvDesc.Texture2DArray.ArraySize = (uint32_t)GA::Utils::s_numCascades;
			srvDesc.Texture2DArray.FirstArraySlice = 0;
			srvDesc.Texture2DArray.MipLevels = 1;
			srvDesc.Texture2DArray.MostDetailedMip = 0;
			m_resLib.Add(SRV_DIRLIGHT_SHADOW_MAP, ShaderResourceView::Create(m_context, srvDesc, tex));
		}

		{
			D3D11_TEXTURE2D_DESC texDesc = {};
			texDesc.Width = SHADOWMAP_SIZE;
			texDesc.Height = SHADOWMAP_SIZE;
			texDesc.ArraySize = (uint32_t)GA::Utils::s_numCascades;
			texDesc.MipLevels = 1;
			texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			texDesc.SampleDesc.Count = 1;
			texDesc.SampleDesc.Quality = 0;
			texDesc.Usage = D3D11_USAGE_DEFAULT;
			texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
			texDesc.CPUAccessFlags = 0;
			texDesc.MiscFlags = 0;
			auto tex = Texture2D::Create(m_context, texDesc, (void*)nullptr);

			D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
			rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
			rtvDesc.Texture2DArray.ArraySize = (uint32_t)GA::Utils::s_numCascades;
			rtvDesc.Texture2DArray.FirstArraySlice = 0;
			rtvDesc.Texture2DArray.MipSlice = 0;
			m_resLib.Add(RTV_DIRLIGHT_SHADOW_MAP, RenderTargetView::Create(m_context, rtvDesc, tex));

			D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
			srvDesc.Texture2DArray.ArraySize = (uint32_t)GA::Utils::s_numCascades;
			srvDesc.Texture2DArray.FirstArraySlice = 0;
			srvDesc.Texture2DArray.MipLevels = 1;
			srvDesc.Texture2DArray.MostDetailedMip = 0;
			m_resLib.Add(RTV_SRV_DIRLIGHT_SHADOW_MAP, ShaderResourceView::Create(m_context, srvDesc, tex));
		}
	}

	std::array<DirectX::XMFLOAT4X4, GA::Utils::s_numCascades> CSMTestRenderGraph::CalculateLightSpace(DirectX::FXMVECTOR xmLightDir)
	{
		std::array<DirectX::XMFLOAT4X4, GA::Utils::s_numCascades> res = {};

		//float csmLvl0FarZ = powf(m_camera->GetDesc().farZ / m_camera->GetDesc().nearZ, 1.0f / m_cascadeFarZDist.size());
		//m_cascadeFarZDist[0] = csmLvl0FarZ;
		//for (int i = 1; i < m_cascadeFarZDist.size(); i++)
		//{
		//	m_cascadeFarZDist[i] = std::min(powf(csmLvl0FarZ, i + 1.0f), m_camera->GetDesc().farZ);
		//}

		m_cascadeFarZDist[0] = m_camera->GetDesc().farZ / 50.0f;
		m_cascadeFarZDist[1] = m_camera->GetDesc().farZ / 25.0f;
		m_cascadeFarZDist[2] = m_camera->GetDesc().farZ / 10.0f;
		m_cascadeFarZDist[3] = m_camera->GetDesc().farZ / 2.0f;
		m_cascadeFarZDist[4] = m_camera->GetDesc().farZ;

		for (int i = 0; i < GA::Utils::s_numCascades; i++)
		{
			FrustumCorners fc = {};
			if (i == 0)
				fc = FrustumCorners(m_camera->GetDesc().fov, m_camera->GetDesc().aspect, m_camera->GetDesc().nearZ, m_cascadeFarZDist[0]);
			else
				fc = FrustumCorners(m_camera->GetDesc().fov, m_camera->GetDesc().aspect, m_cascadeFarZDist[i - 1], m_cascadeFarZDist[i]);

			// transform frustum to world space
			fc.Transform(XMMatrixInverse(nullptr, m_camera->GetViewMatrix()));

			// get frustum center at world space
			XMVECTOR fcCenter = fc.GetCenter();

			// transform frustum to light view space
			XMMATRIX xmLightView = XMMatrixLookAtLH(fcCenter + -xmLightDir * 50.0f, fcCenter, XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));
			fc.Transform(xmLightView);

			// calculate bounding box
			auto [fcMin, fcMax] = fc.GetMinMax();
			XMStoreFloat4x4(&res[i], XMMatrixTranspose(xmLightView * XMMatrixOrthographicOffCenterLH(fcMin.x, fcMax.x, fcMin.y, fcMax.y, fcMin.z, fcMax.z)));
		}

		return res;
	}
}

