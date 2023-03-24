#pragma once
#include <DirectXMath.h>

namespace GA::Utils
{
	// light.ps.hlsl max lights
	static constexpr uint32_t s_maxLights = 32;

	struct LightVSSystemCBuf
	{
		DirectX::XMFLOAT4X4 viewProjection;
		DirectX::XMFLOAT3 viewPos;
		float p0;
	};

	struct LightVSEntityCBuf
	{
		DirectX::XMFLOAT4X4 transform;
		DirectX::XMFLOAT4X4 normalMatrix;
	};

	struct LightPSSystemCBuf
	{
		struct DirectionalLight
		{
			DirectX::XMFLOAT3 color;
			float ambientIntensity;
			DirectX::XMFLOAT3 direction;
			float intensity;
		} dirLights[s_maxLights];

		struct PointLight
		{
			DirectX::XMFLOAT3 color;
			float ambientIntensity;
			DirectX::XMFLOAT3 position;
			float intensity;

			float attConstant;
			float attLinear;
			float attQuadratic;
			float p0;
		} pointLights[s_maxLights];

		struct SpotLight
		{
			DirectX::XMFLOAT3 color;
			float ambientIntensity;
			DirectX::XMFLOAT3 direction;
			float intensity;

			DirectX::XMFLOAT3 position;
			float attConstant;

			float attLinear;
			float attQuadratic;
			float innerCutOffCosAngle;
			float outerCutOffCosAngle;

		} spotLights[s_maxLights];


		uint32_t activeDirLights;
		uint32_t activePointLights;
		uint32_t activeSpotLights;
		uint32_t p0;
	};

	struct LightPSEntityCBuf
	{
		struct Material
		{
			DirectX::XMFLOAT4 color;
			DirectX::XMFLOAT2 tiling;
			float shininess;
			float p0;
		} mat;
	};
}