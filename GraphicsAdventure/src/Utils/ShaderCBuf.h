#pragma once
#include <DirectXMath.h>

namespace GA::Utils
{
	// phong.ps.hlsl max lights
	static constexpr uint32_t s_maxLights = 5;

	struct PhongVSSystemCBuf
	{
		DirectX::XMFLOAT4X4 viewProjection;
		DirectX::XMFLOAT3 viewPos;
		float p0;
	};

	struct PhongVSEntityCBuf
	{
		DirectX::XMFLOAT4X4 transform;
		DirectX::XMFLOAT4X4 normalMatrix;
	};

	struct PhongPSSystemCBuf
	{
		struct DirectionalLight
		{
			DirectX::XMFLOAT3 color;
			float ambientIntensity;
			DirectX::XMFLOAT3 direction;
			float intensity;

			DirectX::XMFLOAT4X4 lightSpace;
		} dirLights[s_maxLights];

		struct PointLight
		{
			DirectX::XMFLOAT3 color;
			float ambientIntensity;
			DirectX::XMFLOAT3 position;
			float intensity;

			float nearZ;
			float farZ;
			float p0;
			float p1;

			DirectX::XMFLOAT4X4 lightSpace;
		} pointLights[s_maxLights];

		struct SpotLight
		{
			DirectX::XMFLOAT3 color;
			float ambientIntensity;
			DirectX::XMFLOAT3 direction;
			float intensity;

			DirectX::XMFLOAT3 position;
			float innerCutOffCosAngle;

			float outerCutOffCosAngle;
			float p0;
			float p1;
			float p2;

			DirectX::XMFLOAT4X4 lightSpace;
		} spotLights[s_maxLights];


		uint32_t activeDirLights;
		uint32_t activePointLights;
		uint32_t activeSpotLights;
		uint32_t p0;
	};

	struct PhongPSEntityCBuf
	{
		struct Material
		{
			DirectX::XMFLOAT4 color;
			DirectX::XMFLOAT2 tiling;
			float shininess;
			BOOL enableNormalMapping;
			BOOL enableParallaxMapping;
			float depthMapScale;
			int p0;
			int p1;
		} mat;

		BOOL receiveShadows;
		float p2;
		float p3;
		float p4;
	};
}