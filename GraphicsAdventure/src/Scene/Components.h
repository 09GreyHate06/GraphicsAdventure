#pragma once
#include <GDX11.h>

namespace GA
{
	struct TransformComponent
	{
		DirectX::XMFLOAT3 position = { 0.0f, 0.0f, 0.0f };
		DirectX::XMFLOAT3 rotation = { 0.0f, 0.0f, 0.0f };
		DirectX::XMFLOAT3 scale = { 1.0f, 1.0f, 1.0f };

		TransformComponent() = default;
		TransformComponent(const TransformComponent& rhs) = default;
		TransformComponent(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& rotation, const DirectX::XMFLOAT3& scale)
			: position(position), rotation(rotation), scale(scale)
		{
		}

		DirectX::XMVECTOR GetOrientation() const
		{
			return DirectX::XMQuaternionRotationRollPitchYaw(
				DirectX::XMConvertToRadians(rotation.x),
				DirectX::XMConvertToRadians(rotation.y),
				DirectX::XMConvertToRadians(rotation.z));
		}

		DirectX::XMVECTOR GetRight() const
		{
			return DirectX::XMVector3Rotate(DirectX::XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f), GetOrientation());
		}

		DirectX::XMVECTOR GetUp() const
		{
			return DirectX::XMVector3Rotate(DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), GetOrientation());
		}

		DirectX::XMVECTOR GetForward() const
		{
			return DirectX::XMVector3Rotate(DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), GetOrientation());
		}

		DirectX::XMMATRIX GetTransform() const
		{
			using namespace DirectX;
			return
				XMMatrixScaling(scale.x, scale.y, scale.z) *
				XMMatrixRotationQuaternion(GetOrientation()) *
				XMMatrixTranslation(position.x, position.y, position.z);
		}
	};

	struct MeshComponent
	{
		std::shared_ptr<GDX11::Buffer> vb;
		std::shared_ptr<GDX11::Buffer> ib;
		D3D11_PRIMITIVE_TOPOLOGY topology;

		bool castShadows;
		bool receiveShadows;
	};

	struct MaterialComponent
	{
		std::shared_ptr<GDX11::ShaderResourceView> diffuseMap;
		std::shared_ptr<GDX11::ShaderResourceView> normalMap;
		std::shared_ptr<GDX11::ShaderResourceView> depthMap; // for parallax mapping
		std::shared_ptr<GDX11::SamplerState> samplerState;

		DirectX::XMFLOAT4 color;
		DirectX::XMFLOAT2 tiling;
		float shininess;
		float depthMapScale; // for parallax mapping
	};

	struct DirectionalLightComponent
	{
		DirectX::XMFLOAT3 color;
		float ambientIntensity;
		float intensity;
	};

	struct PointLightComponent
	{
		DirectX::XMFLOAT3 color;
		float ambientIntensity;
		float intensity;
		float shadowNearZ;
		float shadowFarZ;
	};

	struct SpotLightComponent
	{
		DirectX::XMFLOAT3 color;
		float intensity;
		float ambientIntensity;
		float innerCutOffAngle;
		float outerCutOffAngle;
		float shadowNearZ;
		float shadowFarZ;
	};

	struct SkyboxComponent
	{
		std::shared_ptr<GDX11::ShaderResourceView> skybox;
	};
}