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
		auto mainDSV = m_resLib.Get<DepthStencilView>("main");
		mainDSV->Clear(D3D11_CLEAR_DEPTH, 1.0f, 0xff);


		{
			m_resLib.Get<RasterizerState>("default")->Bind();
			m_resLib.Get<BlendState>("default")->Bind(nullptr, 0xff);
			m_resLib.Get<DepthStencilState>("default")->Bind(0xff);

			auto rtv = m_resLib.Get<RenderTargetView>("scene");
			rtv->Bind(mainDSV.get());
			rtv->Clear(0.0f, 0.0f, 0.0f, 0.0f);

			auto vs = m_resLib.Get<VertexShader>("light");
			auto ps = m_resLib.Get<PixelShader>("light");
			vs->Bind();
			ps->Bind();
			m_resLib.Get<InputLayout>("light")->Bind();

			SetLight(ps);
			DrawPlane(vs, ps, m_resLib.Get<ShaderResourceView>("wood"), m_resLib.Get<ShaderResourceView>("bump_normal"), m_resLib.Get<SamplerState>("anisotropic_wrap"), XMFLOAT3(0.0f, -2.5f, 0.0f), XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(20.0f, 1.0f, 20.0f),
				XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT2(10.0f, 10.0f), 163.0f);


			DrawCube(vs, ps, m_resLib.Get<ShaderResourceView>("marble"), m_resLib.Get<ShaderResourceView>("bump_normal"), m_resLib.Get<SamplerState>("anisotropic_wrap"), XMFLOAT3(0.0f, 0.5f, 0.0f), XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(5.0f, 5.0f, 5.0f),
				XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT2(1.0f, 1.0f), 32.0f);


			// sky box
			m_resLib.Get<DepthStencilState>("skybox")->Bind(0xff);
			m_resLib.Get<RasterizerState>("no_cull")->Bind();

			vs = m_resLib.Get<VertexShader>("skybox");
			ps = m_resLib.Get<PixelShader>("skybox");
			vs->Bind();
			ps->Bind();
			m_resLib.Get<InputLayout>("skybox")->Bind();

			m_resLib.Get<Buffer>("float4x4")->VSBindAsCBuf(vs->GetResBinding("EntityCBuf"));
			XMFLOAT4X4 viewProj; 
			XMStoreFloat4x4(&viewProj, XMMatrixTranspose(m_camera.GetViewMatrix() * m_camera.GetProjectionMatrix()));
			m_resLib.Get<Buffer>("float4x4")->SetData(&viewProj);

			m_resLib.Get<SamplerState>("linear_clamp")->PSBind(ps->GetResBinding("sam"));
			m_resLib.Get<ShaderResourceView>("skybox")->PSBind(ps->GetResBinding("tex"));

			m_resLib.Get<Buffer>("cube_basic.vb")->BindAsVB();
			auto cbIb = m_resLib.Get<Buffer>("cube_basic.ib");
			cbIb->BindAsIB(DXGI_FORMAT_R32_UINT);

			m_context->GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			GDX11_CONTEXT_THROW_INFO_ONLY(m_context->GetDeviceContext()->DrawIndexed(cbIb->GetDesc().ByteWidth / sizeof(uint32_t), 0, 0));

		}

		// post process (gamma)
		{
			m_resLib.Get<RasterizerState>("default")->Bind();
			m_resLib.Get<DepthStencilState>("default_depth_func_always")->Bind(0xff);
			m_resLib.Get<BlendState>("default")->Bind(nullptr, 0xff);

			auto rtv = m_resLib.Get<RenderTargetView>("main");
			rtv->Bind(mainDSV.get());
			rtv->Clear(0.0f, 0.0f, 0.0f, 0.0f);

			// bind gamma correction shader
			m_resLib.Get<VertexShader>("fullscreen_out_tc_pos")->Bind();
			auto ps = m_resLib.Get<PixelShader>("gamma_correction");
			ps->Bind();
			m_resLib.Get<InputLayout>("fullscreen_out_tc_pos")->Bind();

			m_resLib.Get<ShaderResourceView>("scene")->PSBind(ps->GetResBinding("tex"));
			m_resLib.Get<SamplerState>("point_wrap")->PSBind(ps->GetResBinding("samplerState"));

			XMFLOAT4 gamma = { m_gamma, 0.0f, 0.0f, 0.0f };
			m_resLib.Get<Buffer>("float4")->PSBindAsCBuf(ps->GetResBinding("SystemCBuf"));
			m_resLib.Get<Buffer>("float4")->SetData(&gamma);

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

		ImGui::Begin("Gamma correction");
		ImGui::PushItemWidth(80.0f);
		ImGui::DragFloat("Gamma level", &m_gamma, 0.1f, 0.0f, 0.0f, "%.1f");
		ImGui::PopItemWidth();
		ImGui::End();

		ImGui::Begin("Light");
		ImGui::DragFloat3("Direction", &m_lightDir.x, 0.1f);
		ImGui::End();

		m_imguiManager.End();
	}

	void App::SetLight(const std::shared_ptr<GDX11::PixelShader>& ps)
	{
		XMVECTOR rotQuatXM = XMQuaternionRotationRollPitchYaw(XMConvertToRadians(m_lightDir.x), XMConvertToRadians(m_lightDir.y), XMConvertToRadians(m_lightDir.z));
		XMVECTOR directionXM = XMVector3Rotate(XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), rotQuatXM);
		XMFLOAT3 direction;
		XMStoreFloat3(&direction, directionXM);

		GA::Utils::LightPSSystemCBuf cbuf = {};
		cbuf.dirLights[0].color = { 1.0f, 1.0f, 1.0f };
		cbuf.dirLights[0].direction = direction;
		cbuf.dirLights[0].ambientIntensity = 0.2f;
		cbuf.dirLights[0].intensity = 1.0f;

		//cbuf.pointLights[0].color = { 1.0f, 1.0f, 0.0f };
		//cbuf.pointLights[0].position = {-4.0f, -0.5f, 0.0f};
		//cbuf.pointLights[0].ambientIntensity = 0.4f;
		//cbuf.pointLights[0].intensity = 2.0f;

		//cbuf.pointLights[1].color = { 1.0f, 0.0f, 1.0f };
		//cbuf.pointLights[1].position = { 4.0f, -0.5f, 0.0f };
		//cbuf.pointLights[1].ambientIntensity = 0.4f;
		//cbuf.pointLights[1].intensity = 2.0f;

		//cbuf.pointLights[2].color = { 0.0f, 1.0f, 1.0f };
		//cbuf.pointLights[2].position = { 0.0f, -0.5f, -4.0f };
		//cbuf.pointLights[2].ambientIntensity = 0.4f;
		//cbuf.pointLights[2].intensity = 2.0f;

		//cbuf.pointLights[3].color = { 1.0f, 0.5f, 0.5f };
		//cbuf.pointLights[3].position = { 0.0f, -0.5f, 4.0f };
		//cbuf.pointLights[3].ambientIntensity = 0.4f;
		//cbuf.pointLights[3].intensity = 2.0f;

		cbuf.activeDirLights = 1;
		//cbuf.activePointLights = 4;

		m_resLib.Get<Buffer>("light.ps.SystemCBuf")->PSBindAsCBuf(ps->GetResBinding("SystemCBuf"));
		m_resLib.Get<Buffer>("light.ps.SystemCBuf")->SetData(&cbuf);
	}

	void App::DrawPlane(const std::shared_ptr<VertexShader>& vs, const std::shared_ptr<PixelShader>& ps, const std::shared_ptr<ShaderResourceView>& diffuse, const std::shared_ptr<GDX11::ShaderResourceView>& normal, const std::shared_ptr<GDX11::SamplerState>& sam, const XMFLOAT3& pos, const XMFLOAT3& rot, const XMFLOAT3& scale,
		const XMFLOAT4& col, const XMFLOAT2& tiling, float shininess)
	{
		auto vb = m_resLib.Get<Buffer>("plane.vb");
		auto ib = m_resLib.Get<Buffer>("plane.ib");
		vb->BindAsVB();
		ib->BindAsIB(DXGI_FORMAT_R32_UINT);


		diffuse->PSBind(ps->GetResBinding("diffuseMap"));
		sam->PSBind(ps->GetResBinding("mapSampler"));

		if (normal)
		{
			normal->PSBind(ps->GetResBinding("normalMap"));
		}

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
			cbuf.mat.enableNormalMap = normal ? TRUE : FALSE;
			m_resLib.Get<Buffer>("light.ps.EntityCBuf")->PSBindAsCBuf(ps->GetResBinding("EntityCBuf"));
			m_resLib.Get<Buffer>("light.ps.EntityCBuf")->SetData(&cbuf);
		}

		m_context->GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		GDX11_CONTEXT_THROW_INFO_ONLY(m_context->GetDeviceContext()->DrawIndexed(ib->GetDesc().ByteWidth / sizeof(uint32_t), 0, 0));
	}

	void App::DrawCube(const std::shared_ptr<VertexShader>& vs, const std::shared_ptr<PixelShader>& ps, const std::shared_ptr<ShaderResourceView>& diffuse, const std::shared_ptr<GDX11::ShaderResourceView>& normal, const std::shared_ptr<GDX11::SamplerState>& sam, const XMFLOAT3& pos, const XMFLOAT3& rot, const XMFLOAT3& scale,
		const XMFLOAT4& col, const XMFLOAT2& tiling, float shininess)
	{
		auto vb = m_resLib.Get<Buffer>("cube.vb");
		auto ib = m_resLib.Get<Buffer>("cube.ib");
		vb->BindAsVB();
		ib->BindAsIB(DXGI_FORMAT_R32_UINT);


		diffuse->PSBind(ps->GetResBinding("diffuseMap"));
		sam->PSBind(ps->GetResBinding("mapSampler"));

		if (normal)
		{
			normal->PSBind(ps->GetResBinding("normalMap"));
		}

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
			cbuf.mat.enableNormalMap = normal ? TRUE : FALSE;
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
		m_resLib.Add("light", InputLayout::Create(m_context.get(), m_resLib.Get<VertexShader>("light")));

		m_resLib.Add("fullscreen_out_tc_pos", VertexShader::Create(m_context.get(), "res/cso/fullscreen_out_tc_pos.vs.cso"));
		m_resLib.Add("gamma_correction", PixelShader::Create(m_context.get(), "res/cso/gamma_correction.ps.cso"));
		m_resLib.Add("fullscreen_out_tc_pos", InputLayout::Create(m_context.get(), m_resLib.Get<VertexShader>("fullscreen_out_tc_pos")));

		m_resLib.Add("skybox", VertexShader::Create(m_context.get(), "res/cso/skybox.vs.cso"));
		m_resLib.Add("skybox", PixelShader::Create(m_context.get(), "res/cso/skybox.ps.cso"));
		m_resLib.Add("skybox", InputLayout::Create(m_context.get(), m_resLib.Get<VertexShader>("skybox")));
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

		// screen
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

		// cube map
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
			m_resLib.Add("cube_basic.vb", Buffer::Create(m_context.get(), desc, vert.data()));

			desc = {};
			desc.ByteWidth = (uint32_t)ind.size() * sizeof(uint32_t);
			desc.Usage = D3D11_USAGE_DEFAULT;
			desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
			desc.CPUAccessFlags = 0;
			desc.MiscFlags = 0;
			desc.StructureByteStride = sizeof(uint32_t);
			m_resLib.Add("cube_basic.ib", Buffer::Create(m_context.get(), desc, ind.data()));
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

		{
			D3D11_BUFFER_DESC desc = {};
			desc.ByteWidth = sizeof(XMFLOAT4);
			desc.Usage = D3D11_USAGE_DYNAMIC;
			desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
			desc.MiscFlags = 0;
			desc.StructureByteStride = 0;
			m_resLib.Add("float4", Buffer::Create(m_context.get(), desc, nullptr));
		}

		{
			D3D11_BUFFER_DESC desc = {};
			desc.ByteWidth = sizeof(XMFLOAT4X4);
			desc.Usage = D3D11_USAGE_DYNAMIC;
			desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
			desc.MiscFlags = 0;
			desc.StructureByteStride = 0;
			m_resLib.Add("float4x4", Buffer::Create(m_context.get(), desc, nullptr));
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
			D3D11_SAMPLER_DESC desc = CD3D11_SAMPLER_DESC(CD3D11_DEFAULT());
			desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
			desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
			desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
			desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
			for (int i = 0; i < 4; i++)
				desc.BorderColor[i] = 0.0f;
			desc.MinLOD = 0.0f;
			desc.MaxLOD = D3D11_FLOAT32_MAX;
			m_resLib.Add("linear_clamp", SamplerState::Create(m_context.get(), desc));
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
			GDX11::Utils::ImageData image = GDX11::Utils::LoadImageFile("res/textures/groundMarble.png", false, 4);
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

			m_resLib.Add("marble", ShaderResourceView::Create(m_context.get(), srvDesc, Texture2D::Create(m_context.get(), texDesc, image.pixels)));
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


		{
			if (m_resLib.Exist<RenderTargetView>("scene"))
			{
				m_resLib.Remove<RenderTargetView>("scene");
				m_resLib.Remove<ShaderResourceView>("scene");
			}

			D3D11_RENDER_TARGET_VIEW_DESC desc = {};
			desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
			desc.Texture2D.MipSlice = 0;

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

			auto tex = Texture2D::Create(m_context.get(), texDesc, (void*)nullptr);

			D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			srvDesc.Format = texDesc.Format;
			srvDesc.Texture2D.MipLevels = 1;
			srvDesc.Texture2D.MostDetailedMip = 0;
			srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;

			m_resLib.Add("scene", ShaderResourceView::Create(m_context.get(), srvDesc, tex));
			m_resLib.Add("scene", RenderTargetView::Create(m_context.get(), desc, tex));
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
			if (m_resLib.Exist<DepthStencilState>("skybox"))
				m_resLib.Remove<DepthStencilState>("skybox");

			D3D11_DEPTH_STENCIL_DESC desc = CD3D11_DEPTH_STENCIL_DESC(CD3D11_DEFAULT());
			desc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
			desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
			m_resLib.Add("skybox", DepthStencilState::Create(m_context.get(), desc));
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