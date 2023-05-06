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
		SetMainViews();
		SetStates();

		SetShaders();
		SetBuffers();
		SetTextures();

		SetDepthPeelRes();

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
		{
			m_resLib.Get<BlendState>("blend")->Bind(nullptr, 0xff);
			m_resLib.Get<DepthStencilState>("default")->Bind(0xff);
			m_resLib.Get<RasterizerState>("default")->Bind();

			m_resLib.Get<RasterizerState>("no_cull")->Bind();

			auto vs = m_resLib.Get<VertexShader>("light");
			auto ps = m_resLib.Get<PixelShader>("light");
			vs->Bind();
			ps->Bind();
			m_resLib.Get<InputLayout>("light")->Bind();

			SetLight(ps);
			// render screen m_numberDepthPeelPass times
			// compare prev depth to curdepth before rendering
			for (int i = 0; i < m_numDepthPeelPass; i++)
			{
				std::string curId = "depth_peel" + std::to_string(i);
				auto curRTV = m_resLib.Get<RenderTargetView>(curId);
				auto curDSV = m_resLib.Get<DepthStencilView>(curId);
				curRTV->Bind(curDSV.get());
				curRTV->Clear(0.1f, 0.1f, 0.1f, 0.0f);
				curDSV->Clear(D3D11_CLEAR_DEPTH, 1.0f, 0xff);

				// bind prev dsv texture
				m_resLib.Get<ShaderResourceView>("d_depth_peel" + std::to_string(std::clamp(i - 1, 0, m_numDepthPeelPass - 1)))->PSBind(ps->GetResBinding("prevDepthMap"));

				// limitation: fully opaque obj override prev transparent obj
				//DrawPlane(vs, ps, m_resLib.Get<ShaderResourceView>("brickwall"), XMFLOAT3(0.0f, -3.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, 0.0f),
				//	XMFLOAT3(10.0f, 1.0f, 10.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT2(10.0f, 10.0f), 25.0f);

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

							DrawCube(vs, ps, m_resLib.Get<ShaderResourceView>("white"), XMFLOAT3(x * mult, y * mult, z * mult), XMFLOAT3(0.0f, 0.0f, 0.0f), 
								XMFLOAT3(1.0f, 1.0f, 1.0f), col, XMFLOAT2(1.0f, 1.0f), 25.0f);
						}
					}
				}
			}
		}


		// render quad
		{
			m_resLib.Get<RenderTargetView>("main")->Bind(m_resLib.Get<DepthStencilView>("main").get());
			m_resLib.Get<RenderTargetView>("main")->Clear(0.1f, 0.1f, 0.1f, 0.0f);
			m_resLib.Get<DepthStencilView>("main")->Clear(D3D11_CLEAR_DEPTH, 1.0f, 0xff);

			m_resLib.Get<DepthStencilState>("depth_disabled")->Bind(0xff);

			auto vs = m_resLib.Get<VertexShader>("screen");
			auto ps = m_resLib.Get<PixelShader>("basic");

			vs->Bind();
			ps->Bind();
			m_resLib.Get<InputLayout>("screen")->Bind();

			auto vb = m_resLib.Get<Buffer>("screen.vb");
			auto ib = m_resLib.Get<Buffer>("screen.ib");
			vb->BindAsVB();
			ib->BindAsIB(DXGI_FORMAT_R32_UINT);

			// m_showDepthPeelLayer == m_numDepthPeelPass when user pick show All
			if (m_showDepthPeelLayer == m_numDepthPeelPass)
			{
				for (int i = m_numDepthPeelPass - 1; i >= 0; i--)
				{
					m_resLib.Get<ShaderResourceView>("c_depth_peel" + std::to_string(i))->PSBind(ps->GetResBinding("tex"));
					m_resLib.Get<SamplerState>("point_wrap")->PSBind(ps->GetResBinding("samplerState"));

					XMFLOAT4 color = { 1.0f, 1.0f, 1.0f, 1.0f };
					m_resLib.Get<Buffer>("basic.ps.EntityCBuf")->PSBindAsCBuf(ps->GetResBinding("EntityCBuf"));
					m_resLib.Get<Buffer>("basic.ps.EntityCBuf")->SetData(&color);

					m_context->GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
					GDX11_CONTEXT_THROW_INFO_ONLY(m_context->GetDeviceContext()->DrawIndexed(ib->GetDesc().ByteWidth / sizeof(uint32_t), 0, 0));
				}
			}
			else
			{
				m_resLib.Get<ShaderResourceView>("c_depth_peel" + std::to_string(m_showDepthPeelLayer))->PSBind(ps->GetResBinding("tex"));
				m_resLib.Get<SamplerState>("point_wrap")->PSBind(ps->GetResBinding("samplerState"));

				XMFLOAT4 color = { 1.0f, 1.0f, 1.0f, 1.0f };
				m_resLib.Get<Buffer>("basic.ps.EntityCBuf")->PSBindAsCBuf(ps->GetResBinding("EntityCBuf"));
				m_resLib.Get<Buffer>("basic.ps.EntityCBuf")->SetData(&color);

				m_context->GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
				GDX11_CONTEXT_THROW_INFO_ONLY(m_context->GetDeviceContext()->DrawIndexed(ib->GetDesc().ByteWidth / sizeof(uint32_t), 0, 0));
			}
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

		ImGui::Begin("Depth Peel Layers");
		ImGui::PushItemWidth(80.0f);
		static const char* layers[] = { "1", "2", "3", "4", "5", "6", "All" };
		ImGui::Combo("Show Layer", &m_showDepthPeelLayer, layers, IM_ARRAYSIZE(layers));
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

		m_context->GetDeviceContext()->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
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

		m_context->GetDeviceContext()->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		GDX11_CONTEXT_THROW_INFO_ONLY(m_context->GetDeviceContext()->DrawIndexed(ib->GetDesc().ByteWidth / sizeof(uint32_t), 0, 0));
	}

	void App::SetShaders()
	{
		m_resLib.Add("light", VertexShader::Create(m_context.get(), "res/cso/light.vs.cso"));
		m_resLib.Add("light", PixelShader::Create(m_context.get(), "res/cso/light.ps.cso"));
		m_resLib.Add("light", InputLayout::Create(m_context.get(), m_resLib.Get<VertexShader>("light")));

		m_resLib.Add("basic", VertexShader::Create(m_context.get(), "res/cso/basic.vs.cso"));
		m_resLib.Add("basic", PixelShader::Create(m_context.get(), "res/cso/basic.ps.cso"));
		m_resLib.Add("basic", InputLayout::Create(m_context.get(), m_resLib.Get<VertexShader>("basic")));

		m_resLib.Add("screen", VertexShader::Create(m_context.get(), "res/cso/screen.vs.cso"));
		m_resLib.Add("screen", InputLayout::Create(m_context.get(), m_resLib.Get<VertexShader>("screen")));
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
			desc.ByteWidth = sizeof(XMFLOAT4);
			desc.Usage = D3D11_USAGE_DYNAMIC;
			desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
			desc.MiscFlags = 0;
			desc.StructureByteStride = 0;
			m_resLib.Add("basic.ps.EntityCBuf", Buffer::Create(m_context.get(), desc, nullptr));
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

	void App::SetMainViews()
	{
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

		{
			if (m_resLib.Exist<DepthStencilView>("main"))
				m_resLib.Remove<DepthStencilView>("main");

			D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
			dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
			dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
			dsvDesc.Texture2D.MipSlice = 0;

			D3D11_TEXTURE2D_DESC texDesc = {};
			texDesc.Width = m_window->GetDesc().width;
			texDesc.Height = m_window->GetDesc().height;
			texDesc.ArraySize = 1;
			texDesc.MipLevels = 1;
			texDesc.Format = dsvDesc.Format;
			texDesc.SampleDesc.Count =  1;
			texDesc.SampleDesc.Quality = 0;
			texDesc.Usage = D3D11_USAGE_DEFAULT;
			texDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
			texDesc.CPUAccessFlags = 0;
			texDesc.MiscFlags = 0;

			m_resLib.Add("main", DepthStencilView::Create(m_context.get(), dsvDesc, Texture2D::Create(m_context.get(), texDesc, (void*)nullptr)));
		}
	}

	void App::SetDepthPeelRes()
	{
		for (int i = 0; i < m_numDepthPeelPass; i++)
		{
			std::string id = "depth_peel" + std::to_string(i);

			// rtv
			{
				if (m_resLib.Exist<RenderTargetView>(id))
				{
					m_resLib.Remove<RenderTargetView>(id);
					m_resLib.Remove<ShaderResourceView>("c_" + id);
				}

				D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
				rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
				rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
				rtvDesc.Texture2D.MipSlice = 0;

				D3D11_TEXTURE2D_DESC texDesc = {};
				texDesc.Width = m_window->GetDesc().width;
				texDesc.Height = m_window->GetDesc().height;
				texDesc.MipLevels = 1;
				texDesc.ArraySize = 1;
				texDesc.Format = rtvDesc.Format;
				texDesc.SampleDesc.Count = 1;
				texDesc.SampleDesc.Quality = 0;
				texDesc.Usage = D3D11_USAGE_DEFAULT;
				texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
				texDesc.CPUAccessFlags = 0;
				texDesc.MiscFlags = 0;

				m_resLib.Add(id, RenderTargetView::Create(m_context.get(), rtvDesc, Texture2D::Create(m_context.get(), texDesc, (void*)nullptr)));

				D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
				srvDesc.Format = rtvDesc.Format;
				srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
				srvDesc.Texture2D.MostDetailedMip = 0;
				srvDesc.Texture2D.MipLevels = 1;

				m_resLib.Add("c_" + id, ShaderResourceView::Create(m_context.get(), srvDesc, m_resLib.Get<RenderTargetView>(id)->GetTexture()));
			}

			// dsv
			{
				if (m_resLib.Exist<DepthStencilView>(id))
				{
					m_resLib.Remove<DepthStencilView>(id);
					m_resLib.Remove<ShaderResourceView>("d_" + id);
				}

				D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
				dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
				dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
				dsvDesc.Texture2D.MipSlice = 0;

				D3D11_TEXTURE2D_DESC texDesc = {};
				texDesc.Width = m_window->GetDesc().width;
				texDesc.Height = m_window->GetDesc().height;
				texDesc.ArraySize = 1;
				texDesc.MipLevels = 1;
				texDesc.Format = DXGI_FORMAT_R32_TYPELESS;
				texDesc.SampleDesc.Count = 1;
				texDesc.SampleDesc.Quality = 0;
				texDesc.Usage = D3D11_USAGE_DEFAULT;
				texDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
				texDesc.CPUAccessFlags = 0;
				texDesc.MiscFlags = 0;

				m_resLib.Add(id, DepthStencilView::Create(m_context.get(), dsvDesc, Texture2D::Create(m_context.get(), texDesc, (void*)nullptr)));

				D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
				srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
				srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
				srvDesc.Texture2D.MipLevels = 1;
				srvDesc.Texture2D.MostDetailedMip = 0;

				m_resLib.Add("d_" + id, ShaderResourceView::Create(m_context.get(), srvDesc, m_resLib.Get<DepthStencilView>(id)->GetTexture2D()));
			}
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
			if (m_resLib.Exist<BlendState>("blend"))
				m_resLib.Remove<BlendState>("blend");

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
			brt.BlendOpAlpha = D3D11_BLEND_OP_MAX;
			brt.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
			m_resLib.Add("blend", BlendState::Create(m_context.get(), desc));
		}

		{
			if (m_resLib.Exist<BlendState>("default"))
				m_resLib.Remove<BlendState>("default");

			m_resLib.Add("default", BlendState::Create(m_context.get(), CD3D11_BLEND_DESC(CD3D11_DEFAULT())));
		}

		{
			if (m_resLib.Exist<DepthStencilState>("default"))
				m_resLib.Remove<DepthStencilState>("default");

			m_resLib.Add("default", DepthStencilState::Create(m_context.get(), CD3D11_DEPTH_STENCIL_DESC(CD3D11_DEFAULT())));
		}

		{
			if (m_resLib.Exist<DepthStencilState>("depth_disabled"))
				m_resLib.Remove<DepthStencilState>("depth_disabled");

			D3D11_DEPTH_STENCIL_DESC desc = CD3D11_DEPTH_STENCIL_DESC(CD3D11_DEFAULT());
			desc.DepthEnable = FALSE;
			m_resLib.Add("depth_disabled", DepthStencilState::Create(m_context.get(), desc));
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

		SetMainViews();
		SetDepthPeelRes();

		return false;
	}
}