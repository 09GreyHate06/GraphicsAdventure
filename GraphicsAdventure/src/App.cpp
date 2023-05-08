#include "App.h"
#include <iostream>
#include <imgui.h>
#include <GDX11/Utils/Utils.h>
#include <DirectXMath.h>

#include "Utils/BasicMesh.h"
#include "Utils/ShaderCBuf.h"

using namespace GDX11;
using namespace Microsoft::WRL;
using namespace DirectX;

#define EPSILONF 0.00001f

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
		SetViews();
		SetStates();

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
		auto mainRTV = m_resLib.Get<RenderTargetView>("main");
		auto mainDSV = m_resLib.Get<DepthStencilView>("main");

		// solid meshes
		{
			m_resLib.Get<RasterizerState>("no_cull")->Bind();
			m_resLib.Get<BlendState>("default")->Bind(nullptr, 0xff);
			m_resLib.Get<DepthStencilState>("default")->Bind(0xff);

			mainRTV->Bind(mainDSV.get());
			mainRTV->Clear(0.0f, 0.0f, 0.0f, 0.0f);
			mainDSV->Clear(D3D11_CLEAR_DEPTH, 1.0f, 0xff);

			auto vs = m_resLib.Get<VertexShader>("light");
			auto ps = m_resLib.Get<PixelShader>("light");
			vs->Bind();
			ps->Bind();
			m_resLib.Get<InputLayout>("light")->Bind();

			SetLight(ps);
			DrawPlane(vs, ps, m_resLib.Get<ShaderResourceView>("brickwall"), XMFLOAT3(0.0f, 0.0f, 2.5f), XMFLOAT3(-90.0f, 0.0f, 0.0f), XMFLOAT3(10.0f, 1.0f, 10.0f),
				XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT2(10.0f, 10.0f), 32.0f);
		}

		// transparent meshes
		{
			m_resLib.Get<BlendState>("weighted_blended")->Bind(nullptr, 0xff);
			m_resLib.Get<DepthStencilState>("default_depth_mask_zero")->Bind(0xff);

			auto transparentRTVA = m_resLib.Get<RenderTargetViewArray>("weighted_blended");
			RenderTargetView::Bind(*transparentRTVA, mainDSV.get());
			transparentRTVA->at(0)->Clear(0.0f, 0.0f, 0.0f, 0.0f); // accumulation
			transparentRTVA->at(1)->Clear(1.0f, 1.0f, 1.0f, 1.0f); // reveal

			// bind transparent shader

			auto vs = m_resLib.Get<VertexShader>("light");
			auto ps = m_resLib.Get<PixelShader>("weighted_blended");
			vs->Bind();
			ps->Bind();
			m_resLib.Get<InputLayout>("light")->Bind();

			SetLight(ps);

			for (int z = -1; z <= 1; z++)
			{
				for (int y = -1; y <= 1; y++)
				{
					for (int x = -1; x <= 1; x++)
					{
						float mult = 1.5f;
						XMFLOAT4 col;
						switch (z)
						{
						case -1:
							col = { 1.0f, 0.3f, 0.3f, m_redBoxAlpha };
							break;
						case 0:
							col = { 0.3f, 1.0f, 0.3f, m_greenBoxAlpha };
							break;
						case 1:
							col = { 0.3f, 0.3f, 1.0f, m_blueBoxAlpha };
							break;
						}

						if ((col.w - EPSILONF) <= 0.0f) continue;

						DrawCube(vs, ps, m_resLib.Get<ShaderResourceView>("white"), XMFLOAT3(x * mult, y * mult, z * mult), XMFLOAT3(0.0f, 0.0f, 0.0f),
							XMFLOAT3(1.0f, 1.0f, 1.0f), col, XMFLOAT2(1.0f, 1.0f), 25.0f);
					}
				}
			}

		}

		// composite pass
		{
			m_resLib.Get<DepthStencilState>("default_depth_func_always")->Bind(0xff);
			m_resLib.Get<BlendState>("over")->Bind(nullptr, 0xff);

			// bind main 
			mainRTV->Bind(mainDSV.get());

			// bind composite shader
			m_resLib.Get<VertexShader>("weighted_blended_composite")->Bind();
			auto ps = m_resLib.Get<PixelShader>("weighted_blended_composite");
			ps->Bind();
			m_resLib.Get<InputLayout>("weighted_blended_composite")->Bind();

			m_resLib.Get<ShaderResourceView>("weighted_blended_accumulation")->PSBind(ps->GetResBinding("accumulationMap"));
			m_resLib.Get<ShaderResourceView>("weighted_blended_reveal")->PSBind(ps->GetResBinding("revealMap"));

			// draw to quad
			m_resLib.Get<Buffer>("screen.vb")->BindAsVB();
			auto ib = m_resLib.Get<Buffer>("screen.ib");
			ib->BindAsIB(DXGI_FORMAT_R32_UINT);
			m_context->GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			GDX11_CONTEXT_THROW_INFO_ONLY(m_context->GetDeviceContext()->DrawIndexed(ib->GetDesc().ByteWidth / sizeof(uint32_t), 0, 0));
		}
	}

	void App::OnImGuiRender()
	{
		m_imguiManager.Begin();

		ImGui::Begin("Box apha");
		ImGui::PushItemWidth(80.0f);
		ImGui::DragFloat("red boxes", &m_redBoxAlpha, 0.001f, 0.0f, 1.0f);
		ImGui::DragFloat("green boxes", &m_greenBoxAlpha, 0.001f, 0.0f, 1.0f);
		ImGui::DragFloat("blue boxes", &m_blueBoxAlpha, 0.001f, 0.0f, 1.0f);
		ImGui::PopItemWidth();
		ImGui::End();

		m_imguiManager.End();
	}

	void App::SetLight(const std::shared_ptr<GDX11::PixelShader>& ps)
	{
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

	void App::DrawPlane(const std::shared_ptr<VertexShader>& vs, const std::shared_ptr<PixelShader>& ps, const std::shared_ptr<ShaderResourceView>& tex, const XMFLOAT3& pos, const XMFLOAT3& rot, const XMFLOAT3& scale,
		const XMFLOAT4& col, const XMFLOAT2& tiling, float shininess)
	{
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

		m_context->GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		GDX11_CONTEXT_THROW_INFO_ONLY(m_context->GetDeviceContext()->DrawIndexed(ib->GetDesc().ByteWidth / sizeof(uint32_t), 0, 0));
	}

	void App::DrawCube(const std::shared_ptr<VertexShader>& vs, const std::shared_ptr<PixelShader>& ps, const std::shared_ptr<ShaderResourceView>& tex, const XMFLOAT3& pos, const XMFLOAT3& rot, const XMFLOAT3& scale,
		const XMFLOAT4& col, const XMFLOAT2& tiling, float shininess)
	{
		auto vb = m_resLib.Get<Buffer>("cube.vb");
		auto ib = m_resLib.Get<Buffer>("cube.ib");
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

		m_context->GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		GDX11_CONTEXT_THROW_INFO_ONLY(m_context->GetDeviceContext()->DrawIndexed(ib->GetDesc().ByteWidth / sizeof(uint32_t), 0, 0));
	}

	void App::SetShaders()
	{
		m_resLib.Add("light", VertexShader::Create(m_context.get(), "res/cso/light.vs.cso"));
		m_resLib.Add("light", PixelShader::Create(m_context.get(), "res/cso/light.ps.cso"));
		m_resLib.Add("weighted_blended", PixelShader::Create(m_context.get(), "res/cso/weighted_blended_oit.ps.cso"));
		m_resLib.Add("light", InputLayout::Create(m_context.get(), m_resLib.Get<VertexShader>("light")));

		m_resLib.Add("weighted_blended_composite", VertexShader::Create(m_context.get(), "res/cso/weighted_blended_oit_composite.vs.cso"));
		m_resLib.Add("weighted_blended_composite", PixelShader::Create(m_context.get(), "res/cso/weighted_blended_oit_composite.ps.cso"));
		m_resLib.Add("weighted_blended_composite", InputLayout::Create(m_context.get(), m_resLib.Get<VertexShader>("weighted_blended_composite")));
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
			float vert[] = {
				-1.0f,  1.0f,
				 1.0f,  1.0f,
				 1.0f, -1.0f,
				-1.0f, -1.0f
			};
			uint32_t ind[] = {
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
			m_resLib.Add("screen.vb", Buffer::Create(m_context.get(), desc, vert));

			desc = {};
			desc.ByteWidth = sizeof(ind);
			desc.Usage = D3D11_USAGE_DEFAULT;
			desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
			desc.CPUAccessFlags = 0;
			desc.MiscFlags = 0;
			desc.StructureByteStride = sizeof(uint32_t);
			m_resLib.Add("screen.ib", Buffer::Create(m_context.get(), desc, ind));
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
			desc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
			desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
			desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
			desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
			for (int i = 0; i < 4; i++)
				desc.BorderColor[i] = 0.0f;
			desc.MinLOD = 0.0f;
			desc.MaxLOD = D3D11_FLOAT32_MAX;
			m_resLib.Add("point_wrap", SamplerState::Create(m_context.get(), desc));
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

	void App::SetViews()
	{
		// main rtv
		{
			if (m_resLib.Exist<RenderTargetView>("main"))
				m_resLib.Remove<RenderTargetView>("main");

			D3D11_RENDER_TARGET_VIEW_DESC desc = {};
			desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
			desc.Texture2D.MipSlice = 0;
			ComPtr<ID3D11Texture2D> backbuffer;
			HRESULT hr;
			GDX11_CONTEXT_THROW_INFO(m_context->GetSwapChain()->GetBuffer(0, __uuidof(ID3D11Texture2D), &backbuffer));
			m_resLib.Add("main", RenderTargetView::Create(m_context.get(), desc, Texture2D::Create(m_context.get(), backbuffer.Get())));
		}

		// main dsv
		{
			if (m_resLib.Exist<DepthStencilView>("main"))
				m_resLib.Remove<DepthStencilView>("main");

			D3D11_DEPTH_STENCIL_VIEW_DESC desc = {};
			desc.Format = DXGI_FORMAT_D32_FLOAT;
			desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
			desc.Texture2D.MipSlice = 0;

			D3D11_TEXTURE2D_DESC texDesc = {};
			texDesc.Width = m_window->GetDesc().width;
			texDesc.Height = m_window->GetDesc().height;
			texDesc.ArraySize = 1;
			texDesc.MipLevels = 1;
			texDesc.Format = desc.Format;
			texDesc.SampleDesc.Count =  1;
			texDesc.SampleDesc.Quality = 0;
			texDesc.Usage = D3D11_USAGE_DEFAULT;
			texDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
			texDesc.CPUAccessFlags = 0;
			texDesc.MiscFlags = 0;

			m_resLib.Add("main", DepthStencilView::Create(m_context.get(), desc, Texture2D::Create(m_context.get(), texDesc, (void*)nullptr)));
		}


		// transparent mesh rtv
		{
			if (m_resLib.Exist<RenderTargetViewArray>("weighted_blended"))
			{
				m_resLib.Remove<RenderTargetViewArray>("weighted_blended");
				m_resLib.Remove<ShaderResourceView>("weighted_blended_accumulation");
				m_resLib.Remove<ShaderResourceView>("weighted_blended_reveal");
			}

			auto rtvArr = std::make_shared<RenderTargetViewArray>();

			// accumulation rtv
			{
				D3D11_RENDER_TARGET_VIEW_DESC desc = {};
				desc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
				desc.Texture2D.MipSlice = 0;
				desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;

				D3D11_TEXTURE2D_DESC texDesc = {};
				texDesc.Width = m_window->GetDesc().width;
				texDesc.Height = m_window->GetDesc().height;
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

				auto tex = Texture2D::Create(m_context.get(), texDesc, (void*)nullptr);
				auto rtv = RenderTargetView::Create(m_context.get(), desc, tex);
				m_resLib.Add("weighted_blended_accumulation", ShaderResourceView::Create(m_context.get(), srvDesc, tex));

				rtvArr->push_back(rtv);
			}

			// reveal rtv
			{
				D3D11_RENDER_TARGET_VIEW_DESC desc = {};
				desc.Format = DXGI_FORMAT_R8_UNORM;
				desc.Texture2D.MipSlice = 0;
				desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;

				D3D11_TEXTURE2D_DESC texDesc = {};
				texDesc.Width = m_window->GetDesc().width;
				texDesc.Height = m_window->GetDesc().height;
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

				auto tex = Texture2D::Create(m_context.get(), texDesc, (void*)nullptr);
				auto rtv = RenderTargetView::Create(m_context.get(), desc, tex);
				m_resLib.Add("weighted_blended_reveal", ShaderResourceView::Create(m_context.get(), srvDesc, tex));

				rtvArr->push_back(rtv);
			}

			m_resLib.Add("weighted_blended", rtvArr);
		}
	}

	void App::SetStates()
	{
		{
			if (m_resLib.Exist<RasterizerState>("default"))
				m_resLib.Remove<RasterizerState>("default");

			m_resLib.Add("default", RasterizerState::Create(m_context.get(), CD3D11_RASTERIZER_DESC(CD3D11_DEFAULT())));
		}

		{
			if (m_resLib.Exist<RasterizerState>("no_cull"))
				m_resLib.Remove<RasterizerState>("no_cull");

			D3D11_RASTERIZER_DESC desc = CD3D11_RASTERIZER_DESC(CD3D11_DEFAULT());
			desc.CullMode = D3D11_CULL_NONE;
			m_resLib.Add("no_cull", RasterizerState::Create(m_context.get(), desc));
		}



		{
			if (m_resLib.Exist<BlendState>("default"))
				m_resLib.Remove<BlendState>("default");

			m_resLib.Add("default", BlendState::Create(m_context.get(), CD3D11_BLEND_DESC(CD3D11_DEFAULT())));
		}

		{
			if (m_resLib.Exist<BlendState>("over"))
				m_resLib.Remove<BlendState>("over");

			D3D11_BLEND_DESC desc = CD3D11_BLEND_DESC(CD3D11_DEFAULT());
			desc.AlphaToCoverageEnable = FALSE;
			desc.IndependentBlendEnable = FALSE;
			auto& brt = desc.RenderTarget[0];
			brt.BlendEnable = TRUE;
			brt.SrcBlend = D3D11_BLEND_SRC_ALPHA;
			brt.DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
			brt.BlendOp = D3D11_BLEND_OP_ADD;
			brt.SrcBlendAlpha = D3D11_BLEND_ONE;
			brt.DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
			brt.BlendOpAlpha = D3D11_BLEND_OP_ADD;
			brt.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
			m_resLib.Add("over", BlendState::Create(m_context.get(), desc));
		}

		{
			if (m_resLib.Exist<BlendState>("weighted_blended"))
				m_resLib.Remove<BlendState>("weighted_blended");

			D3D11_BLEND_DESC desc = CD3D11_BLEND_DESC(CD3D11_DEFAULT());
			desc.AlphaToCoverageEnable = FALSE;
			desc.IndependentBlendEnable = TRUE;

			auto& brt0 = desc.RenderTarget[0];
			brt0.BlendEnable = TRUE;
			brt0.SrcBlend = D3D11_BLEND_ONE;
			brt0.DestBlend = D3D11_BLEND_ONE;
			brt0.BlendOp = D3D11_BLEND_OP_ADD;
			brt0.SrcBlendAlpha = D3D11_BLEND_ONE;
			brt0.DestBlendAlpha = D3D11_BLEND_ONE;
			brt0.BlendOpAlpha = D3D11_BLEND_OP_ADD;
			brt0.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

			auto& brt1 = desc.RenderTarget[1];
			brt1.BlendEnable = TRUE;
			brt1.SrcBlend = D3D11_BLEND_ZERO;
			brt1.DestBlend = D3D11_BLEND_INV_SRC_COLOR;
			brt1.BlendOp = D3D11_BLEND_OP_ADD;
			brt1.SrcBlendAlpha = D3D11_BLEND_ZERO;
			brt1.DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
			brt1.BlendOpAlpha = D3D11_BLEND_OP_ADD;
			brt1.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

			m_resLib.Add("weighted_blended", BlendState::Create(m_context.get(), desc));
		}


		{
			if (m_resLib.Exist<DepthStencilState>("default"))
				m_resLib.Remove<DepthStencilState>("default");

			m_resLib.Add("default", DepthStencilState::Create(m_context.get(), CD3D11_DEPTH_STENCIL_DESC(CD3D11_DEFAULT())));
		}

		{
			if (m_resLib.Exist<DepthStencilState>("default_depth_func_always"))
				m_resLib.Remove<DepthStencilState>("default_depth_func_always");

			D3D11_DEPTH_STENCIL_DESC desc = CD3D11_DEPTH_STENCIL_DESC(CD3D11_DEFAULT());
			desc.DepthFunc = D3D11_COMPARISON_ALWAYS;
			m_resLib.Add("default_depth_func_always", DepthStencilState::Create(m_context.get(), desc));
		}

		{
			if (m_resLib.Exist<DepthStencilState>("default_depth_mask_zero"))
				m_resLib.Remove<DepthStencilState>("default_depth_mask_zero");

			D3D11_DEPTH_STENCIL_DESC desc = CD3D11_DEPTH_STENCIL_DESC(CD3D11_DEFAULT());
			desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
			m_resLib.Add("default_depth_mask_zero", DepthStencilState::Create(m_context.get(), desc));
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