#include "App.h"
#include <iostream>
#include <imgui.h>
#include <GDX11/Utils/Loader.h>
#include <DirectXMath.h>

#include "Utils/BasicMesh.h"
#include "Utils/ShaderCBuf.h"

using namespace GDX11;
using namespace Microsoft::WRL;
using namespace DirectX;

namespace GA
{
	App::App()
	{
		m_msaaEnabled = true;

		{
			WindowDesc desc = {};
			desc.className = "GraphicsAdventure";
			desc.width = 1280;
			desc.height = 720;
			desc.name = "Graphics Adventure";
			m_window = std::make_unique<Window>(desc);
			m_window->SetEventCallback(GDX11_BIND_EVENT_FN(OnEvent));
		}

		m_context = std::make_unique<GDX11Context>();

		SetSwapChain();
		SetViews();
		SetStates();

		{
			D3D11_BLEND_DESC desc = CD3D11_BLEND_DESC(CD3D11_DEFAULT());
			desc.AlphaToCoverageEnable = FALSE;
			desc.IndependentBlendEnable = FALSE;
			auto& brt = desc.RenderTarget[0];
			brt.BlendEnable = TRUE;
			brt.SrcBlend = D3D11_BLEND_SRC_ALPHA;
			brt.DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
			brt.BlendOp = D3D11_BLEND_OP_ADD;
			brt.SrcBlendAlpha = D3D11_BLEND_ONE;
			brt.DestBlendAlpha = D3D11_BLEND_ONE;
			brt.BlendOpAlpha = D3D11_BLEND_OP_ADD;
			brt.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
			m_resLib.Add("default", BlendState::Create(m_context.get(), desc));
		}

		m_resLib.Add("default", DepthStencilState::Create(m_context.get(), CD3D11_DEPTH_STENCIL_DESC(CD3D11_DEFAULT())));


		SetShaders();
		SetBuffers();
		SetTextures();

		D3D11_VIEWPORT vp = {};
		vp.TopLeftX = 0.0f;
		vp.TopLeftY = 0.0f;
		vp.Width = (float)m_window->GetDesc().width;
		vp.Height = (float)m_window->GetDesc().height;
		vp.MinDepth = 0.0f;
		vp.MaxDepth = 1.0f;
		m_context->GetDeviceContext()->RSSetViewports(1, &vp);

		m_imguiManager.Set(m_window.get(), m_context.get());

		CameraDesc camDesc = {};
		camDesc.fov = 60.0f;
		camDesc.aspect = (float)m_window->GetDesc().width / m_window->GetDesc().height;
		camDesc.nearZ = 0.1f;
		camDesc.farZ = 1000.0f;
		camDesc.position = { 0.0f, 0.0f, 0.0f };
		camDesc.rotation = { 0.0f, 0.0f, 0.0f };
		m_camera.Set(camDesc);
		m_camController.Set(&m_camera, XMFLOAT3(0.0f, 0.0f, 0.0f), 8.0f, 0.8f, 5.0f, 15.0f);
	}

	void App::Run()
	{
		while (!m_window->GetState().shouldClose)
		{
			m_time.UpdateDeltaTime();

			OnUpdate();
			OnRender();
			OnImGuiRender();

			Window::PollEvents();

			HRESULT hr;
			if (FAILED(hr = m_context->GetSwapChain()->Present(1, 0)))
			{
				if (hr == DXGI_ERROR_DEVICE_REMOVED)
					throw GDX11_CONTEXT_DEVICE_REMOVED_EXCEPT(hr);
				else
					throw GDX11_CONTEXT_EXCEPT(hr);
			}
		}
	}

	void App::OnEvent(GDX11::Event& event)
	{
		EventDispatcher dispatcher(event);
		dispatcher.Dispatch<WindowResizeEvent>(GDX11_BIND_EVENT_FN(OnWindowResizedEvent));

		m_camController.OnEvent(event, m_time.GetDeltaTime());
	}

	void App::OnUpdate()
	{
		m_camController.ProcessInput(m_window.get(), m_time.GetDeltaTime());
	}

	void App::OnRender()
	{
		m_resLib.Get<RenderTargetView>("main")->Bind(m_resLib.Get<DepthStencilView>("main").get());
		m_resLib.Get<RenderTargetView>("main")->Clear(0.1f, 0.1f, 0.1f, 1.0f);
		m_resLib.Get<DepthStencilView>("main")->Clear(D3D11_CLEAR_DEPTH, 1.0f, 0xff);
		
		m_resLib.Get<BlendState>("default")->Bind(nullptr, 0xff);
		m_resLib.Get<DepthStencilState>("default")->Bind(0xff);
		m_resLib.Get<RasterizerState>("default")->Bind();

		SetLight();
		DrawPlane(m_resLib.Get<ShaderResourceView>("brickwall"), XMFLOAT3(0.0f, -1.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(10.0f, 1.0f, 10.0f),
			XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT2(10.0f, 10.0f), 25.0f);


		m_resLib.Get<RasterizerState>("no_cull")->Bind();

		DrawPlane(m_resLib.Get<ShaderResourceView>("transparent_window"), XMFLOAT3(-0.5f, 0.0f, 1.0f), XMFLOAT3(-90.0f, 0.0f, 0.0f), XMFLOAT3(1.0f, 1.0f, 1.0f),
			XMFLOAT4(1.0f, 1.0f, 1.0f, m_window3Alpha), XMFLOAT2(1.0f, 1.0f), 25.0f);

		DrawPlane(m_resLib.Get<ShaderResourceView>("transparent_window"), XMFLOAT3(0.5f, 0.0f, 0.0f), XMFLOAT3(-90.0f, 0.0f, 0.0f), XMFLOAT3(1.0f, 1.0f, 1.0f),
			XMFLOAT4(1.0f, 1.0f, 1.0f, m_window2Alpha), XMFLOAT2(1.0f, 1.0f), 25.0f);

		DrawPlane(m_resLib.Get<ShaderResourceView>("transparent_window"), XMFLOAT3(-0.5, 0.0f, -1.0f), XMFLOAT3(-90.0f, 0.0f, 0.0f), XMFLOAT3(1.0f, 1.0f, 1.0f),
			XMFLOAT4(1.0f, 1.0f, 1.0f, m_window1Alpha), XMFLOAT2(1.0f, 1.0f), 25.0f);

		DrawPlane(m_resLib.Get<ShaderResourceView>("transparent_window"), XMFLOAT3(0.0f, 2.0f, -3.0f), XMFLOAT3(-45.0f, 0.0f, 0.0f), XMFLOAT3(3.0f, 1.0f, 3.0f),
			XMFLOAT4(1.0f, 1.0f, 1.0f, m_window0Alpha), XMFLOAT2(3.0f, 3.0f), 25.0f);
	}

	void App::OnImGuiRender()
	{
		m_imguiManager.Begin();

		ImGui::Begin("MSAA");
		if (ImGui::Checkbox("Enable", &m_msaaEnabled))
		{
			SetSwapChain();
			SetViews();
			SetStates();
		}

		static constexpr char* items[] = { "MSAA 2x", "MSAA 4x", "MSAA 8x" };
		ImGui::PushItemWidth(100.0f);
		if (ImGui::Combo("##Samples", &m_sampleCountArrayIndex, items, IM_ARRAYSIZE(items)) && m_msaaEnabled)
		{
			switch (m_sampleCountArrayIndex)
			{
			case 0:
				m_sampleCount = 2;
				break;
			case 1:
				m_sampleCount = 4;
				break;
			case 2:
				m_sampleCount = 8;
				break;
			}

			SetSwapChain();
			SetViews();
			SetStates();
		}
		ImGui::PopItemWidth();
		
		ImGui::End();

		ImGui::Begin("Windows apha");
		ImGui::SliderFloat("Window 0", &m_window0Alpha, 0.0f, 1.0f);
		ImGui::SliderFloat("Window 1", &m_window1Alpha, 0.0f, 1.0f);
		ImGui::SliderFloat("Window 2", &m_window2Alpha, 0.0f, 1.0f);
		ImGui::SliderFloat("Window 3", &m_window3Alpha, 0.0f, 1.0f);
		ImGui::End();

		m_imguiManager.End();
	}

	void App::SetLight()
	{
		auto ps = m_resLib.Get<PixelShader>("light");
		XMVECTOR rotQuatXM = XMQuaternionRotationRollPitchYaw(XMConvertToRadians(50.0f), XMConvertToRadians(-30.0f), 0.0f);
		XMVECTOR directionXM = XMVector3Rotate(XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), rotQuatXM);
		XMFLOAT3 direction;
		XMStoreFloat3(&direction, directionXM);

		GA::Utils::LightPSSystemCBuf cbuf = {};
		cbuf.dirLights[0].color = { 1.0f, 1.0f, 1.0f };
		cbuf.dirLights[0].direction = direction;
		cbuf.dirLights[0].ambientIntensity = 0.2f;
		cbuf.dirLights[0].intensity = 1.0f;

		cbuf.activeDirLights = 1;

		m_resLib.Get<Buffer>("light.ps.SystemCBuf")->PSBindAsCBuf(ps->GetResBinding("SystemCBuf"));
		m_resLib.Get<Buffer>("light.ps.SystemCBuf")->SetData(&cbuf);
	}

	void App::DrawPlane(const std::shared_ptr<ShaderResourceView>& tex, const XMFLOAT3& pos, const XMFLOAT3& rot, const XMFLOAT3& scale,
		const XMFLOAT4& col, const XMFLOAT2& tiling, float shininess)
	{
		auto vs = m_resLib.Get<VertexShader>("light");
		auto ps = m_resLib.Get<PixelShader>("light");
		vs->Bind();
		ps->Bind();
		m_resLib.Get<InputLayout>("light")->Bind();

		auto vb = m_resLib.Get<Buffer>("plane.vb");
		auto ib = m_resLib.Get<Buffer>("plane.ib");
		vb->BindAsVB();
		ib->BindAsIB(DXGI_FORMAT_R32_UINT);


		tex->PSBind(ps->GetResBinding("textureMap"));
		m_resLib.Get<SamplerState>("anisotropic_wrap")->PSBind(ps->GetResBinding("textureMapSampler"));

		// bind cbuf
		XMVECTOR rotQuat = XMQuaternionRotationRollPitchYaw(XMConvertToRadians(rot.x), XMConvertToRadians(rot.y), XMConvertToRadians(rot.z));
		XMMATRIX transformXM = XMMatrixScaling(scale.x, scale.y, scale.z) * XMMatrixRotationQuaternion(rotQuat) * XMMatrixTranslation(pos.x, pos.y, pos.z);

		XMFLOAT4X4 transform;
		XMFLOAT4X4 viewProjection;
		XMFLOAT4X4 normalMatrix;
		XMStoreFloat4x4(&transform, XMMatrixTranspose(transformXM));
		XMStoreFloat4x4(&viewProjection, XMMatrixTranspose(m_camera.GetViewMatrix() * m_camera.GetProjectionMatrix()));
		XMStoreFloat4x4(&normalMatrix, XMMatrixInverse(nullptr, transformXM));

		{
			GA::Utils::LightVSSystemCBuf cbuf = {};
			cbuf.viewProjection = viewProjection;
			cbuf.viewPos = m_camera.GetDesc().position;
			m_resLib.Get<Buffer>("light.vs.SystemCBuf")->VSBindAsCBuf(vs->GetResBinding("SystemCBuf"));
			m_resLib.Get<Buffer>("light.vs.SystemCBuf")->SetData(&cbuf);
		}


		{
			GA::Utils::LightVSEntityCBuf cbuf = {};
			cbuf.transform = transform;
			cbuf.normalMatrix = normalMatrix;
			m_resLib.Get<Buffer>("light.vs.EntityCBuf")->VSBindAsCBuf(vs->GetResBinding("EntityCBuf"));
			m_resLib.Get<Buffer>("light.vs.EntityCBuf")->SetData(&cbuf);
		}

		{
			GA::Utils::LightPSEntityCBuf cbuf = {};
			cbuf.mat.color = col;
			cbuf.mat.tiling = tiling;
			cbuf.mat.shininess = shininess;
			m_resLib.Get<Buffer>("light.ps.EntityCBuf")->PSBindAsCBuf(ps->GetResBinding("EntityCBuf"));
			m_resLib.Get<Buffer>("light.ps.EntityCBuf")->SetData(&cbuf);
		}

		m_context->GetDeviceContext()->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		GDX11_CONTEXT_THROW_INFO_ONLY(m_context->GetDeviceContext()->DrawIndexed(ib->GetDesc().ByteWidth / sizeof(uint32_t), 0, 0));
	}

	void App::SetShaders()
	{
		m_resLib.Add("light", VertexShader::Create(m_context.get(), GDX11::Utils::LoadText("res/shaders/light.vs.hlsl")));
		m_resLib.Add("light", PixelShader::Create(m_context.get(), GDX11::Utils::LoadText("res/shaders/light.ps.hlsl")));
		m_resLib.Add("light", InputLayout::Create(m_context.get(), m_resLib.Get<VertexShader>("light")));
	}

	void App::SetBuffers()
	{
		{
			auto vert = GA::Utils::CreateCubeVertices(true, true);
			auto ind = GA::Utils::CreateCubeIndices();

			D3D11_BUFFER_DESC desc = {};
			desc.ByteWidth = (uint32_t)vert.size() * sizeof(float);
			desc.Usage = D3D11_USAGE_DEFAULT;
			desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
			desc.CPUAccessFlags = 0;
			desc.MiscFlags = 0;
			desc.StructureByteStride = 8 * sizeof(float);
			m_resLib.Add("cube.vb", Buffer::Create(m_context.get(), desc, vert.data()));

			desc = {};
			desc.ByteWidth = (uint32_t)ind.size() * sizeof(uint32_t);
			desc.Usage = D3D11_USAGE_DEFAULT;
			desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
			desc.CPUAccessFlags = 0;
			desc.MiscFlags = 0;
			desc.StructureByteStride = sizeof(uint32_t);
			m_resLib.Add("cube.ib", Buffer::Create(m_context.get(), desc, ind.data()));
		}


		{
			auto vert = GA::Utils::CreatePlaneVertices(true, true);
			auto ind = GA::Utils::CreatePlaneIndices();

			D3D11_BUFFER_DESC desc = {};
			desc.ByteWidth = (uint32_t)vert.size() * sizeof(float);
			desc.Usage = D3D11_USAGE_DEFAULT;
			desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
			desc.CPUAccessFlags = 0;
			desc.MiscFlags = 0;
			desc.StructureByteStride = 8 * sizeof(float);
			m_resLib.Add("plane.vb", Buffer::Create(m_context.get(), desc, vert.data()));

			desc = {};
			desc.ByteWidth = (uint32_t)ind.size() * sizeof(uint32_t);
			desc.Usage = D3D11_USAGE_DEFAULT;
			desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
			desc.CPUAccessFlags = 0;
			desc.MiscFlags = 0;
			desc.StructureByteStride = sizeof(uint32_t);
			m_resLib.Add("plane.ib", Buffer::Create(m_context.get(), desc, ind.data()));
		}


		{
			D3D11_BUFFER_DESC desc = {};
			desc.ByteWidth = sizeof(GA::Utils::LightVSSystemCBuf);
			desc.Usage = D3D11_USAGE_DYNAMIC;
			desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
			desc.MiscFlags = 0;
			desc.StructureByteStride = 0;
			m_resLib.Add("light.vs.SystemCBuf", Buffer::Create(m_context.get(), desc, nullptr));
		}

		{
			D3D11_BUFFER_DESC desc = {};
			desc.ByteWidth = sizeof(GA::Utils::LightVSEntityCBuf);
			desc.Usage = D3D11_USAGE_DYNAMIC;
			desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
			desc.MiscFlags = 0;
			desc.StructureByteStride = 0;
			m_resLib.Add("light.vs.EntityCBuf", Buffer::Create(m_context.get(), desc, nullptr));
		}

		{
			D3D11_BUFFER_DESC desc = {};
			desc.ByteWidth = sizeof(GA::Utils::LightPSSystemCBuf);
			desc.Usage = D3D11_USAGE_DYNAMIC;
			desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
			desc.MiscFlags = 0;
			desc.StructureByteStride = 0;
			m_resLib.Add("light.ps.SystemCBuf", Buffer::Create(m_context.get(), desc, nullptr));
		}

		{
			D3D11_BUFFER_DESC desc = {};
			desc.ByteWidth = sizeof(GA::Utils::LightPSEntityCBuf);
			desc.Usage = D3D11_USAGE_DYNAMIC;
			desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
			desc.MiscFlags = 0;
			desc.StructureByteStride = 0;
			m_resLib.Add("light.ps.EntityCBuf", Buffer::Create(m_context.get(), desc, nullptr));
		}
	}

	void App::SetTextures()
	{
		{
			D3D11_SAMPLER_DESC desc = {};
			desc.Filter = D3D11_FILTER_ANISOTROPIC;
			desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
			desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
			desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
			desc.MipLODBias = 0.0f;
			desc.MaxAnisotropy = D3D11_DEFAULT_MAX_ANISOTROPY;
			desc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
			for (int i = 0; i < 4; i++)
				desc.BorderColor[i] = 0.0f;
			desc.MinLOD = 0.0f;
			desc.MaxLOD = D3D11_FLOAT32_MAX;
			m_resLib.Add("anisotropic_wrap", SamplerState::Create(m_context.get(), desc));
		}

		{
			GDX11::Utils::ImageData image = GDX11::Utils::LoadImageFile("res/textures/brickwall.jpg", false, 4);
			D3D11_TEXTURE2D_DESC texDesc = {};
			texDesc.Width = image.width;
			texDesc.Height = image.height;
			texDesc.MipLevels = 0;
			texDesc.ArraySize = 1;
			texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			texDesc.SampleDesc.Count = 1;
			texDesc.SampleDesc.Quality = 0;
			texDesc.Usage = D3D11_USAGE_DEFAULT;
			texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
			texDesc.CPUAccessFlags = 0;
			texDesc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;

			D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			srvDesc.Format = texDesc.Format;
			srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
			srvDesc.Texture2D.MostDetailedMip = 0;
			srvDesc.Texture2D.MipLevels = -1;

			m_resLib.Add("brickwall", ShaderResourceView::Create(m_context.get(), srvDesc, Texture2D::Create(m_context.get(), texDesc, image.pixels)));
		}

		{
			GDX11::Utils::ImageData image = GDX11::Utils::LoadImageFile("res/textures/transparent_window.png", false, 4);
			D3D11_TEXTURE2D_DESC texDesc = {};
			texDesc.Width = image.width;
			texDesc.Height = image.height;
			texDesc.MipLevels = 0;
			texDesc.ArraySize = 1;
			texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			texDesc.SampleDesc.Count = 1;
			texDesc.SampleDesc.Quality = 0;
			texDesc.Usage = D3D11_USAGE_DEFAULT;
			texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
			texDesc.CPUAccessFlags = 0;
			texDesc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;

			D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			srvDesc.Format = texDesc.Format;
			srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
			srvDesc.Texture2D.MostDetailedMip = 0;
			srvDesc.Texture2D.MipLevels = -1;

			m_resLib.Add("transparent_window", ShaderResourceView::Create(m_context.get(), srvDesc, Texture2D::Create(m_context.get(), texDesc, image.pixels)));
		}
	}

	void App::SetSwapChain()
	{
		DXGI_SWAP_CHAIN_DESC desc = {};
		desc.BufferDesc.Width = 0;
		desc.BufferDesc.Height = 0;
		desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		desc.BufferDesc.RefreshRate.Numerator = 0;
		desc.BufferDesc.RefreshRate.Denominator = 0;
		desc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
		desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
		desc.BufferCount = 1;
		desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		desc.Flags = 0;
		desc.OutputWindow = m_window->GetNativeWindow();
		desc.SampleDesc.Count = m_msaaEnabled ? m_sampleCount : 1;
		desc.SampleDesc.Quality = m_msaaEnabled ? D3D11_STANDARD_MULTISAMPLE_PATTERN : 0;
		desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
		desc.Windowed = TRUE;
		m_context->SetSwapChain(desc);
	}
	void App::SetViews()
	{
		{
			if (m_resLib.Exist<RenderTargetView>("main"))
				m_resLib.Remove<RenderTargetView>("main");

			D3D11_RENDER_TARGET_VIEW_DESC desc = {};
			desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			desc.ViewDimension = m_msaaEnabled ? D3D11_RTV_DIMENSION_TEXTURE2DMS : D3D11_RTV_DIMENSION_TEXTURE2D;
			desc.Texture2D.MipSlice = 0;
			ComPtr<ID3D11Texture2D> backbuffer;
			HRESULT hr;
			GDX11_CONTEXT_THROW_INFO(m_context->GetSwapChain()->GetBuffer(0, __uuidof(ID3D11Texture2D), &backbuffer));
			m_resLib.Add("main", RenderTargetView::Create(m_context.get(), desc, Texture2D::Create(m_context.get(), backbuffer.Get())));
		}

		{
			if (m_resLib.Exist<DepthStencilView>("main"))
				m_resLib.Remove<DepthStencilView>("main");

			D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
			dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
			dsvDesc.ViewDimension = m_msaaEnabled ? D3D11_DSV_DIMENSION_TEXTURE2DMS : D3D11_DSV_DIMENSION_TEXTURE2D;
			dsvDesc.Texture2D.MipSlice = 0;

			D3D11_TEXTURE2D_DESC texDesc = {};
			texDesc.Width = m_window->GetDesc().width;
			texDesc.Height = m_window->GetDesc().height;
			texDesc.ArraySize = 1;
			texDesc.MipLevels = 1;
			texDesc.Format = dsvDesc.Format;
			texDesc.SampleDesc.Count = m_msaaEnabled ? m_sampleCount : 1;
			texDesc.SampleDesc.Quality = m_msaaEnabled ? D3D11_STANDARD_MULTISAMPLE_PATTERN : 0;
			texDesc.Usage = D3D11_USAGE_DEFAULT;
			texDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
			texDesc.CPUAccessFlags = 0;
			texDesc.MiscFlags = 0;

			m_resLib.Add("main", DepthStencilView::Create(m_context.get(), dsvDesc, Texture2D::Create(m_context.get(), texDesc, (void*)nullptr)));
		}
	}
	void App::SetStates()
	{
		{
			if (m_resLib.Exist<RasterizerState>("default"))
				m_resLib.Remove<RasterizerState>("default");

			D3D11_RASTERIZER_DESC desc = CD3D11_RASTERIZER_DESC(CD3D11_DEFAULT());
			desc.MultisampleEnable = m_msaaEnabled;
			m_resLib.Add("default", RasterizerState::Create(m_context.get(), desc));
		}

		{
			if (m_resLib.Exist<RasterizerState>("no_cull"))
				m_resLib.Remove<RasterizerState>("no_cull");

			D3D11_RASTERIZER_DESC desc = CD3D11_RASTERIZER_DESC(CD3D11_DEFAULT());
			desc.MultisampleEnable = m_msaaEnabled;
			desc.CullMode = D3D11_CULL_NONE;
			m_resLib.Add("no_cull", RasterizerState::Create(m_context.get(), desc));
		}
	}

	bool App::OnWindowResizedEvent(GDX11::WindowResizeEvent& event)
	{
		if (event.GetWidth() == 0 || event.GetHeight() == 0) return false;

		m_camera.SetAspect((float)m_window->GetDesc().width / m_window->GetDesc().height);

		D3D11_VIEWPORT vp = {};
		vp.TopLeftX = 0.0f;
		vp.TopLeftY = 0.0f;
		vp.Width = (float)event.GetWidth();
		vp.Height = (float)event.GetHeight();
		vp.MinDepth = 0.0f;
		vp.MaxDepth = 1.0f;
		m_context->GetDeviceContext()->RSSetViewports(1, &vp);

		m_resLib.Remove<RenderTargetView>("main");
		m_resLib.Remove<DepthStencilView>("main");

		HRESULT hr;
		GDX11_CONTEXT_THROW_INFO(m_context->GetSwapChain()->ResizeBuffers(1, event.GetWidth(), event.GetHeight(), DXGI_FORMAT_R8G8B8A8_UNORM, 0));

		SetViews();

		return false;
	}
}