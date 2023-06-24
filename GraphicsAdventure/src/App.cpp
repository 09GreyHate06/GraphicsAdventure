#include "App.h"
#include <iostream>
#include <imgui.h>
#include <GDX11/Utils/Utils.h>
#include <DirectXMath.h>

#include "Utils/BasicMesh.h"
#include "Utils/ShaderCBuf.h"
#include "Scene/Entity.h"
#include "Scene/Components.h"

using namespace GDX11;
using namespace Microsoft::WRL;
using namespace DirectX;

namespace GA
{
	App::App()
	{
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

		m_scene = std::make_unique<Scene>();
		m_lambertianRenderGraph = std::make_unique<LambertianRenderGraph>(m_scene.get(), m_context.get(), m_window->GetDesc().width, m_window->GetDesc().height);


		{
			auto e = m_scene->CreateEntity();
			e.AddComponent<TransformComponent>(XMFLOAT3(0.0f, 2.5f, 0.0f), XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(5.0f, 5.0f, 5.0f));

			auto& mesh = e.AddComponent<MeshComponent>();
			mesh.vb = m_resLib.Get<Buffer>("cube.vb");
			mesh.ib = m_resLib.Get<Buffer>("cube.ib");
			mesh.topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

			auto& mat = e.AddComponent<MaterialComponent>();
			mat.color = { 1.0f, 1.0f, 1.0f, 0.5f };
			mat.tiling = { 1.0f, 1.0f };
			mat.shininess = 150.0f;
			mat.diffuseMap = m_resLib.Get<ShaderResourceView>("wood");
			mat.normalMap = m_resLib.Get<ShaderResourceView>("bump_normal");
			mat.depthMap = m_resLib.Get<ShaderResourceView>("bump_depth");
			mat.samplerState = m_resLib.Get<SamplerState>("anisotropic_wrap");
			mat.depthMapScale = 0.1f;
		}

		{
			auto e = m_scene->CreateEntity();
			e.AddComponent<TransformComponent>(XMFLOAT3(0.0f, -2.5f, 0.0f), XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(20.0f, 1.0f, 20.0f));

			auto& mesh = e.AddComponent<MeshComponent>();
			mesh.vb = m_resLib.Get<Buffer>("plane.vb");
			mesh.ib = m_resLib.Get<Buffer>("plane.ib");
			mesh.topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
			
			auto& mat = e.AddComponent<MaterialComponent>();
			mat.color = { 1.0f, 1.0f, 1.0f, 0.3f };
			mat.tiling = { 10.0f, 10.0f };
			mat.shininess = 150.0f;
			mat.diffuseMap = m_resLib.Get<ShaderResourceView>("wood");
			mat.normalMap = m_resLib.Get<ShaderResourceView>("bump_normal");
			mat.depthMap = m_resLib.Get<ShaderResourceView>("bump_depth");
			mat.samplerState = m_resLib.Get<SamplerState>("anisotropic_wrap");
			mat.depthMapScale = 0.1f;
		}

		{
			auto e = m_scene->CreateEntity();
			e.AddComponent<TransformComponent>(XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(50.0f, -30.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, 00.0f));
			auto& dirLight = e.AddComponent<DirectionalLightComponent>();
			dirLight.color = { 1.0f, 1.0f, 1.0f };
			dirLight.ambientIntensity = 0.2f;
			dirLight.intensity = 1.0f;
			e.AddComponent<SkyboxComponent>().skybox = m_resLib.Get<ShaderResourceView>("skybox");
		}
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
		XMFLOAT4X4 viewProj;
		XMStoreFloat4x4(&viewProj, XMMatrixTranspose(m_camera.GetViewMatrix() * m_camera.GetProjectionMatrix()));
		m_lambertianRenderGraph->Execute(m_camera.GetDesc().position, viewProj);
	}

	void App::OnImGuiRender()
	{
		m_imguiManager.Begin();

		m_imguiManager.End();
	}

	void App::SetBuffers()
	{
		{
			auto vert = GA::Utils::CreateCubeVerticesEx();
			auto ind = GA::Utils::CreateCubeIndicesEx();

			D3D11_BUFFER_DESC desc = {};
			desc.ByteWidth = (uint32_t)vert.size() * sizeof(GA::Utils::Vertex);
			desc.Usage = D3D11_USAGE_DEFAULT;
			desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
			desc.CPUAccessFlags = 0;
			desc.MiscFlags = 0;
			desc.StructureByteStride = sizeof(GA::Utils::Vertex);
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
			auto vert = GA::Utils::CreatePlaneVerticesEx();
			auto ind = GA::Utils::CreatePlaneIndicesEx();

			D3D11_BUFFER_DESC desc = {};
			desc.ByteWidth = (uint32_t)vert.size() * sizeof(GA::Utils::Vertex);
			desc.Usage = D3D11_USAGE_DEFAULT;
			desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
			desc.CPUAccessFlags = 0;
			desc.MiscFlags = 0;
			desc.StructureByteStride = sizeof(GA::Utils::Vertex);
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
	}

	void App::SetTextures()
	{
		{
			D3D11_SAMPLER_DESC desc = CD3D11_SAMPLER_DESC(CD3D11_DEFAULT());

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
			D3D11_SAMPLER_DESC desc = CD3D11_SAMPLER_DESC(CD3D11_DEFAULT());

			desc.Filter = D3D11_FILTER_ANISOTROPIC;
			desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
			desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
			desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
			desc.MipLODBias = 0.0f;
			desc.MaxAnisotropy = D3D11_DEFAULT_MAX_ANISOTROPY;
			desc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
			for (int i = 0; i < 4; i++)
				desc.BorderColor[i] = 0.0f;
			desc.MinLOD = 0.0f;
			desc.MaxLOD = D3D11_FLOAT32_MAX;
			m_resLib.Add("anisotropic_clamp", SamplerState::Create(m_context.get(), desc));
		}

		{
			GDX11::Utils::ImageData image = GDX11::Utils::LoadImageFile("res/textures/wood.png", false, 4);
			D3D11_TEXTURE2D_DESC texDesc = {};
			texDesc.Width = image.width;
			texDesc.Height = image.height;
			texDesc.MipLevels = 0;
			texDesc.ArraySize = 1;
			texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
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

			m_resLib.Add("wood", ShaderResourceView::Create(m_context.get(), srvDesc, Texture2D::Create(m_context.get(), texDesc, image.pixels)));
			GDX11::Utils::FreeImageData(&image);
		}

		{
			D3D11_TEXTURE2D_DESC texDesc = {};
			texDesc.Width = 1;
			texDesc.Height = 1;
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

			uint32_t col = 0xffffffff;
			m_resLib.Add("white", ShaderResourceView::Create(m_context.get(), srvDesc, Texture2D::Create(m_context.get(), texDesc, &col)));
		}


		{
			GDX11::Utils::ImageData images[6];
			for (int i = 0; i < 6; i++)
				images[i] = GDX11::Utils::LoadImageFile("res/textures/skybox/" + std::to_string(i) + ".jpg", false, 4);

			D3D11_TEXTURE2D_DESC texDesc = {};
			texDesc.Width = images[0].width;
			texDesc.Height = images[0].height;
			texDesc.MipLevels = 1;
			texDesc.ArraySize = 6;
			texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
			texDesc.SampleDesc.Count = 1;
			texDesc.SampleDesc.Quality = 0;
			texDesc.Usage = D3D11_USAGE_DEFAULT;
			texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
			texDesc.CPUAccessFlags = 0;
			texDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;
			
			D3D11_SUBRESOURCE_DATA data[6];
			for (int i = 0; i < 6; i++)
			{
				data[i].pSysMem = images[i].pixels;
				data[i].SysMemPitch = 4 * images[i].width;
				data[i].SysMemSlicePitch = 0;
			}

			D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			srvDesc.Format = texDesc.Format;
			srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
			srvDesc.Texture2D.MostDetailedMip = 0;
			srvDesc.Texture2D.MipLevels = 1;

			m_resLib.Add("skybox", ShaderResourceView::Create(m_context.get(), srvDesc, Texture2D::Create(m_context.get(), texDesc, data)));

			for (int i = 0; i < 6; i++)
				GDX11::Utils::FreeImageData(&images[i]);
		}


		{
			GDX11::Utils::ImageData image = GDX11::Utils::LoadImageFile("res/textures/bump_normal.png", false, 4);
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

			m_resLib.Add("bump_normal", ShaderResourceView::Create(m_context.get(), srvDesc, Texture2D::Create(m_context.get(), texDesc, image.pixels)));
			GDX11::Utils::FreeImageData(&image);
		}


		{
			GDX11::Utils::ImageData image = GDX11::Utils::LoadImageFile("res/textures/bump_depth.png", false, 4);
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

			m_resLib.Add("bump_depth", ShaderResourceView::Create(m_context.get(), srvDesc, Texture2D::Create(m_context.get(), texDesc, image.pixels)));
			GDX11::Utils::FreeImageData(&image);
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
		desc.SampleDesc.Count =  1;
		desc.SampleDesc.Quality =  0;
		desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
		desc.Windowed = TRUE;
		m_context->SetSwapChain(desc);
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

		m_lambertianRenderGraph->ResizeViews(event.GetWidth(), event.GetHeight());
		return false;
	}
}